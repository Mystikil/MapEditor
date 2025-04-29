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

#ifndef RME_TILESET_CREATURE_H_
#define RME_TILESET_CREATURE_H_

#include "palette_common.h"

// New class for grid view of creature sprites
class CreatureSpritePanel : public wxScrolledWindow {
public:
	CreatureSpritePanel(wxWindow* parent);
	virtual ~CreatureSpritePanel();

	void Clear();
	void LoadCreatures(const BrushVector& brushlist);
	Brush* GetSelectedBrush() const;
	bool SelectBrush(const Brush* brush);
	void EnsureVisible(const Brush* brush);
	void SelectIndex(int index);
	
	// Getter for sprite size
	int GetSpriteSize() const;
	
	// Drawing handlers
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnScroll(wxScrollWinEvent& event);
	void OnMouseClick(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);

	// Creature brushes in this panel
	BrushVector creatures;

protected:
	void RecalculateGrid();
	int GetSpriteIndexAt(int x, int y) const;
	void DrawSprite(wxDC& dc, int x, int y, CreatureType* ctype, bool selected = false);

	int columns;
	int sprite_size;
	int padding;
	int selected_index;
	int hover_index;
	wxBitmap* buffer;

	DECLARE_EVENT_TABLE();
};

class CreaturePalettePanel : public PalettePanel {
public:
	CreaturePalettePanel(wxWindow* parent, wxWindowID id = wxID_ANY);
	virtual ~CreaturePalettePanel();

	PaletteType GetType() const;

	// Select the first brush
	void SelectFirstBrush();
	// Returns the currently selected brush (first brush if panel is not loaded)
	Brush* GetSelectedBrush() const;
	// Returns the currently selected brush size
	int GetSelectedBrushSize() const;
	// Select the brush in the parameter, this only changes the look of the panel
	bool SelectBrush(const Brush* whatbrush);

	// Updates the palette window to use the current brush size
	void OnUpdateBrushSize(BrushShape shape, int size);
	// Called when this page is displayed
	void OnSwitchIn();
	// Called sometimes?
	void OnUpdate();

protected:
	void SelectTileset(size_t index);
	void SelectCreature(size_t index);
	void SelectCreature(std::string name);
	// Switch between list view and sprite view modes
	void SetViewMode(bool use_sprites);

public:
	// Event handling
	void OnChangeSpawnTime(wxSpinEvent& event);
	void OnChangeSpawnSize(wxSpinEvent& event);

	void OnTilesetChange(wxCommandEvent& event);
	void OnListBoxChange(wxCommandEvent& event);
	void OnClickCreatureBrushButton(wxCommandEvent& event);
	void OnClickSpawnBrushButton(wxCommandEvent& event);
	void OnClickLoadNPCsButton(wxCommandEvent& event);
	void OnClickLoadMonstersButton(wxCommandEvent& event);
	void OnClickPurgeCreaturesButton(wxCommandEvent& event);
	void OnClickSearchButton(wxCommandEvent& event);
	void OnSearchFieldText(wxCommandEvent& event);
	void OnSearchFieldFocus(wxFocusEvent& event);
	void OnSearchFieldKillFocus(wxFocusEvent& event);
	void OnSearchFieldKeyDown(wxKeyEvent& event);
	void OnClickViewToggle(wxCommandEvent& event);
	void OnSpriteSelected(wxCommandEvent& event);

protected:
	void SelectCreatureBrush();
	void SelectSpawnBrush();
	bool LoadNPCsFromFolder(const wxString& folder);
	bool LoadMonstersFromFolder(const wxString& folder);
	bool PurgeCreaturePalettes();
	void FilterCreatures(const wxString& search_text);

	wxChoice* tileset_choice;
	SortableListBox* creature_list;
	CreatureSpritePanel* sprite_panel;
	wxToggleButton* view_toggle;
	wxSizer* view_sizer;
	bool use_sprite_view;
	bool handling_event;

	wxTextCtrl* search_field;
	wxButton* search_button;
	wxButton* load_npcs_button;
	wxButton* load_monsters_button;
	wxButton* purge_creatures_button;
	
	wxSpinCtrl* creature_spawntime_spin;
	wxSpinCtrl* spawn_size_spin;
	wxToggleButton* creature_brush_button;
	wxToggleButton* spawn_brush_button;

	DECLARE_EVENT_TABLE();
};

#endif
