//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "live_server.h"
#include "live_peer.h"
#include "live_tab.h"
#include "live_action.h"

#include "editor.h"

LiveServer::LiveServer(Editor& editor) :
	LiveSocket(),
	clients(), acceptor(nullptr), socket(nullptr), editor(&editor),
	clientIds(0), port(0), stopped(false) {
	//
}

LiveServer::~LiveServer() {
	//
}

bool LiveServer::bind() {
	// Ensure we're on the main thread for initialization
	if (!wxThread::IsMain()) {
		bool success = false;
		wxTheApp->CallAfter([this, &success]() {
			success = this->bind();
		});
		return success;
	}

	NetworkConnection& connection = NetworkConnection::getInstance();
	if (!connection.start()) {
		setLastError("The previous connection has not been terminated yet.");
		return false;
	}

	auto& service = connection.get_service();
	acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(service);

	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
	acceptor->open(endpoint.protocol());

	boost::system::error_code error;
	acceptor->set_option(boost::asio::ip::tcp::no_delay(true), error);
	if (error) {
		setLastError("Error: " + error.message());
		return false;
	}

	acceptor->bind(endpoint);
	acceptor->listen();

	acceptClient();
	return true;
}

void LiveServer::close() {
	for (auto& clientEntry : clients) {
		delete clientEntry.second;
	}
	clients.clear();

	if (log) {
		log->Message("Server was shutdown.");
		log->Disconnect();
		log = nullptr;
	}

	stopped = true;
	if (acceptor) {
		acceptor->close();
	}

	if (socket) {
		socket->close();
	}
}

void LiveServer::acceptClient() {
	// Ensure we're on the main thread
	if (!wxThread::IsMain()) {
		wxTheApp->CallAfter([this]() {
			this->acceptClient();
		});
		return;
	}

	static uint32_t id = 0;
	if (stopped) {
		return;
	}

	if (!socket) {
		socket = std::make_shared<boost::asio::ip::tcp::socket>(
			NetworkConnection::getInstance().get_service()
		);
	}

	acceptor->async_accept(*socket, [this](const boost::system::error_code& error) -> void {
		// Queue the client handling on the main thread
		wxTheApp->CallAfter([this, error]() {
			if (!error) {
				static uint32_t nextId = 0;
				LivePeer* peer = new LivePeer(this, std::move(*socket));
				peer->log = log;
				peer->receiveHeader();

				clients.insert(std::make_pair(nextId++, peer));
				
				// Make sure the host's cursor exists
				if (cursors.find(0) == cursors.end()) {
					LiveCursor hostCursor;
					hostCursor.id = 0;
					hostCursor.color = usedColor;
					hostCursor.pos = Position();
					cursors[0] = hostCursor;
				}
				
				updateClientList();
			}
			// Queue next accept regardless of error
			acceptClient();
		});
	});
}

void LiveServer::removeClient(uint32_t id) {
	// Ensure we're on the main thread
	if (!wxThread::IsMain()) {
		wxTheApp->CallAfter([this, id]() {
			this->removeClient(id);
		});
		return;
	}

	auto it = clients.find(id);
	if (it == clients.end()) {
		return;
	}

	const uint32_t clientId = it->second->getClientId();
	if (clientId != 0) {
		clientIds &= ~clientId;
		editor->map.clearVisible(clientIds);
	}

	clients.erase(it);
	updateClientList();
}

void LiveServer::updateCursor(const Position& position) {
	// Ensure we're on the main thread
	if (!wxThread::IsMain()) {
		wxTheApp->CallAfter([this, position]() {
			this->updateCursor(position);
		});
		return;
	}

	LiveCursor cursor;
	cursor.id = 0;
	cursor.pos = position;
	cursor.color = usedColor;
	
	this->cursors[cursor.id] = cursor;
	this->broadcastCursor(cursor);
	
	g_gui.RefreshView();
}

