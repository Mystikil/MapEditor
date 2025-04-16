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

#include "live_peer.h"
#include "live_server.h"
#include "live_tab.h"
#include "live_action.h"

#include "editor.h"

LivePeer::LivePeer(LiveServer* server, boost::asio::ip::tcp::socket socket) :
	LiveSocket(),
	readMessage(), server(server), socket(std::move(socket)), color(), id(0), clientId(0), connected(false) {
	ASSERT(server != nullptr);
}

LivePeer::~LivePeer() {
	if (socket.is_open()) {
		socket.close();
	}
}

void LivePeer::close() {
	server->removeClient(id);
}

bool LivePeer::handleError(const boost::system::error_code& error) {
	if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
		logMessage(wxString() + getHostName() + ": disconnected.");
		close();
		return true;
	} else if (error == boost::asio::error::connection_aborted) {
		logMessage(name + " have left the server.");
		return true;
	}
	return false;
}

std::string LivePeer::getHostName() const {
	return socket.remote_endpoint().address().to_string();
}

void LivePeer::receiveHeader() {
	readMessage.position = 0;
	
	// Initialize buffer with minimum size to prevent "size 0" errors
	if (readMessage.buffer.size() < 4) {
		readMessage.buffer.resize(1024);
	}
	
	boost::asio::async_read(socket, boost::asio::buffer(readMessage.buffer, 4), [this](const boost::system::error_code& error, size_t bytesReceived) -> void {
		if (error) {
			if (!handleError(error)) {
				logMessage(wxString() + getHostName() + ": " + error.message());
			}
		} else if (bytesReceived < 4) {
			logMessage(wxString::Format("[Client %s]: Could not receive header [size: %zu], attempting to recover...", 
				getHostName(), bytesReceived));
				
			// Try to recover by restarting the header receive process
			if (bytesReceived > 0) {
				wxTheApp->CallAfter([this]() {
					receiveHeader();
				});
			} else {
				// If we received 0 bytes, wait a bit and try again
				wxTheApp->CallAfter([this]() {
					logMessage("Attempting to reconnect header reader...");
					receiveHeader();
				});
			}
		} else {
			// Successfully received header
			uint32_t packetSize = readMessage.read<uint32_t>();
			logMessage(wxString::Format("[Client %s]: Received header, packet size: %u bytes", 
				getHostName(), packetSize));
				
			// Prevent empty packet errors
			if (packetSize == 0) {
				logMessage("[Client]: Empty packet received, skipping and waiting for next header");
				wxTheApp->CallAfter([this]() {
					receiveHeader();
				});
			} else {
				receive(packetSize);
			}
		}
	});
}

void LivePeer::receive(uint32_t packetSize) {
	// Ensure buffer is large enough
	readMessage.buffer.resize(readMessage.position + packetSize);
	
	boost::asio::async_read(socket, boost::asio::buffer(&readMessage.buffer[readMessage.position], packetSize), 
		[this, packetSize](const boost::system::error_code& error, size_t bytesReceived) -> void {
		if (error) {
			if (!handleError(error)) {
				logMessage(wxString::Format("[Client %s]: Network error: %s", 
					getHostName(), error.message()));
			}
		} else if (bytesReceived < packetSize) {
			logMessage(wxString::Format("[Client %s]: Incomplete packet received [got: %zu, expected: %zu], attempting recovery...",
				getHostName(), bytesReceived, packetSize));
			
			// Try to recover by restarting the read process
			if (bytesReceived > 0) {
				// We received some data, attempt to continue from where we left off
				uint32_t remainingBytes = packetSize - bytesReceived;
				wxTheApp->CallAfter([this, remainingBytes, bytesReceived]() {
					logMessage(wxString::Format("Attempting to receive remaining %u bytes...", remainingBytes));
					// Continue reading the remaining bytes
					boost::asio::async_read(socket, 
						boost::asio::buffer(&readMessage.buffer[readMessage.position + bytesReceived], remainingBytes),
						[this](const boost::system::error_code& innerError, size_t innerBytesReceived) {
							if (!innerError && innerBytesReceived > 0) {
								logMessage("Successfully recovered partial packet");
								wxTheApp->CallAfter([this]() {
									if (connected) {
										parseEditorPacket(std::move(readMessage));
									} else {
										parseLoginPacket(std::move(readMessage));
									}
									receiveHeader();
								});
							} else {
								logMessage("Recovery failed, restarting header read");
								wxTheApp->CallAfter([this]() {
									receiveHeader();
								});
							}
						}
					);
				});
			} else {
				// If we received 0 bytes, just restart the header read
				wxTheApp->CallAfter([this]() {
					logMessage("No data received, restarting header read");
					receiveHeader();
				});
			}
		} else {
			// Successfully received the complete packet
			logMessage(wxString::Format("[Client %s]: Successfully received complete packet (%zu bytes)", 
				getHostName(), bytesReceived));
				
			wxTheApp->CallAfter([this]() {
				if (connected) {
					parseEditorPacket(std::move(readMessage));
				} else {
					parseLoginPacket(std::move(readMessage));
				}
				receiveHeader();
			});
		}
	});
}

