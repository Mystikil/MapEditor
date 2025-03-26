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
#include "live_sector.h"

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
			
			// Make sure the host's cursor exists
			if (cursors.find(0) == cursors.end()) {
				LiveCursor hostCursor;
				hostCursor.id = 0;
				hostCursor.color = usedColor;
				hostCursor.pos = Position(); // Default position
				cursors[0] = hostCursor;
			}
			
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
		
		// Create a copy of the position list to work with
		auto posList = dirtyList.GetPosList();
		uint32_t owner = dirtyList.owner;
		
		// Apply changes to host's map immediately
		// This ensures the host can see their own changes
		if (owner == 0) {
			logMessage("[Server]: Applying host changes to local map");
			editor->actionQueue->addAction(static_cast<NetworkedAction*>(editor->actionQueue->createAction(ACTION_REMOTE)));
		} else {
			// For non-host changes, verify sector access for each node
			for (const auto& ind : posList) {
				int32_t ndx = ind.pos >> 18;
				int32_t ndy = (ind.pos >> 4) & 0x3FFF;
				
				// Make sure the client has permission to edit this sector
				if (!verifySectorAccess(owner, ndx * 4, ndy * 4)) {
					// Log and skip this node
					logMessage(wxString::Format("[Server]: Rejecting unauthorized edit at [%d,%d] from client %u", 
						ndx * 4, ndy * 4, owner));
					continue;
				}
			}
		}
		
		// First pass: Get all nodes to update before sending to clients to prevent threading issues
		std::vector<std::tuple<int32_t, int32_t, uint32_t, QTreeNode*>> nodesToUpdate;
		
		for (const auto& ind : posList) {
			int32_t ndx = ind.pos >> 18;
			int32_t ndy = (ind.pos >> 4) & 0x3FFF;
			uint32_t floors = ind.floors;
			
			QTreeNode* node = editor->map.getLeaf(ndx * 4, ndy * 4);
			if (node) {
				nodesToUpdate.push_back(std::make_tuple(ndx, ndy, floors, node));
				nodeCount++;
			}
		}
		
		// Create a local copy of clients to prevent threading issues
		std::unordered_map<uint32_t, LivePeer*> clientsCopy;
		{
			for (auto& pair : clients) {
				clientsCopy[pair.first] = pair.second;
			}
		}
		
		// If we have no nodes to update or no clients, we're done
		if (nodesToUpdate.empty() || clientsCopy.empty()) {
			return;
		}
		
		// Second pass: Send the updates to all clients EXCEPT the owner
		size_t updatedClientCount = 0;
		
		for (auto& clientEntry : clientsCopy) {
			LivePeer* peer = clientEntry.second;
			if (!peer) continue;
			
			uint32_t clientId = peer->getClientId();
			
			// Skip the client who made the changes - they already have them
			if (clientId == owner) continue;
			
			// For each node, send both surface and underground data
			for (const auto& nodeData : nodesToUpdate) {
				int32_t ndx = std::get<0>(nodeData);
				int32_t ndy = std::get<1>(nodeData);
				uint32_t floors = std::get<2>(nodeData);
				QTreeNode* node = std::get<3>(nodeData);
				
				try {
					// Mark the node as visible for this client
					node->setVisible(clientId, true, true);   // Surface
					node->setVisible(clientId, false, true);  // Underground
					
					// Send both underground and surface layers
					// 0xFF00 is for underground, 0x00FF is for surface
					
					// Always send underground data
					if (floors & 0xFF00) {
						peer->sendNode(clientId, node, ndx, ndy, floors & 0xFF00);
					}
					
					// Always send surface data
					if (floors & 0x00FF) {
						peer->sendNode(clientId, node, ndx, ndy, floors & 0x00FF);
					}
				}
				catch (std::exception& e) {
					logMessage(wxString::Format("[Server]: Error sending node to client %s: %s", 
						peer->getName(), e.what()));
				}
			}
			
			updatedClientCount++;
		}
		
		if (nodeCount > 0) {
			logMessage(wxString::Format("[Server]: Broadcasted %zu nodes to %zu clients", 
				nodeCount, updatedClientCount));
		}
		
		// Update views on the main thread
		wxTheApp->CallAfter([]() {
			g_gui.RefreshView();
			g_gui.UpdateMinimap();
		});
		
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

