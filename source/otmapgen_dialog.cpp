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
	wxDialog(parent, wxID_ANY, "Procedural Map Generator", wxDefaultPosition, wxSize(1000, 700), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	current_preview(nullptr),
	current_preview_floor(7), // Start with ground level (floor 7)
	current_zoom(1.0),
	preview_offset_x(0),
	preview_offset_y(0) {
	
	// Create main sizer
	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
	
	// Create notebook for tabs
	wxNotebook* notebook = new wxNotebook(this, wxID_ANY);
	
	// === Main Settings Tab (merged basic + advanced) ===
	wxPanel* main_panel = new wxPanel(notebook);
	wxBoxSizer* settings_main_sizer = new wxBoxSizer(wxHORIZONTAL);
	
	// Left side - All Settings
	wxBoxSizer* left_main_sizer = new wxBoxSizer(wxVERTICAL);
	
	// Basic parameters group (more compact)
	wxStaticBoxSizer* basic_params_sizer = new wxStaticBoxSizer(wxVERTICAL, main_panel, "Basic Parameters");
	wxFlexGridSizer* basic_grid_sizer = new wxFlexGridSizer(3, 4, 5, 5);
	basic_grid_sizer->AddGrowableCol(1);
	basic_grid_sizer->AddGrowableCol(3);
	
	// Row 1: Seed and Width
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Seed:"), 0, wxALIGN_CENTER_VERTICAL);
	seed_text_ctrl = new wxTextCtrl(main_panel, ID_SEED_TEXT, wxString::Format("%lld", (long long)time(nullptr) * 1000));
	seed_text_ctrl->SetToolTip("Enter any integer value (supports 64-bit seeds)");
	basic_grid_sizer->Add(seed_text_ctrl, 1, wxEXPAND);
	
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Width:"), 0, wxALIGN_CENTER_VERTICAL);
	width_spin_ctrl = new wxSpinCtrl(main_panel, ID_WIDTH_SPIN, "256", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 64, 2048, 256);
	basic_grid_sizer->Add(width_spin_ctrl, 1, wxEXPAND);
	
	// Row 2: Version and Height
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Version:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString versions;
	versions.Add("10.98");
	versions.Add("11.00");
	versions.Add("12.00");
	version_choice = new wxChoice(main_panel, ID_VERSION_CHOICE, wxDefaultPosition, wxDefaultSize, versions);
	version_choice->SetSelection(0);
	basic_grid_sizer->Add(version_choice, 1, wxEXPAND);
	
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Height:"), 0, wxALIGN_CENTER_VERTICAL);
	height_spin_ctrl = new wxSpinCtrl(main_panel, ID_HEIGHT_SPIN, "256", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 64, 2048, 256);
	basic_grid_sizer->Add(height_spin_ctrl, 1, wxEXPAND);
	
	// Row 3: Mountain Type and Water Level
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Mountain Type:"), 0, wxALIGN_CENTER_VERTICAL);
	wxArrayString mountain_types;
	mountain_types.Add("MOUNTAIN");
	mountain_types.Add("SNOW");
	mountain_types.Add("SAND");
	mountain_type_choice = new wxChoice(main_panel, ID_MOUNTAIN_TYPE_CHOICE, wxDefaultPosition, wxDefaultSize, mountain_types);
	mountain_type_choice->SetSelection(0);
	basic_grid_sizer->Add(mountain_type_choice, 1, wxEXPAND);
	
	basic_grid_sizer->Add(new wxStaticText(main_panel, wxID_ANY, "Water Level:"), 0, wxALIGN_CENTER_VERTICAL);
	water_level_text = new wxTextCtrl(main_panel, ID_WATER_LEVEL_TEXT, "7");
	water_level_text->SetToolTip("Tibia Z-coordinate (0-15, 7 = ground level)");
	basic_grid_sizer->Add(water_level_text, 1, wxEXPAND);
	
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
	
	wxStaticBoxSizer* preview_sizer = new wxStaticBoxSizer(wxVERTICAL, main_panel, "Map Preview");
	preview_bitmap = new wxStaticBitmap(main_panel, wxID_ANY, wxBitmap(400, 400));
	preview_bitmap->SetBackgroundColour(*wxBLACK);
	preview_bitmap->SetMinSize(wxSize(400, 400));
	preview_sizer->Add(preview_bitmap, 1, wxEXPAND | wxALL, 5);
	
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
	
	preview_sizer->Add(floor_nav_sizer, 0, wxEXPAND | wxALL, 5);
	
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
	
	preview_sizer->Add(zoom_nav_sizer, 0, wxEXPAND | wxALL, 5);
	
	// Preview buttons
	wxBoxSizer* preview_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
	preview_button = new wxButton(main_panel, ID_PREVIEW, "Generate Preview");
	wxButton* refresh_preview_button = new wxButton(main_panel, ID_PREVIEW, "Refresh");
	
	preview_buttons_sizer->Add(preview_button, 1, wxEXPAND | wxALL, 2);
	preview_buttons_sizer->Add(refresh_preview_button, 0, wxALL, 2);
	
	preview_sizer->Add(preview_buttons_sizer, 0, wxEXPAND | wxALL, 5);
	
	right_main_sizer->Add(preview_sizer, 1, wxEXPAND | wxALL, 5);
	
	// Add left and right sides to main tab
	settings_main_sizer->Add(left_main_sizer, 0, wxEXPAND | wxALL, 5);
	settings_main_sizer->Add(right_main_sizer, 1, wxEXPAND | wxALL, 5);
	
	main_panel->SetSizer(settings_main_sizer);
	notebook->AddPage(main_panel, "Map Generation", true);
	
	// === Layout Design Tab ===
	wxPanel* layout_panel = new wxPanel(notebook);
	wxBoxSizer* layout_sizer = new wxBoxSizer(wxVERTICAL);
	
	// Terrain Layers section
	wxStaticBoxSizer* terrain_layers_sizer = new wxStaticBoxSizer(wxHORIZONTAL, layout_panel, "Terrain Layers");
	
	// Terrain layer list
	terrain_layer_list = new wxListCtrl(layout_panel, ID_TERRAIN_LAYER_LIST, wxDefaultPosition, wxSize(300, 200), 
		wxLC_REPORT | wxLC_SINGLE_SEL);
	terrain_layer_list->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 100);
	terrain_layer_list->InsertColumn(1, "Brush", wxLIST_FORMAT_LEFT, 100);
	terrain_layer_list->InsertColumn(2, "Item ID", wxLIST_FORMAT_LEFT, 60);
	terrain_layer_list->InsertColumn(3, "Height", wxLIST_FORMAT_LEFT, 80);
	terrain_layer_list->InsertColumn(4, "Enabled", wxLIST_FORMAT_LEFT, 60);
	
	terrain_layers_sizer->Add(terrain_layer_list, 1, wxEXPAND | wxALL, 5);
	
	// Layer control buttons
	wxBoxSizer* layer_buttons_sizer = new wxBoxSizer(wxVERTICAL);
	add_layer_button = new wxButton(layout_panel, ID_ADD_LAYER, "Add Layer");
	remove_layer_button = new wxButton(layout_panel, ID_REMOVE_LAYER, "Remove Layer");
	move_up_button = new wxButton(layout_panel, ID_MOVE_UP_LAYER, "Move Up");
	move_down_button = new wxButton(layout_panel, ID_MOVE_DOWN_LAYER, "Move Down");
	edit_layer_button = new wxButton(layout_panel, ID_EDIT_LAYER, "Edit Layer");
	
	layer_buttons_sizer->Add(add_layer_button, 0, wxEXPAND | wxALL, 2);
	layer_buttons_sizer->Add(remove_layer_button, 0, wxEXPAND | wxALL, 2);
	layer_buttons_sizer->Add(move_up_button, 0, wxEXPAND | wxALL, 2);
	layer_buttons_sizer->Add(move_down_button, 0, wxEXPAND | wxALL, 2);
	layer_buttons_sizer->Add(edit_layer_button, 0, wxEXPAND | wxALL, 2);
	layer_buttons_sizer->AddStretchSpacer();
	
	terrain_layers_sizer->Add(layer_buttons_sizer, 0, wxEXPAND | wxALL, 5);
	
	// Layer Properties section
	wxStaticBoxSizer* layer_props_sizer = new wxStaticBoxSizer(wxVERTICAL, layout_panel, "Layer Properties");
	layer_properties_panel = new wxPanel(layout_panel);
	wxFlexGridSizer* props_grid_sizer = new wxFlexGridSizer(5, 4, 5, 10);
	props_grid_sizer->AddGrowableCol(1);
	props_grid_sizer->AddGrowableCol(3);
	
	// Row 1: Name and Brush
	props_grid_sizer->Add(new wxStaticText(layer_properties_panel, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
	layer_name_text = new wxTextCtrl(layer_properties_panel, wxID_ANY, "");
	props_grid_sizer->Add(layer_name_text, 1, wxEXPAND);
	
	props_grid_sizer->Add(new wxStaticText(layer_properties_panel, wxID_ANY, "Brush:"), 0, wxALIGN_CENTER_VERTICAL);
	layer_brush_choice = new wxChoice(layer_properties_panel, ID_LAYER_BRUSH_CHOICE);
	props_grid_sizer->Add(layer_brush_choice, 1, wxEXPAND);
	
	// Row 2: Item ID and Z-Order
	props_grid_sizer->Add(new wxStaticText(layer_properties_panel, wxID_ANY, "Item ID:"), 0, wxALIGN_CENTER_VERTICAL);
	layer_item_id_spin = new wxSpinCtrl(layer_properties_panel, ID_LAYER_ITEM_ID_SPIN, "100", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 65535, 100);
	props_grid_sizer->Add(layer_item_id_spin, 1, wxEXPAND);
	
	props_grid_sizer->Add(new wxStaticText(layer_properties_panel, wxID_ANY, "Z-Order:"), 0, wxALIGN_CENTER_VERTICAL);
	z_order_spin = new wxSpinCtrl(layer_properties_panel, wxID_ANY, "1000", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000, 1000);
	props_grid_sizer->Add(z_order_spin, 1, wxEXPAND);
	
	// Row 3: Height range
	props_grid_sizer->Add(new wxStaticText(layer_properties_panel, wxID_ANY, "Height Min:"), 0, wxALIGN_CENTER_VERTICAL);
	height_min_text = new wxTextCtrl(layer_properties_panel, wxID_ANY, "0.0");
	props_grid_sizer->Add(height_min_text, 1, wxEXPAND);
	
	props_grid_sizer->Add(new wxStaticText(layer_properties_panel, wxID_ANY, "Height Max:"), 0, wxALIGN_CENTER_VERTICAL);
	height_max_text = new wxTextCtrl(layer_properties_panel, wxID_ANY, "1.0");
	props_grid_sizer->Add(height_max_text, 1, wxEXPAND);
	
	// Row 4: Moisture range
	props_grid_sizer->Add(new wxStaticText(layer_properties_panel, wxID_ANY, "Moisture Min:"), 0, wxALIGN_CENTER_VERTICAL);
	moisture_min_text = new wxTextCtrl(layer_properties_panel, wxID_ANY, "-1.0");
	props_grid_sizer->Add(moisture_min_text, 1, wxEXPAND);
	
	props_grid_sizer->Add(new wxStaticText(layer_properties_panel, wxID_ANY, "Moisture Max:"), 0, wxALIGN_CENTER_VERTICAL);
	moisture_max_text = new wxTextCtrl(layer_properties_panel, wxID_ANY, "1.0");
	props_grid_sizer->Add(moisture_max_text, 1, wxEXPAND);
	
	// Row 5: Noise scale and Coverage
	props_grid_sizer->Add(new wxStaticText(layer_properties_panel, wxID_ANY, "Noise Scale:"), 0, wxALIGN_CENTER_VERTICAL);
	noise_scale_text = new wxTextCtrl(layer_properties_panel, wxID_ANY, "1.0");
	props_grid_sizer->Add(noise_scale_text, 1, wxEXPAND);
	
	props_grid_sizer->Add(new wxStaticText(layer_properties_panel, wxID_ANY, "Coverage:"), 0, wxALIGN_CENTER_VERTICAL);
	coverage_text = new wxTextCtrl(layer_properties_panel, wxID_ANY, "1.0");
	props_grid_sizer->Add(coverage_text, 1, wxEXPAND);
	
	layer_properties_panel->SetSizer(props_grid_sizer);
	layer_props_sizer->Add(layer_properties_panel, 1, wxEXPAND | wxALL, 5);
	
	// Checkboxes for layer options
	wxBoxSizer* layer_options_sizer = new wxBoxSizer(wxHORIZONTAL);
	use_borders_checkbox = new wxCheckBox(layout_panel, wxID_ANY, "Use Borders");
	use_borders_checkbox->SetValue(true);
	layer_enabled_checkbox = new wxCheckBox(layout_panel, wxID_ANY, "Layer Enabled");
	layer_enabled_checkbox->SetValue(true);
	
	layer_options_sizer->Add(use_borders_checkbox, 0, wxALL, 5);
	layer_options_sizer->Add(layer_enabled_checkbox, 0, wxALL, 5);
	layer_props_sizer->Add(layer_options_sizer, 0, wxEXPAND | wxALL, 5);
	
	layout_sizer->Add(layer_props_sizer, 0, wxEXPAND | wxALL, 5);
	
	// Cave and Water Configuration section
	wxStaticBoxSizer* special_terrain_sizer = new wxStaticBoxSizer(wxHORIZONTAL, layout_panel, "Cave & Water Configuration");
	
	// Cave configuration
	wxBoxSizer* cave_config_sizer = new wxBoxSizer(wxVERTICAL);
	cave_config_sizer->Add(new wxStaticText(layout_panel, wxID_ANY, "Cave Configuration"), 0, wxALL, 2);
	
	wxFlexGridSizer* cave_grid_sizer = new wxFlexGridSizer(2, 2, 5, 10);
	cave_grid_sizer->AddGrowableCol(1);
	
	cave_grid_sizer->Add(new wxStaticText(layout_panel, wxID_ANY, "Cave Brush:"), 0, wxALIGN_CENTER_VERTICAL);
	cave_brush_choice = new wxChoice(layout_panel, ID_CAVE_BRUSH_CHOICE);
	cave_grid_sizer->Add(cave_brush_choice, 1, wxEXPAND);
	
	cave_grid_sizer->Add(new wxStaticText(layout_panel, wxID_ANY, "Cave Item ID:"), 0, wxALIGN_CENTER_VERTICAL);
	cave_item_id_spin = new wxSpinCtrl(layout_panel, ID_CAVE_ITEM_ID_SPIN, "351", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 65535, 351);
	cave_grid_sizer->Add(cave_item_id_spin, 1, wxEXPAND);
	
	cave_config_sizer->Add(cave_grid_sizer, 0, wxEXPAND | wxALL, 5);
	special_terrain_sizer->Add(cave_config_sizer, 1, wxEXPAND | wxALL, 5);
	
	// Water configuration
	wxBoxSizer* water_config_sizer = new wxBoxSizer(wxVERTICAL);
	water_config_sizer->Add(new wxStaticText(layout_panel, wxID_ANY, "Water Configuration"), 0, wxALL, 2);
	
	wxFlexGridSizer* water_grid_sizer = new wxFlexGridSizer(2, 2, 5, 10);
	water_grid_sizer->AddGrowableCol(1);
	
	water_grid_sizer->Add(new wxStaticText(layout_panel, wxID_ANY, "Water Brush:"), 0, wxALIGN_CENTER_VERTICAL);
	water_brush_choice = new wxChoice(layout_panel, ID_WATER_BRUSH_CHOICE);
	water_grid_sizer->Add(water_brush_choice, 1, wxEXPAND);
	
	water_grid_sizer->Add(new wxStaticText(layout_panel, wxID_ANY, "Water Item ID:"), 0, wxALIGN_CENTER_VERTICAL);
	water_item_id_spin = new wxSpinCtrl(layout_panel, ID_WATER_ITEM_ID_SPIN, "4608", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 65535, 4608);
	water_grid_sizer->Add(water_item_id_spin, 1, wxEXPAND);
	
	water_config_sizer->Add(water_grid_sizer, 0, wxEXPAND | wxALL, 5);
	special_terrain_sizer->Add(water_config_sizer, 1, wxEXPAND | wxALL, 5);
	
	layout_sizer->Add(special_terrain_sizer, 0, wxEXPAND | wxALL, 5);
	layout_sizer->Add(terrain_layers_sizer, 1, wxEXPAND | wxALL, 5);
	
	layout_panel->SetSizer(layout_sizer);
	notebook->AddPage(layout_panel, "Layout Design", false);
	
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
	
	// Initialize terrain layers and brush choices
	PopulateBrushChoices();
	
	// Initialize default terrain layers
	GenerationConfig defaultConfig;
	defaultConfig.initializeDefaultLayers();
	working_terrain_layers = defaultConfig.terrain_layers;
	
	// Populate the terrain layer list
	PopulateTerrainLayerList();
	
	// Initially disable layer property controls
	UpdateLayerControls();
	
	// Generate initial random 64-bit seed
	srand(time(nullptr));
	long long initial_seed = ((long long)rand() << 32) | rand();
	seed_text_ctrl->SetValue(wxString::Format("%lld", initial_seed));
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
			int preview_width = 400;  // Updated to match the larger preview
			int preview_height = 400; // Updated to match the larger preview
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
	
	long selected = terrain_layer_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	move_up_button->Enable(hasSelection && selected > 0);
	move_down_button->Enable(hasSelection && selected < static_cast<long>(working_terrain_layers.size()) - 1);
	
	if (hasSelection) {
		SetSelectedLayer(*layer);
	} else {
		// Clear controls
		layer_name_text->SetValue("");
		layer_brush_choice->SetSelection(-1);
		layer_item_id_spin->SetValue(100);
		height_min_text->SetValue("0.0");
		height_max_text->SetValue("1.0");
		moisture_min_text->SetValue("-1.0");
		moisture_max_text->SetValue("1.0");
		noise_scale_text->SetValue("1.0");
		coverage_text->SetValue("1.0");
		use_borders_checkbox->SetValue(false);
		layer_enabled_checkbox->SetValue(false);
		z_order_spin->SetValue(1000);
	}
}

TerrainLayer* OTMapGenDialog::GetSelectedLayer() {
	long selected = terrain_layer_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (selected >= 0 && selected < static_cast<long>(working_terrain_layers.size())) {
		return &working_terrain_layers[selected];
	}
	return nullptr;
}

void OTMapGenDialog::SetSelectedLayer(const TerrainLayer& layer) {
	layer_name_text->SetValue(layer.name);
	
	// Find and select the brush
	for (size_t i = 0; i < available_brushes.size(); ++i) {
		if (available_brushes[i] == layer.brush_name) {
			layer_brush_choice->SetSelection(i);
			break;
		}
	}
	
	layer_item_id_spin->SetValue(layer.item_id);
	height_min_text->SetValue(wxString::Format("%.2f", layer.height_min));
	height_max_text->SetValue(wxString::Format("%.2f", layer.height_max));
	moisture_min_text->SetValue(wxString::Format("%.2f", layer.moisture_min));
	moisture_max_text->SetValue(wxString::Format("%.2f", layer.moisture_max));
	noise_scale_text->SetValue(wxString::Format("%.2f", layer.noise_scale));
	coverage_text->SetValue(wxString::Format("%.2f", layer.coverage));
	use_borders_checkbox->SetValue(layer.use_borders);
	layer_enabled_checkbox->SetValue(layer.enabled);
	z_order_spin->SetValue(layer.z_order);
}

void OTMapGenDialog::GetTilePreviewColor(uint16_t tileId, unsigned char& r, unsigned char& g, unsigned char& b) {
	// First check if this tile ID matches any of our configured terrain layers
	for (const auto& layer : working_terrain_layers) {
		if (layer.item_id == tileId && layer.enabled) {
			// Use layer-specific colors based on layer name
			if (layer.name == "Water" || layer.brush_name == "sea") {
				r = 0; g = 100; b = 255; // Blue for water
				return;
			} else if (layer.name == "Grass" || layer.brush_name == "grass") {
				r = 50; g = 200; b = 50; // Green for grass
				return;
			} else if (layer.name == "Sand" || layer.brush_name == "sand") {
				r = 255; g = 255; b = 100; // Yellow for sand
				return;
			} else if (layer.name == "Mountain" || layer.brush_name == "mountain") {
				r = 139; g = 69; b = 19; // Brown for mountains
				return;
			} else if (layer.brush_name == "snow") {
				r = 255; g = 255; b = 255; // White for snow
				return;
			} else if (layer.brush_name == "cave") {
				r = 64; g = 64; b = 64; // Dark gray for caves
				return;
			} else if (layer.brush_name.find("stone") != std::string::npos) {
				r = 128; g = 128; b = 128; // Gray for stone
				return;
			} else if (layer.brush_name == "ice") {
				r = 200; g = 200; b = 255; // Light blue for ice
				return;
			} else if (layer.brush_name.find("wood") != std::string::npos) {
				r = 139; g = 69; b = 19; // Brown for wood
				return;
			}
		}
	}
	
	// Check cave and water configuration
	if (tileId == cave_item_id_spin->GetValue()) {
		r = 64; g = 64; b = 64; // Dark gray for caves
		return;
	}
	
	if (tileId == water_item_id_spin->GetValue()) {
		r = 0; g = 100; b = 255; // Blue for water
		return;
	}
	
	// Fallback: Convert tile ID to preview colors using common item IDs
	switch (tileId) {
		case 4608: case 4609: case 4610: case 4611: case 4612: case 4613:
			r = 0; g = 100; b = 255; // Blue for water
			break;
		case 4526: case 4527: case 4528: case 4529: case 4530:
			r = 50; g = 200; b = 50; // Green for grass
			break;
		case 231:
			r = 255; g = 255; b = 100; // Yellow for sand
			break;
		case 1284: case 431:
			r = 128; g = 128; b = 128; // Gray for stone
			break;
		case 4597:
			r = 100; g = 100; b = 100; // Dark gray for gravel
			break;
		case 670: case 671:
			r = 255; g = 255; b = 255; // White for snow
			break;
		case 919: case 4468: case 4469:
			r = 139; g = 69; b = 19; // Brown for mountains
			break;
		case 351: case 352: case 353:
			r = 64; g = 64; b = 64; // Dark gray for caves
			break;
		case 106: case 108: case 109:
			r = 0; g = 150; b = 0; // Dark green for lawn
			break;
		case 405: case 448:
			r = 139; g = 69; b = 19; // Brown for wooden floors
			break;
		default:
			r = 64; g = 64; b = 64; // Default dark gray
			break;
	}
} 