void LivePeer::send(NetworkMessage& message) {
	// Validate message size to avoid sending empty messages
	if (message.size == 0) {
		logMessage("[Server]: Attempted to send empty message, ignoring");
		return;
	}
	
	// Write size to the first 4 bytes (header)
	memcpy(&message.buffer[0], &message.size, 4);
	
	// Only log non-cursor packets to reduce excessive logging
	bool isCursorPacket = message.buffer.size() > 0 && message.buffer[4] == PACKET_CURSOR_UPDATE;
	if (!isCursorPacket) {
		logMessage(wxString::Format("[Server]: Sending packet to %s (size: %zu bytes, type: 0x%02X)", 
			getHostName(), message.size + 4, message.buffer[4]));
	}
	
	try {
		boost::asio::async_write(socket, 
			boost::asio::buffer(message.buffer, message.size + 4), 
			[this, msgSize = message.size, isCursorPacket](const boost::system::error_code& error, size_t bytesTransferred) -> void {
				if (error) {
					logMessage(wxString::Format("[Server]: Error sending packet to %s: %s", 
						getHostName(), error.message()));
				} else if (bytesTransferred != msgSize + 4) {
					logMessage(wxString::Format("[Server]: Incomplete packet sent to %s [sent: %zu, expected: %zu]", 
						getHostName(), bytesTransferred, msgSize + 4));
				} else if (!isCursorPacket) {
					// Only log successful sends for non-cursor packets
					logMessage(wxString::Format("[Server]: Successfully sent packet to %s (%zu bytes)", 
						getHostName(), bytesTransferred));
				}
			}
		);
	} catch (std::exception& e) {
		logMessage(wxString::Format("[Server]: Exception sending packet to %s: %s", 
			getHostName(), e.what()));
	}
}

void LivePeer::parseLoginPacket(NetworkMessage message) {
	logMessage(wxString::Format("[Server]: Parsing login packet from %s (buffer size: %zu, position: %zu)", 
		getHostName(), message.buffer.size(), message.position));
		
	if (message.buffer.size() <= message.position) {
		logMessage("[Server]: Empty packet received in login parser, disconnecting");
		close();
		return;
	}
		
	try {
		// Read just one packet type - login packets should only contain a single command
		uint8_t packetType = message.read<uint8_t>();
		logMessage(wxString::Format("[Server]: Login packet type: 0x%02X", packetType));
		
		switch (packetType) {
			case PACKET_HELLO_FROM_CLIENT:
				logMessage("[Server]: Received hello packet from client");
				parseHello(message);
				break;
			case PACKET_READY_CLIENT:
				logMessage("[Server]: Received ready packet from client");
				parseReady(message);
				break;
			default: {
				logMessage(wxString::Format("[Server]: Invalid login packet received (type: 0x%02X), connection severed", 
					packetType));
				close();
				break;
			}
		}
	} catch (std::exception& e) {
		logMessage(wxString::Format("[Server]: Exception while parsing login packet: %s", e.what()));
		close();
	}
}

