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

#include "palette_brushlist.h"
#include "gui.h"
#include "brush.h"
#include "raw_brush.h"
#include "add_tileset_window.h"
#include "add_item_window.h"
#include "materials.h"

// ============================================================================
// Brush Palette Panel
// A common class for terrain/doodad/item/raw palette

BEGIN_EVENT_TABLE(BrushPalettePanel, PalettePanel)
EVT_BUTTON(wxID_ADD, BrushPalettePanel::OnClickAddItemToTileset)
EVT_BUTTON(wxID_NEW, BrushPalettePanel::OnClickAddTileset)
EVT_BUTTON(BUTTON_QUICK_ADD_ITEM, BrushPalettePanel::OnClickQuickAddItem)
EVT_CHOICEBOOK_PAGE_CHANGING(wxID_ANY, BrushPalettePanel::OnSwitchingPage)
EVT_CHOICEBOOK_PAGE_CHANGED(wxID_ANY, BrushPalettePanel::OnPageChanged)
END_EVENT_TABLE()

BrushPalettePanel::BrushPalettePanel(wxWindow* parent, const TilesetContainer& tilesets, TilesetCategoryType category, wxWindowID id) :
	PalettePanel(parent, id),
	palette_type(category),
	choicebook(nullptr),
	size_panel(nullptr),
	quick_add_button(nullptr),
	last_tileset_name("") {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	// Create the tileset panel
	wxSizer* ts_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Tileset");
	wxChoicebook* tmp_choicebook = newd wxChoicebook(this, wxID_ANY, wxDefaultPosition, wxSize(180, 250));
	ts_sizer->Add(tmp_choicebook, 1, wxEXPAND);
	topsizer->Add(ts_sizer, 1, wxEXPAND);

	if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR)) {
		wxSizer* tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
		wxButton* buttonAddTileset = newd wxButton(this, wxID_NEW, "Add new Tileset");
		tmpsizer->Add(buttonAddTileset, wxSizerFlags(0).Center());

		wxButton* buttonAddItemToTileset = newd wxButton(this, wxID_ADD, "Add new Item");
		tmpsizer->Add(buttonAddItemToTileset, wxSizerFlags(0).Center());
		
		// Create the Quick Add Item button, initially disabled
		quick_add_button = newd wxButton(this, BUTTON_QUICK_ADD_ITEM, "Quick Add Item");
		quick_add_button->SetToolTip("Quickly add the currently selected brush to the last used tileset");
		quick_add_button->Enable(false); // Disabled until a tileset is added
		tmpsizer->Add(quick_add_button, wxSizerFlags(0).Center());

		topsizer->Add(tmpsizer, 0, wxCENTER, 10);
	}

	for (TilesetContainer::const_iterator iter = tilesets.begin(); iter != tilesets.end(); ++iter) {
		const TilesetCategory* tcg = iter->second->getCategory(category);
		if (tcg && tcg->size() > 0) {
			BrushPanel* panel = newd BrushPanel(tmp_choicebook);
			panel->AssignTileset(tcg);
			tmp_choicebook->AddPage(panel, wxstr(iter->second->name));
		}
	}

	SetSizerAndFit(topsizer);

	choicebook = tmp_choicebook;
}

BrushPalettePanel::~BrushPalettePanel() {
	////
}

void BrushPalettePanel::InvalidateContents() {
	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->InvalidateContents();
	}
	PalettePanel::InvalidateContents();
}

void BrushPalettePanel::LoadCurrentContents() {
	wxWindow* page = choicebook->GetCurrentPage();
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	if (panel) {
		panel->OnSwitchIn();
	}
	PalettePanel::LoadCurrentContents();
}

void BrushPalettePanel::LoadAllContents() {
	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->LoadContents();
	}
	PalettePanel::LoadAllContents();
}

PaletteType BrushPalettePanel::GetType() const {
	return palette_type;
}

void BrushPalettePanel::SetListType(wxString ltype) {
	if (!choicebook) {
		return;
	}
	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->SetListType(ltype);
	}
}

Brush* BrushPalettePanel::GetSelectedBrush() const {
	if (!choicebook) {
		return nullptr;
	}
	wxWindow* page = choicebook->GetCurrentPage();
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	Brush* res = nullptr;
	if (panel) {
		for (ToolBarList::const_iterator iter = tool_bars.begin(); iter != tool_bars.end(); ++iter) {
			res = (*iter)->GetSelectedBrush();
			if (res) {
				return res;
			}
		}
		res = panel->GetSelectedBrush();
	}
	return res;
}

void BrushPalettePanel::SelectFirstBrush() {
	if (!choicebook) {
		return;
	}
	wxWindow* page = choicebook->GetCurrentPage();
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	panel->SelectFirstBrush();
}

bool BrushPalettePanel::SelectBrush(const Brush* whatbrush) {
	if (!choicebook) {
		return false;
	}

	BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetCurrentPage());
	if (!panel) {
		return false;
	}

	for (PalettePanel* toolBar : tool_bars) {
		if (toolBar->SelectBrush(whatbrush)) {
			panel->SelectBrush(nullptr);
			return true;
		}
	}

	if (panel->SelectBrush(whatbrush)) {
		for (PalettePanel* toolBar : tool_bars) {
			toolBar->SelectBrush(nullptr);
		}
		return true;
	}

	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		if ((int)iz == choicebook->GetSelection()) {
			continue;
		}

		panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		if (panel && panel->SelectBrush(whatbrush)) {
			choicebook->ChangeSelection(iz);
			for (PalettePanel* toolBar : tool_bars) {
				toolBar->SelectBrush(nullptr);
			}
			return true;
		}
	}
	return false;
}

void BrushPalettePanel::OnSwitchingPage(wxChoicebookEvent& event) {
	event.Skip();
	if (!choicebook) {
		return;
	}
	BrushPanel* old_panel = dynamic_cast<BrushPanel*>(choicebook->GetCurrentPage());
	if (old_panel) {
		old_panel->OnSwitchOut();
		for (ToolBarList::iterator iter = tool_bars.begin(); iter != tool_bars.end(); ++iter) {
			Brush* tmp = (*iter)->GetSelectedBrush();
			if (tmp) {
				remembered_brushes[old_panel] = tmp;
			}
		}
	}

	wxWindow* page = choicebook->GetPage(event.GetSelection());
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	if (panel) {
		panel->OnSwitchIn();
		for (ToolBarList::iterator iter = tool_bars.begin(); iter != tool_bars.end(); ++iter) {
			(*iter)->SelectBrush(remembered_brushes[panel]);
		}
	}
}

