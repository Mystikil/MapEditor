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
#include "graphics.h"
#include "items.h"
#include "brush.h"
#include "ground_brush.h"

#include <wx/process.h>
#include <wx/stream.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/dir.h>

// OTMapGenItemButton implementation
wxBEGIN_EVENT_TABLE(OTMapGenItemButton, wxButton)
	EVT_PAINT(OTMapGenItemButton::OnPaint)
	EVT_BUTTON(wxID_ANY, OTMapGenItemButton::OnClick)
wxEND_EVENT_TABLE()

OTMapGenItemButton::OTMapGenItemButton(wxWindow* parent, wxWindowID id) :
	wxButton(parent, id, "", wxDefaultPosition, wxSize(32, 32)),
	m_id(0) {
	SetToolTip("Click to set item ID from current brush");
}

void OTMapGenItemButton::SetItemId(uint16_t id) {
	if (m_id == id) {
		return;
	}
	m_id = id;
	Refresh(); // Trigger repaint
}

void OTMapGenItemButton::OnPaint(wxPaintEvent& event) {
	wxPaintDC dc(this);
	
	// Clear background
	dc.SetBackground(*wxLIGHT_GREY_BRUSH);
	dc.Clear();
	
	if (m_id != 0) {
		// Draw item sprite if available
		const ItemType& it = g_items.getItemType(m_id);
		if (it.id != 0) {
			Sprite* sprite = g_gui.gfx.getSprite(it.clientID);
			if (sprite) {
				wxSize size = GetSize();
				sprite->DrawTo(&dc, SPRITE_SIZE_32x32, 0, 0, size.GetWidth(), size.GetHeight());
			}
		}
		
		// Draw item ID text
		dc.SetTextForeground(*wxBLACK);
		dc.DrawText(wxString::Format("%d", m_id), 2, 2);
	} else {
		// Draw placeholder
		dc.SetTextForeground(*wxBLACK);
		dc.DrawText("Click", 2, 8);
		dc.DrawText("Brush", 2, 18);
	}
}

