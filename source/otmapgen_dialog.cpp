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
#include "otmapgen_dialog.h"
#include "otmapgen.h"
#include "application.h"
#include "editor.h"
#include "settings.h"
#include "action.h"
#include "iomap.h"
#include "gui.h"

#include <wx/process.h>
#include <wx/stream.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/dir.h>

wxBEGIN_EVENT_TABLE(OTMapGenDialog, wxDialog)
	EVT_BUTTON(ID_GENERATE, OTMapGenDialog::OnGenerate)
	EVT_BUTTON(ID_PREVIEW, OTMapGenDialog::OnPreview)
	EVT_BUTTON(wxID_CANCEL, OTMapGenDialog::OnCancel)
	EVT_BUTTON(ID_FLOOR_UP, OTMapGenDialog::OnFloorUp)
	EVT_BUTTON(ID_FLOOR_DOWN, OTMapGenDialog::OnFloorDown)
	EVT_BUTTON(ID_ZOOM_IN, OTMapGenDialog::OnZoomIn)
	EVT_BUTTON(ID_ZOOM_OUT, OTMapGenDialog::OnZoomOut)
	EVT_TEXT(ID_SEED_TEXT, OTMapGenDialog::OnSeedChange)
	EVT_SPINCTRL(ID_WIDTH_SPIN, OTMapGenDialog::OnParameterChange)
	EVT_SPINCTRL(ID_HEIGHT_SPIN, OTMapGenDialog::OnParameterChange)
	EVT_CHOICE(ID_VERSION_CHOICE, OTMapGenDialog::OnParameterChangeText)
	EVT_CHOICE(ID_MOUNTAIN_TYPE_CHOICE, OTMapGenDialog::OnMountainTypeChange)
	EVT_COMMAND_SCROLL(ID_NOISE_INCREMENT_SLIDER, OTMapGenDialog::OnParameterChange)
	EVT_COMMAND_SCROLL(ID_ISLAND_DISTANCE_SLIDER, OTMapGenDialog::OnParameterChange)
	EVT_COMMAND_SCROLL(ID_CAVE_DEPTH_SLIDER, OTMapGenDialog::OnParameterChange)
	EVT_COMMAND_SCROLL(ID_CAVE_ROUGHNESS_SLIDER, OTMapGenDialog::OnParameterChange)
	EVT_COMMAND_SCROLL(ID_CAVE_CHANCE_SLIDER, OTMapGenDialog::OnParameterChange)
	EVT_COMMAND_SCROLL(ID_WATER_LEVEL_SLIDER, OTMapGenDialog::OnParameterChange)
	EVT_COMMAND_SCROLL(ID_EXPONENT_SLIDER, OTMapGenDialog::OnParameterChange)
	EVT_COMMAND_SCROLL(ID_LINEAR_SLIDER, OTMapGenDialog::OnParameterChange)
wxEND_EVENT_TABLE()

