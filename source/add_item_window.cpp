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

#include <wx/grid.h>
#include <wx/choice.h>

#include "tile.h"
#include "item.h"
#include "complexitem.h"
#include "town.h"
#include "house.h"
#include "map.h"
#include "editor.h"
#include "creature.h"
#include "materials.h"
#include "tileset.h"

#include "gui.h"
#include "application.h"
#include "add_item_window.h"
#include "container_properties_window.h"
#include "find_item_window.h"

// ============================================================================
// Add Item Window

BEGIN_EVENT_TABLE(AddItemWindow, wxDialog)
EVT_BUTTON(wxID_OK, AddItemWindow::OnClickOK)
EVT_BUTTON(wxID_CANCEL, AddItemWindow::OnClickCancel)
END_EVENT_TABLE()

static constexpr int OUTFIT_COLOR_MAX = 133;

AddItemWindow::AddItemWindow(wxWindow* win_parent, TilesetCategoryType categoryType, Tileset* tilesetItem, wxPoint pos) :
	ObjectPropertiesWindowBase(win_parent, "Add a Item", pos),
	item_id(0),
	tileset_item(tilesetItem),
	category_type(categoryType),
	item_id_field(nullptr),
	item_id_label(nullptr),
	item_name_label(nullptr),
	item_button(nullptr),
	tileset_choice(nullptr) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxString description = "Add a Item";

	wxSizer* boxsizer = newd wxStaticBoxSizer(wxVERTICAL, this, description);

	wxFlexGridSizer* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);

	uint16_t itemId = 0;
	std::string itemName = "None";
	item_id_label = newd wxStaticText(this, wxID_ANY, "ID " + i2ws(itemId));
	subsizer->Add(item_id_label);
	item_name_label = newd wxStaticText(this, wxID_ANY, "\"" + wxstr(itemName) + "\"");
	subsizer->Add(item_name_label);

	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Item"), wxSizerFlags(1).CenterVertical());
	item_button = newd DCButton(this, wxID_ANY, wxDefaultPosition, DC_BTN_TOGGLE, RENDER_SIZE_32x32, 0);
	subsizer->Add(item_button);

	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Item Id"));
	item_id_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(itemId), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 100000);
	item_id_field->Connect(wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler(AddItemWindow::OnChangeItemId), NULL, this);
	subsizer->Add(item_id_field, wxSizerFlags(1).Expand());
	
	// Add tileset selection dropdown
	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Tileset"));
	tileset_choice = newd wxChoice(this, wxID_ANY);
	
	// Populate the choice with all available tilesets
	int currentSelection = 0;
	int index = 0;
	for (TilesetContainer::iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
		if (iter->second->getCategory(category_type)) {
			tileset_choice->Append(wxstr(iter->first));
			// Set current selection to the provided tileset if it matches
			if (tileset_item && iter->second == tileset_item) {
				currentSelection = index;
			}
			index++;
		}
	}
	
	// Select the current tileset if available
	if (tileset_choice->GetCount() > 0) {
		tileset_choice->SetSelection(currentSelection);
	}
	
	subsizer->Add(tileset_choice, wxSizerFlags(1).Expand());

	boxsizer->Add(subsizer, wxSizerFlags(1).Expand());

	topsizer->Add(boxsizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT, 20));

	wxSizer* subsizer_ = newd wxBoxSizer(wxHORIZONTAL);
	subsizer_->Add(newd wxButton(this, wxID_OK, "Add"), wxSizerFlags(1).Center().Border(wxTOP | wxBOTTOM, 10));
	subsizer_->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center().Border(wxTOP | wxBOTTOM, 10));
	topsizer->Add(subsizer_, wxSizerFlags(0).Center().Border(wxLEFT | wxRIGHT, 20));

	SetSizerAndFit(topsizer);
	Centre(wxBOTH);

	item_button->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(AddItemWindow::OnItemClicked), NULL, this);
}

void AddItemWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	const ItemType& it = g_items.getItemType(item_id_field->GetValue());
	if (it.id != 0) {
		// Get the selected tileset name from the dropdown
		wxString tilesetName;
		if (tileset_choice && tileset_choice->GetSelection() != wxNOT_FOUND) {
			tilesetName = tileset_choice->GetString(tileset_choice->GetSelection());
		}
		
		// If no tileset is selected, use the initial tileset
		if (tilesetName.IsEmpty() && tileset_item) {
			tilesetName = wxstr(tileset_item->name);
		}
		
		// Make sure we have a valid tileset name
		if (!tilesetName.IsEmpty()) {
			std::string stdTilesetName = nstr(tilesetName);
			g_materials.addToTileset(stdTilesetName, it.id, category_type);
			g_materials.modify();
			g_gui.PopupDialog("Item added to Tileset", 
				wxString::Format("'%s' (ID: %d) has been added to tileset '%s'", 
				wxString(it.name.c_str()), it.id, tilesetName), wxOK);

			EndModal(1);
		} else {
			g_gui.PopupDialog("Error", "No tileset selected", wxOK);
		}
	} else {
		g_gui.PopupDialog("Error", "You need to select an item", wxOK);
	}
}

void AddItemWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	// Just close this window
	EndModal(0);
}

void AddItemWindow::OnChangeItemId(wxCommandEvent& WXUNUSED(event)) {
	uint16_t itemId = item_id_field->GetValue();
	ItemType& it = g_items[itemId];
	if (it.id != 0) {
		item_id_label->SetLabelText("ID " + i2ws(it.id));
		item_name_label->SetLabelText("\"" + wxstr(it.name) + "\"");

		item_button->SetSprite(it.clientID);
	} else {
		item_id_field->SetValue(100);
	}
}

void AddItemWindow::OnItemClicked(wxMouseEvent& WXUNUSED(event)) {
	FindItemDialog dialog(this, "Item");
	if (dialog.ShowModal() == wxID_OK) {
		uint16_t id = dialog.getResultID();
		SetItemIdToItemButton(id);
	}
	dialog.Destroy();
}

void AddItemWindow::SetItemIdToItemButton(uint16_t id) {
	if (!item_button) {
		return;
	}

	if (id != 0) {
		const ItemType& it = g_items.getItemType(id);
		if (it.id != 0) {
			item_id_field->SetValue(it.id);
			item_id_label->SetLabelText("ID " + i2ws(it.id));
			item_name_label->SetLabelText("\"" + wxstr(it.name) + "\"");

			item_button->SetSprite(it.clientID);
			return;
		}
	}

	item_button->SetSprite(0);
}
