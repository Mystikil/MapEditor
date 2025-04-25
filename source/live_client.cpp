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

#include "live_client.h"
#include "live_tab.h"
#include "live_action.h"
#include "editor.h"

#include <wx/event.h>

LiveClient::LiveClient() :
	LiveSocket(),
	readMessage(), queryNodeList(), currentOperation(),
	resolver(nullptr), socket(nullptr), editor(nullptr), stopped(false) {
	// Initialize buffer with minimum size to prevent "size 0" errors
	readMessage.buffer.resize(1024);
	readMessage.position = 0;
}

LiveClient::~LiveClient() {
	//
}

bool LiveClient::connect(const std::string& address, uint16_t port) {
	NetworkConnection& connection = NetworkConnection::getInstance();
	if (!connection.start()) {
		setLastError("The previous connection has not been terminated yet.");
		return false;
	}

	auto& service = connection.get_service();
	if (!resolver) {
		resolver = std::make_shared<boost::asio::ip::tcp::resolver>(service);
	}

	if (!socket) {
		socket = std::make_shared<boost::asio::ip::tcp::socket>(service);
	}
	
	// Set socket options for better error detection
	boost::system::error_code ec;
	boost::asio::socket_base::keep_alive keepalive_option(true);
	socket->set_option(keepalive_option, ec);
	if (ec) {
		logMessage("Warning: Could not set keep_alive option: " + ec.message());
	}
	
	// Set connection timeout options
	boost::asio::socket_base::linger linger_option(true, 10);
	socket->set_option(linger_option, ec);
	if (ec) {
		logMessage("Warning: Could not set linger option: " + ec.message());
	}

	// Log connection attempt
	logMessage(wxString::Format("Attempting to connect to %s:%d...", address, port));

	resolver->async_resolve(
		address, 
		std::to_string(port),
		[this](const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type results) -> void {
			if (error) {
				wxString errorMsg = wxString::Format("Resolution error: %s", error.message());
				logMessage(errorMsg);
			} else {
				logMessage(wxString::Format("Host resolved. Connecting to endpoint..."));
				tryConnect(results.begin());
			}
	});

	/*
	if(!client->WaitOnConnect(5, 0)) {
		if(log)
			log->Disconnect();
		last_err = "Connection timed out.";
		client->Destroy();
		client = nullptr;
		delete connection;
		return false;
	}

	if(!client->IsConnected()) {
		if(log)
			log->Disconnect();
		last_err = "Connection refused by peer.";
		client->Destroy();
		client = nullptr;
		delete connection;
		return false;
	}

	if(log)
		log->Message("Connection established!");
	*/
	return true;
}

void LiveClient::tryConnect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iterator) {
	if (stopped) {
		logMessage("Connection attempt aborted: Connection was stopped.");
		return;
	}

	if (endpoint_iterator == boost::asio::ip::tcp::resolver::results_type::iterator()) {
		logMessage("Connection attempt failed: No more endpoints to try.");
		return;
	}

	auto next_endpoint = endpoint_iterator;
	next_endpoint++;

	logMessage("Joining server " + endpoint_iterator->host_name() + ":" + endpoint_iterator->service_name() + "...");

	// Connect to a single endpoint
	socket->async_connect(
		*endpoint_iterator,
		[this, next_endpoint](const boost::system::error_code& error) -> void {
		if (!socket->is_open()) {
			logMessage(wxString::Format("Connection failed: Socket is not open. Trying next endpoint..."));
			tryConnect(next_endpoint);
		} else if (error) {
			wxString errorMsg = wxString::Format("Connection error: %s (code: %d)", error.message(), error.value());
			logMessage(errorMsg);
			
			if (handleError(error)) {
				logMessage("Trying next endpoint...");
				tryConnect(next_endpoint);
			} else {
				wxTheApp->CallAfter([this]() {
					logMessage("All connection attempts failed. Closing connection.");
					close();
					g_gui.CloseLiveEditors(this);
				});
			}
		} else {
			boost::system::error_code ec;
			boost::asio::ip::tcp::no_delay option(true);
			socket->set_option(option, ec);
			if (ec) {
				logMessage(wxString::Format("Warning: Could not set TCP no_delay option: %s", ec.message()));
				wxTheApp->CallAfter([this]() {
					close();
				});
				return;
			}
			
			logMessage("Connection established successfully. Sending hello packet...");
			sendHello();
			receiveHeader();
		}
	});
}

