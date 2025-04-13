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

#ifndef RME_PALETTE_BRUSHLIST_
#define RME_PALETTE_BRUSHLIST_

#include "main.h"
#include "palette_common.h"

enum BrushListType {
	BRUSHLIST_LARGE_ICONS,
	BRUSHLIST_SMALL_ICONS,
	BRUSHLIST_LISTBOX,
	BRUSHLIST_TEXT_LISTBOX,
	BRUSHLIST_GRID,
	BRUSHLIST_DIRECT_DRAW,
	BRUSHLIST_SEAMLESS_GRID
};

// Custom ID for Quick Add button
#define BUTTON_QUICK_ADD_ITEM 1001

class BrushBoxInterface {
public:
	BrushBoxInterface(const TilesetCategory* _tileset) :
		tileset(_tileset), loaded(false) {
		ASSERT(tileset);
	}
	virtual ~BrushBoxInterface() { }

	virtual wxWindow* GetSelfWindow() = 0;

	// Select the first brush
	virtual void SelectFirstBrush() = 0;
	// Returns the currently selected brush (First brush if panel is not loaded)
	virtual Brush* GetSelectedBrush() const = 0;
	// Select the brush in the parameter, this only changes the look of the panel
	virtual bool SelectBrush(const Brush* brush) = 0;

protected:
	const TilesetCategory* tileset;
	bool loaded;
};

// New class for direct drawing of sprites in grid
class DirectDrawBrushPanel : public wxScrolledWindow, public BrushBoxInterface {
public:
	DirectDrawBrushPanel(wxWindow* parent, const TilesetCategory* _tileset);
	~DirectDrawBrushPanel();

	wxWindow* GetSelfWindow() { return this; }

	// Select the first brush
	void SelectFirstBrush();
	// Returns the currently selected brush
	Brush* GetSelectedBrush() const;
	// Select the brush in the parameter
	bool SelectBrush(const Brush* brush);

	// Event handling
	void OnMouseClick(wxMouseEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnScroll(wxScrollWinEvent& event);
	void OnTimer(wxTimerEvent& event);

protected:
	void RecalculateGrid();
	void DrawItemsToPanel(wxDC& dc);
	void UpdateViewableItems(); // Method for lazy loading visible items
	void StartProgressiveLoading(); // Method for progressive loading with visual feedback

protected:
	int columns;
	int item_width;
	int item_height;
	int selected_index;
	wxBitmap* buffer;
	
	// Lazy loading properties
	int first_visible_row;
	int last_visible_row;
	int visible_rows_margin; // Extra rows to load above/below viewport
	int total_rows;
	bool need_full_redraw;
	
	// Progressive loading properties
	bool use_progressive_loading;
	bool is_large_tileset;
	int loading_step;
	int max_loading_steps;
	wxTimer* loading_timer;
	static const int LARGE_TILESET_THRESHOLD = 500; // Number of items considered "large"

	DECLARE_EVENT_TABLE();
};

class BrushListBox : public wxVListBox, public BrushBoxInterface {
public:
	BrushListBox(wxWindow* parent, const TilesetCategory* _tileset);
	~BrushListBox();

	wxWindow* GetSelfWindow() {
		return this;
	}

	// Select the first brush
	void SelectFirstBrush();
	// Returns the currently selected brush (First brush if panel is not loaded)
	Brush* GetSelectedBrush() const;
	// Select the brush in the parameter, this only changes the look of the panel
	bool SelectBrush(const Brush* brush);

	// Event handlers
	virtual void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const;
	virtual wxCoord OnMeasureItem(size_t n) const;

	void OnKey(wxKeyEvent& event);

	DECLARE_EVENT_TABLE();
};

class BrushIconBox : public wxScrolledWindow, public BrushBoxInterface {
public:
	BrushIconBox(wxWindow* parent, const TilesetCategory* _tileset, RenderSize rsz);
	~BrushIconBox();

	wxWindow* GetSelfWindow() {
		return this;
	}

	// Scrolls the window to the position of the named brush button
	void EnsureVisible(BrushButton* btn);
	void EnsureVisible(size_t n);

	// Select the first brush
	void SelectFirstBrush();
	// Returns the currently selected brush (First brush if panel is not loaded)
	Brush* GetSelectedBrush() const;
	// Select the brush in the parameter, this only changes the look of the panel
	bool SelectBrush(const Brush* brush);

	// Event handling...
	void OnClickBrushButton(wxCommandEvent& event);

protected:
	// Used internally to deselect all buttons before selecting a newd one.
	void DeselectAll();

protected:
	std::vector<BrushButton*> brush_buttons;
	RenderSize icon_size;

	DECLARE_EVENT_TABLE();
};

class BrushGridBox : public wxScrolledWindow, public BrushBoxInterface {
public:
	BrushGridBox(wxWindow* parent, const TilesetCategory* _tileset);
	~BrushGridBox();

	wxWindow* GetSelfWindow() { return this; }

	// Select the first brush
	void SelectFirstBrush();
	// Returns the currently selected brush
	Brush* GetSelectedBrush() const;
	// Select the brush in the parameter
	bool SelectBrush(const Brush* brush);

