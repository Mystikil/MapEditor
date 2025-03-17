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

#include "live_tab.h"
#include "live_socket.h"
#include "live_peer.h"

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

	// Replace wxGrid with wxTextCtrl for the log
	log = newd wxTextCtrl(left_pane, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 
                      wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxTE_AUTO_URL);
	log->SetFont(time_font);

	// Connect right-click event to show context menu
	log->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(LiveLogTab::OnLogRightClick), nullptr, this);

	left_sizer->Add(log, 1, wxEXPAND);

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
	wxString time, speaker, message;
	time << format00(t.GetHour()) << ":"
		 << format00(t.GetMinute()) << ":"
		 << format00(t.GetSecond());

	speaker << "Server";
	
	// Format the message with timestamp and speaker
	message << time << " [" << speaker << "]: " << str << "\n";
	
	// Add text to the log
	log->AppendText(message);
	
	// Auto-scroll to the bottom
	log->SetInsertionPointEnd();
}

void LiveLogTab::Chat(const wxString& speaker, const wxString& str) {
	wxDateTime t = wxDateTime::Now();
	wxString time, message;
	time << format00(t.GetHour()) << ":"
		 << format00(t.GetMinute()) << ":"
		 << format00(t.GetSecond());
	
	// Format the chat message with timestamp and speaker
	message << time << " [" << speaker << "]: " << str << "\n";
	
	// Add text to the log
	log->AppendText(message);
	
	// Auto-scroll to the bottom
	log->SetInsertionPointEnd();
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
        wxTheClipboard->SetData(new wxTextDataObject(log->GetStringSelection()));
        wxTheClipboard->Close();
    }
}

void LiveLogTab::UpdateClientList(const std::unordered_map<uint32_t, LivePeer*>& updatedClients) {
	// Delete old rows
	if (user_list->GetNumberRows() > 0) {
		user_list->DeleteRows(0, user_list->GetNumberRows());
	}

	clients = updatedClients;
	user_list->AppendRows(clients.size());

	int32_t i = 0;
	for (auto& clientEntry : clients) {
		LivePeer* peer = clientEntry.second;
		user_list->SetCellBackgroundColour(i, 0, peer->getUsedColor());
		user_list->SetCellValue(i, 1, i2ws((peer->getClientId() >> 1) + 1));
		user_list->SetCellValue(i, 2, peer->getName());
		++i;
	}
}