void LiveClient::close() {
	if (resolver) {
		resolver->cancel();
	}

	if (socket) {
		socket->close();
	}

	if (log) {
		log->Message("Disconnected from server.");
		log->Disconnect();
		log = nullptr;
	}

	stopped = true;
}

bool LiveClient::handleError(const boost::system::error_code& error) {
	if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
		wxTheApp->CallAfter([this]() {
			log->Message(wxString() + getHostName() + ": disconnected.");
			close();
		});
		return true;
	} else if (error == boost::asio::error::connection_aborted) {
		logMessage("You have left the server.");
		return true;
	}
	return false;
}

std::string LiveClient::getHostName() const {
	if (!socket) {
		return "not connected";
	}
	return socket->remote_endpoint().address().to_string();
}

void LiveClient::receiveHeader() {
	// Make sure buffer is properly initialized
	if (readMessage.buffer.size() < 4) {
		readMessage.buffer.resize(1024);
	}
	
	readMessage.position = 0;
	
	// Check if socket is valid before attempting to read
	if (!socket || !socket->is_open()) {
		logMessage("[Client]: Cannot receive header: Socket is not open");
		return;
	}
	
	try {
		logMessage("[Client]: Waiting for incoming packet header...");
		
		boost::asio::async_read(*socket, boost::asio::buffer(readMessage.buffer, 4), 
			[this](const boost::system::error_code& error, size_t bytesReceived) -> void {
				if (error) {
					if (!handleError(error)) {
						wxString errorMsg = wxString::Format("[Client]: Network error: %s (code: %d)", 
							error.message(), error.value());
						logMessage(errorMsg);
					}
				} else if (bytesReceived < 4) {
					wxString errorMsg = wxString::Format("[Client]: Could not receive header [size: %zu], recovery needed", 
													bytesReceived);
					logMessage(errorMsg);
					
					// Additional debug information about the connection
					logMessage(wxString::Format("[Client]: Connection details: Host=%s, Buffer size=%zu", 
											   getHostName(), readMessage.buffer.size()));
					
					// If we received some data but not enough, try again
					if (bytesReceived > 0) {
						logMessage("[Client]: Partial header received, attempting recovery...");
						wxTheApp->CallAfter([this]() {
							receiveHeader();
						});
					} else {
						logMessage("[Client]: No data received, attempting direct reconnection...");
						wxTheApp->CallAfter([this]() {
							// Try to reconnect if possible
							try {
								receiveHeader();
							} catch (std::exception& e) {
								logMessage(wxString::Format("[Client]: Recovery failed: %s", e.what()));
								close();
							}
						});
					}
				} else {
					// Successfully received header, now receive the packet
					uint32_t packetSize = readMessage.read<uint32_t>();
					logMessage(wxString::Format("[Client]: Received header, packet size: %u bytes", packetSize));
					
					// Check for zero packet size
					if (packetSize == 0) {
						logMessage("[Client]: Received zero-size packet, skipping and waiting for next header");
						wxTheApp->CallAfter([this]() {
							receiveHeader();
						});
					} else {
						receive(packetSize);
					}
				}
		});
	} catch (std::exception& e) {
		logMessage(wxString::Format("[Client]: Exception in receiveHeader: %s", e.what()));
		close();
	}
}

