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
		if (error) {
			//
		} else {
			LivePeer* peer = new LivePeer(this, std::move(*socket));
			peer->log = log;
			peer->receiveHeader();

			clients.insert(std::make_pair(id++, peer));
			updateClientList();
		}
		acceptClient();
	});
}

void LiveServer::removeClient(uint32_t id) {
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
	LiveCursor cursor;
	cursor.id = 0; // Host's cursor ID is always 0
	cursor.pos = position;
	cursor.color = usedColor;
	
	// Store the cursor in local map (important for host to see other cursors)
	this->cursors[cursor.id] = cursor;
	
	// Broadcast to clients
	this->broadcastCursor(cursor);
	
	// Update the view to show cursor changes
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
	
	try {
		size_t nodeCount = 0;
		size_t totalClientCount = 0;
		
		// Create a copy of the position list to work with
		auto posList = dirtyList.GetPosList();
		uint32_t owner = dirtyList.owner;
		
		// Apply changes to host's map immediately
		// This ensures the host can see their own changes
		if (owner == 0) {
			logMessage("[Server]: Applying host changes to local map");
			editor->actionQueue->addAction(static_cast<NetworkedAction*>(editor->actionQueue->createAction(ACTION_REMOTE)));
		}
		
		for (const auto& ind : posList) {
			int32_t ndx = ind.pos >> 18;
			int32_t ndy = (ind.pos >> 4) & 0x3FFF;
			uint32_t floors = ind.floors;
			
			QTreeNode* node = editor->map.getLeaf(ndx * 4, ndy * 4);
			if (!node) {
				continue;
			}
			
			nodeCount++;
			
			// Track clients who receive updates
			std::vector<uint32_t> updatedClientIds;
			
			for (auto& clientEntry : clients) {
				LivePeer* peer = clientEntry.second;
				if (!peer) continue;
				
				const uint32_t clientId = peer->getClientId();
				
				// We no longer skip the client who made the change
				// This ensures clients can see changes from other clients
				
				bool sentUpdate = false;
				
				// Check if the node is visible to this client
				if (node->isVisible(clientId, true)) {
					try {
						peer->sendNode(clientId, node, ndx, ndy, floors & 0xFF00);
						sentUpdate = true;
						logMessage(wxString::Format("[Server]: Node [%d,%d,%s] marked as visible for client %u", 
							ndx, ndy, "underground", clientId));
						logMessage(wxString::Format("[Server]: Sending node update to client %u", clientId));
					} catch (std::exception& e) {
						logMessage(wxString::Format("[Server]: Error sending underground node to client %s: %s", 
							peer->getName(), e.what()));
					}
				}
				
				if (node->isVisible(clientId, false)) {
					try {
						peer->sendNode(clientId, node, ndx, ndy, floors & 0x00FF);
						sentUpdate = true;
						logMessage(wxString::Format("[Server]: Node [%d,%d,%s] marked as visible for client %u", 
							ndx, ndy, "surface", clientId));
						logMessage(wxString::Format("[Server]: Sending node update to client %u", clientId));
					} catch (std::exception& e) {
						logMessage(wxString::Format("[Server]: Error sending surface node to client %s: %s", 
							peer->getName(), e.what()));
					}
				}
				
				if (sentUpdate && std::find(updatedClientIds.begin(), updatedClientIds.end(), clientId) == updatedClientIds.end()) {
					updatedClientIds.push_back(clientId);
				}
			}
			
			totalClientCount += updatedClientIds.size();
		}
		
		if (nodeCount > 0) {
			logMessage(wxString::Format("[Server]: Broadcasted %zu nodes to %zu clients", 
				nodeCount, totalClientCount));
		}
		
		// Make sure the host view is refreshed to show changes
		g_gui.RefreshView();
		g_gui.UpdateMinimap();
		
	} catch (std::exception& e) {
		logMessage(wxString::Format("[Server]: Error broadcasting nodes: %s", e.what()));
	}
}

void LiveServer::broadcastCursor(const LiveCursor& cursor) {
	if (clients.empty()) {
		return;
	}

	// Always update cursor in local map regardless of ID
	cursors[cursor.id] = cursor;

	NetworkMessage message;
	message.write<uint8_t>(PACKET_CURSOR_UPDATE);
	writeCursor(message, cursor);

	for (auto& clientEntry : clients) {
		LivePeer* peer = clientEntry.second;
		// Send to all clients, including the one that sent the update
		peer->send(message);
	}
	
	// Always refresh the view to ensure cursor updates are visible
	g_gui.RefreshView();
}

void LiveServer::broadcastChat(const wxString& speaker, const wxString& chatMessage) {
	if (clients.empty()) {
		return;
	}

	// If the speaker is HOST, use the server name
	wxString displayName = (speaker == "HOST") ? name : speaker;

	NetworkMessage message;
	message.write<uint8_t>(PACKET_SERVER_TALK);
	message.write<std::string>(nstr(displayName));
	message.write<std::string>(nstr(chatMessage));

	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}

	// Also log the chat message in the server log
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

void LiveServer::setUsedColor(const wxColor& color) {
	usedColor = color;
}