OTMapGenDialog::OTMapGenDialog(wxWindow* parent) : 
	wxDialog(parent, wxID_ANY, "Procedural Map Generator", wxDefaultPosition, wxSize(800, 650), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	current_preview(nullptr),
	current_preview_floor(7), // Start with ground level (floor 7)
	current_zoom(1.0),
	preview_offset_x(0),
	preview_offset_y(0) {
	
	// Create main sizer
	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
	
	// Create notebook for tabs
	wxNotebook* notebook = new wxNotebook(this, wxID_ANY);
	
	// === Basic Settings Tab ===
	wxPanel* basic_panel = new wxPanel(notebook);
	wxBoxSizer* basic_sizer = new wxBoxSizer(wxVERTICAL);
	
	// Basic parameters group
	wxStaticBoxSizer* basic_params_sizer = new wxStaticBoxSizer(wxVERTICAL, basic_panel, "Basic Parameters");
	
	// Seed
	wxFlexGridSizer* basic_grid_sizer = new wxFlexGridSizer(2, 2, 5, 10);
	basic_grid_sizer->AddGrowableCol(1);
	
	basic_grid_sizer->Add(new wxStaticText(basic_panel, wxID_ANY, "Seed:"), 0, wxALIGN_CENTER_VERTICAL);
	seed_text_ctrl = new wxTextCtrl(basic_panel, ID_SEED_TEXT, wxString::Format("%d", rand()));
	basic_grid_sizer->Add(seed_text_ctrl, 1, wxEXPAND);
	
	// Width
	basic_grid_sizer->Add(new wxStaticText(basic_panel, wxID_ANY, "Width:"), 0, wxALIGN_CENTER_VERTICAL);
	width_spin_ctrl = new wxSpinCtrl(basic_panel, ID_WIDTH_SPIN, "256", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 64, 2048, 256);
	basic_grid_sizer->Add(width_spin_ctrl, 1, wxEXPAND);
	
	// Height
	basic_grid_sizer->Add(new wxStaticText(basic_panel, wxID_ANY, "Height:"), 0, wxALIGN_CENTER_VERTICAL);
	height_spin_ctrl = new wxSpinCtrl(basic_panel, ID_HEIGHT_SPIN, "256", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 64, 2048, 256);
	basic_grid_sizer->Add(height_spin_ctrl, 1, wxEXPAND);
	
	// Version
	basic_grid_sizer->Add(new wxStaticText(basic_panel, wxID_ANY, "Version:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString versions;
	versions.Add("10.98");
	versions.Add("11.00");
	versions.Add("12.00");
	version_choice = new wxChoice(basic_panel, ID_VERSION_CHOICE, wxDefaultPosition, wxDefaultSize, versions);
	version_choice->SetSelection(0);
	basic_grid_sizer->Add(version_choice, 1, wxEXPAND);
	
	// Mountain Type
	basic_grid_sizer->Add(new wxStaticText(basic_panel, wxID_ANY, "Mountain Type:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString mountain_types;
	mountain_types.Add("MOUNTAIN");
	mountain_types.Add("SNOW");
	mountain_types.Add("SAND");
	mountain_type_choice = new wxChoice(basic_panel, ID_MOUNTAIN_TYPE_CHOICE, wxDefaultPosition, wxDefaultSize, mountain_types);
	mountain_type_choice->SetSelection(0);
	basic_grid_sizer->Add(mountain_type_choice, 1, wxEXPAND);
	
	basic_params_sizer->Add(basic_grid_sizer, 0, wxEXPAND | wxALL, 5);
	
	// Checkboxes
	terrain_only_checkbox = new wxCheckBox(basic_panel, wxID_ANY, "Terrain Only (no decorations)");
	sand_biome_checkbox = new wxCheckBox(basic_panel, wxID_ANY, "Enable Sand Biome");
	sand_biome_checkbox->SetValue(true);
	smooth_coastline_checkbox = new wxCheckBox(basic_panel, wxID_ANY, "Smooth Coastlines");
	smooth_coastline_checkbox->SetValue(true);
	add_caves_checkbox = new wxCheckBox(basic_panel, wxID_ANY, "Add Underground Caves");
	add_caves_checkbox->SetValue(true);
	
	basic_params_sizer->Add(terrain_only_checkbox, 0, wxEXPAND | wxALL, 5);
	basic_params_sizer->Add(sand_biome_checkbox, 0, wxEXPAND | wxALL, 5);
	basic_params_sizer->Add(smooth_coastline_checkbox, 0, wxEXPAND | wxALL, 5);
	basic_params_sizer->Add(add_caves_checkbox, 0, wxEXPAND | wxALL, 5);
	
	basic_sizer->Add(basic_params_sizer, 0, wxEXPAND | wxALL, 5);
	
	// Preview section
	wxStaticBoxSizer* preview_sizer = new wxStaticBoxSizer(wxVERTICAL, basic_panel, "Preview");
	preview_bitmap = new wxStaticBitmap(basic_panel, wxID_ANY, wxBitmap(256, 256));
	preview_bitmap->SetBackgroundColour(*wxBLACK);
	preview_sizer->Add(preview_bitmap, 1, wxEXPAND | wxALL, 5);
	
	// Floor navigation
	wxBoxSizer* floor_nav_sizer = new wxBoxSizer(wxHORIZONTAL);
	floor_down_button = new wxButton(basic_panel, ID_FLOOR_DOWN, "Floor -");
	floor_label = new wxStaticText(basic_panel, wxID_ANY, "Floor: 7 (Ground)");
	floor_up_button = new wxButton(basic_panel, ID_FLOOR_UP, "Floor +");
	
	floor_nav_sizer->Add(floor_down_button, 0, wxALL, 2);
	floor_nav_sizer->AddStretchSpacer();
	floor_nav_sizer->Add(floor_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
	floor_nav_sizer->AddStretchSpacer();
	floor_nav_sizer->Add(floor_up_button, 0, wxALL, 2);
	
	preview_sizer->Add(floor_nav_sizer, 0, wxEXPAND | wxALL, 5);
	
	// Zoom navigation
	wxBoxSizer* zoom_nav_sizer = new wxBoxSizer(wxHORIZONTAL);
	zoom_out_button = new wxButton(basic_panel, ID_ZOOM_OUT, "Zoom -");
	zoom_label = new wxStaticText(basic_panel, wxID_ANY, "Zoom: 100%");
	zoom_in_button = new wxButton(basic_panel, ID_ZOOM_IN, "Zoom +");
	
	zoom_nav_sizer->Add(zoom_out_button, 0, wxALL, 2);
	zoom_nav_sizer->AddStretchSpacer();
	zoom_nav_sizer->Add(zoom_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
	zoom_nav_sizer->AddStretchSpacer();
	zoom_nav_sizer->Add(zoom_in_button, 0, wxALL, 2);
	
	preview_sizer->Add(zoom_nav_sizer, 0, wxEXPAND | wxALL, 5);
	
	preview_button = new wxButton(basic_panel, ID_PREVIEW, "Generate Preview");
	preview_sizer->Add(preview_button, 0, wxALIGN_CENTER | wxALL, 5);
	
	basic_sizer->Add(preview_sizer, 1, wxEXPAND | wxALL, 5);
	basic_panel->SetSizer(basic_sizer);
	notebook->AddPage(basic_panel, "Basic Settings", true);
	
	// === Advanced Settings Tab ===
	wxPanel* advanced_panel = new wxPanel(notebook);
	wxBoxSizer* advanced_sizer = new wxBoxSizer(wxVERTICAL);
	
	wxStaticBoxSizer* advanced_params_sizer = new wxStaticBoxSizer(wxVERTICAL, advanced_panel, "Advanced Parameters");
	wxFlexGridSizer* advanced_grid_sizer = new wxFlexGridSizer(3, 3, 5, 10);
	advanced_grid_sizer->AddGrowableCol(1);
	
	// Noise Increment
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "Noise Increment:"), 0, wxALIGN_CENTER_VERTICAL);
	noise_increment_slider = new wxSlider(advanced_panel, ID_NOISE_INCREMENT_SLIDER, 100, 1, 300, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	advanced_grid_sizer->Add(noise_increment_slider, 1, wxEXPAND);
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "0.01 - 3.0"), 0, wxALIGN_CENTER_VERTICAL);
	
	// Island Distance
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "Island Distance:"), 0, wxALIGN_CENTER_VERTICAL);
	island_distance_slider = new wxSlider(advanced_panel, ID_ISLAND_DISTANCE_SLIDER, 92, 50, 99, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	advanced_grid_sizer->Add(island_distance_slider, 1, wxEXPAND);
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "0.5 - 0.99"), 0, wxALIGN_CENTER_VERTICAL);
	
	// Cave Depth
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "Cave Depth:"), 0, wxALIGN_CENTER_VERTICAL);
	cave_depth_slider = new wxSlider(advanced_panel, ID_CAVE_DEPTH_SLIDER, 20, 5, 50, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	advanced_grid_sizer->Add(cave_depth_slider, 1, wxEXPAND);
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "5 - 50"), 0, wxALIGN_CENTER_VERTICAL);
	
	// Cave Roughness
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "Cave Roughness:"), 0, wxALIGN_CENTER_VERTICAL);
	cave_roughness_slider = new wxSlider(advanced_panel, ID_CAVE_ROUGHNESS_SLIDER, 45, 10, 80, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	advanced_grid_sizer->Add(cave_roughness_slider, 1, wxEXPAND);
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "0.1 - 0.8"), 0, wxALIGN_CENTER_VERTICAL);
	
	// Cave Chance
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "Cave Chance:"), 0, wxALIGN_CENTER_VERTICAL);
	cave_chance_slider = new wxSlider(advanced_panel, ID_CAVE_CHANCE_SLIDER, 9, 1, 20, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	advanced_grid_sizer->Add(cave_chance_slider, 1, wxEXPAND);
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "0.01 - 0.2"), 0, wxALIGN_CENTER_VERTICAL);
	
	// Water Level
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "Water Level:"), 0, wxALIGN_CENTER_VERTICAL);
	water_level_slider = new wxSlider(advanced_panel, ID_WATER_LEVEL_SLIDER, 2, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	advanced_grid_sizer->Add(water_level_slider, 1, wxEXPAND);
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "0 - 10"), 0, wxALIGN_CENTER_VERTICAL);
	
	// Exponent
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "Exponent:"), 0, wxALIGN_CENTER_VERTICAL);
	exponent_slider = new wxSlider(advanced_panel, ID_EXPONENT_SLIDER, 140, 100, 300, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	advanced_grid_sizer->Add(exponent_slider, 1, wxEXPAND);
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "1.0 - 3.0"), 0, wxALIGN_CENTER_VERTICAL);
	
	// Linear
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "Linear:"), 0, wxALIGN_CENTER_VERTICAL);
	linear_slider = new wxSlider(advanced_panel, ID_LINEAR_SLIDER, 6, 1, 20, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	advanced_grid_sizer->Add(linear_slider, 1, wxEXPAND);
	advanced_grid_sizer->Add(new wxStaticText(advanced_panel, wxID_ANY, "1 - 20"), 0, wxALIGN_CENTER_VERTICAL);
	
	advanced_params_sizer->Add(advanced_grid_sizer, 1, wxEXPAND | wxALL, 5);
	advanced_sizer->Add(advanced_params_sizer, 1, wxEXPAND | wxALL, 5);
	advanced_panel->SetSizer(advanced_sizer);
	notebook->AddPage(advanced_panel, "Advanced Settings", false);
	
	main_sizer->Add(notebook, 1, wxEXPAND | wxALL, 5);
	
	// Buttons
	wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
	generate_button = new wxButton(this, ID_GENERATE, "Generate Map");
	cancel_button = new wxButton(this, wxID_CANCEL, "Cancel");
	
	button_sizer->Add(generate_button, 0, wxALL, 5);
	button_sizer->AddStretchSpacer();
	button_sizer->Add(cancel_button, 0, wxALL, 5);
	
	main_sizer->Add(button_sizer, 0, wxEXPAND | wxALL, 5);
	
	SetSizer(main_sizer);
	Center();
	
	// Generate initial random seed
	srand(time(nullptr));
	seed_text_ctrl->SetValue(wxString::Format("%d", rand()));
}