void LiveClient::receive(uint32_t packetSize) {
	// Safety check for packet size
	if (packetSize > 1024 * 1024) { // 1MB sanity check
		logMessage(wxString::Format("[Client]: Suspiciously large packet size received: %u bytes, aborting", packetSize));
		close();
		return;
	}

	// Resize buffer to accommodate the incoming packet
	readMessage.buffer.resize(readMessage.position + packetSize);
	
	logMessage(wxString::Format("[Client]: Reading packet body (%u bytes)", packetSize));
	
	boost::asio::async_read(*socket, boost::asio::buffer(&readMessage.buffer[readMessage.position], packetSize), 
	[this, packetSize](const boost::system::error_code& error, size_t bytesReceived) -> void {
		if (error) {
			if (!handleError(error)) {
				wxString errorMsg = wxString::Format("[Client]: Network error reading packet: %s", error.message());
				logMessage(errorMsg);
			}
		} else if (bytesReceived < packetSize) {
			wxString errorMsg = wxString::Format("[Client]: Incomplete packet received [got: %zu, expected: %u], attempting recovery",
				bytesReceived, packetSize);
			logMessage(errorMsg);
			
			// Log packet details for debugging
			logMessage(wxString::Format("[Client]: Packet details: Buffer size=%zu, Position=%zu", 
				readMessage.buffer.size(), readMessage.position));
			
			// Try to recover by reading the remaining bytes
			uint32_t remainingBytes = packetSize - bytesReceived;
			wxTheApp->CallAfter([this, remainingBytes, bytesReceived]() {
				logMessage(wxString::Format("[Client]: Attempting to receive remaining %u bytes...", remainingBytes));
				
				boost::asio::async_read(*socket, 
					boost::asio::buffer(&readMessage.buffer[readMessage.position + bytesReceived], remainingBytes),
					[this](const boost::system::error_code& innerError, size_t innerBytesReceived) {
						if (!innerError && innerBytesReceived > 0) {
							logMessage("[Client]: Successfully recovered partial packet");
							wxTheApp->CallAfter([this]() {
								parsePacket(std::move(readMessage));
								receiveHeader();
							});
						} else {
							logMessage("[Client]: Recovery failed, restarting from header");
							wxTheApp->CallAfter([this]() {
								receiveHeader();
							});
						}
					}
				);
			});
		} else {
			// Successfully received the complete packet
			logMessage(wxString::Format("[Client]: Successfully received complete packet (%zu bytes)", bytesReceived));
			
			wxTheApp->CallAfter([this]() {
				parsePacket(std::move(readMessage));
				receiveHeader();
			});
		}
	});
}

void LiveClient::send(NetworkMessage& message) {
	// Validate message size to avoid sending empty messages
	if (message.size == 0) {
		logMessage("[Client]: Attempted to send empty message, ignoring");
		return;
	}
	
	// Write size to the first 4 bytes (header)
	memcpy(&message.buffer[0], &message.size, 4);
	
	// Log the message we're sending
	logMessage(wxString::Format("[Client]: Sending packet to server (size: %zu bytes)", 
		message.size + 4));
	
	try {
		boost::asio::async_write(*socket, 
			boost::asio::buffer(message.buffer, message.size + 4), 
			[this, msgSize = message.size](const boost::system::error_code& error, size_t bytesTransferred) -> void {
				if (error) {
					logMessage(wxString::Format("[Client]: Error sending packet to server: %s", 
						error.message()));
				} else if (bytesTransferred != msgSize + 4) {
					logMessage(wxString::Format("[Client]: Incomplete packet sent to server [sent: %zu, expected: %zu]", 
						bytesTransferred, msgSize + 4));
				} else {
					logMessage(wxString::Format("[Client]: Successfully sent packet to server (%zu bytes)", 
						bytesTransferred));
				}
			}
		);
	} catch (std::exception& e) {
		logMessage(wxString::Format("[Client]: Exception sending packet to server: %s", e.what()));
	}
}

void LiveClient::updateCursor(const Position& position) {
	LiveCursor cursor;
	cursor.id = 77; // Unimportant, server fixes it for us
	cursor.pos = position;
	cursor.color = wxColor(
		g_settings.getInteger(Config::CURSOR_RED),
		g_settings.getInteger(Config::CURSOR_GREEN),
		g_settings.getInteger(Config::CURSOR_BLUE),
		g_settings.getInteger(Config::CURSOR_ALPHA)
	);

	NetworkMessage message;
	message.write<uint8_t>(PACKET_CLIENT_UPDATE_CURSOR);
	writeCursor(message, cursor);

	send(message);
}

LiveLogTab* LiveClient::createLogWindow(wxWindow* parent) {
	MapTabbook* mtb = dynamic_cast<MapTabbook*>(parent);
	ASSERT(mtb);

	log = newd LiveLogTab(mtb, this);
	log->Message("New Live mapping session started.");

	return log;
}

