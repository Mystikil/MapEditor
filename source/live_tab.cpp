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

#include <wx/splitter.h>
#include <wx/grid.h>
#include <wx/colordlg.h>

#include "live_tab.h"
#include "live_socket.h"
#include "live_peer.h"
#include "live_server.h"
#include "live_client.h"

class myGrid : public wxGrid {
public:
	myGrid(wxWindow* parent, wxWindowID id, wxPoint pos, wxSize size) :
		wxGrid(parent, id, pos, size) { }
	virtual ~myGrid() { }

	void DrawCellHighlight(wxDC& dc, const wxGridCellAttr* attr) {
		// wxGrid::DrawCellHighlight(dc, attr);
	}

	DECLARE_CLASS(myGrid);
};

IMPLEMENT_CLASS(myGrid, wxGrid)

// Menu IDs and control IDs
enum {
    LIVE_LOG_COPY_SELECTED = 10001,
    LIVE_CHAT_INPUT = 10002
};

BEGIN_EVENT_TABLE(LiveLogTab, wxPanel)
EVT_TEXT_ENTER(LIVE_CHAT_INPUT, LiveLogTab::OnChat)
EVT_RIGHT_DOWN(LiveLogTab::OnLogRightClick)
EVT_MENU(LIVE_LOG_COPY_SELECTED, LiveLogTab::OnCopySelectedLogText)
EVT_BOOKCTRL_PAGE_CHANGED(wxID_ANY, LiveLogTab::OnPageChanged)
EVT_GRID_CELL_LEFT_CLICK(LiveLogTab::OnGridCellLeftClick)
END_EVENT_TABLE()

LiveLogTab::LiveLogTab(MapTabbook* aui, LiveSocket* server) :
	EditorTab(),
	wxPanel(aui),
	aui(aui),
	socket(server) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxPanel* splitter = newd wxPanel(this);
	topsizer->Add(splitter, 1, wxEXPAND);

	// Setup left panel
	wxPanel* left_pane = newd wxPanel(splitter);
	wxSizer* left_sizer = newd wxBoxSizer(wxVERTICAL);

	wxFont time_font(*wxSWISS_FONT);

	// Create a notebook for tabs
	notebook = newd wxNotebook(left_pane, wxID_ANY);
	
	// Debug log tab
	debug_log = newd wxTextCtrl(notebook, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 
                      wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxTE_AUTO_URL);
	debug_log->SetFont(time_font);
	debug_log->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(LiveLogTab::OnLogRightClick), nullptr, this);
	notebook->AddPage(debug_log, "Debug");
	
	// Chat log tab
	chat_log = newd wxTextCtrl(notebook, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 
                      wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxTE_AUTO_URL);
	chat_log->SetFont(time_font);
	chat_log->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(LiveLogTab::OnLogRightClick), nullptr, this);
	notebook->AddPage(chat_log, "Chat");
	
	left_sizer->Add(notebook, 1, wxEXPAND);

	input = newd wxTextCtrl(left_pane, LIVE_CHAT_INPUT, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	left_sizer->Add(input, 0, wxEXPAND);

	input->Connect(wxEVT_SET_FOCUS, wxFocusEventHandler(LiveLogTab::OnSelectChatbox), nullptr, this);
	input->Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(LiveLogTab::OnDeselectChatbox), nullptr, this);
	input->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(LiveLogTab::OnKeyDown), nullptr, this);

	left_pane->SetSizerAndFit(left_sizer);

	// Setup right panel
	user_list = newd myGrid(splitter, wxID_ANY, wxDefaultPosition, wxSize(280, 100));
	user_list->CreateGrid(5, 3);
	user_list->DisableDragRowSize();
	user_list->DisableDragColSize();
	user_list->SetSelectionMode(wxGrid::wxGridSelectRows);
	user_list->SetRowLabelSize(0);

	user_list->SetColLabelValue(0, "");
	user_list->SetColSize(0, 24);
	user_list->SetColLabelValue(1, "#");
	user_list->SetColSize(1, 36);
	user_list->SetColLabelValue(2, "Name");
	user_list->SetColSize(2, 200);

	// Finalize
	SetSizerAndFit(topsizer);

	wxSizer* split_sizer = newd wxBoxSizer(wxHORIZONTAL);
	split_sizer->Add(left_pane, wxSizerFlags(1).Expand());
	split_sizer->Add(user_list, wxSizerFlags(0).Expand());
	splitter->SetSizerAndFit(split_sizer);

	aui->AddTab(this, true);
}

LiveLogTab::~LiveLogTab() {
}

wxString LiveLogTab::GetTitle() const {
	if (socket) {
		return "Live Log - " + socket->getHostName();
	}
	return "Live Log - Disconnected";
}