	// Event handling
	void OnClickBrushButton(wxCommandEvent& event);
	void OnSize(wxSizeEvent& event);

protected:
	void RecalculateGrid();
	void DeselectAll();

protected:
	std::vector<BrushButton*> brush_buttons;
	wxFlexGridSizer* grid_sizer;
	int columns;

	DECLARE_EVENT_TABLE();
};

// New class for seamless sprite grid with direct rendering
class SeamlessGridPanel : public wxScrolledWindow, public BrushBoxInterface {
public:
	SeamlessGridPanel(wxWindow* parent, const TilesetCategory* _tileset);
	~SeamlessGridPanel();

	wxWindow* GetSelfWindow() { return this; }

	// Select the first brush
	void SelectFirstBrush();
	// Returns the currently selected brush
	Brush* GetSelectedBrush() const;
	// Select the brush in the parameter
	bool SelectBrush(const Brush* brush);
	
	// Set whether to display item IDs
	void SetShowItemIDs(bool show) { show_item_ids = show; Refresh(); }
	bool IsShowingItemIDs() const { return show_item_ids; }

	// Event handling
	void OnMouseClick(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnScroll(wxScrollWinEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnKeyDown(wxKeyEvent& event); // New keyboard handler

protected:
	void RecalculateGrid();
	void DrawItemsToPanel(wxDC& dc);
	void UpdateViewableItems();
	void StartProgressiveLoading();
	int GetSpriteIndexAt(int x, int y) const; // Returns sprite index at screen position
	void DrawSpriteAt(wxDC& dc, int x, int y, int index); // Helper to draw a sprite at the specified position
	void SelectIndex(int index); // Helper to select and ensure visibility of an index

protected:
	int columns;
	int sprite_size;     // Size of each sprite/item
	int selected_index;  // Currently selected item index
	int hover_index;     // Index of item under mouse cursor
	wxBitmap* buffer;
	bool show_item_ids;  // Whether to show item IDs

	// Viewport and loading management
	int first_visible_row;
	int last_visible_row;
	int visible_rows_margin;
	int total_rows;
	bool need_full_redraw;
	
	// Progressive loading properties
	bool use_progressive_loading;
	bool is_large_tileset;
	int loading_step;
	int max_loading_steps;
	wxTimer* loading_timer;
	static const int LARGE_TILESET_THRESHOLD = 500;

	DECLARE_EVENT_TABLE();
};

// A panel capable of displaying a collection of brushes
class BrushPanel : public wxPanel {
public:
	BrushPanel(wxWindow* parent);
	~BrushPanel();

	// Interface
	void InvalidateContents();
	void LoadContents();

	// Sets the display type (list or icons)
	void SetListType(BrushListType ltype);
	void SetListType(wxString ltype);
	// Assigns a tileset to this list
	void AssignTileset(const TilesetCategory* tileset);

	// Select the first brush
	void SelectFirstBrush();
	// Returns the currently selected brush
	Brush* GetSelectedBrush() const;
	// Select the brush in the parameter
	bool SelectBrush(const Brush* whatbrush);

	// Called when the window is about to be displayed
	void OnSwitchIn();
	// Called when this page is hidden
	void OnSwitchOut();

	// wxWidgets event handlers
	void OnClickListBoxRow(wxCommandEvent& event);
	void OnViewModeToggle(wxCommandEvent& event);
	void OnShowItemIDsToggle(wxCommandEvent& event);

protected:
	void LoadViewMode();

protected:
	const TilesetCategory* tileset;
	wxSizer* sizer;
	BrushBoxInterface* brushbox;
	bool loaded;
	BrushListType list_type;
	wxCheckBox* view_mode_toggle;
	wxChoice* view_type_choice;
	wxCheckBox* show_ids_toggle;

	DECLARE_EVENT_TABLE();
};

class BrushPalettePanel : public PalettePanel {
public:
	BrushPalettePanel(wxWindow* parent, const TilesetContainer& tilesets, TilesetCategoryType category, wxWindowID id = wxID_ANY);
	virtual ~BrushPalettePanel();

	// Structure to hold selection information
	struct SelectionInfo {
		std::vector<Brush*> brushes;
	};
	
	// Get currently selected brushes
	const SelectionInfo& GetSelectionInfo() const;

	virtual void InvalidateContents();
	virtual void LoadCurrentContents();
	virtual void LoadAllContents();

	PaletteType GetType() const;

	// Sets the display type (list or icons)
	void SetListType(BrushListType ltype);
	void SetListType(wxString ltype);

	virtual void SelectFirstBrush();
	virtual Brush* GetSelectedBrush() const;
	virtual bool SelectBrush(const Brush* whatbrush);

	virtual void OnSwitchIn();
	void OnSwitchingPage(wxChoicebookEvent& event);
	void OnPageChanged(wxChoicebookEvent& event);
	void OnClickAddTileset(wxCommandEvent& WXUNUSED(event));
	void OnClickAddItemToTileset(wxCommandEvent& WXUNUSED(event));
	void OnClickQuickAddItem(wxCommandEvent& WXUNUSED(event));

protected:
	wxButton* quick_add_button;
	std::string last_tileset_name;
	wxChoicebook* choicebook;
	TilesetCategoryType palette_type;
	PalettePanel* size_panel;
	std::map<wxWindow*, Brush*> remembered_brushes;

	DECLARE_EVENT_TABLE();
};

#endif