void LiveServer::updateClientList() const {
	log->UpdateClientList(clients);
}

uint16_t LiveServer::getPort() const {
	return port;
}

bool LiveServer::setPort(int32_t newPort) {
	if (newPort < 1 || newPort > 65535) {
		setLastError("Port must be a number in the range 1-65535.");
		return false;
	}
	port = newPort;
	return true;
}

uint32_t LiveServer::getFreeClientId() {
	for (int32_t bit = 1; bit < (1 << 16); bit <<= 1) {
		if (!testFlags(clientIds, bit)) {
			clientIds |= bit;
			return bit;
		}
	}
	return 0;
}

std::string LiveServer::getHostName() const {
	if (acceptor) {
		auto endpoint = acceptor->local_endpoint();
		return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
	}
	return "localhost";
}

void LiveServer::broadcastNodes(DirtyList& dirtyList) {
	if (dirtyList.Empty() || !editor) {
		return;
	}

	// Make a deep copy of the dirty list data to ensure it persists through the async call
	struct BroadcastData {
		std::vector<DirtyList::ValueType> positions;
		uint32_t owner;
		std::vector<std::unique_ptr<Change>> changes;
	};

	auto broadcastData = std::make_shared<BroadcastData>();
	broadcastData->positions.assign(dirtyList.GetPosList().begin(), dirtyList.GetPosList().end());
	broadcastData->owner = dirtyList.owner;
	
	// If this is from the host, copy the changes
	if (dirtyList.owner == 0) {
		auto& changes = dirtyList.GetChanges();
		broadcastData->changes.reserve(changes.size());
		for (Change* change : changes) {
			if (change && change->getType() == CHANGE_TILE) {
				Tile* tile = static_cast<Tile*>(change->getData());
				if (tile) {
					// Create a managed copy
					broadcastData->changes.push_back(std::make_unique<Change>(tile->deepCopy(editor->map)));
				}
			}
		}
	}

	// Process everything on the main thread in a single CallAfter to maintain atomicity
	wxTheApp->CallAfter([this, broadcastData]() {
		try {
			if (!editor) return;

			// Handle host changes first
			if (broadcastData->owner == 0 && !broadcastData->changes.empty()) {
				NetworkedAction* action = static_cast<NetworkedAction*>(editor->actionQueue->createAction(ACTION_REMOTE));
				if (action) {
					action->owner = broadcastData->owner;
					
					// Add the changes to the action - action takes ownership
					for (auto& change : broadcastData->changes) {
						if (change) {
							action->addChange(change.release());
						}
					}
					
					// Add the action to the queue
					editor->actionQueue->addAction(action, 0);
				}
			}

			// Now handle the network broadcasting
			// Create a vector of all the work we need to do
			struct BroadcastWork {
				LivePeer* peer;
				QTreeNode* node;
				int32_t ndx;
				int32_t ndy;
				uint32_t floors;
				uint32_t clientId;
			};
			std::vector<BroadcastWork> workItems;

			// First gather all the work without doing any actual sending
			for (const auto& ind : broadcastData->positions) {
				int32_t ndx = ind.pos >> 18;
				int32_t ndy = (ind.pos >> 4) & 0x3FFF;
				uint32_t floors = ind.floors;

				QTreeNode* node = editor->map.getLeaf(ndx * 4, ndy * 4);
				if (!node) continue;

				for (auto& clientEntry : clients) {
					LivePeer* peer = clientEntry.second;
					if (!peer) continue;

					const uint32_t clientId = peer->getClientId();
					
					if (node->isVisible(clientId, true) || node->isVisible(clientId, false)) {
						workItems.push_back({peer, node, ndx, ndy, floors, clientId});
					}
				}
			}

			// Now process all the work in a single batch
			for (const auto& work : workItems) {
				try {
					if (work.node->isVisible(work.clientId, true)) {
						work.peer->sendNode(work.clientId, work.node, work.ndx, work.ndy, work.floors & 0xFF00);
					}

					if (work.node->isVisible(work.clientId, false)) {
						work.peer->sendNode(work.clientId, work.node, work.ndx, work.ndy, work.floors & 0x00FF);
					}
				} catch (std::exception& e) {
					logMessage(wxString::Format("[Server]: Error sending node to client %s: %s", 
						work.peer->getName(), e.what()));
				}
			}

			// Update the UI once at the end
			g_gui.RefreshView();
			g_gui.UpdateMinimap();

		} catch (std::exception& e) {
			logMessage(wxString::Format("[Server]: Error in broadcast: %s", e.what()));
		}
	});
}

