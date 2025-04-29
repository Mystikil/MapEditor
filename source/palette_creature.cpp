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

#include "settings.h"
#include "brush.h"
#include "gui.h"
#include "palette_creature.h"
#include "creature_brush.h"
#include "spawn_brush.h"
#include "materials.h"
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/textdlg.h>
#include "creature_sprite_manager.h"

// Define the new event ID for the Load NPCs button
#define PALETTE_LOAD_NPCS_BUTTON 1952
#define PALETTE_LOAD_MONSTERS_BUTTON 1953
#define PALETTE_PURGE_CREATURES_BUTTON 1954
#define PALETTE_SEARCH_BUTTON 1955
#define PALETTE_SEARCH_FIELD 1956
#define PALETTE_VIEW_TOGGLE_BUTTON 1957

// ============================================================================
// Creature palette

BEGIN_EVENT_TABLE(CreaturePalettePanel, PalettePanel)
EVT_CHOICE(PALETTE_CREATURE_TILESET_CHOICE, CreaturePalettePanel::OnTilesetChange)

EVT_LISTBOX(PALETTE_CREATURE_LISTBOX, CreaturePalettePanel::OnListBoxChange)
EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_LISTBOX_SELECTED, CreaturePalettePanel::OnSpriteSelected)

EVT_TOGGLEBUTTON(PALETTE_CREATURE_BRUSH_BUTTON, CreaturePalettePanel::OnClickCreatureBrushButton)
EVT_TOGGLEBUTTON(PALETTE_SPAWN_BRUSH_BUTTON, CreaturePalettePanel::OnClickSpawnBrushButton)
EVT_TOGGLEBUTTON(PALETTE_VIEW_TOGGLE_BUTTON, CreaturePalettePanel::OnClickViewToggle)
EVT_BUTTON(PALETTE_LOAD_NPCS_BUTTON, CreaturePalettePanel::OnClickLoadNPCsButton)
EVT_BUTTON(PALETTE_LOAD_MONSTERS_BUTTON, CreaturePalettePanel::OnClickLoadMonstersButton)
EVT_BUTTON(PALETTE_PURGE_CREATURES_BUTTON, CreaturePalettePanel::OnClickPurgeCreaturesButton)
EVT_BUTTON(PALETTE_SEARCH_BUTTON, CreaturePalettePanel::OnClickSearchButton)
EVT_TEXT(PALETTE_SEARCH_FIELD, CreaturePalettePanel::OnSearchFieldText)

EVT_SPINCTRL(PALETTE_CREATURE_SPAWN_TIME, CreaturePalettePanel::OnChangeSpawnTime)
EVT_SPINCTRL(PALETTE_CREATURE_SPAWN_SIZE, CreaturePalettePanel::OnChangeSpawnSize)
END_EVENT_TABLE()

CreaturePalettePanel::CreaturePalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	handling_event(false),
	use_sprite_view(false) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxSizer* sidesizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Creatures");
	
	// Add choice and view toggle in one row
	wxSizer* choiceSizer = newd wxBoxSizer(wxHORIZONTAL);
	
	tileset_choice = newd wxChoice(this, PALETTE_CREATURE_TILESET_CHOICE, wxDefaultPosition, wxDefaultSize, (int)0, (const wxString*)nullptr);
	choiceSizer->Add(tileset_choice, 1, wxEXPAND | wxRIGHT, 5);
	
	view_toggle = newd wxToggleButton(this, PALETTE_VIEW_TOGGLE_BUTTON, "Sprite View", wxDefaultPosition, wxDefaultSize);
	choiceSizer->Add(view_toggle, 0, wxEXPAND);
	
	sidesizer->Add(choiceSizer, 0, wxEXPAND);

	// Add search field and button
	wxSizer* searchSizer = newd wxBoxSizer(wxHORIZONTAL);
	searchSizer->Add(newd wxStaticText(this, wxID_ANY, "Search:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	search_field = newd wxTextCtrl(this, PALETTE_SEARCH_FIELD, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	search_field->SetHint("Name or LT:# for looktype");  // Add placeholder text
	searchSizer->Add(search_field, 1, wxEXPAND);
	search_button = newd wxButton(this, PALETTE_SEARCH_BUTTON, "Go", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	searchSizer->Add(search_button, 0, wxLEFT, 5);
	sidesizer->Add(searchSizer, 0, wxEXPAND | wxTOP, 5);
	
	// Connect the focus events to disable hotkeys during typing
	search_field->Connect(wxEVT_SET_FOCUS, wxFocusEventHandler(CreaturePalettePanel::OnSearchFieldFocus), nullptr, this);
	search_field->Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(CreaturePalettePanel::OnSearchFieldKillFocus), nullptr, this);
	// Connect key down event to handle key presses in the search field
	search_field->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(CreaturePalettePanel::OnSearchFieldKeyDown), nullptr, this);

	// Create view container that will hold both list and sprite views
	view_sizer = newd wxBoxSizer(wxVERTICAL);
	
	// Create both views
	creature_list = newd SortableListBox(this, PALETTE_CREATURE_LISTBOX);
	sprite_panel = newd CreatureSpritePanel(this);
	
	// Add views to sizer (only one will be shown at a time)
	view_sizer->Add(creature_list, 1, wxEXPAND);
	view_sizer->Add(sprite_panel, 1, wxEXPAND);
	sprite_panel->Hide(); // Initially hide the sprite view
	
	sidesizer->Add(view_sizer, 1, wxEXPAND | wxTOP, 5);
	
	// Add buttons for loading NPCs, monsters, and purging creatures
	wxSizer* buttonSizer = newd wxBoxSizer(wxHORIZONTAL);
	
	load_npcs_button = newd wxButton(this, PALETTE_LOAD_NPCS_BUTTON, "Load NPCs Folder");
	buttonSizer->Add(load_npcs_button, 1, wxEXPAND | wxRIGHT, 5);
	
	load_monsters_button = newd wxButton(this, PALETTE_LOAD_MONSTERS_BUTTON, "Load Monsters Folder");
	buttonSizer->Add(load_monsters_button, 1, wxEXPAND | wxLEFT, 5);
	
	sidesizer->Add(buttonSizer, 0, wxEXPAND | wxTOP, 5);
	
	purge_creatures_button = newd wxButton(this, PALETTE_PURGE_CREATURES_BUTTON, "Purge Creatures");
	sidesizer->Add(purge_creatures_button, 0, wxEXPAND | wxTOP, 5);
	
	topsizer->Add(sidesizer, 1, wxEXPAND);

	// Brush selection
	sidesizer = newd wxStaticBoxSizer(newd wxStaticBox(this, wxID_ANY, "Brushes", wxDefaultPosition, wxSize(150, 200)), wxVERTICAL);

	wxFlexGridSizer* grid = newd wxFlexGridSizer(3, 10, 10);
	grid->AddGrowableCol(1);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Spawntime"));
	creature_spawntime_spin = newd wxSpinCtrl(this, PALETTE_CREATURE_SPAWN_TIME, i2ws(g_settings.getInteger(Config::DEFAULT_SPAWNTIME)), wxDefaultPosition, wxSize(50, 20), wxSP_ARROW_KEYS, 0, 86400, g_settings.getInteger(Config::DEFAULT_SPAWNTIME));
	grid->Add(creature_spawntime_spin, 0, wxEXPAND);
	creature_brush_button = newd wxToggleButton(this, PALETTE_CREATURE_BRUSH_BUTTON, "Place Creature");
	grid->Add(creature_brush_button, 0, wxEXPAND);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Spawn size"));
	spawn_size_spin = newd wxSpinCtrl(this, PALETTE_CREATURE_SPAWN_SIZE, i2ws(5), wxDefaultPosition, wxSize(50, 20), wxSP_ARROW_KEYS, 1, g_settings.getInteger(Config::MAX_SPAWN_RADIUS), g_settings.getInteger(Config::CURRENT_SPAWN_RADIUS));
	grid->Add(spawn_size_spin, 0, wxEXPAND);
	spawn_brush_button = newd wxToggleButton(this, PALETTE_SPAWN_BRUSH_BUTTON, "Place Spawn");
	grid->Add(spawn_brush_button, 0, wxEXPAND);

	sidesizer->Add(grid, 0, wxEXPAND);
	topsizer->Add(sidesizer, 0, wxEXPAND);
	SetSizerAndFit(topsizer);

	OnUpdate();
}