void LivePeer::parseEditorPacket(NetworkMessage message) {
	uint8_t packetType;
	while (message.position < message.buffer.size()) {
		packetType = message.read<uint8_t>();
		switch (packetType) {
			case PACKET_REQUEST_NODES:
				parseNodeRequest(message);
				break;
			case PACKET_CHANGE_LIST:
				parseReceiveChanges(message);
				break;
			case PACKET_ADD_HOUSE:
				parseAddHouse(message);
				break;
			case PACKET_EDIT_HOUSE:
				parseEditHouse(message);
				break;
			case PACKET_REMOVE_HOUSE:
				parseRemoveHouse(message);
				break;
			case PACKET_CLIENT_UPDATE_CURSOR:
				parseCursorUpdate(message);
				break;
			case PACKET_CLIENT_TALK:
				parseChatMessage(message);
				break;
			case PACKET_CLIENT_COLOR_UPDATE:
				parseClientColorUpdate(message);
				break;
			default: {
				log->Message("Invalid editor packet receieved, connection severed.");
				close();
				break;
			}
		}
	}
}

void LivePeer::parseHello(NetworkMessage& message) {
	if (connected) {
		logMessage("[Server]: Client already connected but sent hello again, disconnecting");
		close();
		return;
	}

	// Calculate remaining data size
	size_t remainingBytes = message.buffer.size() - message.position;
	logMessage(wxString::Format("[Server]: Hello packet data - %zu bytes remaining to parse", remainingBytes));
	
	// Minimum data needed:
	// 3x uint32_t = 12 bytes (version info)
	// 2x string = variable (name, password), but at least 2 bytes for length
	// Total minimum: 14 bytes
	if (remainingBytes < 14) {
		logMessage(wxString::Format("[Server]: Hello packet is too small (%zu bytes), disconnecting", remainingBytes));
		close();
		return;
	}

	try {
		// Log current position for debugging
		size_t startPos = message.position;
		logMessage(wxString::Format("[Server]: Reading packet from position %zu", startPos));
		
		// Read and validate the RME version
		uint32_t rmeVersion = message.read<uint32_t>();
		logMessage(wxString::Format("[Server]: Client RME version: %u (server: %u), new position: %zu", 
			rmeVersion, __RME_VERSION_ID__, message.position));
			
		if (rmeVersion != __RME_VERSION_ID__) {
			logMessage("[Server]: Client using incompatible RME version, rejecting");
			NetworkMessage outMessage;
			outMessage.write<uint8_t>(PACKET_KICK);
			outMessage.write<std::string>("Wrong editor version.");

			send(outMessage);
			close();
			return;
		}

		// Read and validate the network protocol version
		uint32_t netVersion = message.read<uint32_t>();
		logMessage(wxString::Format("[Server]: Client net protocol version: %u (server: %u), new position: %zu", 
			netVersion, __LIVE_NET_VERSION__, message.position));
			
		if (netVersion != __LIVE_NET_VERSION__) {
			logMessage("[Server]: Client using incompatible net protocol version, rejecting");
			NetworkMessage outMessage;
			outMessage.write<uint8_t>(PACKET_KICK);
			outMessage.write<std::string>("Wrong protocol version.");

			send(outMessage);
			close();
			return;
		}

		// Read the client version
		uint32_t clientVersion = message.read<uint32_t>();
		
		// Check if we have enough bytes for at least the string lengths
		if (message.position + 4 > message.buffer.size()) {
			logMessage("[Server]: Hello packet truncated after client version, disconnecting");
			close();
			return;
		}
		
		// Read nickname
		std::string nickname;
		try {
			nickname = message.read<std::string>();
			logMessage(wxString::Format("[Server]: Read nickname: '%s', new position: %zu", nickname, message.position));
		} catch (std::exception& e) {
			logMessage(wxString::Format("[Server]: Failed to read nickname: %s", e.what()));
			close();
			return;
		}
		
		// Check if we have enough bytes for the password
		if (message.position >= message.buffer.size()) {
			logMessage("[Server]: Hello packet truncated after nickname, disconnecting");
			close();
			return;
		}
		
		// Read password
		std::string password;
		try {
			password = message.read<std::string>();
			logMessage(wxString::Format("[Server]: Read password, length: %zu, new position: %zu", 
				password.length(), message.position));
		} catch (std::exception& e) {
			logMessage(wxString::Format("[Server]: Failed to read password: %s", e.what()));
			close();
			return;
		}
		
		logMessage(wxString::Format("[Server]: Client login - Name: %s, Client version: %u", 
			nickname, clientVersion));

		// Check password
		if (server->getPassword() != wxString(password.c_str(), wxConvUTF8)) {
			logMessage(wxString::Format("[Server]: Client %s used wrong password, connection refused", nickname));
			NetworkMessage outMessage;
			outMessage.write<uint8_t>(PACKET_KICK);
			outMessage.write<std::string>("Invalid password.");

			send(outMessage);
			close();
			return;
		}

		// Set client name and notify
		name = wxString(nickname.c_str(), wxConvUTF8);
		log->Message(name + " (" + getHostName() + ") connected.");

		// Send appropriate response
		NetworkMessage outMessage;
		if (static_cast<ClientVersionID>(clientVersion) != g_gui.GetCurrentVersionID()) {
			logMessage(wxString::Format("[Server]: Client version mismatch, requesting client to change version from %u to %u", 
				clientVersion, g_gui.GetCurrentVersionID()));
				
			outMessage.write<uint8_t>(PACKET_CHANGE_CLIENT_VERSION);
			outMessage.write<uint32_t>(g_gui.GetCurrentVersionID());
		} else {
			logMessage("[Server]: Client version matches, accepting connection");
			outMessage.write<uint8_t>(PACKET_ACCEPTED_CLIENT);
		}
		send(outMessage);
		logMessage("[Server]: Sent response to client hello");
		
	} catch (std::exception& e) {
		logMessage(wxString::Format("[Server]: Error parsing hello packet: %s", e.what()));
		close();
	}
}