bool LiveServer::requestSectorLock(uint32_t clientId, const SectorCoord& sector) {
	bool success = sectorManager.requestLock(clientId, sector);
	
	// Find the client who requested the lock
	LivePeer* requestingPeer = nullptr;
	for (const auto& pair : clients) {
		if (pair.second->getClientId() == clientId) {
			requestingPeer = pair.second;
			break;
		}
	}
	
	if (!requestingPeer) {
		logMessage(wxString::Format("[Server]: Client %u not found for sector lock request", clientId));
		return false;
	}
	
	// Prepare the response
	NetworkMessage response;
	response.write<uint8_t>(PACKET_SECTOR_LOCK_RESPONSE);
	response.write<int32_t>(sector.x);
	response.write<int32_t>(sector.y);
	response.write<uint8_t>(success ? 1 : 0);
	
	if (!success) {
		// Include the ID of the client who has the lock
		response.write<uint32_t>(sectorManager.getLockOwner(sector));
	} else {
		// Send a snapshot of the sector data if the lock was granted
		sendSectorSnapshot(clientId, sector);
	}
	
	// Send the response
	requestingPeer->send(response);
	
	// Log the action
	if (success) {
		logMessage(wxString::Format("[Server]: Granted sector lock at [%d,%d] to client %u", 
			sector.x, sector.y, clientId));
	} else {
		logMessage(wxString::Format("[Server]: Denied sector lock at [%d,%d] to client %u (owned by %u)", 
			sector.x, sector.y, clientId, sectorManager.getLockOwner(sector)));
	}
	
	return success;
}

void LiveServer::releaseSectorLock(uint32_t clientId, const SectorCoord& sector) {
	uint32_t lockOwner = sectorManager.getLockOwner(sector);
	
	// Only the lock owner can release it
	if (lockOwner != clientId) {
		logMessage(wxString::Format("[Server]: Client %u attempted to release lock owned by %u", 
			clientId, lockOwner));
		return;
	}
	
	// If the sector is dirty, increment its version
	if (sectorManager.isDirty(sector)) {
		incrementSectorVersion(sector);
	}
	
	// Release the lock
	sectorManager.releaseLock(clientId, sector);
	
	logMessage(wxString::Format("[Server]: Released sector lock at [%d,%d] from client %u", 
		sector.x, sector.y, clientId));
	
	// Notify other clients that this sector is now available
	NetworkMessage notification;
	notification.write<uint8_t>(PACKET_SECTOR_LOCK_RELEASE);
	notification.write<int32_t>(sector.x);
	notification.write<int32_t>(sector.y);
	notification.write<uint32_t>(clientId);
	
	for (auto& clientEntry : clients) {
		// Don't send to the client who released the lock
		if (clientEntry.second->getClientId() != clientId) {
			clientEntry.second->send(notification);
		}
	}
}

bool LiveServer::verifySectorAccess(uint32_t clientId, int x, int y) {
	try {
		// Convert to sector coordinates
		SectorCoord sector = LiveSectorManager::positionToSector(x, y);
		
		// The host always has access
		if (clientId == 0) {
			return true;
		}
		
		// Check if the client has a lock on this sector
		bool hasLock = sectorManager.hasLock(clientId, sector);
		
		if (hasLock) {
			// Mark the sector as dirty since it's being edited
			sectorManager.markDirty(sector);
			return true;
		}
		
		// At this point, the client doesn't have a lock on this sector
		
		// Get the owner of the sector lock, if any
		uint32_t lockOwner = sectorManager.getLockOwner(sector);
		
		// Log unauthorized access attempt
		logMessage(wxString::Format("[Server]: Client %u attempted unauthorized edit at [%d,%d] in sector [%d,%d]", 
			clientId, x, y, sector.x, sector.y));
		
		if (lockOwner != 0) {
			// There is a conflict - notify the client
			wxTheApp->CallAfter([this, clientId, sector]() {
				handleSectorConflict(clientId, sector);
			});
		}
		
		return false;
	}
	catch (std::exception& e) {
		logMessage(wxString::Format("[Server]: Error in verifySectorAccess: %s", e.what()));
		return false;
	}
}