void BrushPalettePanel::OnPageChanged(wxChoicebookEvent& event) {
	if (!choicebook) {
		return;
	}
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void BrushPalettePanel::OnSwitchIn() {
	LoadCurrentContents();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetBrushSizeInternal(last_brush_size);
	OnUpdateBrushSize(g_gui.GetBrushShape(), last_brush_size);
}

void BrushPalettePanel::OnClickAddTileset(wxCommandEvent& WXUNUSED(event)) {
	if (!choicebook) {
		return;
	}

	wxDialog* w = newd AddTilesetWindow(g_gui.root, palette_type);
	int ret = w->ShowModal();
	w->Destroy();

	if (ret != 0) {
		g_gui.DestroyPalettes();
		g_gui.NewPalette();
	}
}

void BrushPalettePanel::OnClickAddItemToTileset(wxCommandEvent& WXUNUSED(event)) {
	if (!choicebook) {
		return;
	}
	std::string tilesetName = choicebook->GetPageText(choicebook->GetSelection());

	auto _it = g_materials.tilesets.find(tilesetName);
	if (_it != g_materials.tilesets.end()) {
		// Get the currently selected brush
		Brush* brush = GetSelectedBrush();
		uint16_t item_id = 0;
		
		// Try to get the item ID from the brush if it's a RAW brush
		if (brush && brush->isRaw()) {
			RAWBrush* raw_brush = brush->asRaw();
			if (raw_brush) {
				item_id = raw_brush->getItemID();
			}
		}
		
		// Create the Add Item Window
		wxDialog* w = newd AddItemWindow(g_gui.root, palette_type, _it->second);
		
		// If we have a valid item ID, set it in the window
		if (item_id > 0) {
			AddItemWindow* addItemWindow = static_cast<AddItemWindow*>(w);
			addItemWindow->SetItemIdToItemButton(item_id);
		}
		
		int ret = w->ShowModal();
		
		// Get the selected tileset name from the dialog
		AddItemWindow* addItemWindow = static_cast<AddItemWindow*>(w);
		if (addItemWindow->tileset_choice && 
			addItemWindow->tileset_choice->GetSelection() != wxNOT_FOUND) {
			tilesetName = nstr(addItemWindow->tileset_choice->GetString(
				addItemWindow->tileset_choice->GetSelection()));
		}
		
		w->Destroy();

		if (ret != 0) {
			// Item was successfully added, store the tileset name for Quick Add
			last_tileset_name = tilesetName;
			
			// Enable the Quick Add button
			if (quick_add_button) {
				quick_add_button->Enable(true);
			}
			
			g_gui.RebuildPalettes();
		}
	}
}

void BrushPalettePanel::OnClickQuickAddItem(wxCommandEvent& WXUNUSED(event)) {
	// Check if we have a last used tileset name
	if (last_tileset_name.empty()) {
		g_gui.PopupDialog("Error", "No tileset has been used yet. Please use Add Item first.", wxOK);
		return;
	}
	
	// Get the currently selected brush
	Brush* brush = GetSelectedBrush();
	if (!brush) {
		g_gui.PopupDialog("Error", "No brush is currently selected.", wxOK);
		return;
	}
	
	// Check if the brush is a RAW brush that we can add to the tileset
	if (!brush->isRaw()) {
		g_gui.PopupDialog("Error", "Only raw items can be added to tilesets.", wxOK);
		return;
	}
	
	RAWBrush* raw_brush = brush->asRaw();
	if (!raw_brush) {
		g_gui.PopupDialog("Error", "Failed to get item data from the selected brush.", wxOK);
		return;
	}
	
	uint16_t item_id = raw_brush->getItemID();
	
	// Check if the tileset still exists
	auto it = g_materials.tilesets.find(last_tileset_name);
	if (it == g_materials.tilesets.end()) {
		g_gui.PopupDialog("Error", "The last used tileset no longer exists.", wxOK);
		last_tileset_name = "";
		if (quick_add_button) {
			quick_add_button->Enable(false);
		}
		return;
	}
	
	// Add the item to the tileset
	g_materials.addToTileset(last_tileset_name, item_id, palette_type);
	g_materials.modify();
	
	// Show success message with the item name and ID
	const ItemType& item_type = g_items.getItemType(item_id);
	g_gui.PopupDialog("Item Added", wxString::Format("Item '%s' (ID: %d) has been added to tileset '%s'", 
		wxString(item_type.name.c_str()), item_id, wxString(last_tileset_name.c_str())), wxOK);
	
	// Rebuild palettes to show the changes
	g_gui.RebuildPalettes();
}

const BrushPalettePanel::SelectionInfo& BrushPalettePanel::GetSelectionInfo() const {
	static SelectionInfo selection;
	selection.brushes.clear();
	
	// First add the currently selected brush if available
	Brush* selected = GetSelectedBrush();
	if (selected) {
		selection.brushes.push_back(selected);
	}
	
	// Now, depending on the panel type, try to get more brushes
	if (choicebook) {
		wxWindow* page = choicebook->GetCurrentPage();
		BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
		if (panel) {
			// Here we could add additional brushes based on multi-selection
			// if implemented in the various panel types
		}
	}
	
	return selection;
}

// ============================================================================
// Brush Panel
// A container of brush buttons

BEGIN_EVENT_TABLE(BrushPanel, wxPanel)
EVT_LISTBOX(wxID_ANY, BrushPanel::OnClickListBoxRow)
EVT_CHECKBOX(wxID_ANY, BrushPanel::OnViewModeToggle)
END_EVENT_TABLE()

BrushPanel::BrushPanel(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	tileset(nullptr),
	brushbox(nullptr),
	loaded(false),
	list_type(BRUSHLIST_LISTBOX),
	view_mode_toggle(nullptr),
	view_type_choice(nullptr),
	show_ids_toggle(nullptr) {
	
	sizer = new wxBoxSizer(wxVERTICAL);
	
	// Add view mode toggle checkbox
	view_mode_toggle = new wxCheckBox(this, wxID_ANY, "Grid View");
	view_mode_toggle->SetValue(false);
	sizer->Add(view_mode_toggle, 0, wxALL, 5);
	
	// Always add the Show Item IDs checkbox right after the Grid View checkbox
	show_ids_toggle = new wxCheckBox(this, wxID_ANY, "Show Item IDs");
	show_ids_toggle->SetValue(false);
	show_ids_toggle->Bind(wxEVT_CHECKBOX, &BrushPanel::OnShowItemIDsToggle, this);
	sizer->Add(show_ids_toggle, 0, wxALL, 5);
	
	// Add a choice for view types if we're in RAW palette
	if(tileset && tileset->getType() == TILESET_RAW) {
		wxBoxSizer* choice_sizer = new wxBoxSizer(wxHORIZONTAL);
		wxStaticText* label = new wxStaticText(this, wxID_ANY, "View Type:");
		choice_sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		
		view_type_choice = new wxChoice(this, wxID_ANY);
		view_type_choice->Append("Normal");
		view_type_choice->Append("Direct Draw");
		view_type_choice->SetSelection(0);
		view_type_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& event) {
			if(loaded) {
				LoadViewMode();
			}
		});
		choice_sizer->Add(view_type_choice, 1);
		sizer->Add(choice_sizer, 0, wxEXPAND | wxALL, 5);
	} else {
		view_type_choice = nullptr;
	}
	
	SetSizer(sizer);
}

BrushPanel::~BrushPanel() {
	////
}

void BrushPanel::AssignTileset(const TilesetCategory* _tileset) {
	if (_tileset != tileset) {
		InvalidateContents();
		tileset = _tileset;
	}
}

void BrushPanel::SetListType(BrushListType ltype) {
	if (list_type != ltype) {
		InvalidateContents();
		list_type = ltype;
		
		// Update the checkbox state when list type changes
		if (view_mode_toggle) {
			view_mode_toggle->SetValue(false);
		}
	}
}

void BrushPanel::SetListType(wxString ltype) {
	if (ltype == "small icons") {
		SetListType(BRUSHLIST_SMALL_ICONS);
	} else if (ltype == "large icons") {
		SetListType(BRUSHLIST_LARGE_ICONS);
	} else if (ltype == "listbox") {
		SetListType(BRUSHLIST_LISTBOX);
	} else if (ltype == "textlistbox") {
		SetListType(BRUSHLIST_TEXT_LISTBOX);
	} else if (ltype == "direct draw") {
		SetListType(BRUSHLIST_DIRECT_DRAW);
	} else if (ltype == "seamless grid") {
		SetListType(BRUSHLIST_SEAMLESS_GRID);
	}
}