OTMapGenDialog::~OTMapGenDialog() {
	if (current_preview) {
		delete current_preview;
	}
}

void OTMapGenDialog::OnGenerate(wxCommandEvent& event) {
	if (GenerateMap()) {
		EndModal(wxID_OK);
	}
}

void OTMapGenDialog::OnCancel(wxCommandEvent& event) {
	EndModal(wxID_CANCEL);
}

void OTMapGenDialog::OnPreview(wxCommandEvent& event) {
	UpdatePreview();
}

void OTMapGenDialog::OnSeedChange(wxCommandEvent& event) {
	// Optional: Auto-update preview when seed changes
}

void OTMapGenDialog::OnParameterChange(wxSpinEvent& event) {
	// Optional: Auto-update preview when parameters change
}

void OTMapGenDialog::OnParameterChange(wxScrollEvent& event) {
	// Optional: Auto-update preview when slider parameters change
}

void OTMapGenDialog::OnParameterChangeText(wxCommandEvent& event) {
	// Optional: Auto-update preview when parameters change
}

void OTMapGenDialog::OnMountainTypeChange(wxCommandEvent& event) {
	// Optional: Auto-update preview when mountain type changes
}

void OTMapGenDialog::OnFloorUp(wxCommandEvent& event) {
	if (current_preview_floor > 0) {
		current_preview_floor--;
		UpdateFloorLabel();
		UpdatePreviewFloor();
	}
}