void LiveServer::handleSectorConflict(uint32_t requestingClient, const SectorCoord& sector) {
	uint32_t lockOwner = sectorManager.getLockOwner(sector);
	
	// Make sure there is actually a conflict
	if (lockOwner == 0 || lockOwner == requestingClient) {
		return;
	}
	
	// Find the client who requested the edit
	LivePeer* requestingPeer = nullptr;
	for (const auto& pair : clients) {
		if (pair.second->getClientId() == requestingClient) {
			requestingPeer = pair.second;
			break;
		}
	}
	
	if (!requestingPeer) {
		return;
	}
	
	// Send conflict notification
	NetworkMessage conflictNotification;
	conflictNotification.write<uint8_t>(PACKET_SECTOR_CONFLICT);
	conflictNotification.write<int32_t>(sector.x);
	conflictNotification.write<int32_t>(sector.y);
	conflictNotification.write<uint32_t>(lockOwner);
	
	requestingPeer->send(conflictNotification);
	
	logMessage(wxString::Format("[Server]: Notified client %u of sector conflict at [%d,%d] with client %u", 
		requestingClient, sector.x, sector.y, lockOwner));
}

void LiveServer::sendSectorSnapshot(uint32_t clientId, const SectorCoord& sector) {
	// Find the nodes in this sector
	std::vector<Position> nodes = sectorManager.getSectorNodes(sector);
	
	// Find the client
	LivePeer* targetPeer = nullptr;
	for (const auto& pair : clients) {
		if (pair.second->getClientId() == clientId) {
			targetPeer = pair.second;
			break;
		}
	}
	
	if (!targetPeer) {
		logMessage(wxString::Format("[Server]: Client %u not found for sector snapshot", clientId));
		return;
	}
	
	// Prepare the header message
	NetworkMessage headerMsg;
	headerMsg.write<uint8_t>(PACKET_SECTOR_SNAPSHOT);
	headerMsg.write<int32_t>(sector.x);
	headerMsg.write<int32_t>(sector.y);
	headerMsg.write<uint32_t>(sectorManager.getSectorVersion(sector));
	headerMsg.write<uint32_t>(nodes.size());
	
	targetPeer->send(headerMsg);
	
	// Now send data for each node
	for (const Position& pos : nodes) {
		QTreeNode* node = editor->map.getLeaf(pos.x, pos.y);
		
		if (node) {
			// Send the node with all visible floors (mask 0xFF means all floors)
			targetPeer->sendNode(clientId, node, pos.x / 4, pos.y / 4, 0xFFFF);
		}
	}
	
	logMessage(wxString::Format("[Server]: Sent sector [%d,%d] snapshot with %zu nodes to client %u", 
		sector.x, sector.y, nodes.size(), clientId));
}

void LiveServer::incrementSectorVersion(const SectorCoord& sector) {
	uint32_t newVersion = sectorManager.incrementSectorVersion(sector);
	
	// Notify all clients of the new version
	NetworkMessage versionUpdate;
	versionUpdate.write<uint8_t>(PACKET_SECTOR_VERSION);
	versionUpdate.write<int32_t>(sector.x);
	versionUpdate.write<int32_t>(sector.y);
	versionUpdate.write<uint32_t>(newVersion);
	
	for (auto& clientEntry : clients) {
		clientEntry.second->send(versionUpdate);
	}
	
	logMessage(wxString::Format("[Server]: Updated sector [%d,%d] to version %u", 
		sector.x, sector.y, newVersion));
}

uint32_t LiveServer::getSectorVersion(const SectorCoord& sector) {
	return sectorManager.getSectorVersion(sector);
}
