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

// Define the new event ID for the Load NPCs button
#define PALETTE_LOAD_NPCS_BUTTON 1952
#define PALETTE_LOAD_MONSTERS_BUTTON 1953
#define PALETTE_PURGE_CREATURES_BUTTON 1954

// ============================================================================
// Creature palette

BEGIN_EVENT_TABLE(CreaturePalettePanel, PalettePanel)
EVT_CHOICE(PALETTE_CREATURE_TILESET_CHOICE, CreaturePalettePanel::OnTilesetChange)

EVT_LISTBOX(PALETTE_CREATURE_LISTBOX, CreaturePalettePanel::OnListBoxChange)

EVT_TOGGLEBUTTON(PALETTE_CREATURE_BRUSH_BUTTON, CreaturePalettePanel::OnClickCreatureBrushButton)
EVT_TOGGLEBUTTON(PALETTE_SPAWN_BRUSH_BUTTON, CreaturePalettePanel::OnClickSpawnBrushButton)
EVT_BUTTON(PALETTE_LOAD_NPCS_BUTTON, CreaturePalettePanel::OnClickLoadNPCsButton)
EVT_BUTTON(PALETTE_LOAD_MONSTERS_BUTTON, CreaturePalettePanel::OnClickLoadMonstersButton)
EVT_BUTTON(PALETTE_PURGE_CREATURES_BUTTON, CreaturePalettePanel::OnClickPurgeCreaturesButton)

EVT_SPINCTRL(PALETTE_CREATURE_SPAWN_TIME, CreaturePalettePanel::OnChangeSpawnTime)
EVT_SPINCTRL(PALETTE_CREATURE_SPAWN_SIZE, CreaturePalettePanel::OnChangeSpawnSize)
END_EVENT_TABLE()

CreaturePalettePanel::CreaturePalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	handling_event(false) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxSizer* sidesizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Creatures");
	tileset_choice = newd wxChoice(this, PALETTE_CREATURE_TILESET_CHOICE, wxDefaultPosition, wxDefaultSize, (int)0, (const wxString*)nullptr);
	sidesizer->Add(tileset_choice, 0, wxEXPAND);

	creature_list = newd SortableListBox(this, PALETTE_CREATURE_LISTBOX);
	sidesizer->Add(creature_list, 1, wxEXPAND);
	
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

	// sidesizer->Add(180, 1, wxEXPAND);

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
		if (creature_list->GetCount() == 0) {
			return nullptr;
		}
		Brush* brush = reinterpret_cast<Brush*>(creature_list->GetClientData(creature_list->GetSelection()));
		if (brush && brush->isCreature()) {
			g_gui.SetSpawnTime(creature_spawntime_spin->GetValue());
			return brush;
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
					SelectCreature(whatbrush->getName());
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
						SelectCreature(whatbrush->getName());
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

	for (TilesetContainer::const_iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
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
	if (tileset_choice->GetCount() == 0) {
		// No tilesets :(
		creature_brush_button->Enable(false);
	} else {
		const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
		// Select first house
		for (BrushVector::const_iterator iter = tsc->brushlist.begin();
			 iter != tsc->brushlist.end();
			 ++iter) {
			creature_list->Append(wxstr((*iter)->getName()), *iter);
		}
		creature_list->Sort();
		SelectCreature(0);

		tileset_choice->SetSelection(index);
	}
}

void CreaturePalettePanel::SelectCreature(size_t index) {
	// Save the old g_settings
	ASSERT(creature_list->GetCount() >= index);

	if (creature_list->GetCount() > 0) {
		creature_list->SetSelection(index);
	}

	SelectCreatureBrush();
}

void CreaturePalettePanel::SelectCreature(std::string name) {
	if (creature_list->GetCount() > 0) {
		if (!creature_list->SetStringSelection(wxstr(name))) {
			creature_list->SetSelection(0);
		}
	}

	SelectCreatureBrush();
}

void CreaturePalettePanel::SelectCreatureBrush() {
	if (creature_list->GetCount() > 0) {
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
	SelectCreature(event.GetSelection());
	g_gui.ActivatePalette(GetParentPalette());
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
	
	// Collect creature brushes from the "NPCs" and "Others" tilesets
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