void BrushPanel::InvalidateContents() {
	sizer->Clear(true);
	loaded = false;
	brushbox = nullptr;
	
	// Add the view mode toggle back after clearing
	view_mode_toggle = new wxCheckBox(this, wxID_ANY, "Grid View");
	view_mode_toggle->SetValue(false);
	sizer->Add(view_mode_toggle, 0, wxALL, 5);
	
	// Always add the Show Item IDs checkbox right after the Grid View checkbox
	show_ids_toggle = new wxCheckBox(this, wxID_ANY, "Show Item IDs");
	show_ids_toggle->SetValue(false);
	show_ids_toggle->Bind(wxEVT_CHECKBOX, &BrushPanel::OnShowItemIDsToggle, this);
	sizer->Add(show_ids_toggle, 0, wxALL, 5);
	
	// Add a choice for view types if we're in RAW palette
	if(tileset && tileset->getType() == TILESET_RAW) {
		wxBoxSizer* choice_sizer = new wxBoxSizer(wxHORIZONTAL);
		wxStaticText* label = new wxStaticText(this, wxID_ANY, "View Type:");
		choice_sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		
		view_type_choice = new wxChoice(this, wxID_ANY);
		view_type_choice->Append("Normal");
		view_type_choice->Append("Direct Draw");
		view_type_choice->SetSelection(0);
		view_type_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& event) {
			if(loaded) {
				LoadViewMode();
			}
		});
		choice_sizer->Add(view_type_choice, 1);
		sizer->Add(choice_sizer, 0, wxEXPAND | wxALL, 5);
	} else {
		view_type_choice = nullptr;
	}
}

void BrushPanel::LoadContents() {
	if (loaded) {
		return;
	}
	loaded = true;
	ASSERT(tileset != nullptr);
	
	LoadViewMode();
}

void BrushPanel::LoadViewMode() {
	// Remove old brushbox if it exists
	if (brushbox) {
		sizer->Detach(brushbox->GetSelfWindow());
		brushbox->GetSelfWindow()->Destroy();
		brushbox = nullptr;
	}
	
	// Check if we're using DirectDraw for RAW palette
	if(tileset && tileset->getType() == TILESET_RAW && view_type_choice && view_type_choice->GetSelection() == 1) {
		brushbox = new DirectDrawBrushPanel(this, tileset);
	}
	// Direct draw from list type setting
	else if(list_type == BRUSHLIST_DIRECT_DRAW && tileset && tileset->getType() == TILESET_RAW) {
		brushbox = new DirectDrawBrushPanel(this, tileset);
	}
	// Seamless Grid View - new option that takes precedence when Grid View is enabled
	else if (view_mode_toggle && view_mode_toggle->GetValue()) {
		SeamlessGridPanel* sgp = new SeamlessGridPanel(this, tileset);
		// Set show IDs based on the checkbox value if it exists
		if (show_ids_toggle) {
			sgp->SetShowItemIDs(show_ids_toggle->GetValue());
		}
		brushbox = sgp;
	}
	// Otherwise use the list type setting
	else {
		switch (list_type) {
			case BRUSHLIST_LARGE_ICONS:
				brushbox = new BrushIconBox(this, tileset, RENDER_SIZE_32x32);
				break;
			case BRUSHLIST_SMALL_ICONS:
				brushbox = new BrushIconBox(this, tileset, RENDER_SIZE_16x16);
				break;
			case BRUSHLIST_SEAMLESS_GRID: {
				SeamlessGridPanel* sgp = new SeamlessGridPanel(this, tileset);
				// Set show IDs based on the checkbox value if it exists
				if (show_ids_toggle) {
					sgp->SetShowItemIDs(show_ids_toggle->GetValue());
				}
				brushbox = sgp;
				break;
			}
			case BRUSHLIST_LISTBOX:
			default:
				brushbox = new BrushListBox(this, tileset);
				break;
		}
	}
	
	sizer->Add(brushbox->GetSelfWindow(), 1, wxEXPAND);
	Layout();
	brushbox->SelectFirstBrush();
}

void BrushPanel::SelectFirstBrush() {
	if (loaded) {
		ASSERT(brushbox != nullptr);
		brushbox->SelectFirstBrush();
	}
}

Brush* BrushPanel::GetSelectedBrush() const {
	if (loaded) {
		ASSERT(brushbox != nullptr);
		return brushbox->GetSelectedBrush();
	}

	if (tileset && tileset->size() > 0) {
		return tileset->brushlist[0];
	}
	return nullptr;
}

bool BrushPanel::SelectBrush(const Brush* whatbrush) {
	if (loaded) {
		ASSERT(brushbox != nullptr);
		return brushbox->SelectBrush(whatbrush);
	}

	for (BrushVector::const_iterator iter = tileset->brushlist.begin(); iter != tileset->brushlist.end(); ++iter) {
		if (*iter == whatbrush) {
			LoadContents();
			return brushbox->SelectBrush(whatbrush);
		}
	}
	return false;
}

void BrushPanel::OnSwitchIn() {
	LoadContents();
}

void BrushPanel::OnSwitchOut() {
	////
}

void BrushPanel::OnClickListBoxRow(wxCommandEvent& event) {
	ASSERT(tileset->getType() >= TILESET_UNKNOWN && tileset->getType() <= TILESET_HOUSE);
	// We just notify the GUI of the action, it will take care of everything else
	ASSERT(brushbox);
	size_t n = event.GetSelection();

	wxWindow* w = this;
	while ((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr)
		;

	if (w) {
		g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
	}

	g_gui.SelectBrush(tileset->brushlist[n], tileset->getType());
}

void BrushPanel::OnViewModeToggle(wxCommandEvent& event) {
	if (loaded) {
		LoadViewMode();
	}
}

void BrushPanel::OnShowItemIDsToggle(wxCommandEvent& event) {
	if (loaded && brushbox) {
		// If we're using a seamless grid panel, update the display of item IDs
		SeamlessGridPanel* sgp = dynamic_cast<SeamlessGridPanel*>(brushbox);
		if (sgp) {
			sgp->SetShowItemIDs(show_ids_toggle->GetValue());
		} else {
			// If we're not using a seamless grid panel, reload the view
			LoadViewMode();
		}
	}
}

// ============================================================================
// BrushIconBox

BEGIN_EVENT_TABLE(BrushIconBox, wxScrolledWindow)
// Listbox style
EVT_TOGGLEBUTTON(wxID_ANY, BrushIconBox::OnClickBrushButton)
END_EVENT_TABLE()

BrushIconBox::BrushIconBox(wxWindow* parent, const TilesetCategory* _tileset, RenderSize rsz) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL),
	BrushBoxInterface(_tileset),
	icon_size(rsz) {
	ASSERT(tileset->getType() >= TILESET_UNKNOWN && tileset->getType() <= TILESET_HOUSE);
	int width;
	if (icon_size == RENDER_SIZE_32x32) {
		width = max(g_settings.getInteger(Config::PALETTE_COL_COUNT) / 2 + 1, 1);
	} else {
		width = max(g_settings.getInteger(Config::PALETTE_COL_COUNT) + 1, 1);
	}

	// Create buttons
	wxSizer* stacksizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* rowsizer = nullptr;
	int item_counter = 0;
	for (BrushVector::const_iterator iter = tileset->brushlist.begin(); iter != tileset->brushlist.end(); ++iter) {
		ASSERT(*iter);
		++item_counter;

		if (!rowsizer) {
			rowsizer = newd wxBoxSizer(wxHORIZONTAL);
		}

		BrushButton* bb = newd BrushButton(this, *iter, rsz);
		rowsizer->Add(bb);
		brush_buttons.push_back(bb);

		if (item_counter % width == 0) { // newd row
			stacksizer->Add(rowsizer);
			rowsizer = nullptr;
		}
	}
	if (rowsizer) {
		stacksizer->Add(rowsizer);
	}

	SetScrollbars(20, 20, 8, item_counter / width, 0, 0);
	SetSizer(stacksizer);
}

BrushIconBox::~BrushIconBox() {
	////
}