void LivePeer::parseReady(NetworkMessage& message) {
	// Safety check - client already connected
	if (connected) {
		logMessage("[Server]: Client already connected but sent READY packet again, disconnecting");
		close();
		return;
	}

	try {
		// Mark client as connected
		connected = true;
		logMessage(wxString::Format("[Server]: Client %s entering READY state", getHostName()));

		// Find free client id
		clientId = server->getFreeClientId();
		if (clientId == 0) {
			logMessage("[Server]: No free client IDs available, server is full");
			NetworkMessage outMessage;
			outMessage.write<uint8_t>(PACKET_KICK);
			outMessage.write<std::string>("Server is full.");

			send(outMessage);
			close();
			return;
		}

		// Assign a default color to the new client
		color = wxColor(
			128 + rand() % 127,  // R: 128-255
			128 + rand() % 127,  // G: 128-255
			128 + rand() % 127,  // B: 128-255
			255                  // A: fully opaque
		);

		// Update the client list in the UI
		server->updateClientList();
		logMessage(wxString::Format("[Server]: Assigned client ID %u to %s", clientId, getHostName()));

		// Send HELLO_FROM_SERVER packet with map information
		try {
			NetworkMessage outMessage;
			outMessage.write<uint8_t>(PACKET_HELLO_FROM_SERVER);

			Map& map = server->getEditor()->map;
			outMessage.write<std::string>(map.getName());
			outMessage.write<uint16_t>(map.getWidth());
			outMessage.write<uint16_t>(map.getHeight());

			send(outMessage);
			logMessage("[Server]: Sent HELLO packet with map information to client");
		} catch (std::exception& e) {
			logMessage(wxString::Format("[Server]: Error sending HELLO packet: %s", e.what()));
			close();
			return;
		}
		
		// Now send the host's cursor to the new client
		try {
			LiveCursor hostCursor;
			hostCursor.id = 0;
			hostCursor.color = server->getUsedColor();
			hostCursor.pos = Position(); // Default position
			
			NetworkMessage cursorMessage;
			cursorMessage.write<uint8_t>(PACKET_CURSOR_UPDATE);
			server->writeCursorToMessage(cursorMessage, hostCursor);
			send(cursorMessage);
			logMessage("[Server]: Sent host cursor information to client");
		} catch (std::exception& e) {
			logMessage(wxString::Format("[Server]: Error sending cursor information: %s", e.what()));
			// Non-fatal error, continue
		}

		// Send ACCEPTED_CLIENT packet to confirm client is ready for drawing operations
		try {
			NetworkMessage acceptedMessage;
			acceptedMessage.write<uint8_t>(PACKET_ACCEPTED_CLIENT);
			send(acceptedMessage);
			logMessage(wxString::Format("[Server]: Client %s (ID: %u) is now fully connected and ready", 
				name, clientId));
		} catch (std::exception& e) {
			logMessage(wxString::Format("[Server]: Error sending ACCEPTED packet: %s", e.what()));
			close();
			return;
		}
		
		// Also send the colors of all existing clients to the new client
		try {
			for (const auto& clientPair : server->getClients()) {
				LivePeer* peer = clientPair.second;
				if (peer && peer->getClientId() != 0 && peer->getClientId() != clientId) {
					// Send this client's color
					NetworkMessage colorMessage;
					colorMessage.write<uint8_t>(PACKET_COLOR_UPDATE);
					colorMessage.write<uint32_t>(peer->getClientId());
					colorMessage.write<uint8_t>(peer->getUsedColor().Red());
					colorMessage.write<uint8_t>(peer->getUsedColor().Green());
					colorMessage.write<uint8_t>(peer->getUsedColor().Blue());
					colorMessage.write<uint8_t>(peer->getUsedColor().Alpha());
					send(colorMessage);
					
					// Also send this client's cursor position
					LiveCursor peerCursor;
					peerCursor.id = peer->getClientId();
					peerCursor.color = peer->getUsedColor();
					peerCursor.pos = Position(); // Default position
					
					NetworkMessage peerCursorMessage;
					peerCursorMessage.write<uint8_t>(PACKET_CURSOR_UPDATE);
					server->writeCursorToMessage(peerCursorMessage, peerCursor);
					send(peerCursorMessage);
				}
			}
			logMessage("[Server]: Sent information about other clients to the new client");
		} catch (std::exception& e) {
			logMessage(wxString::Format("[Server]: Error sending client information: %s", e.what()));
			// Non-fatal error, continue
		}
	} catch (std::exception& e) {
		// Handle any other unexpected errors during client initialization
		logMessage(wxString::Format("[Server]: Error initializing client: %s", e.what()));
		close();
	}
}