void LiveLogTab::Disconnect() {
	socket->log = nullptr;
	input->SetWindowStyle(input->GetWindowStyle() | wxTE_READONLY);
	socket = nullptr;
	Refresh();
}



wxString format00(wxDateTime::wxDateTime_t t) {
	wxString str;
	if (t < 10) {
		str << "0";
	}
	str << t;
	return str;
}

void LiveLogTab::Message(const wxString& str) {
	wxDateTime t = wxDateTime::Now();
	wxString time, message;
	time << format00(t.GetHour()) << ":"
		 << format00(t.GetMinute()) << ":"
		 << format00(t.GetSecond());
	
	// Format the message with timestamp
	message << time << " - " << str << "\n";
	
	// Add text to the debug log
	debug_log->AppendText(message);
	
	// Auto-scroll to the bottom
	debug_log->SetInsertionPointEnd();
}

void LiveLogTab::Chat(const wxString& speaker, const wxString& str) {
	wxDateTime t = wxDateTime::Now();
	wxString time, message;
	time << format00(t.GetHour()) << ":"
		 << format00(t.GetMinute()) << ":"
		 << format00(t.GetSecond());
	
	// Format the chat message with timestamp and speaker
	message << time << " [" << speaker << "]: " << str << "\n";
	
	// Add text to the chat log
	chat_log->AppendText(message);
	
	// Auto-scroll to the bottom
	chat_log->SetInsertionPointEnd();
	
	// Switch to the chat tab automatically when receiving messages
	notebook->SetSelection(1); // 1 is the index of the chat tab
}

void LiveLogTab::OnChat(wxCommandEvent& evt) {
    wxString message = input->GetValue();
    if (message.IsEmpty()) {
        return;
    }
    
    if (socket) {
        // Send the chat message
        if (socket->getName() == "HOST") {
            // If this is the host, the message appears with HOST prefix
            socket->sendChat(message);
            // Display the message locally too
            Chat("HOST", message);
        } else {
            socket->sendChat(message);
        }
        
        // Clear the input field
        input->Clear();
    }
}

void LiveLogTab::OnKeyDown(wxKeyEvent& evt) {
    if (evt.GetKeyCode() == WXK_RETURN && !evt.ShiftDown()) {
        // Send the message when Enter is pressed (without Shift)
        wxString message = input->GetValue();
        if (!message.IsEmpty() && socket) {
            if (socket->getName() == "HOST") {
                socket->sendChat(message);
                Chat("HOST", message);
            } else {
                socket->sendChat(message);
            }
            input->Clear();
        }
    } else {
        evt.Skip();
    }
}

void LiveLogTab::OnResizeChat(wxSizeEvent& evt) {
	// No need to adjust anything for wxTextCtrl
	evt.Skip();
}

void LiveLogTab::OnResizeClientList(wxSizeEvent& evt) {
}

void LiveLogTab::OnSelectChatbox(wxFocusEvent& evt) {
	g_gui.DisableHotkeys();
}

void LiveLogTab::OnDeselectChatbox(wxFocusEvent& evt) {
	g_gui.EnableHotkeys();
}

void LiveLogTab::OnLogRightClick(wxMouseEvent& evt) {
    wxMenu menu;
    menu.Append(LIVE_LOG_COPY_SELECTED, "Copy Selected Text");
    PopupMenu(&menu);
}

void LiveLogTab::OnCopySelectedLogText(wxCommandEvent& evt) {
    if(wxTheClipboard->Open()) {
        // Determine which log has focus and copy from that one
        wxTextCtrl* activeLog = nullptr;
        
        if (wxWindow::FindFocus() == debug_log) {
            activeLog = debug_log;
        } else if (wxWindow::FindFocus() == chat_log) {
            activeLog = chat_log;
        } else {
            // Default to currently visible page
            int selection = notebook->GetSelection();
            activeLog = (selection == 0) ? debug_log : chat_log;
        }
        
        wxTheClipboard->SetData(new wxTextDataObject(activeLog->GetStringSelection()));
        wxTheClipboard->Close();
    }
}