CreaturePalettePanel::~CreaturePalettePanel() {
	////
}

PaletteType CreaturePalettePanel::GetType() const {
	return TILESET_CREATURE;
}

void CreaturePalettePanel::SelectFirstBrush() {
	SelectCreatureBrush();
}

Brush* CreaturePalettePanel::GetSelectedBrush() const {
	if (creature_brush_button->GetValue()) {
		if (use_sprite_view) {
			// Get selection from sprite panel
			return sprite_panel->GetSelectedBrush();
		} else {
			// Get selection from list box
			if (creature_list->GetCount() == 0) {
				return nullptr;
			}
			Brush* brush = reinterpret_cast<Brush*>(creature_list->GetClientData(creature_list->GetSelection()));
			if (brush && brush->isCreature()) {
				g_gui.SetSpawnTime(creature_spawntime_spin->GetValue());
				return brush;
			}
		}
	} else if (spawn_brush_button->GetValue()) {
		g_settings.setInteger(Config::CURRENT_SPAWN_RADIUS, spawn_size_spin->GetValue());
		g_settings.setInteger(Config::DEFAULT_SPAWNTIME, creature_spawntime_spin->GetValue());
		return g_gui.spawn_brush;
	}
	return nullptr;
}

bool CreaturePalettePanel::SelectBrush(const Brush* whatbrush) {
	if (!whatbrush) {
		return false;
	}

	if (whatbrush->isCreature()) {
		int current_index = tileset_choice->GetSelection();
		if (current_index != wxNOT_FOUND) {
			const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(current_index));
			// Select first house
			for (BrushVector::const_iterator iter = tsc->brushlist.begin(); iter != tsc->brushlist.end(); ++iter) {
				if (*iter == whatbrush) {
					if (use_sprite_view) {
						// Select in sprite view
						sprite_panel->SelectBrush(whatbrush);
					} else {
						// Select in list view
						SelectCreature(whatbrush->getName());
					}
					return true;
				}
			}
		}
		// Not in the current display, search the hidden one's
		for (size_t i = 0; i < tileset_choice->GetCount(); ++i) {
			if (current_index != (int)i) {
				const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(i));
				for (BrushVector::const_iterator iter = tsc->brushlist.begin();
					 iter != tsc->brushlist.end();
					 ++iter) {
					if (*iter == whatbrush) {
						SelectTileset(i);
						if (use_sprite_view) {
							// Select in sprite view
							sprite_panel->SelectBrush(whatbrush);
						} else {
							// Select in list view
							SelectCreature(whatbrush->getName());
						}
						return true;
					}
				}
			}
		}
	} else if (whatbrush->isSpawn()) {
		SelectSpawnBrush();
		return true;
	}
	return false;
}

int CreaturePalettePanel::GetSelectedBrushSize() const {
	return spawn_size_spin->GetValue();
}