void LivePeer::parseNodeRequest(NetworkMessage& message) {
	Map& map = server->getEditor()->map;
	for (uint32_t nodes = message.read<uint32_t>(); nodes != 0; --nodes) {
		uint32_t ind = message.read<uint32_t>();

		int32_t ndx = ind >> 18;
		int32_t ndy = (ind >> 4) & 0x3FFF;
		bool underground = ind & 1;

		QTreeNode* node = map.createLeaf(ndx * 4, ndy * 4);
		if (node) {
			sendNode(clientId, node, ndx, ndy, underground ? 0xFF00 : 0x00FF);
		}
	}
}

void LivePeer::parseReceiveChanges(NetworkMessage& message) {
	try {
		// Read the change data
		const std::string& data = message.read<std::string>();
		
		// Log the change reception
		logMessage(wxString::Format("[Server]: Received changes from client %s (data size: %zu bytes)", 
			name, data.size()));
			
		// Process the changes on the main thread
		wxTheApp->CallAfter([this, data]() {
			if (!server || !server->getEditor()) {
				logMessage("[Server]: Error - cannot process changes, editor not available");
				return;
			}
			
			Editor& editor = *server->getEditor();
			
			try {
				// Create the action to hold the changes
				NetworkedAction* action = static_cast<NetworkedAction*>(editor.actionQueue->createAction(ACTION_REMOTE));
				action->owner = clientId;
				
				// Parse the change data and create the tiles
				mapReader.assign(reinterpret_cast<const uint8_t*>(data.c_str() - 1), data.size());
				BinaryNode* rootNode = mapReader.getRootNode();
				BinaryNode* tileNode = rootNode->getChild();
				
				bool anyChanges = false;
				
				if (tileNode) {
					do {
						Tile* tile = readTile(tileNode, editor, nullptr);
						if (tile) {
							action->addChange(newd Change(tile));
							anyChanges = true;
						}
					} while (tileNode->advance());
				}
				mapReader.close();
				
				// Only add the action if we have changes
				if (anyChanges) {
					editor.actionQueue->addAction(action);
					g_gui.RefreshView();
					g_gui.UpdateMinimap();
					
					logMessage(wxString::Format("[Server]: Successfully processed changes from client %s", name));
				} else {
					// Instead of deleting, just add the action as-is
					// An empty action won't do anything but will be safely managed by the queue
					editor.actionQueue->addAction(action);
					logMessage(wxString::Format("[Server]: No valid changes found in packet from client %s", name));
				}
			} catch (std::exception& e) {
				logMessage(wxString::Format("[Server]: Error processing changes: %s", e.what()));
			}
		});
	} catch (std::exception& e) {
		logMessage(wxString::Format("[Server]: Error parsing changes packet: %s", e.what()));
	}
}

