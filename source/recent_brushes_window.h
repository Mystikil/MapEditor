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

#ifndef RME_RECENT_BRUSHES_WINDOW_H_
#define RME_RECENT_BRUSHES_WINDOW_H_

#include "main.h"
#include "palette_common.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <vector>
#include <deque>

class Brush;
class BrushButton;

class RecentBrushesPanel : public wxScrolledWindow {
public:
	RecentBrushesPanel(wxWindow* parent);
	~RecentBrushesPanel();

	// Interface
	void AddRecentBrush(Brush* brush);
	void ClearRecentBrushes();
	void RefreshDisplay();
	
	// Get the currently selected brush
	Brush* GetSelectedBrush() const;
	// Select a specific brush
	bool SelectBrush(const Brush* whatbrush);
	// Deselect all brushes
	void DeselectAll();

	// Event handlers
	void OnBrushClick(wxCommandEvent& event);
	void OnRightClick(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnEraseBackground(wxEraseEvent& event);

private:
	void RecalculateLayout();
	void UpdateButtonLayout();
	
	std::deque<Brush*> recent_brushes;
	std::vector<BrushButton*> brush_buttons;
	wxFlexGridSizer* grid_sizer;
	int columns;
	static const size_t MAX_RECENT_BRUSHES = 20;

	DECLARE_EVENT_TABLE()
};

class RecentBrushesWindow : public wxPanel {
public:
	RecentBrushesWindow(wxWindow* parent);
	~RecentBrushesWindow();

	// Interface
	void AddRecentBrush(Brush* brush);
	void ClearRecentBrushes();
	void RefreshDisplay();
	
	// Get the currently selected brush
	Brush* GetSelectedBrush() const;
	// Select a specific brush
	bool SelectBrush(const Brush* whatbrush);

	// Event handlers
	void OnKey(wxKeyEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnClearButton(wxCommandEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnEraseBackground(wxEraseEvent& event);

private:
	RecentBrushesPanel* recent_panel;
	wxButton* clear_button;

	DECLARE_EVENT_TABLE()
};

#endif 