MapTab* LiveClient::createEditorWindow() {
	MapTabbook* mtb = dynamic_cast<MapTabbook*>(g_gui.tabbook);
	ASSERT(mtb);

	MapTab* edit = newd MapTab(mtb, editor);
	edit->OnSwitchEditorMode(g_gui.IsSelectionMode() ? SELECTION_MODE : DRAWING_MODE);

	return edit;
}

void LiveClient::sendHello() {
	logMessage("[Client]: Preparing hello packet...");
	
	NetworkMessage message;
	message.write<uint8_t>(PACKET_HELLO_FROM_CLIENT);
	message.write<uint32_t>(__RME_VERSION_ID__);
	message.write<uint32_t>(__LIVE_NET_VERSION__);
	message.write<uint32_t>(g_gui.GetCurrentVersionID());
	message.write<std::string>(nstr(name));
	message.write<std::string>(nstr(password));
	
	// Calculate overall packet size for logging
	size_t packetSize = message.size + 4; // Including header size
	
	logMessage(wxString::Format("[Client]: Sending hello packet (size: %zu bytes, name: %s)", 
		packetSize, name));
		
	try {
		send(message);
		logMessage("[Client]: Hello packet sent successfully");
	} catch (std::exception& e) {
		logMessage(wxString::Format("[Client]: Error sending hello packet: %s", e.what()));
	}
}

void LiveClient::sendNodeRequests() {
	if (queryNodeList.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_REQUEST_NODES);

	message.write<uint32_t>(queryNodeList.size());
	for (uint32_t node : queryNodeList) {
		message.write<uint32_t>(node);
	}

	send(message);
	queryNodeList.clear();
}

void LiveClient::sendChanges(DirtyList& dirtyList) {
	ChangeList& changeList = dirtyList.GetChanges();
	if (changeList.empty()) {
		return;
	}

	try {
		// Count tiles for logging
		int tileCount = 0;

		// Reset the writer and serialize changes
		mapWriter.reset();
		
		for (Change* change : changeList) {
			switch (change->getType()) {
				case CHANGE_TILE: {
					const Position& position = static_cast<Tile*>(change->getData())->getPosition();
					sendTile(mapWriter, editor->map.getTile(position), &position);
					tileCount++;
					break;
				}
				default:
					break;
			}
		}
		mapWriter.endNode();
		
		// Create and send the message
		NetworkMessage message;
		message.write<uint8_t>(PACKET_CHANGE_LIST);
		
		std::string data(reinterpret_cast<const char*>(mapWriter.getMemory()), mapWriter.getSize());
		message.write<std::string>(data);
		
		logMessage(wxString::Format("[Client]: Sending %d tile changes to server (data size: %zu bytes)",
			tileCount, data.size()));
			
		send(message);
	} catch (std::exception& e) {
		logMessage(wxString::Format("[Client]: Error sending changes: %s", e.what()));
	}
}

void LiveClient::sendChat(const wxString& chatMessage) {
	// Don't send empty messages
	if (chatMessage.IsEmpty()) {
		return;
	}

	logMessage(wxString::Format("Sending chat message: %s", chatMessage));

	NetworkMessage message;
	message.write<uint8_t>(PACKET_CLIENT_TALK);
	message.write<std::string>(nstr(chatMessage));
	send(message);
}

void LiveClient::sendReady() {
	NetworkMessage message;
	message.write<uint8_t>(PACKET_READY_CLIENT);
	send(message);
}

void LiveClient::queryNode(int32_t ndx, int32_t ndy, bool underground) {
	uint32_t nd = 0;
	nd |= ((ndx >> 2) << 18);
	nd |= ((ndy >> 2) << 4);
	nd |= (underground ? 1 : 0);
	queryNodeList.insert(nd);
}