void LiveServer::broadcastCursor(const LiveCursor& cursor) {
	// Ensure we're on the main thread
	if (!wxThread::IsMain()) {
		wxTheApp->CallAfter([this, cursor]() {
			this->broadcastCursor(cursor);
		});
		return;
	}

	if (clients.empty()) {
		return;
	}

	cursors[cursor.id] = cursor;

	NetworkMessage message;
	message.write<uint8_t>(PACKET_CURSOR_UPDATE);
	writeCursor(message, cursor);

	for (auto& clientEntry : clients) {
		LivePeer* peer = clientEntry.second;
		peer->send(message);
	}
	
	g_gui.RefreshView();
}

void LiveServer::broadcastChat(const wxString& speaker, const wxString& chatMessage) {
	// Ensure we're on the main thread
	if (!wxThread::IsMain()) {
		wxTheApp->CallAfter([this, speaker, chatMessage]() {
			this->broadcastChat(speaker, chatMessage);
		});
		return;
	}

	if (clients.empty()) {
		return;
	}

	wxString displayName = (speaker == "HOST") ? name : speaker;

	NetworkMessage message;
	message.write<uint8_t>(PACKET_SERVER_TALK);
	message.write<std::string>(nstr(displayName));
	message.write<std::string>(nstr(chatMessage));

	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}

	if (log) {
		log->Chat(displayName, chatMessage);
	}
}

void LiveServer::sendChat(const wxString& chatMessage) {
	// For server, sending a chat message means broadcasting it from HOST
	broadcastChat("HOST", chatMessage);
}

void LiveServer::startOperation(const wxString& operationMessage) {
	if (clients.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_START_OPERATION);
	message.write<std::string>(nstr(operationMessage));

	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}
}

void LiveServer::updateOperation(int32_t percent) {
	if (clients.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_UPDATE_OPERATION);
	message.write<uint32_t>(percent);

	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}
}

LiveLogTab* LiveServer::createLogWindow(wxWindow* parent) {
	MapTabbook* mapTabBook = dynamic_cast<MapTabbook*>(parent);
	ASSERT(mapTabBook);

	log = newd LiveLogTab(mapTabBook, this);
	log->Message("New Live mapping session started.");
	log->Message("Hosted on server " + getHostName() + ".");

	updateClientList();
	return log;
}

void LiveServer::broadcastColorChange(uint32_t clientId, const wxColor& color) {
	if (clients.empty()) {
		return;
	}

	// Prepare the color update packet
	NetworkMessage message;
	message.write<uint8_t>(PACKET_COLOR_UPDATE);
	message.write<uint32_t>(clientId);
	message.write<uint8_t>(color.Red());
	message.write<uint8_t>(color.Green());
	message.write<uint8_t>(color.Blue());
	message.write<uint8_t>(color.Alpha());

	// Log the color change for debugging
	logMessage(wxString::Format("[Server]: Broadcasting color change for client %u: RGB(%d,%d,%d)", 
		clientId, color.Red(), color.Green(), color.Blue()));

	// Send to all clients
	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}

	// Update client list in all open log tabs
	updateClientList();
}

void LiveServer::setUsedColor(const wxColor& color) {
	usedColor = color;
	
	// Broadcast the host's color change (host always has clientId 0)
	broadcastColorChange(0, color);
}