void OTMapGenDialog::OnFloorDown(wxCommandEvent& event) {
	if (current_preview_floor < 7) { // Only go up to floor 7
		current_preview_floor++;
		UpdateFloorLabel();
		UpdatePreviewFloor();
	}
}

void OTMapGenDialog::OnZoomIn(wxCommandEvent& event) {
	if (current_zoom < 4.0) { // Max zoom 400%
		current_zoom *= 1.25; // 25% increase
		UpdateZoomLabel();
		UpdatePreviewFloor();
	}
}

void OTMapGenDialog::OnZoomOut(wxCommandEvent& event) {
	if (current_zoom > 0.25) { // Min zoom 25%
		current_zoom /= 1.25; // 25% decrease
		UpdateZoomLabel();
		UpdatePreviewFloor();
	}
}

void OTMapGenDialog::UpdatePreview() {
	preview_button->SetLabel("Generating...");
	preview_button->Enable(false);
	
	try {
		// Create configuration from dialog values
		GenerationConfig config = BuildGenerationConfig();
		
		// Generate preview using C++ implementation
		OTMapGenerator generator;
		current_layers = generator.generateLayers(config);
		
		// Update the preview for the current floor
		UpdatePreviewFloor();
		
	} catch (const std::exception& e) {
		wxMessageBox(wxString::Format("Failed to generate preview: %s", e.what()), "Preview Error", wxOK | wxICON_ERROR);
	}
	
	preview_button->SetLabel("Generate Preview");
	preview_button->Enable(true);
}