void CreaturePalettePanel::OnUpdate() {
	tileset_choice->Clear();
	g_materials.createOtherTileset();

	// Create an "All Creatures" tileset that contains all creatures
	Tileset* allCreatures = nullptr;
	TilesetCategory* allCreaturesCategory = nullptr;
	
	// Check if the "All Creatures" tileset already exists, if not create it
	if (g_materials.tilesets.count("All Creatures") > 0) {
		allCreatures = g_materials.tilesets["All Creatures"];
		allCreaturesCategory = allCreatures->getCategory(TILESET_CREATURE);
		allCreaturesCategory->brushlist.clear();
	} else {
		allCreatures = newd Tileset(g_brushes, "All Creatures");
		g_materials.tilesets["All Creatures"] = allCreatures;
		allCreaturesCategory = allCreatures->getCategory(TILESET_CREATURE);
	}

	// Track added creatures to avoid duplicates
	std::set<std::string> addedCreatures;

	// Collect all creature brushes from all tilesets
	for (TilesetContainer::const_iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
		if (iter->first == "All Creatures") continue;  // Skip ourselves to avoid duplication
		
		const TilesetCategory* tsc = iter->second->getCategory(TILESET_CREATURE);
		if (tsc && tsc->size() > 0) {
			// Add all creature brushes from this category to the All Creatures category
			for (BrushVector::const_iterator brushIter = tsc->brushlist.begin(); brushIter != tsc->brushlist.end(); ++brushIter) {
				if ((*brushIter)->isCreature()) {
					// Only add if not already added (avoid duplicates)
					std::string creatureName = (*brushIter)->getName();
					if (addedCreatures.count(creatureName) == 0) {
						allCreaturesCategory->brushlist.push_back(*brushIter);
						addedCreatures.insert(creatureName);
					}
				}
			}
		}
	}

	// Add the "All Creatures" tileset first
	tileset_choice->Append(wxstr(allCreatures->name), allCreaturesCategory);

	// Add the rest of the tilesets as before
	for (TilesetContainer::const_iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
		if (iter->first == "All Creatures") continue;  // Skip since we already added it

		const TilesetCategory* tsc = iter->second->getCategory(TILESET_CREATURE);
		if (tsc && tsc->size() > 0) {
			tileset_choice->Append(wxstr(iter->second->name), const_cast<TilesetCategory*>(tsc));
		} else if (iter->second->name == "NPCs" || iter->second->name == "Others") {
			Tileset* ts = const_cast<Tileset*>(iter->second);
			TilesetCategory* rtsc = ts->getCategory(TILESET_CREATURE);
			tileset_choice->Append(wxstr(ts->name), rtsc);
		}
	}
	SelectTileset(0);
}

void CreaturePalettePanel::OnUpdateBrushSize(BrushShape shape, int size) {
	return spawn_size_spin->SetValue(size);
}

void CreaturePalettePanel::OnSwitchIn() {
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetBrushSize(spawn_size_spin->GetValue());
}

void CreaturePalettePanel::SelectTileset(size_t index) {
	ASSERT(tileset_choice->GetCount() >= index);

	creature_list->Clear();
	sprite_panel->Clear();
	
	if (tileset_choice->GetCount() == 0) {
		// No tilesets :(
		creature_brush_button->Enable(false);
	} else {
		const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
		
		// Add creatures to appropriate view
		if (use_sprite_view) {
			sprite_panel->LoadCreatures(tsc->brushlist);
		} else {
			// Add creatures to list view
			for (BrushVector::const_iterator iter = tsc->brushlist.begin();
				iter != tsc->brushlist.end();
				++iter) {
				creature_list->Append(wxstr((*iter)->getName()), *iter);
			}
			creature_list->Sort();
		}
		
		// Apply filter if search field has text
		if (!search_field->IsEmpty()) {
			FilterCreatures(search_field->GetValue());
		} else {
			SelectCreature(0);
		}

		tileset_choice->SetSelection(index);
	}
}

void CreaturePalettePanel::SelectCreature(size_t index) {
	// Select creature by index
	if (use_sprite_view) {
		// In sprite view, select by index
		if (index < sprite_panel->creatures.size()) {
			sprite_panel->SelectIndex(index);
		}
	} else {
		// In list view, select by index
		if (creature_list->GetCount() > 0 && index < creature_list->GetCount()) {
			creature_list->SetSelection(index);
		}
	}

	SelectCreatureBrush();
}

void CreaturePalettePanel::SelectCreature(std::string name) {
	if (use_sprite_view) {
		// In sprite view, find and select brush by name
		for (size_t i = 0; i < sprite_panel->creatures.size(); ++i) {
			if (sprite_panel->creatures[i]->getName() == name) {
				sprite_panel->SelectIndex(i);
				break;
			}
		}
	} else {
		// In list view, select by name string
		if (creature_list->GetCount() > 0) {
			if (!creature_list->SetStringSelection(wxstr(name))) {
				creature_list->SetSelection(0);
			}
		}
	}

	SelectCreatureBrush();
}

void CreaturePalettePanel::SelectCreatureBrush() {
	bool has_selection = false;
	
	if (use_sprite_view) {
		has_selection = (sprite_panel->GetSelectedBrush() != nullptr);
	} else {
		has_selection = (creature_list->GetCount() > 0);
	}
	
	if (has_selection) {
		creature_brush_button->Enable(true);
		creature_brush_button->SetValue(true);
		spawn_brush_button->SetValue(false);
	} else {
		creature_brush_button->Enable(false);
		SelectSpawnBrush();
	}
}

void CreaturePalettePanel::SelectSpawnBrush() {
	// g_gui.house_exit_brush->setHouse(house);
	creature_brush_button->SetValue(false);
	spawn_brush_button->SetValue(true);
}

