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
#include "recent_brushes_window.h"
#include "palette_common.h"
#include "gui.h"
#include "brush.h"
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/menu.h>
#include <algorithm>

// Event table for RecentBrushesPanel
BEGIN_EVENT_TABLE(RecentBrushesPanel, wxScrolledWindow)
EVT_SIZE(RecentBrushesPanel::OnSize)
EVT_RIGHT_DOWN(RecentBrushesPanel::OnRightClick)
EVT_PAINT(RecentBrushesPanel::OnPaint)
EVT_ERASE_BACKGROUND(RecentBrushesPanel::OnEraseBackground)
END_EVENT_TABLE()

// Event table for RecentBrushesWindow
BEGIN_EVENT_TABLE(RecentBrushesWindow, wxPanel)
	EVT_KEY_DOWN(RecentBrushesWindow::OnKey)
	EVT_CLOSE(RecentBrushesWindow::OnClose)
	EVT_BUTTON(wxID_CLEAR, RecentBrushesWindow::OnClearButton)
END_EVENT_TABLE()

//=============================================================================
// RecentBrushesPanel Implementation
//=============================================================================

RecentBrushesPanel::RecentBrushesPanel(wxWindow* parent) 
	: wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxHSCROLL),
	  grid_sizer(nullptr),
	  columns(2) {
	
	// Set background style and color properly
	SetBackgroundStyle(wxBG_STYLE_SYSTEM);
	SetBackgroundColour(wxColour(255, 255, 255)); // Set white background
	
	// Create grid sizer
	grid_sizer = new wxFlexGridSizer(0, columns, 2, 2); // 0 rows (dynamic), 2 columns, 2px gaps
	SetSizer(grid_sizer);
	
	// Enable scrolling
	SetScrollRate(32, 32);
	
	// Force immediate background update
	ClearBackground();
	
	// Calculate initial grid layout
	RecalculateLayout();
}

RecentBrushesPanel::~RecentBrushesPanel() {
	// Clear all brush buttons
	for (BrushButton* button : brush_buttons) {
		button->Destroy();
	}
	brush_buttons.clear();
}

void RecentBrushesPanel::AddRecentBrush(Brush* brush) {
	if (!brush) {
		return;
	}
	
	// Check if brush already exists - if so, don't change position to prevent crashes
	auto it = std::find(recent_brushes.begin(), recent_brushes.end(), brush);
	if (it != recent_brushes.end()) {
		// Brush already exists, don't reorder
		return;
	}
	
	// Add to the front (only for new brushes)
	recent_brushes.push_front(brush);
	
	// Limit the size
	while (recent_brushes.size() > MAX_RECENT_BRUSHES) {
		recent_brushes.pop_back();
	}
	
	// Refresh the display
	RefreshDisplay();
}

void RecentBrushesPanel::ClearRecentBrushes() {
	recent_brushes.clear();
	RefreshDisplay();
}

void RecentBrushesPanel::RefreshDisplay() {
	// Clear existing buttons
	for (BrushButton* button : brush_buttons) {
		grid_sizer->Detach(button);
		button->Destroy();
	}
	brush_buttons.clear();
	
	// Create new buttons for recent brushes
	for (Brush* brush : recent_brushes) {
		BrushButton* button = new BrushButton(this, brush, RENDER_SIZE_32x32);
		
		// Set tooltip with brush name and additional info
		wxString tooltip;
		if (brush->isRaw()) {
			RAWBrush* raw = static_cast<RAWBrush*>(brush);
			tooltip = wxString::Format("%s [%d]", raw->getName(), raw->getItemID());
		} else {
			tooltip = wxString::Format("%s", brush->getName());
		}
		button->SetToolTip(tooltip);
		
		// Bind click event
		button->Bind(wxEVT_TOGGLEBUTTON, &RecentBrushesPanel::OnBrushClick, this);
		
		grid_sizer->Add(button, 0, wxALL, 1);
		brush_buttons.push_back(button);
	}
	
	// Update layout
	UpdateButtonLayout();
}

Brush* RecentBrushesPanel::GetSelectedBrush() const {
	for (BrushButton* button : brush_buttons) {
		if (button->GetValue()) {
			return button->brush;
		}
	}
	return nullptr;
}

bool RecentBrushesPanel::SelectBrush(const Brush* whatbrush) {
	DeselectAll();
	for (BrushButton* button : brush_buttons) {
		if (button->brush == whatbrush) {
			button->SetValue(true);
			return true;
		}
	}
	return false;
}

void RecentBrushesPanel::DeselectAll() {
	for (BrushButton* button : brush_buttons) {
		button->SetValue(false);
	}
}

void RecentBrushesPanel::OnBrushClick(wxCommandEvent& event) {
	BrushButton* button = dynamic_cast<BrushButton*>(event.GetEventObject());
	if (button) {
		// Deselect all other buttons
		for (BrushButton* other_button : brush_buttons) {
			if (other_button != button) {
				other_button->SetValue(false);
			}
		}
		
		// If this brush is already selected globally, deselect it
		if (button->brush == g_gui.GetCurrentBrush()) {
			g_gui.SelectBrush(nullptr);
			button->SetValue(false);
		} else {
			// Select this brush
			g_gui.SelectBrush(button->brush);
			button->SetValue(true);
		}
	}
}