void OTMapGenDialog::UpdatePreviewFloor() {
	if (current_layers.empty()) {
		return; // No data to preview
	}
	
	try {
		// Create configuration for scaling
		GenerationConfig config = BuildGenerationConfig();
		
		// Use the SAME mapping as OTBM generation:
		// Preview floor 7 = layers[0] (ground level)
		// Preview floor 6 = layers[1] (above ground 1)
		// ...
		// Preview floor 0 = layers[7] (highest above ground)
		int layerIndex = 7 - current_preview_floor;
		
		// Clamp to valid range
		layerIndex = std::max(0, std::min(7, layerIndex));
		
		if (layerIndex < static_cast<int>(current_layers.size()) && !current_layers[layerIndex].empty()) {
			const auto& layerData = current_layers[layerIndex];
			int preview_width = 256;
			int preview_height = 256;
			wxImage preview_image(preview_width, preview_height);
			
			// Calculate zoom-adjusted scale and center point
			double base_scale_x = (double)config.width / preview_width;
			double base_scale_y = (double)config.height / preview_height;
			
			// Apply zoom - higher zoom means smaller scale (show less area)
			double scale_x = base_scale_x / current_zoom;
			double scale_y = base_scale_y / current_zoom;
			
			// Calculate center offset for zoomed view
			int center_x = config.width / 2 + preview_offset_x;
			int center_y = config.height / 2 + preview_offset_y;
			
			for (int y = 0; y < preview_height; ++y) {
				for (int x = 0; x < preview_width; ++x) {
					// Calculate source coordinates with zoom and offset
					int src_x = center_x + (int)((x - preview_width/2) * scale_x);
					int src_y = center_y + (int)((y - preview_height/2) * scale_y);
					
					// Set default color (black for out of bounds)
					unsigned char r = 0, g = 0, b = 0;
					
					if (src_x >= 0 && src_x < config.width && src_y >= 0 && src_y < config.height) {
						// Calculate index in the 1D layer
						int tileIndex = src_y * config.width + src_x;
						
						if (tileIndex < static_cast<int>(layerData.size())) {
							uint16_t tileId = layerData[tileIndex];
							
							// Convert tile ID to color for preview
							GetTilePreviewColor(tileId, r, g, b);
						}
					}
					
					preview_image.SetRGB(x, y, r, g, b);
				}
			}
			
			if (current_preview) {
				delete current_preview;
			}
			current_preview = new wxBitmap(preview_image);
			preview_bitmap->SetBitmap(*current_preview);
			preview_bitmap->Refresh();
		}
		
	} catch (const std::exception& e) {
		wxMessageBox(wxString::Format("Failed to update preview floor: %s", e.what()), "Preview Error", wxOK | wxICON_ERROR);
	}
}

void OTMapGenDialog::UpdateFloorLabel() {
	wxString label;
	if (current_preview_floor == 7) {
		label = "Floor: 7 (Ground)";
	} else if (current_preview_floor < 7) {
		label = wxString::Format("Floor: %d (Above Ground %d)", current_preview_floor, 7 - current_preview_floor);
	} else {
		label = wxString::Format("Floor: %d", current_preview_floor);
	}
	floor_label->SetLabel(label);
}

void OTMapGenDialog::UpdateZoomLabel() {
	wxString label = wxString::Format("Zoom: %.0f%%", current_zoom * 100);
	zoom_label->SetLabel(label);
}