void OTMapGenItemButton::OnClick(wxCommandEvent& event) {
	// Forward the event to parent with our specific ID
	wxCommandEvent newEvent(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
	newEvent.SetEventObject(this);
	GetParent()->GetEventHandler()->ProcessEvent(newEvent);
}

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
	
	// Terrain layer management events
	EVT_LIST_ITEM_SELECTED(ID_TERRAIN_LAYER_LIST, OTMapGenDialog::OnTerrainLayerSelect)
	EVT_BUTTON(ID_ADD_LAYER, OTMapGenDialog::OnTerrainLayerAdd)
	EVT_BUTTON(ID_REMOVE_LAYER, OTMapGenDialog::OnTerrainLayerRemove)
	EVT_BUTTON(ID_MOVE_UP_LAYER, OTMapGenDialog::OnTerrainLayerMoveUp)
	EVT_BUTTON(ID_MOVE_DOWN_LAYER, OTMapGenDialog::OnTerrainLayerMoveDown)
	EVT_BUTTON(ID_EDIT_LAYER, OTMapGenDialog::OnTerrainLayerEdit)
	EVT_CHOICE(ID_LAYER_BRUSH_CHOICE, OTMapGenDialog::OnBrushChoice)
	EVT_COMMAND(ID_LAYER_ITEM_ID_SPIN, wxEVT_COMMAND_SPINCTRL_UPDATED, OTMapGenDialog::OnItemIdChange)
	EVT_CHOICE(ID_CAVE_BRUSH_CHOICE, OTMapGenDialog::OnBrushChoice)
	EVT_COMMAND(ID_CAVE_ITEM_ID_SPIN, wxEVT_COMMAND_SPINCTRL_UPDATED, OTMapGenDialog::OnItemIdChange)
	EVT_CHOICE(ID_WATER_BRUSH_CHOICE, OTMapGenDialog::OnBrushChoice)
	EVT_COMMAND(ID_WATER_ITEM_ID_SPIN, wxEVT_COMMAND_SPINCTRL_UPDATED, OTMapGenDialog::OnItemIdChange)
wxEND_EVENT_TABLE()

OTMapGenDialog::OTMapGenDialog(wxWindow* parent) : 
	wxDialog(parent, wxID_ANY, "Procedural Map Generator", wxDefaultPosition, wxSize(1200, 800), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	current_preview(nullptr),
	current_preview_floor(7), // Start with ground level (floor 7)
	current_zoom(1.0),
	preview_offset_x(0),
	preview_offset_y(0),
	preview_actual_width(256),
	preview_actual_height(256),
	preview_is_scaled(false),
	current_generator_type(GENERATOR_ISLAND) {
	
	// Create main sizer
	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
	
	// Create notebook for tabs
	notebook = new wxNotebook(this, ID_NOTEBOOK_TAB_CHANGED);
	
	// === Main Procedural Generator Tab ===
	wxPanel* main_panel = new wxPanel(notebook);
	wxBoxSizer* main_sizer_tab = new wxBoxSizer(wxHORIZONTAL);
	
	// Left side - All Settings
	wxBoxSizer* left_main_sizer = new wxBoxSizer(wxVERTICAL);
	
	// Basic parameters group (more compact)
	wxStaticBoxSizer* basic_params_sizer = new wxStaticBoxSizer(wxVERTICAL, main_panel, "Basic Parameters");
	wxFlexGridSizer* basic_grid_sizer = new wxFlexGridSizer(3, 4, 5, 5);
	basic_grid_sizer->AddGrowableCol(1);
	basic_grid_sizer->AddGrowableCol(3);
	
	// Row 1: Seed and Width
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Seed:"), 0, wxALIGN_CENTER_VERTICAL);
	main_seed_text_ctrl = new wxTextCtrl(main_panel, ID_SEED_TEXT, wxString::Format("%lld", (long long)time(nullptr) * 1000));
	main_seed_text_ctrl->SetToolTip("Enter any integer value (supports 64-bit seeds)");
	basic_grid_sizer->Add(main_seed_text_ctrl, 1, wxEXPAND);
	
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Width:"), 0, wxALIGN_CENTER_VERTICAL);
	main_width_spin_ctrl = new wxSpinCtrl(main_panel, ID_WIDTH_SPIN, "256", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 64, 8192, 256);
	basic_grid_sizer->Add(main_width_spin_ctrl, 1, wxEXPAND);
	
	// Row 2: Version and Height
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Version:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString versions;
	versions.Add("10.98");
	versions.Add("11.00");
	versions.Add("12.00");
	main_version_choice = new wxChoice(main_panel, ID_VERSION_CHOICE, wxDefaultPosition, wxDefaultSize, versions);
	main_version_choice->SetSelection(0);
	basic_grid_sizer->Add(main_version_choice, 1, wxEXPAND);
	
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Height:"), 0, wxALIGN_CENTER_VERTICAL);
	main_height_spin_ctrl = new wxSpinCtrl(main_panel, ID_HEIGHT_SPIN, "256", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 64, 8192, 256);
	basic_grid_sizer->Add(main_height_spin_ctrl, 1, wxEXPAND);
	
	// Row 3: Mountain Type and Water Level
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Mountain Type:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString mountain_types;
	mountain_types.Add("MOUNTAIN");
	mountain_types.Add("SNOW");
	mountain_types.Add("SAND");
	mountain_type_choice = new wxChoice(main_panel, ID_MOUNTAIN_TYPE_CHOICE, wxDefaultPosition, wxDefaultSize, mountain_types);
	mountain_type_choice->SetSelection(0);
	basic_grid_sizer->Add(mountain_type_choice, 1, wxEXPAND);
	
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Floors:"), 0, wxALIGN_CENTER_VERTICAL);
	floors_to_generate_spin = new wxSpinCtrl(main_panel, wxID_ANY, "8", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 15, 8);
	floors_to_generate_spin->SetToolTip("Number of floors to generate (1-15)");
	basic_grid_sizer->Add(floors_to_generate_spin, 1, wxEXPAND);
	
	// Row 4: Water Level (moved down)
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Water Level:"), 0, wxALIGN_CENTER_VERTICAL);
	water_level_text = new wxTextCtrl(main_panel, ID_WATER_LEVEL_TEXT, "7");
	water_level_text->SetToolTip("Tibia Z-coordinate (0-15, 7 = ground level)");
	basic_grid_sizer->Add(water_level_text, 1, wxEXPAND);
	
	// Add spacers for remaining cells in the 4x4 grid
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, ""), 0);
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, ""), 0);
	
	basic_params_sizer->Add(basic_grid_sizer, 0, wxEXPAND | wxALL, 5);
	
	// Checkboxes in horizontal layout
	wxBoxSizer* checkbox_sizer = new wxBoxSizer(wxHORIZONTAL);
	terrain_only_checkbox = new wxCheckBox(main_panel, wxID_ANY, "Terrain Only");
	sand_biome_checkbox = new wxCheckBox(main_panel, wxID_ANY, "Sand Biome");
	sand_biome_checkbox->SetValue(true);
	smooth_coastline_checkbox = new wxCheckBox(main_panel, wxID_ANY, "Smooth Coastlines");
	smooth_coastline_checkbox->SetValue(true);
	add_caves_checkbox = new wxCheckBox(main_panel, wxID_ANY, "Underground Caves");
	add_caves_checkbox->SetValue(true);
	
	checkbox_sizer->Add(terrain_only_checkbox, 0, wxALL, 5);
	checkbox_sizer->Add(sand_biome_checkbox, 0, wxALL, 5);
	checkbox_sizer->Add(smooth_coastline_checkbox, 0, wxALL, 5);
	checkbox_sizer->Add(add_caves_checkbox, 0, wxALL, 5);
	
	basic_params_sizer->Add(checkbox_sizer, 0, wxEXPAND | wxALL, 5);
	left_main_sizer->Add(basic_params_sizer, 0, wxEXPAND | wxALL, 5);
	
	// Advanced parameters group (more compact)
	wxStaticBoxSizer* advanced_params_sizer = new wxStaticBoxSizer(wxVERTICAL, main_panel, "Noise & Generation Parameters");
	wxFlexGridSizer* advanced_grid_sizer = new wxFlexGridSizer(4, 4, 5, 5);
	advanced_grid_sizer->AddGrowableCol(1);
	advanced_grid_sizer->AddGrowableCol(3);
	
	// Row 1: Noise Increment and Island Distance
	advanced_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Noise Increment:"), 0, wxALIGN_CENTER_VERTICAL);
	noise_increment_text = new wxTextCtrl(main_panel, ID_NOISE_INCREMENT_TEXT, "1.0");
	noise_increment_text->SetToolTip("Range: 0.001 - 100.0 (higher = more detail)");
	advanced_grid_sizer->Add(noise_increment_text, 1, wxEXPAND);
	
	advanced_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Island Distance:"), 0, wxALIGN_CENTER_VERTICAL);
	island_distance_text = new wxTextCtrl(main_panel, ID_ISLAND_DISTANCE_TEXT, "0.92");
	island_distance_text->SetToolTip("Range: 0.001 - 100.0 (lower = more island effect)");
	advanced_grid_sizer->Add(island_distance_text, 1, wxEXPAND);
	
	// Row 2: Exponent and Linear
	advanced_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Exponent:"), 0, wxALIGN_CENTER_VERTICAL);
	exponent_text = new wxTextCtrl(main_panel, ID_EXPONENT_TEXT, "1.4");
	exponent_text->SetToolTip("Range: 0.001 - 100.0 (height curve shaping)");
	advanced_grid_sizer->Add(exponent_text, 1, wxEXPAND);
	
	advanced_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Linear:"), 0, wxALIGN_CENTER_VERTICAL);
	linear_text = new wxTextCtrl(main_panel, ID_LINEAR_TEXT, "6.0");
	linear_text->SetToolTip("Range: 0.001 - 100.0 (height multiplier)");
	advanced_grid_sizer->Add(linear_text, 1, wxEXPAND);
	
	// Row 3: Cave Depth and Cave Roughness
	advanced_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Cave Depth:"), 0, wxALIGN_CENTER_VERTICAL);
	cave_depth_text = new wxTextCtrl(main_panel, ID_CAVE_DEPTH_TEXT, "20");
	cave_depth_text->SetToolTip("Range: 1 - 100 (number of underground floors)");
	advanced_grid_sizer->Add(cave_depth_text, 1, wxEXPAND);
	
	advanced_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Cave Roughness:"), 0, wxALIGN_CENTER_VERTICAL);
	cave_roughness_text = new wxTextCtrl(main_panel, ID_CAVE_ROUGHNESS_TEXT, "0.45");
	cave_roughness_text->SetToolTip("Range: 0.001 - 100.0 (noise scale for caves)");
	advanced_grid_sizer->Add(cave_roughness_text, 1, wxEXPAND);
	
	// Row 4: Cave Chance (spanning two columns)
	advanced_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Cave Chance:"), 0, wxALIGN_CENTER_VERTICAL);
	cave_chance_text = new wxTextCtrl(main_panel, ID_CAVE_CHANCE_TEXT, "0.09");
	cave_chance_text->SetToolTip("Range: 0.001 - 1.0 (probability of cave generation)");
	advanced_grid_sizer->Add(cave_chance_text, 1, wxEXPAND);
	
	// Add spacers for the remaining cells
	advanced_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, ""), 0);
	advanced_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, ""), 0);
	
	advanced_params_sizer->Add(advanced_grid_sizer, 0, wxEXPAND | wxALL, 5);
	left_main_sizer->Add(advanced_params_sizer, 0, wxEXPAND | wxALL, 5);
	
	// Right side - Large Preview section
	wxBoxSizer* right_main_sizer = new wxBoxSizer(wxVERTICAL);
	
	wxStaticBoxSizer* main_preview_sizer = new wxStaticBoxSizer(wxVERTICAL, main_panel, "Map Preview");
	main_preview_bitmap = new wxStaticBitmap(main_panel, wxID_ANY, wxBitmap(400, 400));
	main_preview_bitmap->SetBackgroundColour(*wxBLACK);
	main_preview_bitmap->SetMinSize(wxSize(400, 400));
	main_preview_sizer->Add(main_preview_bitmap, 1, wxEXPAND | wxALL, 5);
	
	// Floor navigation
	wxBoxSizer* floor_nav_sizer = new wxBoxSizer(wxHORIZONTAL);
	floor_down_button = new wxButton(main_panel, ID_FLOOR_DOWN, "Floor -");
	floor_label = new wxStaticText(main_panel, wxID_ANY, "Floor: 7 (Ground)");
	floor_up_button = new wxButton(main_panel, ID_FLOOR_UP, "Floor +");
	
	floor_nav_sizer->Add(floor_down_button, 0, wxALL, 2);
	floor_nav_sizer->AddStretchSpacer();
	floor_nav_sizer->Add(floor_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
	floor_nav_sizer->AddStretchSpacer();
	floor_nav_sizer->Add(floor_up_button, 0, wxALL, 2);
	
	main_preview_sizer->Add(floor_nav_sizer, 0, wxEXPAND | wxALL, 5);
	
	// Zoom navigation
	wxBoxSizer* zoom_nav_sizer = new wxBoxSizer(wxHORIZONTAL);
	zoom_out_button = new wxButton(main_panel, ID_ZOOM_OUT, "Zoom -");
	zoom_label = new wxStaticText(main_panel, wxID_ANY, "Zoom: 100%");
	zoom_in_button = new wxButton(main_panel, ID_ZOOM_IN, "Zoom +");
	
	zoom_nav_sizer->Add(zoom_out_button, 0, wxALL, 2);
	zoom_nav_sizer->AddStretchSpacer();
	zoom_nav_sizer->Add(zoom_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
	zoom_nav_sizer->AddStretchSpacer();
	zoom_nav_sizer->Add(zoom_in_button, 0, wxALL, 2);
	
	main_preview_sizer->Add(zoom_nav_sizer, 0, wxEXPAND | wxALL, 5);
	
	// Preview buttons
	wxBoxSizer* preview_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
	main_preview_button = new wxButton(main_panel, ID_PREVIEW, "Generate Preview");
	wxButton* refresh_preview_button = new wxButton(main_panel, ID_PREVIEW, "Refresh");
	
	preview_buttons_sizer->Add(main_preview_button, 1, wxEXPAND | wxALL, 2);
	preview_buttons_sizer->Add(refresh_preview_button, 0, wxALL, 2);
	
	main_preview_sizer->Add(preview_buttons_sizer, 0, wxEXPAND | wxALL, 5);
	
	right_main_sizer->Add(main_preview_sizer, 1, wxEXPAND | wxALL, 5);
	
	// Add left and right sides to main tab
	main_sizer_tab->Add(left_main_sizer, 0, wxEXPAND | wxALL, 5);
	main_sizer_tab->Add(right_main_sizer, 1, wxEXPAND | wxALL, 5);
	
	main_panel->SetSizer(main_sizer_tab);
	notebook->AddPage(main_panel, "Procedural Map Generation", true);
	
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
	
	// Initialize brush choices
	PopulateBrushChoices();
	
	// Initialize default terrain layers
	GenerationConfig defaultConfig;
	defaultConfig.initializeDefaultLayers();
	working_terrain_layers = defaultConfig.terrain_layers;
	
	// Generate initial random 64-bit seed
	srand(time(nullptr));
	long long initial_seed = ((long long)rand() << 32) | rand();
	main_seed_text_ctrl->SetValue(wxString::Format("%lld", initial_seed));
	
	// Initialize shared control pointers for procedural generation
	seed_text_ctrl = main_seed_text_ctrl;
	width_spin_ctrl = main_width_spin_ctrl;
	height_spin_ctrl = main_height_spin_ctrl;
	version_choice = main_version_choice;
	
	// Set current generator type to procedural
	current_generator_type = GENERATOR_CUSTOM;
}

OTMapGenDialog::~OTMapGenDialog() {
	if (current_preview) {
		delete current_preview;
	}
}

void OTMapGenDialog::OnGenerate(wxCommandEvent& event) {
	// Generate procedural map
	bool success = GenerateMap();
	
	if (success) {
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
	// Set the preview button state
	main_preview_button->SetLabel("Generating...");
	main_preview_button->Enable(false);
	
	try {
		// Generate procedural preview
		GenerationConfig config = BuildGenerationConfig();
		
		// OPTIMIZATION: For large maps, generate smaller preview
		bool use_fast_preview = false;
		if (config.width > 1024 || config.height > 1024) {
			use_fast_preview = true;
			double scale = std::min(512.0 / config.width, 512.0 / config.height);
			config.width = std::max(64, (int)(config.width * scale));
			config.height = std::max(64, (int)(config.height * scale));
			
			main_preview_button->SetLabel(wxString::Format("Generating fast preview (%dx%d)...", config.width, config.height));
		}
		
		// Generate preview using C++ implementation
		OTMapGenerator generator;
		current_layers = generator.generateLayers(config);
		
		// Store the actual preview dimensions
		preview_actual_width = config.width;
		preview_actual_height = config.height;
		preview_is_scaled = use_fast_preview;
		
		// Update the preview for the current floor
		UpdatePreviewFloor();
		
	} catch (const std::exception& e) {
		wxMessageBox(wxString::Format("Failed to generate preview: %s", e.what()), "Preview Error", wxOK | wxICON_ERROR);
	}
	
	// Reset button state
	if (preview_is_scaled) {
		main_preview_button->SetLabel("Generate Fast Preview");
	} else {
		main_preview_button->SetLabel("Generate Preview");
	}
	main_preview_button->Enable(true);
}

void OTMapGenDialog::UpdatePreviewFloor() {
	if (current_layers.empty()) {
		return; // No data to preview
	}
	
	// Use the main preview bitmap for procedural generation
	wxStaticBitmap* target_preview = main_preview_bitmap;
	
	if (!target_preview) {
		return; // No valid preview target
	}
	
	try {
		// Use cached preview dimensions for performance (avoid repeated function calls in loops)
		int current_width = preview_actual_width;
		int current_height = preview_actual_height;
		
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
			int preview_width = 400;  // Updated to match the larger preview
			int preview_height = 400; // Updated to match the larger preview
			wxImage preview_image(preview_width, preview_height);
			
			// Calculate zoom-adjusted scale and center point
			double base_scale_x = (double)current_width / preview_width;
			double base_scale_y = (double)current_height / preview_height;
			
			// Apply zoom - higher zoom means smaller scale (show less area)
			double scale_x = base_scale_x / current_zoom;
			double scale_y = base_scale_y / current_zoom;
			
			// Calculate center offset for zoomed view
			int center_x = current_width / 2 + preview_offset_x;
			int center_y = current_height / 2 + preview_offset_y;
			
			for (int y = 0; y < preview_height; ++y) {
				for (int x = 0; x < preview_width; ++x) {
					// Calculate source coordinates with zoom and offset
					int src_x = center_x + (int)((x - preview_width/2) * scale_x);
					int src_y = center_y + (int)((y - preview_height/2) * scale_y);
					
					// Set default color (black for out of bounds)
					unsigned char r = 0, g = 0, b = 0;
					
					if (src_x >= 0 && src_x < current_width && src_y >= 0 && src_y < current_height) {
						// Calculate index in the 1D layer
						int tileIndex = src_y * current_width + src_x;
						
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
			target_preview->SetBitmap(*current_preview);
			target_preview->Refresh();
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
					if (tileId == 4526 && dist(decoration_rng) < 0.03) { // Use grass item ID directly
						Position pos(x, y, 7); // Surface level in Tibia coordinates
						Tile* tile = tempMap.getTile(pos);
						if (tile) {
							// Choose random decoration
							uint16_t decorationId;
							double rand_val = dist(decoration_rng);
							if (rand_val < 0.6) {
								decorationId = 2700; // Tree ID
							} else if (rand_val < 0.8) {
								decorationId = 2785; // Bush ID
							} else {
								decorationId = 2782; // Flower ID
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
	
	// Basic parameters using helper functions
	config.seed = GetCurrentSeed();
	config.width = GetCurrentWidth();
	config.height = GetCurrentHeight();
	config.floors_to_generate = floors_to_generate_spin->GetValue();
	config.version = GetCurrentVersion();
	config.mountain_type = mountain_type_choice->GetStringSelection().ToStdString();
	
	// Boolean flags
	config.terrain_only = terrain_only_checkbox->GetValue();
	config.sand_biome = sand_biome_checkbox->GetValue();
	config.smooth_coastline = smooth_coastline_checkbox->GetValue();
	config.add_caves = add_caves_checkbox->GetValue();
	
	// Advanced parameters - parse text inputs with enhanced validation and ranges
	double noise_increment = 1.0;
	double island_distance = 0.92;
	double cave_depth = 20.0;
	double cave_roughness = 0.45;
	double cave_chance = 0.09;
	double water_level = 7.0;
	double exponent = 1.4;
	double linear = 6.0;
	
	if (!noise_increment_text->GetValue().ToDouble(&noise_increment)) noise_increment = 1.0;
	if (!island_distance_text->GetValue().ToDouble(&island_distance)) island_distance = 0.92;
	if (!cave_depth_text->GetValue().ToDouble(&cave_depth)) cave_depth = 20.0;
	if (!cave_roughness_text->GetValue().ToDouble(&cave_roughness)) cave_roughness = 0.45;
	if (!cave_chance_text->GetValue().ToDouble(&cave_chance)) cave_chance = 0.09;
	if (!water_level_text->GetValue().ToDouble(&water_level)) water_level = 7.0;
	if (!exponent_text->GetValue().ToDouble(&exponent)) exponent = 1.4;
	if (!linear_text->GetValue().ToDouble(&linear)) linear = 6.0;
	
	// Apply enhanced bounds checking with expanded ranges
	noise_increment = std::max(0.001, std::min(100.0, noise_increment));
	island_distance = std::max(0.001, std::min(100.0, island_distance));
	cave_depth = std::max(1.0, std::min(100.0, cave_depth));
	cave_roughness = std::max(0.001, std::min(100.0, cave_roughness));
	cave_chance = std::max(0.001, std::min(1.0, cave_chance));
	water_level = std::max(0.0, std::min(15.0, water_level));
	exponent = std::max(0.001, std::min(100.0, exponent));
	linear = std::max(0.001, std::min(100.0, linear));
	
	config.noise_increment = noise_increment;
	config.island_distance_decrement = island_distance;
	config.cave_depth = (int)cave_depth;
	config.cave_roughness = cave_roughness;
	config.cave_chance = cave_chance;
	config.water_level = (int)water_level;
	config.exponent = exponent;
	config.linear = linear;
	
	// Set both water_level and base_floor for compatibility
	config.water_level = (int)water_level;
	config.base_floor = (int)water_level;
	
	// Add some default frequency settings for better island generation
	config.frequencies.clear();
	config.frequencies.push_back({1.0, 1.0});    // Base frequency
	config.frequencies.push_back({2.0, 0.5});    // Higher frequency, lower weight
	config.frequencies.push_back({4.0, 0.25});   // Even higher, even lower weight
	config.frequencies.push_back({8.0, 0.125});  // Finest detail
	
	// Enhanced island generation parameters
	config.euclidean = true; // Use euclidean distance for smoother island shapes
	config.island_distance_exponent = 2.0; // Smooth falloff
	
	// Use the working terrain layers from the layout design tab
	config.terrain_layers = working_terrain_layers;
	
	// Update the sand biome layer's enabled state based on checkbox
	for (auto& layer : config.terrain_layers) {
		if (layer.name == "Sand") {
			layer.enabled = config.sand_biome;
		}
	}
	
	// Cave configuration
	int caveSelection = cave_brush_choice->GetSelection();
	if (caveSelection >= 0 && caveSelection < static_cast<int>(available_brushes.size())) {
		config.cave_brush_name = available_brushes[caveSelection];
	}
	config.cave_item_id = cave_item_id_spin->GetValue();
	
	// Water configuration  
	int waterSelection = water_brush_choice->GetSelection();
	if (waterSelection >= 0 && waterSelection < static_cast<int>(available_brushes.size())) {
		config.water_brush_name = available_brushes[waterSelection];
	}
	config.water_item_id = water_item_id_spin->GetValue();
	
	return config;
}

// Terrain layer management event handlers
void OTMapGenDialog::OnTerrainLayerSelect(wxListEvent& event) {
	UpdateLayerControls();
}

void OTMapGenDialog::OnTerrainLayerAdd(wxCommandEvent& event) {
	TerrainLayer newLayer;
	newLayer.name = "New Layer";
	newLayer.brush_name = "grass";
	newLayer.item_id = 4526;
	newLayer.height_min = 0.0;
	newLayer.height_max = 1.0;
	newLayer.moisture_min = -1.0;
	newLayer.moisture_max = 1.0;
	newLayer.noise_scale = 1.0;
	newLayer.coverage = 1.0;
	newLayer.use_borders = true;
	newLayer.enabled = true;
	newLayer.z_order = 1000;
	
	working_terrain_layers.push_back(newLayer);
	PopulateTerrainLayerList();
	
	// Select the new layer
	terrain_layer_list->SetItemState(working_terrain_layers.size() - 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	UpdateLayerControls();
}

void OTMapGenDialog::OnTerrainLayerRemove(wxCommandEvent& event) {
	long selected = terrain_layer_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (selected >= 0 && selected < static_cast<long>(working_terrain_layers.size())) {
		working_terrain_layers.erase(working_terrain_layers.begin() + selected);
		PopulateTerrainLayerList();
		UpdateLayerControls();
	}
}

void OTMapGenDialog::OnTerrainLayerMoveUp(wxCommandEvent& event) {
	long selected = terrain_layer_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (selected > 0 && selected < static_cast<long>(working_terrain_layers.size())) {
		std::swap(working_terrain_layers[selected], working_terrain_layers[selected - 1]);
		PopulateTerrainLayerList();
		terrain_layer_list->SetItemState(selected - 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
		UpdateLayerControls();
	}
}

void OTMapGenDialog::OnTerrainLayerMoveDown(wxCommandEvent& event) {
	long selected = terrain_layer_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (selected >= 0 && selected < static_cast<long>(working_terrain_layers.size()) - 1) {
		std::swap(working_terrain_layers[selected], working_terrain_layers[selected + 1]);
		PopulateTerrainLayerList();
		terrain_layer_list->SetItemState(selected + 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
		UpdateLayerControls();
	}
}

void OTMapGenDialog::OnTerrainLayerEdit(wxCommandEvent& event) {
	// Save current layer properties back to the selected layer
	TerrainLayer* layer = GetSelectedLayer();
	if (layer) {
		layer->name = layer_name_text->GetValue().ToStdString();
		
		int brushSelection = layer_brush_choice->GetSelection();
		if (brushSelection >= 0 && brushSelection < static_cast<int>(available_brushes.size())) {
			layer->brush_name = available_brushes[brushSelection];
		}
		
		layer->item_id = layer_item_id_spin->GetValue();
		
		double height_min, height_max, moisture_min, moisture_max, noise_scale, coverage;
		if (height_min_text->GetValue().ToDouble(&height_min)) layer->height_min = height_min;
		if (height_max_text->GetValue().ToDouble(&height_max)) layer->height_max = height_max;
		if (moisture_min_text->GetValue().ToDouble(&moisture_min)) layer->moisture_min = moisture_min;
		if (moisture_max_text->GetValue().ToDouble(&moisture_max)) layer->moisture_max = moisture_max;
		if (noise_scale_text->GetValue().ToDouble(&noise_scale)) layer->noise_scale = noise_scale;
		if (coverage_text->GetValue().ToDouble(&coverage)) layer->coverage = coverage;
		
		layer->use_borders = use_borders_checkbox->GetValue();
		layer->enabled = layer_enabled_checkbox->GetValue();
		layer->z_order = z_order_spin->GetValue();
		
		PopulateTerrainLayerList();
	}
}

void OTMapGenDialog::OnBrushChoice(wxCommandEvent& event) {
	// Auto-update item ID when brush is changed
	wxChoice* choice = dynamic_cast<wxChoice*>(event.GetEventObject());
	if (choice) {
		int selection = choice->GetSelection();
		if (selection >= 0 && selection < static_cast<int>(available_brushes.size())) {
			wxString brushName = available_brushes[selection];
			
			// Try to get the primary item ID from the brush (placeholder - needs implementation)
			// For now, use some default mappings based on brush name
			uint16_t itemId = 100; // Default
			if (brushName == "grass") itemId = 4526;
			else if (brushName == "sea") itemId = 4608;
			else if (brushName == "sand") itemId = 231;
			else if (brushName == "mountain") itemId = 919;
			else if (brushName == "cave") itemId = 351;
			else if (brushName == "snow") itemId = 670;
			else if (brushName == "stone") itemId = 1284;
			
			if (choice->GetId() == ID_LAYER_BRUSH_CHOICE) {
				layer_item_id_spin->SetValue(itemId);
			} else if (choice->GetId() == ID_CAVE_BRUSH_CHOICE) {
				cave_item_id_spin->SetValue(itemId);
			} else if (choice->GetId() == ID_WATER_BRUSH_CHOICE) {
				water_item_id_spin->SetValue(itemId);
			}
		}
	}
}

void OTMapGenDialog::OnItemIdChange(wxCommandEvent& event) {
	// Item ID changed - could trigger preview update
}

// Helper functions for terrain layer management
void OTMapGenDialog::PopulateTerrainLayerList() {
	terrain_layer_list->DeleteAllItems();
	
	for (size_t i = 0; i < working_terrain_layers.size(); ++i) {
		const TerrainLayer& layer = working_terrain_layers[i];
		
		long index = terrain_layer_list->InsertItem(i, layer.name);
		terrain_layer_list->SetItem(index, 1, layer.brush_name);
		terrain_layer_list->SetItem(index, 2, wxString::Format("%d", layer.item_id));
		terrain_layer_list->SetItem(index, 3, wxString::Format("%.1f-%.1f", layer.height_min, layer.height_max));
		terrain_layer_list->SetItem(index, 4, layer.enabled ? "Yes" : "No");
	}
}

void OTMapGenDialog::PopulateBrushChoices() {
	// Populate with common brush names from grounds.xml
	// This should be replaced with actual brush parsing from the XML files
	available_brushes.clear();
	available_brushes.push_back("grass");
	available_brushes.push_back("sea");
	available_brushes.push_back("sand");
	available_brushes.push_back("mountain");
	available_brushes.push_back("cave");
	available_brushes.push_back("snow");
	available_brushes.push_back("stone floor");
	available_brushes.push_back("wooden floor");
	available_brushes.push_back("lawn");
	available_brushes.push_back("ice");
	
	// Populate all brush choice controls
	layer_brush_choice->Clear();
	cave_brush_choice->Clear();
	water_brush_choice->Clear();
	
	for (const std::string& brush : available_brushes) {
		layer_brush_choice->Append(brush);
		cave_brush_choice->Append(brush);
		water_brush_choice->Append(brush);
	}
	
	// Set default selections
	cave_brush_choice->SetStringSelection("cave");
	water_brush_choice->SetStringSelection("sea");
}

void OTMapGenDialog::UpdateLayerControls() {
	TerrainLayer* layer = GetSelectedLayer();
	bool hasSelection = (layer != nullptr);
	
	// Enable/disable controls based on selection
	layer_properties_panel->Enable(hasSelection);
	remove_layer_button->Enable(hasSelection);
	edit_layer_button->Enable(hasSelection);
	
	if (hasSelection) {
		// Populate controls with layer data
		layer_name_text->SetValue(layer->name);
		layer_brush_choice->SetStringSelection(layer->brush_name);
		layer_item_id_spin->SetValue(layer->item_id);
		height_min_text->SetValue(wxString::Format("%.3f", layer->height_min));
		height_max_text->SetValue(wxString::Format("%.3f", layer->height_max));
		moisture_min_text->SetValue(wxString::Format("%.3f", layer->moisture_min));
		moisture_max_text->SetValue(wxString::Format("%.3f", layer->moisture_max));
		noise_scale_text->SetValue(wxString::Format("%.3f", layer->noise_scale));
		coverage_text->SetValue(wxString::Format("%.3f", layer->coverage));
		use_borders_checkbox->SetValue(layer->use_borders);
		layer_enabled_checkbox->SetValue(layer->enabled);
		z_order_spin->SetValue(layer->z_order);
	}
	
	move_up_button->Enable(hasSelection && terrain_layer_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) > 0);
	move_down_button->Enable(hasSelection && terrain_layer_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) < static_cast<long>(working_terrain_layers.size()) - 1);
}

TerrainLayer* OTMapGenDialog::GetSelectedLayer() {
	long selected = terrain_layer_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (selected >= 0 && selected < static_cast<long>(working_terrain_layers.size())) {
		return &working_terrain_layers[selected];
	}
	return nullptr;
}

void OTMapGenDialog::SetSelectedLayer(const TerrainLayer& layer) {
	TerrainLayer* selectedLayer = GetSelectedLayer();
	if (selectedLayer) {
		*selectedLayer = layer;
		PopulateTerrainLayerList();
	}
}

void OTMapGenDialog::GetTilePreviewColor(uint16_t tileId, unsigned char& r, unsigned char& g, unsigned char& b) {
	// Define color mapping for different tile types
	switch (tileId) {
		// Water tiles (blue shades)
		case 4608: case 4609: case 4610: case 4611:
			r = 50; g = 100; b = 200;
			break;
		
		// Grass tiles (green shades)
		case 4526:
			r = 50; g = 150; b = 50;
			break;
		
		// Sand tiles (yellow/brown shades)
		case 231: case 232: case 233:
			r = 200; g = 180; b = 100;
			break;
		
		// Mountain/stone tiles (gray shades)
		case 919: case 920: case 921:
			r = 120; g = 120; b = 120;
			break;
		
		// Cave tiles (dark gray)
		case 351: case 352:
			r = 60; g = 60; b = 60;
			break;
		
		// Snow tiles (white)
		case 670: case 671:
			r = 240; g = 240; b = 250;
			break;
		
		// Default color for unknown tiles (magenta for debugging)
		default:
			r = 255; g = 0; b = 255;
			break;
		}
	}
	
// Island Generator Event Handlers
void OTMapGenDialog::OnIslandParameterChange(wxCommandEvent& event) {
	// Auto-update preview when island parameters change (optional)
	// Could trigger UpdatePreview() here if desired
}

void OTMapGenDialog::OnIslandParameterSpin(wxSpinEvent& event) {
	// Auto-update preview when island spin parameters change (optional)
	// Could trigger UpdatePreview() here if desired
}

void OTMapGenDialog::OnRollDice(wxCommandEvent& event) {
	RollIslandDice();
	// Don't call UpdateIslandControlsFromConfig() here - it overwrites the random values!
}

void OTMapGenDialog::OnTabChanged(wxBookCtrlEvent& event) {
	int selection = event.GetSelection();
	if (selection == 0) {
		current_generator_type = GENERATOR_ISLAND;
		// Disable floor controls for island generator (always floor 7)
		floor_up_button->Enable(false);
		floor_down_button->Enable(false);
		current_preview_floor = 7;
		UpdateFloorLabel();
		
		// Switch shared control pointers to island tab controls
		seed_text_ctrl = island_seed_text_ctrl;
		width_spin_ctrl = island_width_spin_ctrl;
		height_spin_ctrl = island_height_spin_ctrl;
		version_choice = island_version_choice;
	} else if (selection == 2) { // Dungeon tab (before Layout Design tab)
		current_generator_type = GENERATOR_DUNGEON;
		// Disable floor controls for dungeon generator (always floor 7)
		floor_up_button->Enable(false);
		floor_down_button->Enable(false);
		current_preview_floor = 7;
		UpdateFloorLabel();
		
		// Switch shared control pointers to dungeon tab controls
		seed_text_ctrl = dungeon_seed_text_ctrl;
		width_spin_ctrl = dungeon_width_spin_ctrl;
		height_spin_ctrl = dungeon_height_spin_ctrl;
		version_choice = dungeon_version_choice;
	} else {
		current_generator_type = GENERATOR_CUSTOM;
		// Enable floor controls for other generators
		floor_up_button->Enable(true);
		floor_down_button->Enable(true);
		
		// Switch shared control pointers to main tab controls
		seed_text_ctrl = main_seed_text_ctrl;
		width_spin_ctrl = main_width_spin_ctrl;
		height_spin_ctrl = main_height_spin_ctrl;
		version_choice = main_version_choice;
	}
}

bool OTMapGenDialog::GenerateIslandMap() {
	try {
		// Get island configuration
		IslandConfig config = BuildIslandConfig();
		int width = GetCurrentWidth();
		int height = GetCurrentHeight();
		std::string seed = GetCurrentSeed();
		
		// Check if we need batched processing for large maps
		if (width > 2048 || height > 2048) {
			return GenerateIslandMapBatched(config, width, height, seed);
		}
		
		wxProgressDialog progress("Generating Island Map", "Please wait while the island is being generated...", 100, this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT);
		progress.Pulse();
		
		// Generate island layer using C++ implementation
		OTMapGenerator generator;
		auto islandLayer = generator.generateIslandLayer(config, width, height, seed);
		
		if (islandLayer.empty()) {
				wxMessageBox("Failed to generate island data.", "Generation Error", wxOK | wxICON_ERROR);
				return false;
		}
		
		// Create a temporary map to hold our generated content
		Map tempMap;
		tempMap.setWidth(width);
		tempMap.setHeight(height);
		tempMap.setName("Generated Island");
		tempMap.setMapDescription("Procedurally generated island");
		
		// Place generated tiles into the temporary map (only floor 7 for islands)
		progress.SetLabel("Placing island tiles...");
		progress.Pulse();
		
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				uint16_t tileId = islandLayer[y][x];
				if (tileId != 0) {
					Position pos(x, y, 7); // Islands are always on floor 7
					
					// Create tile in temp map
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
		
		// Create temporary OTBM file
		wxString tempDir = wxStandardPaths::Get().GetTempDir();
		wxString tempFileName = wxString::Format("generated_island_%ld.otbm", wxGetLocalTime());
		wxString tempFilePath = tempDir + wxFileName::GetPathSeparator() + tempFileName;
		
		// Save the temporary map as OTBM
		progress.SetLabel("Saving temporary map file...");
		
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
		
		progress.SetLabel("Loading generated island...");
		progress.Pulse();
		
		// Load the temporary file into the editor
		bool loadSuccess = false;
		try {
			loadSuccess = g_gui.LoadMap(wxFileName(tempFilePath));
		} catch (...) {
			loadSuccess = false;
		}
		
		// Clean up the temporary file
		if (wxFileExists(tempFilePath)) {
			wxRemoveFile(tempFilePath);
		}
		
		if (loadSuccess) {
			wxMessageBox("Island generated and loaded successfully!", "Success", wxOK | wxICON_INFORMATION);
			return true;
		} else {
			wxMessageBox("Failed to load the generated island.", "Load Error", wxOK | wxICON_ERROR);
			return false;
		}
		
	} catch (const std::exception& e) {
		wxMessageBox(wxString::Format("Island generation failed with error: %s", e.what()), "Generation Error", wxOK | wxICON_ERROR);
	}
	
	return false;
}

bool OTMapGenDialog::GenerateIslandMapBatched(const IslandConfig& config, int width, int height, const std::string& seed) {
	try {
		const int BATCH_SIZE = 1024; // Process in 1024x1024 chunks
		int batchesX = (width + BATCH_SIZE - 1) / BATCH_SIZE;
		int batchesY = (height + BATCH_SIZE - 1) / BATCH_SIZE;
		int totalBatches = batchesX * batchesY;
		
		wxProgressDialog progress("Generating Large Island Map", 
			wxString::Format("Processing large map (%dx%d) in %d batches...", width, height, totalBatches), 
			totalBatches, this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT);
		
		// Create a temporary map to hold our generated content
		Map tempMap;
		tempMap.setWidth(width);
		tempMap.setHeight(height);
		tempMap.setName(wxString::Format("Generated Large Island (%dx%d)", width, height).ToStdString());
		tempMap.setMapDescription("Procedurally generated large island map (Floor 7 only)");
		
		// Generate island in batches
		OTMapGenerator generator;
		int currentBatch = 0;
		
		for (int batchY = 0; batchY < batchesY; ++batchY) {
			for (int batchX = 0; batchX < batchesX; ++batchX) {
				currentBatch++;
				
				// Calculate batch boundaries
				int startX = batchX * BATCH_SIZE;
				int startY = batchY * BATCH_SIZE;
				int endX = std::min(startX + BATCH_SIZE, width);
				int endY = std::min(startY + BATCH_SIZE, height);
				int batchWidth = endX - startX;
				int batchHeight = endY - startY;
				
				// Update progress
				if (!progress.Update(currentBatch, wxString::Format("Processing batch %d/%d (region %d,%d to %d,%d)", 
					currentBatch, totalBatches, startX, startY, endX-1, endY-1))) {
					// User cancelled
					wxMessageBox("Island generation cancelled by user.", "Generation Cancelled", wxOK | wxICON_INFORMATION);
					return false;
				}
				
				// Generate island data for this batch with offset
				IslandConfig batchConfig = config;
				auto batchLayer = generator.generateIslandLayerBatch(batchConfig, batchWidth, batchHeight, seed, startX, startY, width, height);
				
				// Apply batch tiles to the map
				for (int y = 0; y < batchHeight; ++y) {
					for (int x = 0; x < batchWidth; ++x) {
						uint16_t tileId = batchLayer[y][x];
						if (tileId != 0) {
							Position pos(startX + x, startY + y, 7); // Floor 7 only
							
							// Create tile location and allocate tile
							TileLocation* location = tempMap.createTileL(pos);
							Tile* tile = tempMap.allocator(location);
							
							if (tile) {
								// Create ground item
								Item* groundItem = Item::Create(tileId);
								if (groundItem) {
									tile->ground = groundItem;
								}
								
								// Set the tile in the map
								tempMap.setTile(pos, tile);
							}
						}
					}
				}
			}
		}
		
		// Save and import the map
		progress.SetLabel("Saving large island map...");
		
		wxString tempDir = wxStandardPaths::Get().GetTempDir();
		wxString tempFileName = wxString::Format("generated_large_island_%dx%d_%ld.otbm", width, height, wxGetLocalTime());
		wxString tempFilePath = tempDir + wxFileName::GetPathSeparator() + tempFileName;
		
		bool saveSuccess = false;
		try {
			IOMapOTBM mapLoader(tempMap.getVersion());
			saveSuccess = mapLoader.saveMap(tempMap, wxFileName(tempFilePath));
		} catch (...) {
			saveSuccess = false;
		}
		
		if (!saveSuccess) {
			wxMessageBox("Failed to save large island map file.", "Save Error", wxOK | wxICON_ERROR);
			return false;
		}
		
		// Import the generated map
		progress.SetLabel("Loading large island map into editor...");
		
		// Create new editor tab for large maps to avoid overwriting existing work
		bool newMapSuccess = g_gui.NewMap();
		if (newMapSuccess) {
			bool loadSuccess = g_gui.LoadMap(FileName(tempFilePath));
			if (loadSuccess) {
				g_gui.SetStatusText(wxString::Format("Large island map (%dx%d) generated successfully!", width, height));
				wxMessageBox(wxString::Format("Large island map (%dx%d) generated successfully!\n\nGenerated in %d batches for optimal performance.", 
					width, height, totalBatches), "Large Map Generated", wxOK | wxICON_INFORMATION);
			} else {
				wxMessageBox("Failed to load generated large island into editor.", "Import Error", wxOK | wxICON_ERROR);
				return false;
			}
		} else {
			wxMessageBox("Failed to create new editor for large island.", "Import Error", wxOK | wxICON_ERROR);
			return false;
		}
		
		// Clean up temporary file
		wxRemoveFile(tempFilePath);
		
		return true;
		
	} catch (const std::exception& e) {
		wxMessageBox(wxString::Format("Failed to generate large island map: %s", e.what()), "Generation Error", wxOK | wxICON_ERROR);
		return false;
	}
}

IslandConfig OTMapGenDialog::BuildIslandConfig() {
	IslandConfig config;
	
	// Parse text inputs with validation
	double noise_scale = 0.005;
	double noise_persistence = 0.5;
	double noise_lacunarity = 2.0;
	double island_size = 0.8;
	double island_falloff = 3.0;
	double island_threshold = 0.0;
	double water_level = 3.0;
	
	if (!island_noise_scale_text->GetValue().ToDouble(&noise_scale)) noise_scale = 0.005;
	if (!island_noise_persistence_text->GetValue().ToDouble(&noise_persistence)) noise_persistence = 0.5;
	if (!island_noise_lacunarity_text->GetValue().ToDouble(&noise_lacunarity)) noise_lacunarity = 2.0;
	if (!island_size_text->GetValue().ToDouble(&island_size)) island_size = 0.8;
	if (!island_falloff_text->GetValue().ToDouble(&island_falloff)) island_falloff = 3.0;
	if (!island_threshold_text->GetValue().ToDouble(&island_threshold)) island_threshold = 0.0;
	if (!island_water_level_text->GetValue().ToDouble(&water_level)) water_level = 3.0;
	
	// Apply bounds checking
	config.noise_scale = std::max(0.0001, std::min(1.0, noise_scale));
	config.noise_octaves = island_noise_octaves_spin->GetValue();
	config.noise_persistence = std::max(0.01, std::min(1.0, noise_persistence));
	config.noise_lacunarity = std::max(1.0, std::min(10.0, noise_lacunarity));
	config.island_size = std::max(0.1, std::min(2.0, island_size));
	config.island_falloff = std::max(0.1, std::min(10.0, island_falloff));
	config.island_threshold = std::max(-1.0, std::min(1.0, island_threshold));
	config.water_level = std::max(0.0, std::min(10.0, water_level));
	config.water_id = island_water_id_spin->GetValue();
	config.ground_id = island_ground_id_spin->GetValue();
	
	// Cleanup settings
	config.enable_cleanup = island_enable_cleanup_checkbox->GetValue();
	config.min_land_patch_size = island_min_patch_size_spin->GetValue();
	config.max_water_hole_size = island_max_hole_size_spin->GetValue();
	config.smoothing_passes = island_smoothing_passes_spin->GetValue();
	
	return config;
}

void OTMapGenDialog::RollIslandDice() {
	// Generate random island parameters for variety
	std::random_device rd;
	std::mt19937 gen(rd());
	
	// Debug message to confirm function is called
	wxLogDebug("RollIslandDice called");
	
	// Add null checks for all controls before setting values
	if (island_noise_scale_text) {
		// Noise scale: 0.008 to 0.013 (updated range for better normal results)
		std::uniform_real_distribution<> noise_scale_dist(0.008, 0.013);
		island_noise_scale_text->SetValue(wxString::Format("%.6f", noise_scale_dist(gen)));
	}
	
	if (island_noise_octaves_spin) {
		// Noise octaves: 4 to 8
		std::uniform_int_distribution<> octaves_dist(4, 8);
		island_noise_octaves_spin->SetValue(octaves_dist(gen));
	}
	
	if (island_noise_persistence_text) {
		// Persistence: 0.3 to 0.7
		std::uniform_real_distribution<> persistence_dist(0.3, 0.7);
		island_noise_persistence_text->SetValue(wxString::Format("%.3f", persistence_dist(gen)));
	}
	
	if (island_noise_lacunarity_text) {
		// Lacunarity: 1.8 to 2.5
		std::uniform_real_distribution<> lacunarity_dist(1.8, 2.5);
		island_noise_lacunarity_text->SetValue(wxString::Format("%.3f", lacunarity_dist(gen)));
	}
	
	if (island_size_text) {
		// Island size: 0.6 to 1.2
		std::uniform_real_distribution<> size_dist(0.6, 1.2);
		island_size_text->SetValue(wxString::Format("%.3f", size_dist(gen)));
	}
	
	if (island_falloff_text) {
		// Island falloff: 2.0 to 5.0
		std::uniform_real_distribution<> falloff_dist(2.0, 5.0);
		island_falloff_text->SetValue(wxString::Format("%.3f", falloff_dist(gen)));
	}
	
	if (island_threshold_text) {
		// Threshold: -0.2 to 0.2
		std::uniform_real_distribution<> threshold_dist(-0.2, 0.2);
		island_threshold_text->SetValue(wxString::Format("%.3f", threshold_dist(gen)));
	}
	
	if (island_seed_text_ctrl) {
		// Generate a random seed as well
		std::uniform_int_distribution<> seed_dist(1000, 9999);
		island_seed_text_ctrl->SetValue(wxString::Format("%d", seed_dist(gen)));
	}
	
	// Force refresh of the controls
	if (island_panel) {
		island_panel->Refresh();
		island_panel->Update();
	}
}

void OTMapGenDialog::UpdateIslandControlsFromConfig() {
	// Update UI controls from the current island_config
	island_noise_scale_text->SetValue(wxString::Format("%.6f", island_config.noise_scale));
	island_noise_octaves_spin->SetValue(island_config.noise_octaves);
	island_noise_persistence_text->SetValue(wxString::Format("%.3f", island_config.noise_persistence));
	island_noise_lacunarity_text->SetValue(wxString::Format("%.3f", island_config.noise_lacunarity));
	island_size_text->SetValue(wxString::Format("%.3f", island_config.island_size));
	island_falloff_text->SetValue(wxString::Format("%.3f", island_config.island_falloff));
	island_threshold_text->SetValue(wxString::Format("%.3f", island_config.island_threshold));
	island_water_level_text->SetValue(wxString::Format("%.3f", island_config.water_level));
	island_water_id_spin->SetValue(island_config.water_id);
	island_ground_id_spin->SetValue(island_config.ground_id);
	island_enable_cleanup_checkbox->SetValue(island_config.enable_cleanup);
	island_min_patch_size_spin->SetValue(island_config.min_land_patch_size);
	island_max_hole_size_spin->SetValue(island_config.max_water_hole_size);
	island_smoothing_passes_spin->SetValue(island_config.smoothing_passes);
}

// Brush integration helpers
void OTMapGenDialog::OnWaterIdButtonClick(wxCommandEvent& event) {
	// Handle water ID button click for item selection
	uint16_t currentId = island_water_id_spin->GetValue();
	
	// Create a simple item selection dialog using wxGetTextFromUser
	wxString result = wxGetTextFromUser(
		"Enter water tile ID (100-65000):", 
		"Water Tile ID", 
		wxString::Format("%d", currentId), 
		this
	);
	
	if (!result.IsEmpty()) {
		long newId;
		if (result.ToLong(&newId) && newId >= 100 && newId <= 65000) {
			island_water_id_spin->SetValue((int)newId);
			island_water_id_text->SetValue(wxString::Format("%ld", newId));
			
			// Update the button preview
			if (island_water_id_button) {
				island_water_id_button->SetItemId((uint16_t)newId);
				island_water_id_button->Refresh();
			}
		} else {
			wxMessageBox("Invalid tile ID. Please enter a number between 100 and 10000.", "Invalid Input", wxOK | wxICON_WARNING);
		}
	}
}

void OTMapGenDialog::OnGroundIdButtonClick(wxCommandEvent& event) {
	// Handle ground ID button click for item selection
	uint16_t currentId = island_ground_id_spin->GetValue();
	
	// Create a simple item selection dialog using wxGetTextFromUser
	wxString result = wxGetTextFromUser(
		"Enter ground tile ID (100-65000):", 
		"Ground Tile ID", 
		wxString::Format("%d", currentId), 
		this
	);
	
	if (!result.IsEmpty()) {
		long newId;
		if (result.ToLong(&newId) && newId >= 100 && newId <= 65000) {
			island_ground_id_spin->SetValue((int)newId);
			island_ground_id_text->SetValue(wxString::Format("%ld", newId));
			
			// Update the button preview
			if (island_ground_id_button) {
				island_ground_id_button->SetItemId((uint16_t)newId);
				island_ground_id_button->Refresh();
			}
		} else {
			wxMessageBox("Invalid tile ID. Please enter a number between 100 and 65000.", "Invalid Input", wxOK | wxICON_WARNING);
		}
	}
}

void OTMapGenDialog::OnItemIdTextChange(wxCommandEvent& event) {
	// Handle text changes for item ID fields
	wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
	if (textCtrl) {
		long value;
		if (textCtrl->GetValue().ToLong(&value) && value >= 100 && value <= 65000) {
			if (textCtrl->GetId() == ID_ISLAND_WATER_ID_TEXT) {
				island_water_id_spin->SetValue((int)value);
				if (island_water_id_button) {
					island_water_id_button->SetItemId((uint16_t)value);
					island_water_id_button->Refresh();
				}
			} else if (textCtrl->GetId() == ID_ISLAND_GROUND_ID_TEXT) {
				island_ground_id_spin->SetValue((int)value);
				if (island_ground_id_button) {
					island_ground_id_button->SetItemId((uint16_t)value);
					island_ground_id_button->Refresh();
				}
			}
		}
	}
}

// Tab-specific value getters
int OTMapGenDialog::GetCurrentWidth() {
	return main_width_spin_ctrl->GetValue();
}

int OTMapGenDialog::GetCurrentHeight() {
	return main_height_spin_ctrl->GetValue();
}

std::string OTMapGenDialog::GetCurrentSeed() {
	return main_seed_text_ctrl->GetValue().ToStdString();
}

std::string OTMapGenDialog::GetCurrentVersion() {
	return main_version_choice->GetStringSelection().ToStdString();
}

// Preset Management Functions
void OTMapGenDialog::LoadGenerationPresets() {
	generation_presets.clear();
	preset_choice->Clear();
	
	if (!wxFileExists(presets_file_path)) {
		// Create default presets if file doesn't exist
		GenerationPreset defaultIsland;
		defaultIsland.name = "Default Island";
		defaultIsland.type = GENERATOR_ISLAND;
		defaultIsland.width = 256;
		defaultIsland.height = 256;
		defaultIsland.version = "10.98";
		defaultIsland.seed = "12345";  // Add default seed
		
		// Set default island config
		defaultIsland.island_config.noise_scale = 0.01;
		defaultIsland.island_config.noise_octaves = 4;
		defaultIsland.island_config.noise_persistence = 0.5;
		defaultIsland.island_config.noise_lacunarity = 2.0;
		defaultIsland.island_config.island_size = 0.8;
		defaultIsland.island_config.island_falloff = 2.0;
		defaultIsland.island_config.island_threshold = 0.3;
		defaultIsland.island_config.water_level = 0.5;
		defaultIsland.island_config.water_id = 4608;
		defaultIsland.island_config.ground_id = 4526;
		defaultIsland.island_config.enable_cleanup = true;
		defaultIsland.island_config.min_land_patch_size = 4;
		defaultIsland.island_config.max_water_hole_size = 3;
		defaultIsland.island_config.smoothing_passes = 2;
		
		generation_presets.push_back(defaultIsland);
		SaveGenerationPresets();
	} else {
		// Load presets from XML file
		try {
			wxXmlDocument doc;
			if (doc.Load(presets_file_path)) {
				wxXmlNode* root = doc.GetRoot();
				if (root && root->GetName() == "GenerationPresets") {
					wxXmlNode* presetNode = root->GetChildren();
					while (presetNode) {
						if (presetNode->GetName() == "Preset") {
							GenerationPreset preset;
							preset.name = presetNode->GetAttribute("name", "Unnamed");
							
							wxString typeStr = presetNode->GetAttribute("type", "0");
							long typeVal;
							typeStr.ToLong(&typeVal);
							preset.type = static_cast<GeneratorType>(typeVal);
							
							wxString widthStr = presetNode->GetAttribute("width", "256");
							widthStr.ToLong((long*)&preset.width);
							
							wxString heightStr = presetNode->GetAttribute("height", "256");
							heightStr.ToLong((long*)&preset.height);
							
							preset.version = presetNode->GetAttribute("version", "10.98");
							preset.seed = presetNode->GetAttribute("seed", "12345");  // Add seed loading
							
							// Load island config if present
							wxXmlNode* islandNode = presetNode->GetChildren();
							while (islandNode) {
								if (islandNode->GetName() == "IslandConfig") {
									double val;
									long intVal;
									
									if (islandNode->GetAttribute("noise_scale", "0.01").ToDouble(&val))
										preset.island_config.noise_scale = val;
									if (islandNode->GetAttribute("noise_octaves", "4").ToLong(&intVal))
										preset.island_config.noise_octaves = intVal;
									if (islandNode->GetAttribute("noise_persistence", "0.5").ToDouble(&val))
										preset.island_config.noise_persistence = val;
									if (islandNode->GetAttribute("noise_lacunarity", "2.0").ToDouble(&val))
										preset.island_config.noise_lacunarity = val;
									if (islandNode->GetAttribute("island_size", "0.8").ToDouble(&val))
										preset.island_config.island_size = val;
									if (islandNode->GetAttribute("island_falloff", "2.0").ToDouble(&val))
										preset.island_config.island_falloff = val;
									if (islandNode->GetAttribute("island_threshold", "0.3").ToDouble(&val))
										preset.island_config.island_threshold = val;
									if (islandNode->GetAttribute("water_level", "0.5").ToDouble(&val))
										preset.island_config.water_level = val;
									if (islandNode->GetAttribute("water_id", "4608").ToLong(&intVal))
										preset.island_config.water_id = intVal;
									if (islandNode->GetAttribute("ground_id", "4526").ToLong(&intVal))
										preset.island_config.ground_id = intVal;
									
									preset.island_config.enable_cleanup = islandNode->GetAttribute("enable_cleanup", "1") == "1";
									if (islandNode->GetAttribute("min_land_patch_size", "4").ToLong(&intVal))
										preset.island_config.min_land_patch_size = intVal;
									if (islandNode->GetAttribute("max_water_hole_size", "3").ToLong(&intVal))
										preset.island_config.max_water_hole_size = intVal;
									if (islandNode->GetAttribute("smoothing_passes", "2").ToLong(&intVal))
										preset.island_config.smoothing_passes = intVal;
								}
								islandNode = islandNode->GetNext();
							}
							
							generation_presets.push_back(preset);
						}
						presetNode = presetNode->GetNext();
					}
				}
			}
		} catch (...) {
			wxMessageBox("Error loading generation presets.", "Load Error", wxOK | wxICON_WARNING);
		}
	}
	
	// Populate the preset choice control
	for (const auto& preset : generation_presets) {
		preset_choice->Append(preset.name);
	}
	
	// Enable/disable delete button based on selection
	preset_delete_button->Enable(preset_choice->GetSelection() != wxNOT_FOUND);
}

void OTMapGenDialog::SaveGenerationPresets() {
	try {
		wxXmlDocument doc;
		wxXmlNode* root = new wxXmlNode(wxXML_ELEMENT_NODE, "GenerationPresets");
		doc.SetRoot(root);
		
		for (const auto& preset : generation_presets) {
			wxXmlNode* presetNode = new wxXmlNode(wxXML_ELEMENT_NODE, "Preset");
			presetNode->AddAttribute("name", preset.name);
			presetNode->AddAttribute("type", wxString::Format("%d", static_cast<int>(preset.type)));
			presetNode->AddAttribute("width", wxString::Format("%d", preset.width));
			presetNode->AddAttribute("height", wxString::Format("%d", preset.height));
			presetNode->AddAttribute("version", preset.version);
			presetNode->AddAttribute("seed", preset.seed);  // Add seed to preset save
			
			// Save island config
			if (preset.type == GENERATOR_ISLAND) {
				wxXmlNode* islandNode = new wxXmlNode(wxXML_ELEMENT_NODE, "IslandConfig");
				islandNode->AddAttribute("noise_scale", wxString::Format("%.6f", preset.island_config.noise_scale));
				islandNode->AddAttribute("noise_octaves", wxString::Format("%d", preset.island_config.noise_octaves));
				islandNode->AddAttribute("noise_persistence", wxString::Format("%.3f", preset.island_config.noise_persistence));
				islandNode->AddAttribute("noise_lacunarity", wxString::Format("%.3f", preset.island_config.noise_lacunarity));
				islandNode->AddAttribute("island_size", wxString::Format("%.3f", preset.island_config.island_size));
				islandNode->AddAttribute("island_falloff", wxString::Format("%.3f", preset.island_config.island_falloff));
				islandNode->AddAttribute("island_threshold", wxString::Format("%.3f", preset.island_config.island_threshold));
				islandNode->AddAttribute("water_level", wxString::Format("%.3f", preset.island_config.water_level));
				islandNode->AddAttribute("water_id", wxString::Format("%d", preset.island_config.water_id));
				islandNode->AddAttribute("ground_id", wxString::Format("%d", preset.island_config.ground_id));
				islandNode->AddAttribute("enable_cleanup", preset.island_config.enable_cleanup ? "1" : "0");
				islandNode->AddAttribute("min_land_patch_size", wxString::Format("%d", preset.island_config.min_land_patch_size));
				islandNode->AddAttribute("max_water_hole_size", wxString::Format("%d", preset.island_config.max_water_hole_size));
				islandNode->AddAttribute("smoothing_passes", wxString::Format("%d", preset.island_config.smoothing_passes));
				
				presetNode->AddChild(islandNode);
			}
			
			root->AddChild(presetNode);
		}
		
		doc.Save(presets_file_path);
	} catch (...) {
		wxMessageBox("Error saving generation presets.", "Save Error", wxOK | wxICON_WARNING);
	}
}

void OTMapGenDialog::LoadPreset(const std::string& presetName) {
	for (const auto& preset : generation_presets) {
		if (preset.name == presetName) {
			// Load basic settings
			if (preset.type == GENERATOR_ISLAND) {
				island_width_spin_ctrl->SetValue(preset.width);
				island_height_spin_ctrl->SetValue(preset.height);
				island_version_choice->SetStringSelection(preset.version);
				island_seed_text_ctrl->SetValue(preset.seed);  // Load seed into UI
				
				// Load island configuration
				island_noise_scale_text->SetValue(wxString::Format("%.6f", preset.island_config.noise_scale));
				island_noise_octaves_spin->SetValue(preset.island_config.noise_octaves);
				island_noise_persistence_text->SetValue(wxString::Format("%.3f", preset.island_config.noise_persistence));
				island_noise_lacunarity_text->SetValue(wxString::Format("%.3f", preset.island_config.noise_lacunarity));
				island_size_text->SetValue(wxString::Format("%.3f", preset.island_config.island_size));
				island_falloff_text->SetValue(wxString::Format("%.3f", preset.island_config.island_falloff));
				island_threshold_text->SetValue(wxString::Format("%.3f", preset.island_config.island_threshold));
				island_water_level_text->SetValue(wxString::Format("%.3f", preset.island_config.water_level));
				island_water_id_spin->SetValue(preset.island_config.water_id);
				island_ground_id_spin->SetValue(preset.island_config.ground_id);
				island_enable_cleanup_checkbox->SetValue(preset.island_config.enable_cleanup);
				island_min_patch_size_spin->SetValue(preset.island_config.min_land_patch_size);
				island_max_hole_size_spin->SetValue(preset.island_config.max_water_hole_size);
				island_smoothing_passes_spin->SetValue(preset.island_config.smoothing_passes);
			}
			
			break;
		}
	}
}

void OTMapGenDialog::SaveCurrentAsPreset(const std::string& presetName) {
	// Remove existing preset with same name
	for (auto it = generation_presets.begin(); it != generation_presets.end(); ++it) {
		if (it->name == presetName) {
			generation_presets.erase(it);
			break;
		}
	}
	
	// Create new preset
	GenerationPreset preset;
	preset.name = presetName;
	preset.type = current_generator_type;
	
	if (current_generator_type == GENERATOR_ISLAND) {
		preset.width = island_width_spin_ctrl->GetValue();
		preset.height = island_height_spin_ctrl->GetValue();
		preset.version = island_version_choice->GetStringSelection().ToStdString();
		preset.seed = island_seed_text_ctrl->GetValue().ToStdString();  // Save current seed
		preset.island_config = BuildIslandConfig();
	} else {
		preset.width = main_width_spin_ctrl->GetValue();
		preset.height = main_height_spin_ctrl->GetValue();
		preset.version = main_version_choice->GetStringSelection().ToStdString();
		preset.seed = main_seed_text_ctrl->GetValue().ToStdString();  // Save current seed for main tab
	}
	
	generation_presets.push_back(preset);
	SaveGenerationPresets();
	LoadGenerationPresets(); // Refresh the choice control
}

// Event handlers for preset management
void OTMapGenDialog::OnPresetLoad(wxCommandEvent& event) {
	int selection = preset_choice->GetSelection();
	if (selection != wxNOT_FOUND && selection < static_cast<int>(generation_presets.size())) {
		LoadPreset(generation_presets[selection].name);
		preset_name_text->SetValue(generation_presets[selection].name);
	}
	preset_delete_button->Enable(selection != wxNOT_FOUND);
}

void OTMapGenDialog::OnPresetSave(wxCommandEvent& event) {
	wxString presetName = preset_name_text->GetValue().Trim();
	if (presetName.IsEmpty()) {
		wxMessageBox("Please enter a preset name.", "Save Preset", wxOK | wxICON_WARNING);
		return;
	}
	
	// Check if preset already exists and confirm overwrite
	bool exists = false;
	for (const auto& preset : generation_presets) {
		if (preset.name == presetName.ToStdString()) {
			exists = true;
			break;
	}
} 
	
	if (exists) {
		int result = wxMessageBox(wxString::Format("Preset '%s' already exists. Overwrite?", presetName), 
			"Confirm Overwrite", wxYES_NO | wxICON_QUESTION);
		if (result != wxYES) {
			return;
		}
	}
	
	SaveCurrentAsPreset(presetName.ToStdString());
	wxMessageBox(wxString::Format("Preset '%s' saved successfully!", presetName), "Preset Saved", wxOK | wxICON_INFORMATION);
}

void OTMapGenDialog::OnPresetDelete(wxCommandEvent& event) {
	int selection = preset_choice->GetSelection();
	if (selection != wxNOT_FOUND && selection < static_cast<int>(generation_presets.size())) {
		wxString presetName = generation_presets[selection].name;
		
		int result = wxMessageBox(wxString::Format("Delete preset '%s'?", presetName), 
			"Confirm Delete", wxYES_NO | wxICON_QUESTION);
		
		if (result == wxYES) {
			generation_presets.erase(generation_presets.begin() + selection);
			SaveGenerationPresets();
			LoadGenerationPresets(); // Refresh the choice control
			preset_name_text->SetValue("Custom Preset");
			wxMessageBox(wxString::Format("Preset '%s' deleted.", presetName), "Preset Deleted", wxOK | wxICON_INFORMATION);
		}
	}
}

DungeonConfig OTMapGenDialog::BuildDungeonConfig() {
	DungeonConfig config;
	
	// Wall brush configuration
	if (dungeon_wall_brush_choice && dungeon_wall_brush_choice->GetSelection() >= 0) {
		config.wall_brush = dungeon_wall_brush_choice->GetStringSelection().ToStdString();
	}
	
	// TODO: Map wall brush name to actual wall ID from walls.xml
	// For now, use default brick wall IDs
	if (config.wall_brush == "brick wall") {
		config.wall_id = 1026; // Brick wall horizontal
	} else if (config.wall_brush == "stone wall") {
		config.wall_id = 1050; // Stone wall horizontal
	} else if (config.wall_brush == "framework wall") {
		config.wall_id = 1037; // Framework wall horizontal
	} else if (config.wall_brush == "wooden wall") {
		config.wall_id = 5262; // Wooden wall horizontal
	} else {
		config.wall_id = 1026; // Default to brick wall
	}
	
	// Layout parameters
	if (dungeon_corridor_width_spin) {
		config.corridor_width = dungeon_corridor_width_spin->GetValue();
	}
	if (dungeon_room_min_size_spin) {
		config.room_min_size = dungeon_room_min_size_spin->GetValue();
	}
	if (dungeon_room_max_size_spin) {
		config.room_max_size = dungeon_room_max_size_spin->GetValue();
	}
	if (dungeon_room_count_spin) {
		config.room_count = dungeon_room_count_spin->GetValue();
	}
	if (dungeon_corridor_count_spin) {
		config.corridor_count = dungeon_corridor_count_spin->GetValue();
	}
	
	// Complexity
	if (dungeon_complexity_text) {
		double complexity_val;
		if (dungeon_complexity_text->GetValue().ToDouble(&complexity_val)) {
			config.complexity = std::max(0.1, std::min(0.8, complexity_val));
		}
	}
	
	// Generation options
	if (dungeon_add_dead_ends_checkbox) {
		config.add_dead_ends = dungeon_add_dead_ends_checkbox->GetValue();
	}
	if (dungeon_circular_rooms_checkbox) {
		config.circular_rooms = dungeon_circular_rooms_checkbox->GetValue();
	}
	
	// Intersection options (new feature)
	if (dungeon_triple_intersections_checkbox) {
		config.add_triple_intersections = dungeon_triple_intersections_checkbox->GetValue();
	}
	if (dungeon_quad_intersections_checkbox) {
		config.add_quad_intersections = dungeon_quad_intersections_checkbox->GetValue();
	}
	if (dungeon_intersection_count_spin) {
		config.intersection_count = dungeon_intersection_count_spin->GetValue();
	}
	if (dungeon_intersection_size_spin) {
		config.intersection_size = dungeon_intersection_size_spin->GetValue();
	}
	if (dungeon_intersection_probability_text) {
		double prob_val;
		if (dungeon_intersection_probability_text->GetValue().ToDouble(&prob_val)) {
			config.intersection_probability = std::max(0.1, std::min(0.8, prob_val));
		}
	}
	
	// Corridor length controls
	if (dungeon_max_corridor_length_spin) {
		config.max_corridor_length = dungeon_max_corridor_length_spin->GetValue();
	}
	if (dungeon_smart_pathfinding_checkbox) {
		config.use_smart_pathfinding = dungeon_smart_pathfinding_checkbox->GetValue();
	}
	if (dungeon_prefer_intersections_checkbox) {
		config.prefer_intersections = dungeon_prefer_intersections_checkbox->GetValue();
	}
	
	return config;
}

// Dungeon Generator Event Handlers
void OTMapGenDialog::OnDungeonParameterChange(wxCommandEvent& event) {
	// Auto-update preview when dungeon parameters change (optional)
	// Could trigger UpdatePreview() here if desired
}

void OTMapGenDialog::OnDungeonParameterSpin(wxSpinEvent& event) {
	// Auto-update preview when dungeon spin parameters change (optional)
	// Could trigger UpdatePreview() here if desired
}

void OTMapGenDialog::OnDungeonWallBrushChange(wxCommandEvent& event) {
	// Update wall preview when brush changes
	// TODO: Implement wall preview update
}

void OTMapGenDialog::OnDungeonRollDice(wxCommandEvent& event) {
	// Generate random dungeon parameters for variety
	std::random_device rd;
	std::mt19937 gen(rd());
	
	// Add null checks for all controls before setting values
	if (dungeon_corridor_width_spin) {
		// Corridor width: 1 to 3
		std::uniform_int_distribution<> corridor_dist(1, 3);
		dungeon_corridor_width_spin->SetValue(corridor_dist(gen));
	}
	
	if (dungeon_room_min_size_spin) {
		// Room min size: 3 to 6
		std::uniform_int_distribution<> min_size_dist(3, 6);
		dungeon_room_min_size_spin->SetValue(min_size_dist(gen));
	}
	
	if (dungeon_room_max_size_spin) {
		// Room max size: 6 to 10 (ensure it's >= min size)
		int min_size = dungeon_room_min_size_spin ? dungeon_room_min_size_spin->GetValue() : 6;
		std::uniform_int_distribution<> max_size_dist(std::max(6, min_size), 10);
		dungeon_room_max_size_spin->SetValue(max_size_dist(gen));
	}
	
	if (dungeon_room_count_spin) {
		// Room count: 6 to 15
		std::uniform_int_distribution<> room_count_dist(6, 15);
		dungeon_room_count_spin->SetValue(room_count_dist(gen));
	}
	
	if (dungeon_corridor_count_spin) {
		// Corridor count: 10 to 25
		std::uniform_int_distribution<> corridor_count_dist(10, 25);
		dungeon_corridor_count_spin->SetValue(corridor_count_dist(gen));
	}
	
	if (dungeon_complexity_text) {
		// Complexity: 0.2 to 0.6
		std::uniform_real_distribution<> complexity_dist(0.2, 0.6);
		dungeon_complexity_text->SetValue(wxString::Format("%.2f", complexity_dist(gen)));
	}
	
	if (dungeon_seed_text_ctrl) {
		// Generate a random seed as well
		std::uniform_int_distribution<> seed_dist(1000, 9999);
		dungeon_seed_text_ctrl->SetValue(wxString::Format("%d", seed_dist(gen)));
	}
	
	// Randomize intersection settings (new feature)
	if (dungeon_triple_intersections_checkbox) {
		std::uniform_int_distribution<> bool_dist(0, 1);
		dungeon_triple_intersections_checkbox->SetValue(bool_dist(gen) == 1);
	}
	if (dungeon_quad_intersections_checkbox) {
		std::uniform_int_distribution<> bool_dist(0, 1);
		dungeon_quad_intersections_checkbox->SetValue(bool_dist(gen) == 1);
	}
	if (dungeon_intersection_count_spin) {
		std::uniform_int_distribution<> intersection_count_dist(2, 6);
		dungeon_intersection_count_spin->SetValue(intersection_count_dist(gen));
	}
	if (dungeon_intersection_size_spin) {
		std::uniform_int_distribution<> intersection_size_dist(1, 3);
		dungeon_intersection_size_spin->SetValue(intersection_size_dist(gen));
	}
	if (dungeon_intersection_probability_text) {
		std::uniform_real_distribution<> intersection_prob_dist(0.2, 0.6);
		dungeon_intersection_probability_text->SetValue(wxString::Format("%.2f", intersection_prob_dist(gen)));
	}
	
	// Randomize corridor length settings
	if (dungeon_max_corridor_length_spin) {
		// Max corridor length: 30 to 80 for good variety
		std::uniform_int_distribution<> max_corridor_dist(30, 80);
		dungeon_max_corridor_length_spin->SetValue(max_corridor_dist(gen));
	}
	
	if (dungeon_smart_pathfinding_checkbox) {
		std::uniform_int_distribution<> bool_dist(0, 1);
		dungeon_smart_pathfinding_checkbox->SetValue(bool_dist(gen) == 1);
	}
	
	if (dungeon_prefer_intersections_checkbox) {
		std::uniform_int_distribution<> bool_dist(0, 1);
		dungeon_prefer_intersections_checkbox->SetValue(bool_dist(gen) == 1);
	}
	
	// Force refresh of the controls
	if (dungeon_panel) {
		dungeon_panel->Refresh();
		dungeon_panel->Update();
	}
}