void RecentBrushesPanel::OnRightClick(wxMouseEvent& event) {
	wxMenu contextMenu;
	contextMenu.Append(wxID_CLEAR, "Clear Recent Brushes");
	
	contextMenu.Bind(wxEVT_COMMAND_MENU_SELECTED, [this](wxCommandEvent& evt) {
		if (evt.GetId() == wxID_CLEAR) {
			ClearRecentBrushes();
		}
	});
	
	PopupMenu(&contextMenu);
}

void RecentBrushesPanel::OnSize(wxSizeEvent& event) {
	RecalculateLayout();
	event.Skip();
}

void RecentBrushesPanel::RecalculateLayout() {
	if (!grid_sizer) {
		return;
	}
	
	// Calculate how many columns can fit
	int window_width = GetClientSize().GetWidth();
	int button_width = 36; // 32px + 4px padding
	int new_columns = std::max(1, (window_width - 4) / button_width);
	
	if (new_columns != columns) {
		columns = new_columns;
		grid_sizer->SetCols(columns);
		UpdateButtonLayout();
	}
}

void RecentBrushesPanel::UpdateButtonLayout() {
	if (grid_sizer) {
		grid_sizer->Layout();
		FitInside(); // Update scroll window virtual size
		Refresh();
	}
}

void RecentBrushesPanel::OnPaint(wxPaintEvent& event) {
	wxPaintDC dc(this);
	PrepareDC(dc);
	
	// Fill background with white
	dc.SetBackground(wxBrush(wxColour(255, 255, 255)));
	dc.Clear();
	
	event.Skip();
}

void RecentBrushesPanel::OnEraseBackground(wxEraseEvent& event) {
	wxDC* dc = event.GetDC();
	if (dc) {
		dc->SetBackground(wxBrush(wxColour(255, 255, 255)));
		dc->Clear();
	}
}

//=============================================================================
// RecentBrushesWindow Implementation
//=============================================================================

RecentBrushesWindow::RecentBrushesWindow(wxWindow* parent) 
	: wxPanel(parent, wxID_ANY),
	  recent_panel(nullptr),
	  clear_button(nullptr) {
	
	// Set white background for the entire window
	SetBackgroundStyle(wxBG_STYLE_SYSTEM);
	SetBackgroundColour(wxColour(255, 255, 255));
	
	// Create main sizer
	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
	
	// Create title
	wxStaticText* title = new wxStaticText(this, wxID_ANY, "Recent Brushes");
	wxFont font = title->GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	title->SetFont(font);
	title->SetBackgroundColour(wxColour(255, 255, 255));
	main_sizer->Add(title, 0, wxALL | wxALIGN_CENTER, 5);
	
	// Create recent brushes panel
	recent_panel = new RecentBrushesPanel(this);
	main_sizer->Add(recent_panel, 1, wxEXPAND | wxALL, 2);
	
	// Create clear button
	clear_button = new wxButton(this, wxID_CLEAR, "Clear All");
	main_sizer->Add(clear_button, 0, wxALL | wxALIGN_CENTER, 2);
	
	SetSizer(main_sizer);
	
	// Set minimum size
	SetMinSize(wxSize(120, 200));
	
	// Force initial refresh to apply background
	Refresh();
	Update();
}

RecentBrushesWindow::~RecentBrushesWindow() {
	// Cleanup handled by wxWidgets
}

void RecentBrushesWindow::AddRecentBrush(Brush* brush) {
	if (recent_panel) {
		recent_panel->AddRecentBrush(brush);
	}
}

void RecentBrushesWindow::ClearRecentBrushes() {
	if (recent_panel) {
		recent_panel->ClearRecentBrushes();
	}
}

void RecentBrushesWindow::RefreshDisplay() {
	if (recent_panel) {
		recent_panel->RefreshDisplay();
	}
}

Brush* RecentBrushesWindow::GetSelectedBrush() const {
	if (recent_panel) {
		return recent_panel->GetSelectedBrush();
	}
	return nullptr;
}

bool RecentBrushesWindow::SelectBrush(const Brush* whatbrush) {
	if (recent_panel) {
		return recent_panel->SelectBrush(whatbrush);
	}
	return false;
}

void RecentBrushesWindow::OnKey(wxKeyEvent& event) {
	// Forward key events to the main map window
	if (g_gui.GetCurrentTab() != nullptr) {
		g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
	}
}

void RecentBrushesWindow::OnClose(wxCloseEvent& event) {
	if (!event.CanVeto()) {
		// We can't do anything! This sucks!
		// (application is closed, we have to destroy ourselves)
		Destroy();
	} else {
		Show(false);
		event.Veto(true);
	}
}

void RecentBrushesWindow::OnClearButton(wxCommandEvent& event) {
	ClearRecentBrushes();
}

void RecentBrushesWindow::OnPaint(wxPaintEvent& event) {
	wxPaintDC dc(this);
	
	// Fill background with white
	dc.SetBackground(wxBrush(wxColour(255, 255, 255)));
	dc.Clear();
	
	event.Skip();
}

void RecentBrushesWindow::OnEraseBackground(wxEraseEvent& event) {
	wxDC* dc = event.GetDC();
	if (dc) {
		dc->SetBackground(wxBrush(wxColour(255, 255, 255)));
		dc->Clear();
	}
} 