bool OTMapGenDialog::GenerateMap() {
	try {
		// Create configuration from dialog values
		GenerationConfig config = BuildGenerationConfig();
		
		wxProgressDialog progress("Generating Map", "Please wait while the map is being generated...", 100, this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT);
		progress.Pulse();
		
		// Generate the map data using C++ implementation
		OTMapGenerator generator;
		auto layers = generator.generateLayers(config);
		
		if (layers.empty()) {
			wxMessageBox("Failed to generate map data.", "Generation Error", wxOK | wxICON_ERROR);
			return false;
		}
		
		// Create a temporary map to hold our generated content
		Map tempMap;
		tempMap.setWidth(config.width);
		tempMap.setHeight(config.height);
		tempMap.setName("Generated Map");
		tempMap.setMapDescription("Procedurally generated map");
		
		// Set map properties
		tempMap.setSpawnFilename("");
		tempMap.setHouseFilename("");
		
		// Place generated tiles into the temporary map
		// Floor mapping for Tibia coordinates:
		// - Floor 7 = ground level (surface) ← layers[0] should go here
		// - Floors 6-0 = above ground (6 is just above surface, 0 is highest) ← layers[1-7] go here
		// 
		// We need to REVERSE the mapping when writing to OTBM:
		// - layers[0] → Tibia floor 7 (ground level)
		// - layers[1] → Tibia floor 6 (above ground level 1)  
		// - layers[7] → Tibia floor 0 (highest above ground)
		
		for (int layerIndex = 0; layerIndex < static_cast<int>(layers.size()) && layerIndex < 8; ++layerIndex) {
			// REVERSE the mapping: layers[0]→floor 7, layers[1]→floor 6, ..., layers[7]→floor 0
			int tibiaZ = 7 - layerIndex; // This puts ground level (layers[0]) at floor 7
			
			// Extract tiles from the 1D layer data
			const auto& layerData = layers[layerIndex];
			int tileIndex = 0;
			
			for (int y = 0; y < config.height; ++y) {
				for (int x = 0; x < config.width; ++x) {
					if (tileIndex >= static_cast<int>(layerData.size())) {
						break; // Safety check
					}
					
					uint16_t tileId = layerData[tileIndex++];
					if (tileId != 0) {
						Position pos(x, y, tibiaZ);
						
						// Create tile in temp map using the safe method
						TileLocation* location = tempMap.createTileL(pos);
						Tile* tile = tempMap.allocator(location);
						
						// Create ground item
						Item* groundItem = Item::Create(tileId);
						if (groundItem) {
							tile->ground = groundItem;
							tempMap.setTile(pos, tile);
						} else {
							delete tile; // Clean up if item creation failed
						}
					}
				}
			}
		}
		
		// Add decorations if enabled (only on surface level - Tibia Z=7)
		if (!config.terrain_only && layers.size() >= 8) {
			std::mt19937 decoration_rng(std::hash<std::string>{}(config.seed));
			std::uniform_real_distribution<double> dist(0.0, 1.0);
			
			// Use the ground level layer (layers[0] will become Tibia floor 7)
			const auto& surfaceLayer = layers[0];
			int tileIndex = 0;
			
			for (int y = 0; y < config.height; ++y) {
				for (int x = 0; x < config.width; ++x) {
					if (tileIndex >= static_cast<int>(surfaceLayer.size())) {
						break; // Safety check
					}
					
					uint16_t tileId = surfaceLayer[tileIndex++];
					if (tileId == OTMapGenItems::GRASS_TILE_ID && dist(decoration_rng) < 0.03) {
						Position pos(x, y, 7); // Surface level in Tibia coordinates
						Tile* tile = tempMap.getTile(pos);
						if (tile) {
							// Choose random decoration
							uint16_t decorationId;
							double rand_val = dist(decoration_rng);
							if (rand_val < 0.6) {
								decorationId = OTMapGenItems::TREE_ID;
							} else if (rand_val < 0.8) {
								decorationId = OTMapGenItems::BUSH_ID;
							} else {
								decorationId = OTMapGenItems::FLOWER_ID;
							}
							
							Item* decoration = Item::Create(decorationId);
							if (decoration) {
								tile->addItem(decoration);
							}
						}
					}
				}
			}
		}
		
		// Create a temporary OTBM file
		wxString tempDir = wxStandardPaths::Get().GetTempDir();
		wxString tempFileName = wxString::Format("generated_map_%ld.otbm", wxGetLocalTime());
		wxString tempFilePath = tempDir + wxFileName::GetPathSeparator() + tempFileName;
		
		// Save the temporary map as OTBM
		progress.SetLabel("Saving temporary map file...");
		
		// Use the map's save functionality
		bool saveSuccess = false;
		try {
			IOMapOTBM mapLoader(tempMap.getVersion());
			saveSuccess = mapLoader.saveMap(tempMap, wxFileName(tempFilePath));
		} catch (...) {
			saveSuccess = false;
		}
		
		if (!saveSuccess) {
			wxMessageBox("Failed to save temporary map file.", "Save Error", wxOK | wxICON_ERROR);
			return false;
		}
		
		progress.SetLabel("Loading generated map...");
		progress.Pulse();
		
		// Load the temporary file into the editor
		bool loadSuccess = false;
		try {
			// Use the GUI's LoadMap function which handles everything properly
			loadSuccess = g_gui.LoadMap(wxFileName(tempFilePath));
		} catch (...) {
			loadSuccess = false;
		}
		
		// Clean up the temporary file
		if (wxFileExists(tempFilePath)) {
			wxRemoveFile(tempFilePath);
		}
		
		if (loadSuccess) {
			wxMessageBox("Procedural map generated and loaded successfully!", "Success", wxOK | wxICON_INFORMATION);
			return true;
		} else {
			wxMessageBox("Failed to load the generated map.", "Load Error", wxOK | wxICON_ERROR);
			return false;
		}
		
	} catch (const std::exception& e) {
		wxMessageBox(wxString::Format("Map generation failed with error: %s", e.what()), "Generation Error", wxOK | wxICON_ERROR);
	}
	
	return false;
}