void LiveClient::parsePacket(NetworkMessage message) {
	uint8_t packetType;
	
	try {
		while (message.position < message.buffer.size()) {
			// Log packet position for debugging
			size_t packetStart = message.position;
			
			// Check if we have at least 1 byte to read the packet type
			if (message.position + 1 > message.buffer.size()) {
				logMessage("[Client]: Warning - incomplete packet at end of buffer, ignoring");
				break;
			}
			
			packetType = message.read<uint8_t>();
			logMessage(wxString::Format("[Client]: Parsing packet type 0x%02X at position %zu", 
				packetType, packetStart));
				
			try {
				switch (packetType) {
					case PACKET_HELLO_FROM_SERVER:
						parseHello(message);
						break;
					case PACKET_KICK:
						parseKick(message);
						break;
					case PACKET_ACCEPTED_CLIENT:
						parseClientAccepted(message);
						break;
					case PACKET_CHANGE_CLIENT_VERSION:
						parseChangeClientVersion(message);
						break;
					case PACKET_SERVER_TALK:
						parseServerTalk(message);
						break;
					case PACKET_NODE:
						parseNode(message);
						break;
					case PACKET_CURSOR_UPDATE:
						parseCursorUpdate(message);
						break;
					case PACKET_START_OPERATION:
						parseStartOperation(message);
						break;
					case PACKET_UPDATE_OPERATION:
						parseUpdateOperation(message);
						break;
					case PACKET_COLOR_UPDATE:
						parseColorUpdate(message);
						break;
					default: {
						logMessage(wxString::Format("[Client]: Unknown packet type 0x%02X received, disconnecting", packetType));
						close();
						return;
					}
				}
			} catch (std::exception& e) {
				logMessage(wxString::Format("[Client]: Error parsing packet type 0x%02X: %s", 
					packetType, e.what()));
				// Continue to next packet if possible
			}
		}
	} catch (std::exception& e) {
		logMessage(wxString::Format("[Client]: Fatal error parsing packet: %s", e.what()));
		close();
	}
}

void LiveClient::parseHello(NetworkMessage& message) {
	ASSERT(editor == nullptr);
	editor = newd Editor(g_gui.copybuffer, this);

	Map& map = editor->map;
	map.setName("Live Map - " + message.read<std::string>());
	map.setWidth(message.read<uint16_t>());
	map.setHeight(message.read<uint16_t>());

	createEditorWindow();
}

void LiveClient::parseKick(NetworkMessage& message) {
	const std::string& kickMessage = message.read<std::string>();
	close();

	g_gui.PopupDialog("Disconnected", wxstr(kickMessage), wxOK);
}

void LiveClient::parseClientAccepted(NetworkMessage& message) {
	// Initialize the host's cursor when we're accepted
	LiveCursor hostCursor;
	hostCursor.id = 0; // Host is always ID 0
	hostCursor.color = wxColor(255, 0, 0); // Default red color for host
	hostCursor.pos = Position(); // Default position
	cursors[0] = hostCursor; // Add host cursor to our list
	
	sendReady();
}

void LiveClient::parseChangeClientVersion(NetworkMessage& message) {
	ClientVersionID clientVersion = static_cast<ClientVersionID>(message.read<uint32_t>());
	if (!g_gui.CloseAllEditors()) {
		close();
		return;
	}

	wxString error;
	wxArrayString warnings;
	g_gui.LoadVersion(clientVersion, error, warnings);

	sendReady();
}

void LiveClient::parseServerTalk(NetworkMessage& message) {
	const std::string& speaker = message.read<std::string>();
	const std::string& chatMessage = message.read<std::string>();
	log->Chat(
		wxstr(speaker),
		wxstr(chatMessage)
	);
}