void LivePeer::parseAddHouse(NetworkMessage& message) {
}

void LivePeer::parseEditHouse(NetworkMessage& message) {
}

void LivePeer::parseRemoveHouse(NetworkMessage& message) {
}

void LivePeer::parseCursorUpdate(NetworkMessage& message) {
	LiveCursor cursor = readCursor(message);
	cursor.id = clientId;

	// Only log and update client list if the color changes, not for movement
	if (cursor.color != color) {
		setUsedColor(cursor.color);
		server->updateClientList();
		// Only log color changes, not cursor movements
		logMessage(wxString::Format("[Server]: Client %s changed cursor color", name));
	}

	server->broadcastCursor(cursor);
	g_gui.RefreshView();
}

void LivePeer::parseChatMessage(NetworkMessage& message) {
	const std::string& chatMessage = message.read<std::string>();
	
	// Log the received message for debugging
	logMessage(wxString::Format("Chat message received from %s: %s", 
		name, wxstr(chatMessage)));
	
	// Broadcast the chat message to all clients including the sender
	server->broadcastChat(name, wxstr(chatMessage));
}

void LivePeer::parseClientColorUpdate(NetworkMessage& message) {
	// Read the target client ID
	uint32_t targetClientId = message.read<uint32_t>();
	
	// Read the color components
	uint8_t r = message.read<uint8_t>();
	uint8_t g = message.read<uint8_t>();
	uint8_t b = message.read<uint8_t>();
	uint8_t a = message.read<uint8_t>();
	wxColor newColor(r, g, b, a);
	
	// Log the request
	logMessage(wxString::Format("[Server]: Client %s requested color change for client %u to RGB(%d,%d,%d)", 
		name, targetClientId, r, g, b));
	
	// If the client is changing their own color
	if (targetClientId == clientId) {
		// Update the client's color
		setUsedColor(newColor);
		
		// Log the change
		logMessage(wxString::Format("[Server]: Updated color for client %s (ID: %u)", 
			name, clientId));
	}
	
	// Broadcast the color change to all clients
	server->broadcastColorChange(targetClientId, newColor);
	
	// Update the client list to reflect the change in the UI
	server->updateClientList();
}

void LivePeer::sendChat(const wxString& chatMessage) {
	// For a peer, sending a chat message means forwarding it to the server
	// which will then broadcast it to all clients
	NetworkMessage message;
	message.write<uint8_t>(PACKET_CLIENT_TALK);
	message.write<std::string>(nstr(chatMessage));
	send(message);
}

void LivePeer::updateCursor(const Position& position) {
	// Forward cursor updates to the server
	LiveCursor cursor;
	cursor.id = clientId;
	cursor.pos = position;
	
	// Use client's color for the cursor
	cursor.color = color;
	
	server->broadcastCursor(cursor);
}