GenerationConfig OTMapGenDialog::BuildGenerationConfig() {
	GenerationConfig config;
	
	// Basic parameters
	config.seed = seed_text_ctrl->GetValue().ToStdString();
	config.width = width_spin_ctrl->GetValue();
	config.height = height_spin_ctrl->GetValue();
	config.version = version_choice->GetStringSelection().ToStdString();
	config.mountain_type = mountain_type_choice->GetStringSelection().ToStdString();
	
	// Boolean flags
	config.terrain_only = terrain_only_checkbox->GetValue();
	config.sand_biome = sand_biome_checkbox->GetValue();
	config.smooth_coastline = smooth_coastline_checkbox->GetValue();
	config.add_caves = add_caves_checkbox->GetValue();
	
	// Advanced parameters (convert slider values to appropriate ranges)
	config.noise_increment = noise_increment_slider->GetValue() / 100.0;
	config.island_distance_decrement = island_distance_slider->GetValue() / 100.0;
	config.cave_depth = cave_depth_slider->GetValue();
	config.cave_roughness = cave_roughness_slider->GetValue() / 100.0;
	config.cave_chance = cave_chance_slider->GetValue() / 100.0;
	config.water_level = water_level_slider->GetValue();
	config.exponent = exponent_slider->GetValue() / 100.0;
	config.linear = linear_slider->GetValue();
	
	return config;
}

void OTMapGenDialog::GetTilePreviewColor(uint16_t tileId, unsigned char& r, unsigned char& g, unsigned char& b) {
	// Convert tile IDs to preview colors
	switch (tileId) {
		case OTMapGenItems::WATER_TILE_ID:
			r = 0; g = 100; b = 255; // Blue for water
			break;
		case OTMapGenItems::GRASS_TILE_ID:
			r = 50; g = 200; b = 50; // Green for grass
			break;
		case OTMapGenItems::SAND_TILE_ID:
			r = 255; g = 255; b = 100; // Yellow for sand
			break;
		case OTMapGenItems::STONE_TILE_ID:
			r = 128; g = 128; b = 128; // Gray for stone
			break;
		case OTMapGenItems::GRAVEL_TILE_ID:
			r = 100; g = 100; b = 100; // Dark gray for gravel
			break;
		case OTMapGenItems::SNOW_TILE_ID:
			r = 255; g = 255; b = 255; // White for snow
			break;
		case OTMapGenItems::MOUNTAIN_TILE_ID:
			r = 139; g = 69; b = 19; // Brown for mountains
			break;
		case OTMapGenItems::SNOW_MOUNTAIN_TILE_ID:
			r = 200; g = 200; b = 255; // Light blue for snow mountains
			break;
		case OTMapGenItems::SAND_MOUNTAIN_TILE_ID:
			r = 205; g = 133; b = 63; // Sandy brown for sand mountains
			break;
		default:
			r = 64; g = 64; b = 64; // Default dark gray
			break;
	}
} 