void LiveClient::parseNode(NetworkMessage& message) {
	try {
		uint32_t nodeid = message.read<uint32_t>();
		int32_t ndx = nodeid >> 18;
		int32_t ndy = (nodeid >> 4) & 0x3FFF;
		bool underground = (nodeid & 1) == 1;

		// Log node reception for debugging
		logMessage(wxString::Format("[Client]: Received node update [%d,%d,%s]", 
			ndx, ndy, underground ? "underground" : "surface"));

		// Queue the node processing on the main thread to avoid threading issues
		wxTheApp->CallAfter([this, message = std::move(message), ndx, ndy, underground]() mutable {
			if (!editor) {
				logMessage("[Client]: Warning - received node update but no editor available");
				return;
			}

			NetworkedAction* action = static_cast<NetworkedAction*>(editor->actionQueue->createAction(ACTION_REMOTE));
			
			// Process the node data safely
			receiveNode(message, *editor, action, ndx, ndy, underground);
			
			// Only add the action if it contains changes
			if (action->size() > 0) {
				editor->actionQueue->addAction(action);
				g_gui.RefreshView();
				g_gui.UpdateMinimap();

				logMessage(wxString::Format("[Client]: Applying node update [%d,%d,%s]", 
					ndx, ndy, underground ? "underground" : "surface"));
				logMessage("[Client]: Node update applied successfully.");
			} else {
				// Use proper action destruction
				
			}
		});
	} catch (std::exception& e) {
		logMessage(wxString::Format("[Client]: Error parsing node packet: %s", e.what()));
	}
}

void LiveClient::parseCursorUpdate(NetworkMessage& message) {
	LiveCursor cursor = readCursor(message);
	cursors[cursor.id] = cursor;

	// Update client list after receiving cursor updates
	if (log) {
		wxTheApp->CallAfter([this]() {
			std::unordered_map<uint32_t, LivePeer*> dummyPeers;
			log->UpdateClientList(dummyPeers);
		});
	}

	g_gui.RefreshView();
}

void LiveClient::parseStartOperation(NetworkMessage& message) {
	const std::string& operation = message.read<std::string>();

	currentOperation = wxstr(operation);
	g_gui.SetStatusText("Server Operation in Progress: " + currentOperation + "... (0%)");
}

void LiveClient::parseUpdateOperation(NetworkMessage& message) {
	int32_t percent = message.read<uint32_t>();
	if (percent >= 100) {
		g_gui.SetStatusText("Server Operation Finished.");
	} else {
		g_gui.SetStatusText("Server Operation in Progress: " + currentOperation + "... (" + std::to_string(percent) + "%)");
	}
}

void LiveClient::parseColorUpdate(NetworkMessage& message) {
	// Read client ID whose color changed
	uint32_t clientId = message.read<uint32_t>();
	
	// Read the color components
	uint8_t r = message.read<uint8_t>();
	uint8_t g = message.read<uint8_t>();
	uint8_t b = message.read<uint8_t>();
	uint8_t a = message.read<uint8_t>();
	wxColor newColor(r, g, b, a);
	
	logMessage(wxString::Format("[Client]: Received color update for client %u: RGB(%d,%d,%d)", 
		clientId, r, g, b));
	
	// Update the color in our local cursor list
	if (clientId == 0) {
		// Server's color (host)
		// Update the host's cursor in our list
		for (auto& cursorPair : cursors) {
			if (cursorPair.first == 0) {
				LiveCursor& cursor = cursorPair.second;
				cursor.color = newColor;
				break;
			}
		}
	} else {
		// Another client's color
		// Find this client ID in our cursor list and update it
		for (auto& cursorPair : cursors) {
			if (cursorPair.first == clientId) {
				LiveCursor& cursor = cursorPair.second;
				cursor.color = newColor;
				break;
			}
		}
	}
	
	// Refresh view to show updated cursor color
	g_gui.RefreshView();
	
	// Also update the client list UI if the log tab exists
	if (log) {
		wxTheApp->CallAfter([this]() {
			// We don't have direct access to the peer list as a client,
			// so just tell the log tab to update based on cursor information
			std::unordered_map<uint32_t, LivePeer*> dummyPeers;
			log->UpdateClientList(dummyPeers);
		});
	}
}

// Send a request to change user color
void LiveClient::sendColorUpdate(uint32_t targetClientId, const wxColor& color) {
	logMessage(wxString::Format("[Client]: Sending color update request for client %u: RGB(%d,%d,%d)", 
		targetClientId, color.Red(), color.Green(), color.Blue()));
		
	NetworkMessage message;
	message.write<uint8_t>(PACKET_CLIENT_COLOR_UPDATE);
	message.write<uint32_t>(targetClientId);
	message.write<uint8_t>(color.Red());
	message.write<uint8_t>(color.Green());
	message.write<uint8_t>(color.Blue());
	message.write<uint8_t>(color.Alpha());
	
	send(message);
}