void BrushIconBox::SelectFirstBrush() {
	if (tileset && tileset->size() > 0) {
		DeselectAll();
		brush_buttons[0]->SetValue(true);
		EnsureVisible((size_t)0);
	}
}

Brush* BrushIconBox::GetSelectedBrush() const {
	if (!tileset) {
		return nullptr;
	}

	for (std::vector<BrushButton*>::const_iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		if ((*it)->GetValue()) {
			return (*it)->brush;
		}
	}
	return nullptr;
}

bool BrushIconBox::SelectBrush(const Brush* whatbrush) {
	DeselectAll();
	for (std::vector<BrushButton*>::iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		if ((*it)->brush == whatbrush) {
			(*it)->SetValue(true);
			EnsureVisible(*it);
			return true;
		}
	}
	return false;
}

void BrushIconBox::DeselectAll() {
	for (std::vector<BrushButton*>::iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		(*it)->SetValue(false);
	}
}

void BrushIconBox::EnsureVisible(BrushButton* btn) {
	int windowSizeX, windowSizeY;
	GetVirtualSize(&windowSizeX, &windowSizeY);

	int scrollUnitX;
	int scrollUnitY;
	GetScrollPixelsPerUnit(&scrollUnitX, &scrollUnitY);

	wxRect rect = btn->GetRect();
	int y;
	CalcUnscrolledPosition(0, rect.y, nullptr, &y);

	int maxScrollPos = windowSizeY / scrollUnitY;
	int scrollPosY = std::min(maxScrollPos, (y / scrollUnitY));

	int startScrollPosY;
	GetViewStart(nullptr, &startScrollPosY);

	int clientSizeX, clientSizeY;
	GetClientSize(&clientSizeX, &clientSizeY);
	int endScrollPosY = startScrollPosY + clientSizeY / scrollUnitY;

	if (scrollPosY < startScrollPosY || scrollPosY > endScrollPosY) {
		// only scroll if the button isnt visible
		Scroll(-1, scrollPosY);
	}
}

void BrushIconBox::EnsureVisible(size_t n) {
	EnsureVisible(brush_buttons[n]);
}