void CreaturePalettePanel::OnTilesetChange(wxCommandEvent& event) {
	SelectTileset(event.GetSelection());
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnListBoxChange(wxCommandEvent& event) {
	// Get the selected brush before updating
	Brush* old_brush = g_gui.GetCurrentBrush();
	
	// Update selection
	SelectCreature(event.GetSelection());
	g_gui.ActivatePalette(GetParentPalette());
	
	// Get the newly selected brush
	Brush* new_brush = g_gui.GetCurrentBrush();
	
	// If we selected the same brush, first set to nullptr then reselect
	if(old_brush && new_brush && old_brush == new_brush) {
		g_gui.SelectBrush(nullptr, TILESET_CREATURE);
	}
	
	// Now select the brush (either for the first time or re-selecting)
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickCreatureBrushButton(wxCommandEvent& event) {
	SelectCreatureBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickSpawnBrushButton(wxCommandEvent& event) {
	SelectSpawnBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickLoadNPCsButton(wxCommandEvent& event) {
	wxDirDialog dlg(g_gui.root, "Select NPC folder", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK) {
		wxString folder = dlg.GetPath();
		LoadNPCsFromFolder(folder);
	}
}

void CreaturePalettePanel::OnClickLoadMonstersButton(wxCommandEvent& event) {
	wxDirDialog dlg(g_gui.root, "Select Monsters folder", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK) {
		wxString folder = dlg.GetPath();
		LoadMonstersFromFolder(folder);
	}
}

void CreaturePalettePanel::OnClickPurgeCreaturesButton(wxCommandEvent& event) {
	// Confirmation dialog
	long response = wxMessageBox("Are you sure you want to purge all creatures from the palette? This cannot be undone.", 
		"Confirm Purge", wxYES_NO | wxICON_QUESTION, g_gui.root);
	
	if (response == wxYES) {
		PurgeCreaturePalettes();
	}
}

bool CreaturePalettePanel::LoadNPCsFromFolder(const wxString& folder) {
	// Get all .xml files in the folder
	wxArrayString files;
	wxDir::GetAllFiles(folder, &files, "*.xml", wxDIR_FILES);
	
	if (files.GetCount() == 0) {
		wxMessageBox("No XML files found in the selected folder.", "Error", wxOK | wxICON_INFORMATION, g_gui.root);
		return false;
	}
	
	wxArrayString warnings;
	int loadedCount = 0;
	
	for (size_t i = 0; i < files.GetCount(); ++i) {
		wxString error;
		bool ok = g_creatures.importXMLFromOT(FileName(files[i]), error, warnings);
		if (ok) {
			loadedCount++;
		} else {
			warnings.Add("Failed to load " + files[i] + ": " + error);
		}
	}
	
	if (!warnings.IsEmpty()) {
		g_gui.ListDialog("NPC loader messages", warnings);
	}
	
	if (loadedCount > 0) {
		g_gui.PopupDialog("Success", wxString::Format("Successfully loaded %d NPC files.", loadedCount), wxOK);
		
		// Refresh the palette
		g_gui.RefreshPalettes();
		
		// Refresh current tileset and creature list
		OnUpdate();
		
		return true;
	} else {
		wxMessageBox("No NPCs could be loaded from the selected folder.", "Error", wxOK | wxICON_INFORMATION, g_gui.root);
		return false;
	}
}

bool CreaturePalettePanel::LoadMonstersFromFolder(const wxString& folder) {
	// Get all .xml files in the folder
	wxArrayString files;
	wxDir::GetAllFiles(folder, &files, "*.xml", wxDIR_FILES);
	
	if (files.GetCount() == 0) {
		wxMessageBox("No XML files found in the selected folder.", "Error", wxOK | wxICON_INFORMATION, g_gui.root);
		return false;
	}
	
	wxArrayString warnings;
	int loadedCount = 0;
	
	for (size_t i = 0; i < files.GetCount(); ++i) {
		wxString error;
		bool ok = g_creatures.importXMLFromOT(FileName(files[i]), error, warnings);
		if (ok) {
			loadedCount++;
		} else {
			warnings.Add("Failed to load " + files[i] + ": " + error);
		}
	}
	
	if (!warnings.IsEmpty()) {
		g_gui.ListDialog("Monster loader messages", warnings);
	}
	
	if (loadedCount > 0) {
		g_gui.PopupDialog("Success", wxString::Format("Successfully loaded %d monster files.", loadedCount), wxOK);
		
		// Refresh the palette
		g_gui.RefreshPalettes();
		
		// Refresh current tileset and creature list
		OnUpdate();
		
		return true;
	} else {
		wxMessageBox("No monsters could be loaded from the selected folder.", "Error", wxOK | wxICON_INFORMATION, g_gui.root);
		return false;
	}
}

bool CreaturePalettePanel::PurgeCreaturePalettes() {
	// Track success
	bool success = false;
	
	// Create vectors to store brushes that need to be removed
	std::vector<Brush*> brushesToRemove;
	
	// Collect creature brushes from the "NPCs", "Others", and "All Creatures" tilesets
	if (g_materials.tilesets.count("All Creatures") > 0) {
		Tileset* allCreaturesTileset = g_materials.tilesets["All Creatures"];
		TilesetCategory* allCreaturesCategory = allCreaturesTileset->getCategory(TILESET_CREATURE);
		if (allCreaturesCategory) {
			allCreaturesCategory->brushlist.clear();
			success = true;
		}
	}
	
	if (g_materials.tilesets.count("NPCs") > 0) {
		Tileset* npcTileset = g_materials.tilesets["NPCs"];
		TilesetCategory* npcCategory = npcTileset->getCategory(TILESET_CREATURE);
		if (npcCategory) {
			for (BrushVector::iterator it = npcCategory->brushlist.begin(); it != npcCategory->brushlist.end(); ++it) {
				brushesToRemove.push_back(*it);
			}
			npcCategory->brushlist.clear();
			success = true;
		}
	}
	
	if (g_materials.tilesets.count("Others") > 0) {
		Tileset* othersTileset = g_materials.tilesets["Others"];
		TilesetCategory* othersCategory = othersTileset->getCategory(TILESET_CREATURE);
		if (othersCategory) {
			for (BrushVector::iterator it = othersCategory->brushlist.begin(); it != othersCategory->brushlist.end(); ++it) {
				brushesToRemove.push_back(*it);
			}
			othersCategory->brushlist.clear();
			success = true;
		}
	}
	
	// Remove creature brushes from g_brushes
	// We need to collect the keys to remove first to avoid modifying the map during iteration
	const BrushMap& allBrushes = g_brushes.getMap();
	std::vector<std::string> brushKeysToRemove;
	
	for (BrushMap::const_iterator it = allBrushes.begin(); it != allBrushes.end(); ++it) {
		if (it->second && it->second->isCreature()) {
			brushKeysToRemove.push_back(it->first);
		}
	}
	
	// Now remove the brushes from g_brushes
	for (std::vector<std::string>::iterator it = brushKeysToRemove.begin(); it != brushKeysToRemove.end(); ++it) {
		g_brushes.removeBrush(*it);
	}
	
	// Delete the brush objects to prevent memory leaks
	for (std::vector<Brush*>::iterator it = brushesToRemove.begin(); it != brushesToRemove.end(); ++it) {
		delete *it;
	}
	
	// Clear creature database
	g_creatures.clear();
	
	// Recreate empty tilesets if needed
	g_materials.createOtherTileset();
	
	// Refresh the palette
	g_gui.RefreshPalettes();
	
	// Refresh current tileset and creature list in this panel
	OnUpdate();
	
	if (success) {
		g_gui.PopupDialog("Success", "All creatures have been purged from the palettes.", wxOK);
	} else {
		wxMessageBox("There was a problem purging the creatures.", "Error", wxOK | wxICON_ERROR, g_gui.root);
	}
	
	return success;
}

void CreaturePalettePanel::OnChangeSpawnTime(wxSpinEvent& event) {
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetSpawnTime(event.GetPosition());
}

void CreaturePalettePanel::OnChangeSpawnSize(wxSpinEvent& event) {
	if (!handling_event) {
		handling_event = true;
		g_gui.ActivatePalette(GetParentPalette());
		g_gui.SetBrushSize(event.GetPosition());
		handling_event = false;
	}
}

void CreaturePalettePanel::OnClickSearchButton(wxCommandEvent& event) {
	// Get the text from the search field and filter
	wxString searchText = search_field->GetValue();
	FilterCreatures(searchText);
}

void CreaturePalettePanel::OnSearchFieldText(wxCommandEvent& event) {
	// Filter as user types
	FilterCreatures(search_field->GetValue());
}

void CreaturePalettePanel::OnSearchFieldFocus(wxFocusEvent& event) {
	// Disable hotkeys when search field receives focus
	g_gui.DisableHotkeys();
	event.Skip();
}

void CreaturePalettePanel::OnSearchFieldKillFocus(wxFocusEvent& event) {
	// Re-enable hotkeys when search field loses focus
	g_gui.EnableHotkeys();
	event.Skip();
}

void CreaturePalettePanel::OnSearchFieldKeyDown(wxKeyEvent& event) {
	// Handle Enter key specially
	if (event.GetKeyCode() == WXK_RETURN) {
		FilterCreatures(search_field->GetValue());
	} else if (event.GetKeyCode() == WXK_ESCAPE) {
		// Clear search field and reset the list on Escape
		search_field->Clear();
		FilterCreatures(wxEmptyString);
		// Set focus back to the map
		wxWindow* mapCanvas = g_gui.root->FindWindowByName("MapCanvas");
		if (mapCanvas) {
			mapCanvas->SetFocus();
		}
	} else {
		// Process the event normally for all other keys
		event.Skip();
	}
}

void CreaturePalettePanel::FilterCreatures(const wxString& search_text) {
	if (search_text.IsEmpty()) {
		// Reset to show all creatures
		int currentSelection = tileset_choice->GetSelection();
		if (currentSelection != wxNOT_FOUND) {
			SelectTileset(currentSelection);
		}
		return;
	}
	
	// Remember the current selection
	int index = tileset_choice->GetSelection();
	if (index == wxNOT_FOUND) return;
	
	const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
	
	// Create a filtered list of brushes
	BrushVector filtered_brushes;
	
	// Used to track seen creature names to avoid duplicates (except for All Creatures category)
	std::set<std::string> seenCreatures;
	bool isAllCreaturesCategory = (tileset_choice->GetString(index) == "All Creatures");
	
	// Convert search text to lowercase for case-insensitive search
	wxString searchLower = search_text.Lower();
	
	// Check if the search is for a looktype (LT:123 format)
	bool isLooktypeSearch = false;
	long searchLooktype = 0;
	
	if (searchLower.StartsWith("lt:") || searchLower.StartsWith("looktype:")) {
		wxString lookTypeStr = searchLower.AfterFirst(':').Trim();
		if (lookTypeStr.ToLong(&searchLooktype)) {
			isLooktypeSearch = true;
		}
	}
	
	for (BrushVector::const_iterator iter = tsc->brushlist.begin(); iter != tsc->brushlist.end(); ++iter) {
		if (!(*iter)->isCreature()) continue;
		
		CreatureBrush* creatureBrush = dynamic_cast<CreatureBrush*>(*iter);
		if (!creatureBrush) continue;
		
		std::string creatureName = (*iter)->getName();
		wxString name = wxstr(creatureName).Lower();
		
		// For "All Creatures" category, don't add duplicates
		if (!isAllCreaturesCategory && seenCreatures.count(creatureName) > 0) {
			continue;
		}
		
		bool match = false;
		
		// Check if this is a looktype search
		if (isLooktypeSearch) {
			// Match by looktype
			if (creatureBrush->getType() && creatureBrush->getType()->outfit.lookType == searchLooktype) {
				match = true;
			}
		} else {
			// Standard name search
			if (name.Find(searchLower) != wxNOT_FOUND) {
				match = true;
			}
		}
		
		if (match) {
			filtered_brushes.push_back(*iter);
			seenCreatures.insert(creatureName);
		}
	}
	
	// Apply the filtered list to the appropriate view
	if (use_sprite_view) {
		// Update sprite view
		sprite_panel->Clear();
		sprite_panel->LoadCreatures(filtered_brushes);
	} else {
		// Update list view
		creature_list->Clear();
		
		for (BrushVector::const_iterator iter = filtered_brushes.begin(); iter != filtered_brushes.end(); ++iter) {
			creature_list->Append(wxstr((*iter)->getName()), *iter);
		}
		
		// Sort the filtered list
		creature_list->Sort();
	}
	
	// Select first result if any
	if (!filtered_brushes.empty()) {
		SelectCreature(0);
		creature_brush_button->Enable(true);
	} else {
		creature_brush_button->Enable(false);
	}
}

void CreaturePalettePanel::OnSpriteSelected(wxCommandEvent& event) {
	Brush* old_brush = g_gui.GetCurrentBrush();
	
	// Update selection
	SelectCreatureBrush();
	g_gui.ActivatePalette(GetParentPalette());
	
	// Get the newly selected brush
	Brush* new_brush = g_gui.GetCurrentBrush();
	
	// If we selected the same brush, first set to nullptr then reselect
	if(old_brush && new_brush && old_brush == new_brush) {
		g_gui.SelectBrush(nullptr, TILESET_CREATURE);
	}
	
	// Now select the brush (either for the first time or re-selecting)
	g_gui.SelectBrush();
}

// New method to switch between list and sprite view
void CreaturePalettePanel::SetViewMode(bool use_sprites) {
	// Store original selection
	Brush* selected_brush = GetSelectedBrush();
	
	// Update mode flag
	use_sprite_view = use_sprites;
	
	// Update UI elements
	if (use_sprites) {
		// Switch to sprite view
		creature_list->Hide();
		
		// Load creatures from the current category
		int index = tileset_choice->GetSelection();
		if (index != wxNOT_FOUND) {
			const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
			// Pre-generate creature sprites when switching to sprite view
			int sprite_size = sprite_panel->GetSpriteSize();
			g_creature_sprites.generateCreatureSprites(tsc->brushlist, sprite_size, sprite_size);
			sprite_panel->LoadCreatures(tsc->brushlist);
		}
		
		sprite_panel->Show();
	} else {
		// Switch to list view
		sprite_panel->Hide();
		creature_list->Show();
	}
	
	// Update layout
	view_sizer->Layout();
	
	// Restore selection
	if (selected_brush) {
		SelectBrush(selected_brush);
	}
}

void CreaturePalettePanel::OnClickViewToggle(wxCommandEvent& event) {
	SetViewMode(view_toggle->GetValue());
}

// ============================================================================
// CreatureSpritePanel - Panel to display creature sprites in a grid

BEGIN_EVENT_TABLE(CreatureSpritePanel, wxScrolledWindow)
EVT_PAINT(CreatureSpritePanel::OnPaint)
EVT_SIZE(CreatureSpritePanel::OnSize)
EVT_LEFT_DOWN(CreatureSpritePanel::OnMouseClick)
EVT_MOTION(CreatureSpritePanel::OnMouseMove)
END_EVENT_TABLE()

CreatureSpritePanel::CreatureSpritePanel(wxWindow* parent) : 
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS),
	columns(0),
	sprite_size(40),
	padding(6),
	selected_index(-1),
	hover_index(-1),
	buffer(nullptr) {
	
	// Set background color
	SetBackgroundColour(wxColour(245, 245, 245));
	
	// Enable scrolling
	SetScrollRate(1, 10);
}

CreatureSpritePanel::~CreatureSpritePanel() {
	delete buffer;
}

void CreatureSpritePanel::Clear() {
	creatures.clear();
	selected_index = -1;
	hover_index = -1;
	Refresh();
}

void CreatureSpritePanel::LoadCreatures(const BrushVector& brushlist) {
	// Clear any existing creatures
	creatures.clear();
	selected_index = -1;
	hover_index = -1;
	
	// Copy valid creature brushes
	for (BrushVector::const_iterator iter = brushlist.begin(); iter != brushlist.end(); ++iter) {
		if ((*iter)->isCreature()) {
			creatures.push_back(*iter);
		}
	}
	
	// Select first creature if any
	if (!creatures.empty()) {
		selected_index = 0;
	}
	
	// Calculate layout and refresh
	RecalculateGrid();
	Refresh();
}

void CreatureSpritePanel::RecalculateGrid() {
	// Get the client size of the panel
	int panel_width, panel_height;
	GetClientSize(&panel_width, &panel_height);
	
	// Calculate number of columns based on available width
	columns = std::max(1, (panel_width - padding) / (sprite_size + padding));
	
	// Calculate number of rows
	int rows = creatures.empty() ? 0 : (creatures.size() + columns - 1) / columns;
	
	// Set virtual size for scrolling
	int virtual_height = rows * (sprite_size + padding) + padding;
	SetVirtualSize(panel_width, virtual_height);
	
	// Recreate buffer with new size if needed
	if (buffer) {
		delete buffer;
		buffer = nullptr;
	}
	
	if (panel_width > 0 && panel_height > 0) {
		buffer = new wxBitmap(panel_width, panel_height);
	}
}

void CreatureSpritePanel::OnPaint(wxPaintEvent& event) {
	wxPaintDC dc(this);
	DoPrepareDC(dc);
	
	// Check if we need to redraw the buffer
	if (!buffer || buffer->GetWidth() != dc.GetSize().GetWidth() || buffer->GetHeight() != dc.GetSize().GetHeight()) {
		if (buffer) {
			delete buffer;
		}
		buffer = new wxBitmap(dc.GetSize().GetWidth(), dc.GetSize().GetHeight());
	}
	
	wxMemoryDC memDC;
	memDC.SelectObject(*buffer);
	
	// Fill background
	memDC.SetBackground(wxBrush(GetBackgroundColour()));
	memDC.Clear();
	
	// Get visible region
	int x_start, y_start;
	GetViewStart(&x_start, &y_start);
	int width, height;
	GetClientSize(&width, &height);
	
	// Calculate first and last visible row
	int first_row = std::max(0, y_start / (sprite_size + padding));
	int last_row = std::min((int)((creatures.size() + columns - 1) / columns), 
						   (y_start + height) / (sprite_size + padding) + 1);
	
	// Draw visible sprites
	for (int row = first_row; row < last_row; ++row) {
		for (int col = 0; col < columns; ++col) {
			int index = row * columns + col;
			if (index < (int)creatures.size()) {
				int x = padding + col * (sprite_size + padding);
				int y = padding + row * (sprite_size + padding);
				
				CreatureType* ctype = static_cast<CreatureBrush*>(creatures[index])->getType();
				DrawSprite(memDC, x, y, ctype, index == selected_index);
			}
		}
	}
	
	// Draw to screen
	dc.Blit(0, 0, width, height, &memDC, 0, 0);
}

void CreatureSpritePanel::DrawSprite(wxDC& dc, int x, int y, CreatureType* ctype, bool selected) {
	// Check if we have a valid creature type
	if (!ctype) return;
	
	// Draw the sprite box background
	dc.SetBrush(wxBrush(wxColour(240, 240, 240)));
	dc.SetPen(wxPen(wxColour(180, 180, 180)));
	dc.DrawRectangle(x, y, sprite_size, sprite_size);
	
	// Check if creature has a looktype
	if (ctype->outfit.lookType != 0) {
		// Get the sprite bitmap if available
		wxBitmap* bitmap = g_creature_sprites.getSpriteBitmap(ctype->outfit.lookType, sprite_size, sprite_size);
		
		if (bitmap) {
			// Draw the actual creature sprite
			dc.DrawBitmap(*bitmap, x, y, true);
			
			// Draw creature name at the bottom
			wxString name = wxstr(ctype->name);
			if (name.length() > 15) {
				name = name.Left(12) + "...";
			}
			
			// Add a semi-transparent background for the name text
			dc.SetBrush(wxBrush(wxColour(0, 0, 0, 180)));
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.DrawRectangle(x, y + sprite_size - 14, sprite_size, 14);
			
			// Draw the name text
			dc.SetTextForeground(wxColour(255, 255, 255));
			dc.SetFont(wxFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
			dc.DrawLabel(name, wxRect(x, y + sprite_size - 14, sprite_size, 14), wxALIGN_CENTER);
			
			// Draw looktype in top-right corner
			wxString lookinfo = wxString::Format("%d", ctype->outfit.lookType);
			
			// Semi-transparent background for looktype
			if (ctype->isNpc) {
				dc.SetBrush(wxBrush(wxColour(200, 150, 100, 180)));
			} else {
				dc.SetBrush(wxBrush(wxColour(100, 150, 200, 180)));
			}
			dc.DrawRectangle(x + sprite_size - 24, y, 24, 14);
			
			// Draw the looktype text
			dc.SetTextForeground(wxColour(255, 255, 255));
			dc.SetFont(wxFont(6, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
			dc.DrawLabel(lookinfo, wxRect(x + sprite_size - 24, y, 24, 14), wxALIGN_CENTER);
		} else {
			// No sprite found, draw a warning placeholder
			dc.SetBrush(wxBrush(wxColour(255, 200, 200)));
			dc.SetPen(wxPen(wxColour(180, 100, 100)));
			dc.DrawRectangle(x, y, sprite_size, sprite_size);
			
			wxString name = wxstr(ctype->name);
			if (name.length() > 12) {
				name = name.Left(9) + "...";
			}
			
			dc.SetTextForeground(wxColour(200, 0, 0));
			dc.SetFont(wxFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
			dc.DrawLabel(name, wxRect(x, y + sprite_size/2, sprite_size, sprite_size/2), wxALIGN_CENTER);
			
			wxString lookinfo = wxString::Format("LT: %d", ctype->outfit.lookType);
			dc.DrawLabel(lookinfo, wxRect(x, y, sprite_size, sprite_size/2), wxALIGN_CENTER);
			
			// "Missing" text
			dc.SetTextForeground(wxColour(200, 0, 0));
			dc.DrawLabel("Missing", wxRect(x, y+6, sprite_size, 12), wxALIGN_CENTER);
		}
	} else if (ctype->outfit.lookItem != 0) {
		// Draw as an item
		dc.SetBrush(wxBrush(wxColour(230, 230, 200)));
		dc.SetPen(wxPen(wxColour(180, 180, 150)));
		dc.DrawRectangle(x, y, sprite_size, sprite_size);
		
		wxString name = wxstr(ctype->name);
		if (name.length() > 12) {
			name = name.Left(9) + "...";
		}
		
		dc.SetTextForeground(wxColour(0, 0, 0));
		dc.DrawLabel(name, wxRect(x, y + sprite_size/2, sprite_size, sprite_size/2), wxALIGN_CENTER);
		
		wxString itemText = wxString::Format("Item: %d", ctype->outfit.lookItem);
		dc.DrawLabel(itemText, wxRect(x, y, sprite_size, sprite_size/2), wxALIGN_CENTER);
	} else {
		// No looktype or lookitem, draw placeholder
		dc.SetBrush(wxBrush(wxColour(200, 200, 200)));
		dc.SetPen(wxPen(wxColour(100, 100, 100)));
		dc.DrawRectangle(x, y, sprite_size, sprite_size);
		
		// Draw creature name
		wxString name = wxstr(ctype->name);
		if (name.length() > 10) {
			name = name.Left(7) + "...";
		}
		
		dc.SetTextForeground(wxColour(0, 0, 0));
		dc.SetFont(wxFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
		dc.DrawLabel(name, wxRect(x, y, sprite_size, sprite_size), wxALIGN_CENTER);
		
		// "No sprite" text
		dc.SetTextForeground(wxColour(80, 80, 80));
		dc.DrawLabel("No sprite", wxRect(x, y+sprite_size-14, sprite_size, 14), wxALIGN_CENTER);
	}
	
	// Draw selection or hover highlight
	if (selected) {
		dc.SetPen(wxPen(wxColour(0, 0, 200), 2));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawRectangle(x - 1, y - 1, sprite_size + 2, sprite_size + 2);
	} else if ((int)creatures.size() > hover_index && hover_index >= 0 && 
			   GetSpriteIndexAt(x, y) >= 0 && GetSpriteIndexAt(x, y) == hover_index) {
		dc.SetPen(wxPen(wxColour(200, 200, 0), 1));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawRectangle(x - 1, y - 1, sprite_size + 2, sprite_size + 2);
	}
}

void CreatureSpritePanel::OnSize(wxSizeEvent& event) {
	RecalculateGrid();
	Refresh();
}

void CreatureSpritePanel::OnScroll(wxScrollWinEvent& event) {
	Refresh();
	event.Skip();
}

void CreatureSpritePanel::OnMouseClick(wxMouseEvent& event) {
	// Get the position in scroll coordinates
	int x, y;
	CalcUnscrolledPosition(event.GetX(), event.GetY(), &x, &y);
	
	// Find which sprite was clicked
	int index = GetSpriteIndexAt(x, y);
	
	if (index >= 0 && index < (int)creatures.size()) {
		SelectIndex(index);
		
		// Send selection event to parent
		wxCommandEvent selectionEvent(wxEVT_COMMAND_LISTBOX_SELECTED);
		selectionEvent.SetEventObject(this);
		GetParent()->GetEventHandler()->ProcessEvent(selectionEvent);
	}
}

void CreatureSpritePanel::OnMouseMove(wxMouseEvent& event) {
	// Get the position in scroll coordinates
	int x, y;
	CalcUnscrolledPosition(event.GetX(), event.GetY(), &x, &y);
	
	// Find which sprite is under the mouse
	int index = GetSpriteIndexAt(x, y);
	
	if (index != hover_index) {
		hover_index = index;
		Refresh();
	}
	
	event.Skip();
}

int CreatureSpritePanel::GetSpriteIndexAt(int x, int y) const {
	// Adjust for padding
	if (x < padding || y < padding) {
		return -1;
	}
	
	// Calculate row and column
	int col = (x - padding) / (sprite_size + padding);
	int row = (y - padding) / (sprite_size + padding);
	
	// Check if within sprite bounds
	int sprite_x = padding + col * (sprite_size + padding);
	int sprite_y = padding + row * (sprite_size + padding);
	
	if (x >= sprite_x && x < sprite_x + sprite_size &&
		y >= sprite_y && y < sprite_y + sprite_size) {
		
		// Convert to index
		int index = row * columns + col;
		if (index >= 0 && index < (int)creatures.size()) {
			return index;
		}
	}
	
	return -1;
}

void CreatureSpritePanel::SelectIndex(int index) {
	if (index >= 0 && index < (int)creatures.size() && index != selected_index) {
		selected_index = index;
		Refresh();
		
		// Ensure the selected creature is visible
		if (selected_index >= 0) {
			int row = selected_index / columns;
			int col = selected_index % columns;
			int x = padding + col * (sprite_size + padding);
			int y = padding + row * (sprite_size + padding);
			
			// Scroll to make the selected creature visible
			int client_width, client_height;
			GetClientSize(&client_width, &client_height);
			
			int x_scroll, y_scroll;
			GetViewStart(&x_scroll, &y_scroll);
			
			// Adjust vertical scroll if needed
			if (y < y_scroll) {
				Scroll(-1, y / 10); // / 10 because of scroll rate
			} else if (y + sprite_size > y_scroll + client_height) {
				Scroll(-1, (y + sprite_size - client_height) / 10 + 1);
			}
		}
	}
}

Brush* CreatureSpritePanel::GetSelectedBrush() const {
	if (selected_index >= 0 && selected_index < (int)creatures.size()) {
		return creatures[selected_index];
	}
	return nullptr;
}

bool CreatureSpritePanel::SelectBrush(const Brush* brush) {
	if (!brush || !brush->isCreature()) {
		return false;
	}
	
	for (size_t i = 0; i < creatures.size(); ++i) {
		if (creatures[i] == brush) {
			SelectIndex(i);
			return true;
		}
	}
	
	return false;
}

void CreatureSpritePanel::EnsureVisible(const Brush* brush) {
	if (!brush || !brush->isCreature()) {
		return;
	}
	
	for (size_t i = 0; i < creatures.size(); ++i) {
		if (creatures[i] == brush) {
			int row = i / columns;
			int y = padding + row * (sprite_size + padding);
			
			int client_height;
			GetClientSize(nullptr, &client_height);
			
			int x_scroll, y_scroll;
			GetViewStart(&x_scroll, &y_scroll);
			
			// Adjust vertical scroll if needed
			if (y < y_scroll || y + sprite_size > y_scroll + client_height) {
				Scroll(-1, y / 10); // / 10 because of scroll rate
			}
			break;
		}
	}
}

int CreatureSpritePanel::GetSpriteSize() const {
	return sprite_size;
}