void LiveLogTab::UpdateClientList(const std::unordered_map<uint32_t, LivePeer*>& updatedClients) {
	// Delete old rows
	if (user_list->GetNumberRows() > 0) {
		user_list->DeleteRows(0, user_list->GetNumberRows());
	}

	clients = updatedClients;
	
	// For server, use the provided client list
	if (socket && socket->IsServer()) {
		// Make sure we have data to display
		if (clients.empty()) {
			// At least show the host
			user_list->AppendRows(1);
			user_list->SetCellBackgroundColour(0, 0, dynamic_cast<LiveServer*>(socket)->getUsedColor());
			user_list->SetCellValue(0, 1, "Host");
			user_list->SetCellValue(0, 2, "HOST");
		} else {
			user_list->AppendRows(clients.size());

			int32_t i = 0;
			for (auto& clientEntry : clients) {
				LivePeer* peer = clientEntry.second;
				if (peer) {
					user_list->SetCellBackgroundColour(i, 0, peer->getUsedColor());
					user_list->SetCellValue(i, 1, i2ws((peer->getClientId() >> 1) + 1));
					user_list->SetCellValue(i, 2, peer->getName());
					++i;
				}
			}
		}
	} 
	// For client, use cursor information
	else if (socket && socket->IsClient()) {
		// Get cursor list from the socket
		std::vector<LiveCursor> cursors = socket->getCursorList();
		
		if (cursors.empty()) {
			// If no cursors, at least show the host
			user_list->AppendRows(1);
			user_list->SetCellBackgroundColour(0, 0, wxColor(255, 0, 0)); // Default red for host
			user_list->SetCellValue(0, 1, "Host");
			user_list->SetCellValue(0, 2, "HOST");
		} else {
			// Add a row for each cursor
			user_list->AppendRows(cursors.size());
			
			int32_t i = 0;
			for (const auto& cursor : cursors) {
				user_list->SetCellBackgroundColour(i, 0, cursor.color);
				
				// In display, clientId 0 is the host, others are +1 and shifted
				wxString displayId;
				if (cursor.id == 0) {
					displayId = "Host";
				} else {
					displayId = i2ws((cursor.id >> 1) + 1);
				}
				
				user_list->SetCellValue(i, 1, displayId);
				
				// Name is not available in cursor, so we use the socket name for host
				// and "Client X" for others
				wxString name;
				if (cursor.id == 0) {
					name = "HOST";
				} else {
					name = wxString::Format("Client %s", displayId);
				}
				
				user_list->SetCellValue(i, 2, name);
				++i;
			}
		}
		
		// Debug message to verify cursor list content
		Message(wxString::Format("Updated client list with %zu cursors", cursors.size()));
	}
	
	// Refresh the grid
	user_list->Refresh();
}

void LiveLogTab::OnPageChanged(wxBookCtrlEvent& evt) {
    // If switching to chat tab, set focus to the input box
    if (evt.GetSelection() == 1) { // Chat tab index
        input->SetFocus();
    }
    evt.Skip();
}

void LiveLogTab::OnGridCellLeftClick(wxGridEvent& evt) {
    int row = evt.GetRow();
    int col = evt.GetCol();
    
    // Column 0 is the color column
    if (col == 0) {
        // Get the current color from the background
        wxColor currentColor = user_list->GetCellBackgroundColour(row, 0);
        
        // Show color picker dialog
        wxColourData colorData;
        colorData.SetColour(currentColor);
        
        wxColourDialog dialog(this, &colorData);
        dialog.SetTitle("Choose Color");
        
        if (dialog.ShowModal() == wxID_OK) {
            wxColour newColor = dialog.GetColourData().GetColour();
            ChangeUserColor(row, newColor);
        }
    }
    
    evt.Skip();
}

void LiveLogTab::ChangeUserColor(int row, const wxColor& color) {
    // Make sure we have a valid client selected
    if (row < 0 || row >= user_list->GetNumberRows()) {
        return;
    }
    
    // Update the color in the UI grid
    user_list->SetCellBackgroundColour(row, 0, color);
    user_list->Refresh();
    
    // Get the client ID from the row
    wxString clientIdStr = user_list->GetCellValue(row, 1);
    long displayId = 0;
    uint32_t clientId = 0;
    
    if (clientIdStr.ToLong(&displayId)) {
        // Convert display ID to actual client ID
        clientId = (displayId - 1) << 1;
    }
    
    // If we're the host, directly update and broadcast
    auto server = dynamic_cast<LiveServer*>(socket);
    if (server) {
        if (row == 0) {
            // Update the host's color
            server->setUsedColor(color);
        } else {
            // Update a client's color by broadcasting
            server->broadcastColorChange(clientId, color);
            
            // Also update the local representation
            for (const auto& pair : clients) {
                LivePeer* peer = pair.second;
                if (peer && peer->getClientId() == clientId) {
                    peer->setUsedColor(color);
                    break;
                }
            }
        }
    } else if (socket && socket->IsClient()) {
        // We're a client - send a color update request to the server
        LiveClient* client = static_cast<LiveClient*>(socket);
        
        // Row 0 is always the host with client ID 0
        uint32_t targetClientId = (row == 0) ? 0 : clientId;
        client->sendColorUpdate(targetClientId, color);
    }
}