void BrushIconBox::OnClickBrushButton(wxCommandEvent& event) {
	wxObject* obj = event.GetEventObject();
	BrushButton* btn = dynamic_cast<BrushButton*>(obj);
	if (btn) {
		wxWindow* w = this;
		while ((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr)
			;
		if (w) {
			g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
		}
		g_gui.SelectBrush(btn->brush, tileset->getType());
	}
}

// ============================================================================
// BrushListBox

BEGIN_EVENT_TABLE(BrushListBox, wxVListBox)
EVT_KEY_DOWN(BrushListBox::OnKey)
END_EVENT_TABLE()

BrushListBox::BrushListBox(wxWindow* parent, const TilesetCategory* tileset) :
	wxVListBox(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE),
	BrushBoxInterface(tileset) {
	SetItemCount(tileset->size());
}

BrushListBox::~BrushListBox() {
	////
}

void BrushListBox::SelectFirstBrush() {
	SetSelection(0);
	wxWindow::ScrollLines(-1);
}

Brush* BrushListBox::GetSelectedBrush() const {
	if (!tileset) {
		return nullptr;
	}

	int n = GetSelection();
	if (n != wxNOT_FOUND) {
		return tileset->brushlist[n];
	} else if (tileset->size() > 0) {
		return tileset->brushlist[0];
	}
	return nullptr;
}

bool BrushListBox::SelectBrush(const Brush* whatbrush) {
	for (size_t n = 0; n < tileset->size(); ++n) {
		if (tileset->brushlist[n] == whatbrush) {
			SetSelection(n);
			return true;
		}
	}
	return false;
}

void BrushListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const {
	ASSERT(n < tileset->size());
	Sprite* spr = g_gui.gfx.getSprite(tileset->brushlist[n]->getLookID());
	if (spr) {
		spr->DrawTo(&dc, SPRITE_SIZE_32x32, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
	}
	if (IsSelected(n)) {
		if (HasFocus()) {
			dc.SetTextForeground(wxColor(0xFF, 0xFF, 0xFF));
		} else {
			dc.SetTextForeground(wxColor(0x00, 0x00, 0xFF));
		}
	} else {
		dc.SetTextForeground(wxColor(0x00, 0x00, 0x00));
	}
	dc.DrawText(wxstr(tileset->brushlist[n]->getName()), rect.GetX() + 40, rect.GetY() + 6);
}

wxCoord BrushListBox::OnMeasureItem(size_t n) const {
	return 32;
}

void BrushListBox::OnKey(wxKeyEvent& event) {
	switch (event.GetKeyCode()) {
		case WXK_UP:
		case WXK_DOWN:
		case WXK_LEFT:
		case WXK_RIGHT:
			if (g_settings.getInteger(Config::LISTBOX_EATS_ALL_EVENTS)) {
				case WXK_PAGEUP:
				case WXK_PAGEDOWN:
				case WXK_HOME:
				case WXK_END:
					event.Skip(true);
			} else {
				[[fallthrough]];
				default:
					if (g_gui.GetCurrentTab() != nullptr) {
						g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
					}
			}
	}
}

BEGIN_EVENT_TABLE(BrushGridBox, wxScrolledWindow)
EVT_TOGGLEBUTTON(wxID_ANY, BrushGridBox::OnClickBrushButton)
EVT_SIZE(BrushGridBox::OnSize)
END_EVENT_TABLE()

BrushGridBox::BrushGridBox(wxWindow* parent, const TilesetCategory* _tileset) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL),
	BrushBoxInterface(_tileset),
	grid_sizer(nullptr),
	columns(1) {
	
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	
	// Create grid sizer
	grid_sizer = new wxFlexGridSizer(0, columns, 2, 2); // 0 rows (dynamic), 1 column, 2px gaps
	
	// Create buttons for each brush
	for(BrushVector::const_iterator iter = tileset->brushlist.begin(); iter != tileset->brushlist.end(); ++iter) {
		ASSERT(*iter);
		BrushButton* bb = new BrushButton(this, *iter, RENDER_SIZE_32x32);
		
		// Set tooltip with item name and ID
		wxString tooltip;
		if((*iter)->isRaw()) {
			RAWBrush* raw = static_cast<RAWBrush*>(*iter);
			tooltip = wxString::Format("%s [%d]", raw->getName(), raw->getItemID());
		} else {
			tooltip = wxString::Format("%s", (*iter)->getName());
		}
		bb->SetToolTip(tooltip);
		
		grid_sizer->Add(bb, 0, wxALL, 1);
		brush_buttons.push_back(bb);
	}
	
	SetSizer(grid_sizer);
	
	// Enable scrolling
	FitInside();
	SetScrollRate(32, 32);
	
	// Calculate initial grid layout
	RecalculateGrid();
}

BrushGridBox::~BrushGridBox() {
	// Grid sizer will delete all children automatically
}

void BrushGridBox::SelectFirstBrush() {
	if(tileset && tileset->size() > 0) {
		DeselectAll();
		brush_buttons[0]->SetValue(true);
	}
}

Brush* BrushGridBox::GetSelectedBrush() const {
	if(!tileset) return nullptr;
	
	for(std::vector<BrushButton*>::const_iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		if((*it)->GetValue()) {
			return (*it)->brush;
		}
	}
	return nullptr;
}

bool BrushGridBox::SelectBrush(const Brush* whatbrush) {
	DeselectAll();
	for(std::vector<BrushButton*>::iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		if((*it)->brush == whatbrush) {
			(*it)->SetValue(true);
			return true;
		}
	}
	return false;
}

void BrushGridBox::DeselectAll() {
	for(std::vector<BrushButton*>::iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		(*it)->SetValue(false);
	}
}

void BrushGridBox::OnClickBrushButton(wxCommandEvent& event) {
	wxObject* obj = event.GetEventObject();
	BrushButton* btn = dynamic_cast<BrushButton*>(obj);
	if(btn) {
		wxWindow* w = this;
		while((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr);
		if(w) {
			g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
		}
		g_gui.SelectBrush(btn->brush, tileset->getType());
	}
}

void BrushGridBox::OnSize(wxSizeEvent& event) {
	RecalculateGrid();
	event.Skip();
}

void BrushGridBox::RecalculateGrid() {
	if(!grid_sizer) return;
	
	// Calculate how many columns can fit
	int window_width = GetClientSize().GetWidth();
	int button_width = 36; // 32px + 4px padding
	int new_columns = std::max(1, (window_width - 4) / button_width);
	
	if(new_columns != columns) {
		columns = new_columns;
		grid_sizer->SetCols(columns);
		grid_sizer->Layout();
		FitInside(); // Update scroll window virtual size
	}
}

// Implementation of DirectDrawBrushPanel

BEGIN_EVENT_TABLE(DirectDrawBrushPanel, wxScrolledWindow)
EVT_LEFT_DOWN(DirectDrawBrushPanel::OnMouseClick)
EVT_PAINT(DirectDrawBrushPanel::OnPaint)
EVT_SIZE(DirectDrawBrushPanel::OnSize)
EVT_SCROLLWIN(DirectDrawBrushPanel::OnScroll)
EVT_TIMER(wxID_ANY, DirectDrawBrushPanel::OnTimer)
END_EVENT_TABLE()

DirectDrawBrushPanel::DirectDrawBrushPanel(wxWindow* parent, const TilesetCategory* _tileset) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL),
	BrushBoxInterface(_tileset),
	columns(1),
	item_width(32),
	item_height(32),
	selected_index(-1),
	buffer(nullptr),
	first_visible_row(0),
	last_visible_row(0),
	visible_rows_margin(10),
	total_rows(0),
	need_full_redraw(true),
	use_progressive_loading(true),
	is_large_tileset(false),
	loading_step(0),
	max_loading_steps(5),
	loading_timer(nullptr) {
	
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	
	// Default values
	columns = 10;
	item_width = 36; // 32px + 4px padding
	item_height = 36;
	
	// Check if we're dealing with a large tileset
	is_large_tileset = tileset && tileset->size() > LARGE_TILESET_THRESHOLD;
	
	// Create loading timer for large tilesets
	if (is_large_tileset && use_progressive_loading) {
		loading_timer = new wxTimer(this);
		max_loading_steps = 10; // Increase steps for smoother progress indication
	}
	
	// Calculate initial grid layout
	RecalculateGrid();
	
	// Enable scrolling and focus events
	SetScrollRate(5, 5);
	SetFocusIgnoringChildren();
	
	// Start progressive loading if needed
	if (is_large_tileset && use_progressive_loading) {
		StartProgressiveLoading();
	}
}

DirectDrawBrushPanel::~DirectDrawBrushPanel() {
	if (buffer) {
		delete buffer;
	}
	
	if (loading_timer) {
		loading_timer->Stop();
		delete loading_timer;
	}
}

void DirectDrawBrushPanel::StartProgressiveLoading() {
	if (!loading_timer) return;
	
	// Reset loading step
	loading_step = 0;
	
	// Set initial small margin
	visible_rows_margin = 3;
	
	// Force full redraw
	need_full_redraw = true;
	
	// Only show a limited number of items initially
	int itemsToShowInitially = std::min(100, static_cast<int>(tileset->size()));
	
	// Calculate how many items to add with each loading step
	int itemsPerStep = (tileset->size() - itemsToShowInitially) / max_loading_steps;
	if (itemsPerStep < 50) {
		// For smaller tilesets, load faster
		max_loading_steps = std::max(3, static_cast<int>(tileset->size() / 50));
	}
	
	// Start timer for progressive loading
	loading_timer->Start(200); // 200ms interval for better visual feedback
	
	// Force initial redraw to show progress
	Refresh();
}

void DirectDrawBrushPanel::OnTimer(wxTimerEvent& event) {
	// Progressively increase the visible margin
	loading_step++;
	
	// Gradually increase the margin with each step
	visible_rows_margin = std::min(3 + loading_step * 5, 30);
	
	// Update viewable items with new margin
	UpdateViewableItems();
	
	// Force redraw to update progress
	Refresh();
	
	// Stop timer when we've reached max loading steps
	if (loading_step >= max_loading_steps) {
		loading_timer->Stop();
		need_full_redraw = true;
		Refresh();
	}
}

void DirectDrawBrushPanel::OnScroll(wxScrollWinEvent& event) {
	// Handle scroll events to update visible items
	UpdateViewableItems();
	
	// Reset progressive loading on scroll for large tilesets
	if (is_large_tileset && use_progressive_loading) {
		// Stop any existing timer
		if (loading_timer && loading_timer->IsRunning()) {
			loading_timer->Stop();
		}
		
		// If we haven't completed loading yet
		if (loading_step < max_loading_steps) {
			// Temporarily use small margin for immediate response
			visible_rows_margin = 3;
			UpdateViewableItems();
			
			// Show loading message in the area being scrolled to
			Refresh();
			
			// Restart progressive loading
			StartProgressiveLoading();
		} else {
			// If loading was already complete, just continue with full view
			visible_rows_margin = 30;
			UpdateViewableItems();
			Refresh();
		}
	}
	
	event.Skip();
}

void DirectDrawBrushPanel::UpdateViewableItems() {
	int xStart, yStart;
	GetViewStart(&xStart, &yStart);
	int ppuX, ppuY;
	GetScrollPixelsPerUnit(&ppuX, &ppuY);
	yStart *= ppuY;
	
	int width, height;
	GetClientSize(&width, &height);
	
	// Calculate visible range with margins
	int new_first_row = std::max(0, (yStart / item_height) - visible_rows_margin);
	int new_last_row = std::min(total_rows - 1, ((yStart + height) / item_height) + visible_rows_margin);
	
	// Only trigger redraw if visible range changes
	if (new_first_row != first_visible_row || new_last_row != last_visible_row) {
		first_visible_row = new_first_row;
		last_visible_row = new_last_row;
		Refresh();
	}
}

void DirectDrawBrushPanel::DrawItemsToPanel(wxDC& dc) {
	if(!tileset || tileset->size() == 0) return;
	
	// Calculate client area size
	int width, height;
	GetClientSize(&width, &height);
	
	// Draw loading progress for large datasets during initial load
	if (is_large_tileset && loading_step < max_loading_steps) {
		// Draw progress bar
		int progressWidth = width - 40;
		int progressHeight = 20;
		int progressX = 20;
		int progressY = 20;
		
		// Progress background
		dc.SetBrush(wxBrush(wxColor(200, 200, 200)));
		dc.SetPen(wxPen(wxColor(100, 100, 100)));
		dc.DrawRectangle(progressX, progressY, progressWidth, progressHeight);
		
		// Progress fill
		float progress = static_cast<float>(loading_step + 1) / max_loading_steps;
		dc.SetBrush(wxBrush(wxColor(0, 150, 0)));
		dc.SetPen(wxPen(wxColor(0, 100, 0)));
		dc.DrawRectangle(progressX, progressY, progressWidth * progress, progressHeight);
		
		// Progress text
		wxString loadingMsg = wxString::Format("Loading %zu items... (%d%%)", 
			tileset->size(), 
			static_cast<int>((loading_step + 1) * 100 / max_loading_steps));
		
		wxSize textSize = dc.GetTextExtent(loadingMsg);
		dc.SetTextForeground(wxColor(0, 0, 0));
		dc.DrawText(loadingMsg, (width - textSize.GetWidth()) / 2, progressY + progressHeight + 5);
		
		// Draw info about how many items are being processed
		int itemsProcessed = tileset->size() * progress;
		wxString itemsMsg = wxString::Format("Processed: %d / %zu items", 
			itemsProcessed, tileset->size());
		
		textSize = dc.GetTextExtent(itemsMsg);
		dc.DrawText(itemsMsg, (width - textSize.GetWidth()) / 2, progressY + progressHeight + 25);
		
		// Only draw a limited number of items during loading phase
		int maxItemsToDraw = itemsProcessed;
		
		// Only draw items in the visible range up to the processed amount
		for(int row = first_visible_row; row <= last_visible_row; ++row) {
			for(int col = 0; col < columns; ++col) {
				int index = row * columns + col;
				if(index >= static_cast<int>(tileset->size()) || index >= maxItemsToDraw) break;
				
				int x = col * item_width;
				int y = row * item_height;
				
				// Skip items that would appear where our progress bar is
				if (y < progressY + progressHeight + 40) continue;
				
				// Draw background for selected item
				if(index == selected_index) {
					dc.SetBrush(wxBrush(wxColor(180, 180, 255)));
					dc.SetPen(wxPen(wxColor(100, 100, 200)));
					dc.DrawRectangle(x, y, item_width, item_height);
				}
				
				// Draw item sprite
				Brush* brush = tileset->brushlist[index];
				if(brush) {
					Sprite* sprite = g_gui.gfx.getSprite(brush->getLookID());
					if(sprite) {
						sprite->DrawTo(&dc, SPRITE_SIZE_32x32, x + 2, y + 2);
					}
					
					// For RAW brushes, also draw the ID
					if(brush->isRaw()) {
						RAWBrush* raw = static_cast<RAWBrush*>(brush);
						wxString label = wxString::Format("%d", raw->getItemID());
						dc.SetTextForeground(wxColor(0, 0, 0));
						dc.DrawText(label, x + 2, y + item_height - 16);
					}
				}
			}
		}
	} else {
		// Normal drawing when fully loaded
		// Only draw items in the visible range
		for(int row = first_visible_row; row <= last_visible_row; ++row) {
			for(int col = 0; col < columns; ++col) {
				int index = row * columns + col;
				if(index >= static_cast<int>(tileset->size())) break;
				
				int x = col * item_width;
				int y = row * item_height;
				
				// Draw background for selected item
				if(index == selected_index) {
					dc.SetBrush(wxBrush(wxColor(180, 180, 255)));
					dc.SetPen(wxPen(wxColor(100, 100, 200)));
					dc.DrawRectangle(x, y, item_width, item_height);
				}
				
				// Draw item sprite
				Brush* brush = tileset->brushlist[index];
				if(brush) {
					Sprite* sprite = g_gui.gfx.getSprite(brush->getLookID());
					if(sprite) {
						sprite->DrawTo(&dc, SPRITE_SIZE_32x32, x + 2, y + 2);
					}
					
					// For RAW brushes, also draw the ID
					if(brush->isRaw()) {
						RAWBrush* raw = static_cast<RAWBrush*>(brush);
						wxString label = wxString::Format("%d", raw->getItemID());
						dc.SetTextForeground(wxColor(0, 0, 0));
						dc.DrawText(label, x + 2, y + item_height - 16);
					}
				}
			}
		}
	}
}

void DirectDrawBrushPanel::OnPaint(wxPaintEvent& event) {
	wxAutoBufferedPaintDC dc(this);
	DoPrepareDC(dc);  // For correct scrolling
	
	// Clear background
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();
	
	// Draw only the visible items
	DrawItemsToPanel(dc);
}

void DirectDrawBrushPanel::OnSize(wxSizeEvent& event) {
	RecalculateGrid();
	if(buffer) {
		delete buffer;
		buffer = nullptr;
	}
	Refresh();
	event.Skip();
}

void DirectDrawBrushPanel::SelectFirstBrush() {
	if(tileset && tileset->size() > 0) {
		selected_index = 0;
		Refresh();
	}
}

Brush* DirectDrawBrushPanel::GetSelectedBrush() const {
	if(!tileset || selected_index < 0 || selected_index >= static_cast<int>(tileset->size())) {
		return nullptr;
	}
	
	return tileset->brushlist[selected_index];
}

bool DirectDrawBrushPanel::SelectBrush(const Brush* whatbrush) {
	if(!tileset) return false;
	
	for(size_t i = 0; i < tileset->size(); ++i) {
		if(tileset->brushlist[i] == whatbrush) {
			selected_index = i;
			Refresh();
			
			// Ensure the selected item is visible
			int row = selected_index / columns;
			int yPos = row * item_height;
			
			// Get the visible area
			int xStart, yStart;
			GetViewStart(&xStart, &yStart);
			int ppuX, ppuY;
			GetScrollPixelsPerUnit(&ppuX, &ppuY);
			yStart *= ppuY;
			
			int clientHeight;
			GetClientSize(nullptr, &clientHeight);
			
			// Scroll if necessary
			if(yPos < yStart) {
				Scroll(-1, yPos / ppuY);
				UpdateViewableItems();
			} else if(yPos + item_height > yStart + clientHeight) {
				Scroll(-1, (yPos + item_height - clientHeight) / ppuY + 1);
				UpdateViewableItems();
			}
			
			return true;
		}
	}
	return false;
}

void DirectDrawBrushPanel::OnMouseClick(wxMouseEvent& event) {
	// Convert mouse position to logical position (accounting for scrolling)
	int xPos, yPos;
	CalcUnscrolledPosition(event.GetX(), event.GetY(), &xPos, &yPos);
	
	// Calculate which item was clicked
	int col = xPos / item_width;
	int row = yPos / item_height;
	
	// Bounds check
	if(col >= 0 && col < columns) {
		int index = row * columns + col;
		if(index >= 0 && index < static_cast<int>(tileset->size())) {
			selected_index = index;
			Refresh();
			
			// Notify parent about the selection
			wxWindow* w = this;
			while((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr);
			if(w) {
				g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
			}
			g_gui.SelectBrush(tileset->brushlist[index], tileset->getType());
		}
	}
	
	event.Skip();
}

void DirectDrawBrushPanel::RecalculateGrid() {
	// Calculate columns based on client width
	int width;
	GetClientSize(&width, nullptr);
	columns = std::max(1, width / item_width);
	
	// Calculate total rows needed
	total_rows = (tileset->size() + columns - 1) / columns;  // Ceiling division
	int virtual_height = total_rows * item_height;
	
	// Set virtual size for scrolling
	SetVirtualSize(width, virtual_height);
	
	// Update which items are currently visible
	UpdateViewableItems();
	
	need_full_redraw = true;
}

// ============================================================================
// SeamlessGridPanel
// A direct rendering class for dense sprite grid with zero margins

BEGIN_EVENT_TABLE(SeamlessGridPanel, wxScrolledWindow)
EVT_LEFT_DOWN(SeamlessGridPanel::OnMouseClick)
EVT_MOTION(SeamlessGridPanel::OnMouseMove)
EVT_PAINT(SeamlessGridPanel::OnPaint)
EVT_SIZE(SeamlessGridPanel::OnSize)
EVT_SCROLLWIN(SeamlessGridPanel::OnScroll)
EVT_TIMER(wxID_ANY, SeamlessGridPanel::OnTimer)
EVT_KEY_DOWN(SeamlessGridPanel::OnKeyDown)
END_EVENT_TABLE()

SeamlessGridPanel::SeamlessGridPanel(wxWindow* parent, const TilesetCategory* _tileset) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxWANTS_CHARS),
	BrushBoxInterface(_tileset),
	columns(1),
	sprite_size(32),
	selected_index(-1),
	hover_index(-1),
	buffer(nullptr),
	show_item_ids(false),
	first_visible_row(0),
	last_visible_row(0),
	visible_rows_margin(10),
	total_rows(0),
	need_full_redraw(true),
	use_progressive_loading(true),
	is_large_tileset(false),
	loading_step(0),
	max_loading_steps(5),
	loading_timer(nullptr) {
	
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	
	// Enable keyboard focus
	SetWindowStyle(GetWindowStyle() | wxWANTS_CHARS);
	
	// Check if we're dealing with a large tileset
	is_large_tileset = tileset && tileset->size() > LARGE_TILESET_THRESHOLD;
	
	// Create loading timer for large tilesets
	if (is_large_tileset && use_progressive_loading) {
		loading_timer = new wxTimer(this);
		max_loading_steps = 10; // Increase steps for smoother progress indication
	}
	
	// Enable scrolling
	SetScrollRate(sprite_size, sprite_size);
	
	// Calculate initial grid layout
	RecalculateGrid();
	
	// Start with first brush selected
	SelectFirstBrush();
	
	// Start progressive loading if needed
	if (is_large_tileset && use_progressive_loading) {
		StartProgressiveLoading();
	}
}

SeamlessGridPanel::~SeamlessGridPanel() {
	if (loading_timer) {
		loading_timer->Stop();
		delete loading_timer;
	}
}

void SeamlessGridPanel::StartProgressiveLoading() {
	if (!loading_timer) return;
	
	// Reset loading step
	loading_step = 0;
	
	// Set initial small margin
	visible_rows_margin = 3;
	
	// Force full redraw
	need_full_redraw = true;
	
	// Only show a limited number of items initially
	int itemsToShowInitially = std::min(100, static_cast<int>(tileset->size()));
	
	// Calculate how many items to add with each loading step
	int itemsPerStep = (tileset->size() - itemsToShowInitially) / max_loading_steps;
	if (itemsPerStep < 50) {
		// For smaller tilesets, load faster
		max_loading_steps = std::max(3, static_cast<int>(tileset->size() / 50));
	}
	
	// Start timer for progressive loading
	loading_timer->Start(200); // 200ms interval for better visual feedback
	
	// Force initial redraw to show progress
	Refresh();
}

void SeamlessGridPanel::OnTimer(wxTimerEvent& event) {
	// Progressively increase the visible margin
	loading_step++;
	
	// Update viewable items with new margin
	UpdateViewableItems();
	
	// Force redraw to update progress
	Refresh();
	
	// Stop timer when we've reached max loading steps
	if (loading_step >= max_loading_steps) {
		loading_timer->Stop();
		need_full_redraw = true;
		Refresh();
	}
}

void SeamlessGridPanel::UpdateViewableItems() {
	int xStart, yStart;
	GetViewStart(&xStart, &yStart);
	int ppuX, ppuY;
	GetScrollPixelsPerUnit(&ppuX, &ppuY);
	yStart *= ppuY;
	
	int width, height;
	GetClientSize(&width, &height);
	
	// Calculate visible range with margins
	int new_first_row = std::max(0, (yStart / sprite_size) - visible_rows_margin);
	int new_last_row = std::min(total_rows - 1, ((yStart + height) / sprite_size) + visible_rows_margin);
	
	// Only trigger redraw if visible range changes
	if (new_first_row != first_visible_row || new_last_row != last_visible_row) {
		first_visible_row = new_first_row;
		last_visible_row = new_last_row;
		Refresh();
	}
}

void SeamlessGridPanel::RecalculateGrid() {
	// Calculate columns based on client width
	int width;
	GetClientSize(&width, nullptr);
	columns = std::max(1, width / sprite_size);
	
	// Calculate total rows needed
	total_rows = (tileset->size() + columns - 1) / columns;  // Ceiling division
	int virtual_height = total_rows * sprite_size;
	
	// Set virtual size for scrolling
	SetVirtualSize(width, virtual_height);
	
	// Update which items are currently visible
	UpdateViewableItems();
	
	need_full_redraw = true;
}

void SeamlessGridPanel::DrawItemsToPanel(wxDC& dc) {
	if(!tileset || tileset->size() == 0) return;
	
	// Calculate client area size
	int width, height;
	GetClientSize(&width, &height);
	
	// Draw loading progress for large datasets during initial load
	if (loading_step < max_loading_steps) {
		// Draw progress bar
		int progressWidth = width - 40;
		int progressHeight = 20;
		int progressX = 20;
		int progressY = 20;
		
		// Progress background
		dc.SetBrush(wxBrush(wxColor(200, 200, 200)));
		dc.SetPen(wxPen(wxColor(100, 100, 100)));
		dc.DrawRectangle(progressX, progressY, progressWidth, progressHeight);
		
		// Progress fill
		float progress = static_cast<float>(loading_step + 1) / max_loading_steps;
		dc.SetBrush(wxBrush(wxColor(0, 150, 0)));
		dc.SetPen(wxPen(wxColor(0, 100, 0)));
		dc.DrawRectangle(progressX, progressY, progressWidth * progress, progressHeight);
		
		// Progress text
		wxString loadingMsg = wxString::Format("Loading %zu items... (%d%%)", 
			tileset->size(), 
			static_cast<int>((loading_step + 1) * 100 / max_loading_steps));
		
		wxSize textSize = dc.GetTextExtent(loadingMsg);
		dc.SetTextForeground(wxColor(0, 0, 0));
		dc.DrawText(loadingMsg, (width - textSize.GetWidth()) / 2, progressY + progressHeight + 5);
		
		// Draw info about how many items are being processed
		int itemsProcessed = tileset->size() * progress;
		wxString itemsMsg = wxString::Format("Processed: %d / %zu items", 
			itemsProcessed, tileset->size());
		
		textSize = dc.GetTextExtent(itemsMsg);
		dc.DrawText(itemsMsg, (width - textSize.GetWidth()) / 2, progressY + progressHeight + 25);
		
		// Only draw a limited number of items during loading phase
		int maxItemsToDraw = itemsProcessed;
		
		// Only draw items in the visible range up to the processed amount
		for(int row = first_visible_row; row <= last_visible_row; ++row) {
			for(int col = 0; col < columns; ++col) {
				int index = row * columns + col;
				if(index >= static_cast<int>(tileset->size()) || index >= maxItemsToDraw) break;
				
				int x = col * sprite_size;
				int y = row * sprite_size;
				
				// Skip items that would appear where our progress bar is
				if (y < progressY + progressHeight + 40) continue;
				
				DrawSpriteAt(dc, x, y, index);
			}
		}
	} else {
		// Normal drawing when fully loaded
		// Only draw items in the visible range
		for(int row = first_visible_row; row <= last_visible_row; ++row) {
			for(int col = 0; col < columns; ++col) {
				int index = row * columns + col;
				if(index >= static_cast<int>(tileset->size())) break;
				
				int x = col * sprite_size;
				int y = row * sprite_size;
				
				DrawSpriteAt(dc, x, y, index);
			}
		}
	}
}

void SeamlessGridPanel::DrawSpriteAt(wxDC& dc, int x, int y, int index) {
	// Common drawing function to reduce code duplication
	Brush* brush = tileset->brushlist[index];
	if (!brush) return;
	
	// Draw background for selected/hover items
	if (index == selected_index) {
		dc.SetBrush(wxBrush(wxColor(120, 120, 200)));
		dc.SetPen(wxPen(wxColor(80, 80, 160), 2));
		dc.DrawRectangle(x, y, sprite_size, sprite_size);
	} else if (index == hover_index) {
		dc.SetBrush(wxBrush(wxColor(200, 200, 255, 128)));
		dc.SetPen(wxPen(wxColor(150, 150, 230, 200), 1));
		dc.DrawRectangle(x, y, sprite_size, sprite_size);
	}
	
	// Draw item sprite
	Sprite* sprite = g_gui.gfx.getSprite(brush->getLookID());
	if (sprite) {
		sprite->DrawTo(&dc, SPRITE_SIZE_32x32, x, y);
	}
	
	// For RAW brushes, draw the ID if enabled
	if (show_item_ids && brush->isRaw()) {
		RAWBrush* raw = static_cast<RAWBrush*>(brush);
		wxString label = wxString::Format("%d", raw->getItemID());
		
		// Draw with semi-transparent background for better readability
		wxSize textSize = dc.GetTextExtent(label);
		dc.SetBrush(wxBrush(wxColor(0, 0, 0, 128)));
		dc.SetPen(wxPen(wxColor(0, 0, 0, 0)));
		dc.DrawRectangle(x, y + sprite_size - 14, textSize.GetWidth() + 4, 14);
		
		dc.SetTextForeground(wxColor(255, 255, 255));
		dc.DrawText(label, x + 2, y + sprite_size - 14);
	}
}

void SeamlessGridPanel::OnPaint(wxPaintEvent& event) {
	wxAutoBufferedPaintDC dc(this);
	DoPrepareDC(dc);  // For correct scrolling
	
	// Clear background
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();
	
	// Draw items
	DrawItemsToPanel(dc);
}

void SeamlessGridPanel::OnSize(wxSizeEvent& event) {
	RecalculateGrid();
	Refresh();
	event.Skip();
}

void SeamlessGridPanel::OnScroll(wxScrollWinEvent& event) {
	// Handle scroll events to update visible items
	UpdateViewableItems();
	
	// Reset progressive loading on scroll for large tilesets
	if (loading_step < max_loading_steps) {
		// Temporarily use small margin for immediate response
		visible_rows_margin = 3;
		UpdateViewableItems();
		
		// Show loading message in the area being scrolled to
		Refresh();
		
		// Restart progressive loading
		StartProgressiveLoading();
	} else {
		// If loading was already complete, just continue with full view
		visible_rows_margin = 30;
		UpdateViewableItems();
		Refresh();
	}
	
	event.Skip();
}

int SeamlessGridPanel::GetSpriteIndexAt(int x, int y) const {
	// Convert mouse position to logical position (accounting for scrolling)
	int logX, logY;
	CalcUnscrolledPosition(x, y, &logX, &logY);
	
	// Calculate row and column
	int col = logX / sprite_size;
	int row = logY / sprite_size;
	
	// Calculate index
	int index = row * columns + col;
	
	// Check if this is a valid index
	if (index >= 0 && index < static_cast<int>(tileset->size()) && 
		col >= 0 && col < columns) {
		return index;
	}
	
	return -1;
}

void SeamlessGridPanel::OnMouseClick(wxMouseEvent& event) {
	int index = GetSpriteIndexAt(event.GetX(), event.GetY());
	if (index != -1) {
		selected_index = index;
		Refresh();
		
		// Notify parent about the selection
		wxWindow* w = this;
		while((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr);
		if(w) {
			g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
		}
		
		g_gui.SelectBrush(tileset->brushlist[index], tileset->getType());
	}
	
	event.Skip();
}

void SeamlessGridPanel::OnMouseMove(wxMouseEvent& event) {
	// Update hover effect
	int index = GetSpriteIndexAt(event.GetX(), event.GetY());
	if (index != hover_index) {
		hover_index = index;
		Refresh();
	}
	
	event.Skip();
}

void SeamlessGridPanel::SelectFirstBrush() {
	if (tileset && tileset->size() > 0) {
		selected_index = 0;
		Refresh();
	}
}

Brush* SeamlessGridPanel::GetSelectedBrush() const {
	if (!tileset || selected_index < 0 || selected_index >= static_cast<int>(tileset->size())) {
		return nullptr;
	}
	
	return tileset->brushlist[selected_index];
}

bool SeamlessGridPanel::SelectBrush(const Brush* whatbrush) {
	if (!tileset) return false;
	
	for (size_t i = 0; i < tileset->size(); ++i) {
		if (tileset->brushlist[i] == whatbrush) {
			selected_index = i;
			hover_index = -1;
			Refresh();
			
			// Ensure the selected item is visible
			int row = selected_index / columns;
			int yPos = row * sprite_size;
			
			// Get the visible area
			int xStart, yStart;
			GetViewStart(&xStart, &yStart);
			int ppuX, ppuY;
			GetScrollPixelsPerUnit(&ppuX, &ppuY);
			yStart *= ppuY;
			
			int clientHeight;
			GetClientSize(nullptr, &clientHeight);
			
			// Scroll if necessary
			if (yPos < yStart) {
				Scroll(-1, yPos / ppuY);
				UpdateViewableItems();
			} else if (yPos + sprite_size > yStart + clientHeight) {
				Scroll(-1, (yPos + sprite_size - clientHeight) / ppuY + 1);
				UpdateViewableItems();
			}
			
			return true;
		}
	}
	return false;
}

void SeamlessGridPanel::SelectIndex(int index) {
	if (!tileset || index < 0 || index >= static_cast<int>(tileset->size())) {
		return;
	}

	selected_index = index;
	hover_index = -1;
	Refresh();

	// Ensure the selected item is visible
	int row = selected_index / columns;
	int yPos = row * sprite_size;

	// Get the visible area
	int xStart, yStart;
	GetViewStart(&xStart, &yStart);
	int ppuX, ppuY;
	GetScrollPixelsPerUnit(&ppuX, &ppuY);
	yStart *= ppuY;

	int clientHeight;
	GetClientSize(nullptr, &clientHeight);

	// Scroll if necessary
	if (yPos < yStart) {
		Scroll(-1, yPos / ppuY);
		UpdateViewableItems();
	} else if (yPos + sprite_size > yStart + clientHeight) {
		Scroll(-1, (yPos + sprite_size - clientHeight) / ppuY + 1);
		UpdateViewableItems();
	}

	// Notify parent about the selection
	wxWindow* w = this;
	while((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr);
	if(w) {
		g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
	}
	g_gui.SelectBrush(tileset->brushlist[index], tileset->getType());
}

void SeamlessGridPanel::OnKeyDown(wxKeyEvent& event) {
	if (!tileset || tileset->size() == 0) {
		event.Skip();
		return;
	}

	int newIndex = selected_index;
	bool handled = true;

	switch (event.GetKeyCode()) {
		case WXK_LEFT:
			if (selected_index > 0) {
				newIndex--;
			}
			break;

		case WXK_RIGHT:
			if (selected_index < static_cast<int>(tileset->size()) - 1) {
				newIndex++;
			}
			break;

		case WXK_UP:
			if (selected_index >= columns) {
				newIndex -= columns;
			}
			break;

		case WXK_DOWN:
			if (selected_index + columns < static_cast<int>(tileset->size())) {
				newIndex += columns;
			}
			break;

		case WXK_HOME:
			newIndex = 0;
			break;

		case WXK_END:
			newIndex = tileset->size() - 1;
			break;

		case WXK_PAGEUP: {
			int clientHeight;
			GetClientSize(nullptr, &clientHeight);
			int rowsPerPage = clientHeight / sprite_size;
			newIndex = std::max(0, selected_index - (rowsPerPage * columns));
			break;
		}

		case WXK_PAGEDOWN: {
			int clientHeight;
			GetClientSize(nullptr, &clientHeight);
			int rowsPerPage = clientHeight / sprite_size;
			newIndex = std::min(static_cast<int>(tileset->size()) - 1, 
							  selected_index + (rowsPerPage * columns));
			break;
		}

		default:
			handled = false;
			break;
	}

	if (handled && newIndex != selected_index) {
		SelectIndex(newIndex);
		SetFocus(); // Keep focus for more keyboard input
	} else {
		event.Skip(); // Allow other handlers to process the event
	}
}
