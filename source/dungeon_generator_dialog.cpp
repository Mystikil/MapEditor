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
#include "dungeon_generator_dialog.h"
#include "gui.h"
#include "artprovider.h"
#include "items.h"
#include "brush.h"
#include "ground_brush.h"
#include "otmapgen_dungeon.h"
#include <random>
#include <ctime>

BEGIN_EVENT_TABLE(DungeonGeneratorDialog, wxDialog)
    EVT_BUTTON(ID_DUNGEON_GENERATE, DungeonGeneratorDialog::OnGenerateClick)
    EVT_BUTTON(ID_DUNGEON_CANCEL, DungeonGeneratorDialog::OnCancelClick)
    EVT_BUTTON(ID_DUNGEON_PREVIEW, DungeonGeneratorDialog::OnPreviewClick)
    EVT_BUTTON(ID_DUNGEON_RANDOM_SEED, DungeonGeneratorDialog::OnRandomizeSeed)
    EVT_BUTTON(ID_DUNGEON_RESET_DEFAULTS, DungeonGeneratorDialog::OnResetDefaults)
    EVT_CHOICE(ID_DUNGEON_ROOM_TYPE_CHANGE, DungeonGeneratorDialog::OnRoomTypeChange)
    EVT_CHOICE(ID_DUNGEON_BRUSH_CHANGE, DungeonGeneratorDialog::OnBrushChange)
END_EVENT_TABLE()

DungeonPreviewButton::DungeonPreviewButton(wxWindow* parent) :
    DCButton(parent, wxID_ANY, wxDefaultPosition, DC_BTN_TOGGLE, RENDER_SIZE_32x32, 0),
    m_id(0) {
    ////
}

void DungeonPreviewButton::SetItemId(uint16_t id) {
    if (m_id == id) {
        return;
    }

    m_id = id;

    if (m_id != 0) {
        const ItemType& it = g_items.getItemType(m_id);
        if (it.id != 0) {
            SetSprite(it.clientID);
            return;
        }
    }

    SetSprite(0);
}

DungeonGeneratorDialog::DungeonGeneratorDialog(wxWindow* parent) :
    wxDialog(parent, wxID_ANY, "Dungeon Generator", wxDefaultPosition, wxSize(1000, 700), 
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    
    // Create main notebook for organization
    wxNotebook* notebook = new wxNotebook(this, wxID_ANY);
    
    // Basic Settings Panel
    wxPanel* basic_panel = new wxPanel(notebook);
    wxBoxSizer* basic_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Basic generation settings
    wxStaticBoxSizer* gen_sizer = new wxStaticBoxSizer(wxVERTICAL, basic_panel, "Generation Settings");
    wxFlexGridSizer* gen_grid = new wxFlexGridSizer(5, 2, 5, 5);
    
    gen_grid->Add(new wxStaticText(basic_panel, wxID_ANY, "Seed:"), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* seed_row = new wxBoxSizer(wxHORIZONTAL);
    seed_input = new wxTextCtrl(basic_panel, wxID_ANY, "dungeon_seed");
    random_seed_button = new wxButton(basic_panel, ID_DUNGEON_RANDOM_SEED, "Random", wxDefaultPosition, wxSize(80, -1));
    seed_row->Add(seed_input, 1, wxEXPAND | wxRIGHT, 5);
    seed_row->Add(random_seed_button, 0);
    gen_grid->Add(seed_row, 1, wxEXPAND);
    
    gen_grid->Add(new wxStaticText(basic_panel, wxID_ANY, "Width:"), 0, wxALIGN_CENTER_VERTICAL);
    width_spin = new wxSpinCtrl(basic_panel, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, 
                               wxSP_ARROW_KEYS, 20, 500, 100);
    gen_grid->Add(width_spin, 1, wxEXPAND);

    gen_grid->Add(new wxStaticText(basic_panel, wxID_ANY, "Height:"), 0, wxALIGN_CENTER_VERTICAL);
    height_spin = new wxSpinCtrl(basic_panel, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, 
                                wxSP_ARROW_KEYS, 20, 500, 100);
    gen_grid->Add(height_spin, 1, wxEXPAND);

    gen_grid->Add(new wxStaticText(basic_panel, wxID_ANY, "Version:"), 0, wxALIGN_CENTER_VERTICAL);
    version_choice = new wxChoice(basic_panel, wxID_ANY);
    version_choice->Append("7.72");
    version_choice->Append("8.60");
    version_choice->Append("10.98");
    version_choice->Append("12.00");
    version_choice->SetSelection(0);
    gen_grid->Add(version_choice, 1, wxEXPAND);

    gen_grid->AddGrowableCol(1);
    gen_sizer->Add(gen_grid, 0, wxEXPAND | wxALL, 5);
    basic_sizer->Add(gen_sizer, 0, wxEXPAND | wxALL, 5);

    // Position settings
    wxStaticBoxSizer* pos_sizer = new wxStaticBoxSizer(wxVERTICAL, basic_panel, "Starting Position");
    wxFlexGridSizer* pos_grid = new wxFlexGridSizer(3, 2, 5, 5);
    
    pos_grid->Add(new wxStaticText(basic_panel, wxID_ANY, "X:"), 0, wxALIGN_CENTER_VERTICAL);
    pos_x_spin = new wxSpinCtrl(basic_panel, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, 
                               wxSP_ARROW_KEYS, 0, 65535, 0);
    pos_grid->Add(pos_x_spin, 1, wxEXPAND);

    pos_grid->Add(new wxStaticText(basic_panel, wxID_ANY, "Y:"), 0, wxALIGN_CENTER_VERTICAL);
    pos_y_spin = new wxSpinCtrl(basic_panel, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, 
                               wxSP_ARROW_KEYS, 0, 65535, 0);
    pos_grid->Add(pos_y_spin, 1, wxEXPAND);

    pos_grid->Add(new wxStaticText(basic_panel, wxID_ANY, "Z:"), 0, wxALIGN_CENTER_VERTICAL);
    pos_z_spin = new wxSpinCtrl(basic_panel, wxID_ANY, "7", wxDefaultPosition, wxDefaultSize, 
                               wxSP_ARROW_KEYS, 0, 15, 7);
    pos_grid->Add(pos_z_spin, 1, wxEXPAND);

    pos_grid->AddGrowableCol(1);
    pos_sizer->Add(pos_grid, 0, wxEXPAND | wxALL, 5);
    basic_sizer->Add(pos_sizer, 0, wxEXPAND | wxALL, 5);

    basic_panel->SetSizer(basic_sizer);
    notebook->AddPage(basic_panel, "Basic Settings");

    // Room Settings Panel
    wxPanel* room_panel = new wxPanel(notebook);
    wxBoxSizer* room_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Room configuration
    wxStaticBoxSizer* room_config_sizer = new wxStaticBoxSizer(wxVERTICAL, room_panel, "Room Configuration");
    wxFlexGridSizer* room_grid = new wxFlexGridSizer(4, 2, 5, 5);
    
    room_grid->Add(new wxStaticText(room_panel, wxID_ANY, "Room Count:"), 0, wxALIGN_CENTER_VERTICAL);
    room_count_spin = new wxSpinCtrl(room_panel, wxID_ANY, "8", wxDefaultPosition, wxDefaultSize, 
                                    wxSP_ARROW_KEYS, 3, 50, 8);
    room_grid->Add(room_count_spin, 1, wxEXPAND);
    
    room_grid->Add(new wxStaticText(room_panel, wxID_ANY, "Min Size:"), 0, wxALIGN_CENTER_VERTICAL);
    room_min_size_spin = new wxSpinCtrl(room_panel, wxID_ANY, "4", wxDefaultPosition, wxDefaultSize, 
                                       wxSP_ARROW_KEYS, 3, 20, 4);
    room_grid->Add(room_min_size_spin, 1, wxEXPAND);
    
    room_grid->Add(new wxStaticText(room_panel, wxID_ANY, "Max Size:"), 0, wxALIGN_CENTER_VERTICAL);
    room_max_size_spin = new wxSpinCtrl(room_panel, wxID_ANY, "12", wxDefaultPosition, wxDefaultSize, 
                                       wxSP_ARROW_KEYS, 5, 30, 12);
    room_grid->Add(room_max_size_spin, 1, wxEXPAND);
    
    room_grid->AddGrowableCol(1);
    room_config_sizer->Add(room_grid, 0, wxEXPAND | wxALL, 5);
    
    // Room type options
    circular_rooms_checkbox = new wxCheckBox(room_panel, wxID_ANY, "Allow circular rooms");
    circular_rooms_checkbox->SetValue(true);
    room_config_sizer->Add(circular_rooms_checkbox, 0, wxALL, 5);
    
    room_sizer->Add(room_config_sizer, 0, wxEXPAND | wxALL, 5);
    
    room_panel->SetSizer(room_sizer);
    notebook->AddPage(room_panel, "Rooms");

    // Corridor Settings Panel
    wxPanel* corridor_panel = new wxPanel(notebook);
    wxBoxSizer* corridor_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Corridor configuration
    wxStaticBoxSizer* corridor_config_sizer = new wxStaticBoxSizer(wxVERTICAL, corridor_panel, "Corridor Configuration");
    wxFlexGridSizer* corridor_grid = new wxFlexGridSizer(4, 2, 5, 5);
    
    corridor_grid->Add(new wxStaticText(corridor_panel, wxID_ANY, "Width:"), 0, wxALIGN_CENTER_VERTICAL);
    corridor_width_spin = new wxSpinCtrl(corridor_panel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, 
                                        wxSP_ARROW_KEYS, 1, 5, 1);
    corridor_grid->Add(corridor_width_spin, 1, wxEXPAND);
    
    corridor_grid->Add(new wxStaticText(corridor_panel, wxID_ANY, "Count:"), 0, wxALIGN_CENTER_VERTICAL);
    corridor_count_spin = new wxSpinCtrl(corridor_panel, wxID_ANY, "15", wxDefaultPosition, wxDefaultSize, 
                                        wxSP_ARROW_KEYS, 5, 100, 15);
    corridor_grid->Add(corridor_count_spin, 1, wxEXPAND);
    
    corridor_grid->Add(new wxStaticText(corridor_panel, wxID_ANY, "Max Length:"), 0, wxALIGN_CENTER_VERTICAL);
    max_corridor_length_spin = new wxSpinCtrl(corridor_panel, wxID_ANY, "20", wxDefaultPosition, wxDefaultSize, 
                                             wxSP_ARROW_KEYS, 5, 100, 20);
    corridor_grid->Add(max_corridor_length_spin, 1, wxEXPAND);
    
    corridor_grid->AddGrowableCol(1);
    corridor_config_sizer->Add(corridor_grid, 0, wxEXPAND | wxALL, 5);
    
    // Corridor options
    add_dead_ends_checkbox = new wxCheckBox(corridor_panel, wxID_ANY, "Add dead ends");
    add_dead_ends_checkbox->SetValue(false);
    corridor_config_sizer->Add(add_dead_ends_checkbox, 0, wxALL, 5);
    
    smart_pathfinding_checkbox = new wxCheckBox(corridor_panel, wxID_ANY, "Smart pathfinding (A*)");
    smart_pathfinding_checkbox->SetValue(true);
    corridor_config_sizer->Add(smart_pathfinding_checkbox, 0, wxALL, 5);
    
    corridor_sizer->Add(corridor_config_sizer, 0, wxEXPAND | wxALL, 5);
    
    corridor_panel->SetSizer(corridor_sizer);
    notebook->AddPage(corridor_panel, "Corridors");

    // Intersection Settings Panel
    wxPanel* intersection_panel = new wxPanel(notebook);
    wxBoxSizer* intersection_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Intersection configuration
    wxStaticBoxSizer* intersection_config_sizer = new wxStaticBoxSizer(wxVERTICAL, intersection_panel, "Intersection Configuration");
    
    triple_intersections_checkbox = new wxCheckBox(intersection_panel, wxID_ANY, "Enable triple intersections (T-junctions)");
    triple_intersections_checkbox->SetValue(true);
    intersection_config_sizer->Add(triple_intersections_checkbox, 0, wxALL, 5);
    
    quad_intersections_checkbox = new wxCheckBox(intersection_panel, wxID_ANY, "Enable quadruple intersections (crossroads)");
    quad_intersections_checkbox->SetValue(true);
    intersection_config_sizer->Add(quad_intersections_checkbox, 0, wxALL, 5);
    
    prefer_intersections_checkbox = new wxCheckBox(intersection_panel, wxID_ANY, "Prefer intersection-based routing");
    prefer_intersections_checkbox->SetValue(true);
    intersection_config_sizer->Add(prefer_intersections_checkbox, 0, wxALL, 5);
    
    wxFlexGridSizer* intersection_grid = new wxFlexGridSizer(3, 2, 5, 5);
    
    intersection_grid->Add(new wxStaticText(intersection_panel, wxID_ANY, "Intersection Count:"), 0, wxALIGN_CENTER_VERTICAL);
    intersection_count_spin = new wxSpinCtrl(intersection_panel, wxID_ANY, "5", wxDefaultPosition, wxDefaultSize, 
                                           wxSP_ARROW_KEYS, 0, 20, 5);
    intersection_grid->Add(intersection_count_spin, 1, wxEXPAND);
    
    intersection_grid->Add(new wxStaticText(intersection_panel, wxID_ANY, "Size:"), 0, wxALIGN_CENTER_VERTICAL);
    intersection_size_spin = new wxSpinCtrl(intersection_panel, wxID_ANY, "2", wxDefaultPosition, wxDefaultSize, 
                                           wxSP_ARROW_KEYS, 1, 5, 2);
    intersection_grid->Add(intersection_size_spin, 1, wxEXPAND);
    
    intersection_grid->Add(new wxStaticText(intersection_panel, wxID_ANY, "Probability:"), 0, wxALIGN_CENTER_VERTICAL);
    intersection_probability_text = new wxTextCtrl(intersection_panel, wxID_ANY, "0.3");
    intersection_grid->Add(intersection_probability_text, 1, wxEXPAND);
    
    intersection_grid->AddGrowableCol(1);
    intersection_config_sizer->Add(intersection_grid, 0, wxEXPAND | wxALL, 5);
    
    intersection_sizer->Add(intersection_config_sizer, 0, wxEXPAND | wxALL, 5);
    
    intersection_panel->SetSizer(intersection_sizer);
    notebook->AddPage(intersection_panel, "Intersections");

    // Wall & Material Settings Panel
    wxPanel* material_panel = new wxPanel(notebook);
    wxBoxSizer* material_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Wall material selection
    wxStaticBoxSizer* wall_sizer = new wxStaticBoxSizer(wxVERTICAL, material_panel, "Wall Material");
    wxBoxSizer* wall_row = new wxBoxSizer(wxHORIZONTAL);
    
    wall_button = new DungeonPreviewButton(material_panel);
    wall_button->SetItemId(1775); // Default stone wall
    wall_button->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(DungeonGeneratorDialog::OnWallClick), NULL, this);
    wall_row->Add(wall_button, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    wall_brush_choice = new wxChoice(material_panel, ID_DUNGEON_BRUSH_CHANGE);
    PopulateBrushChoices();
    wall_row->Add(wall_brush_choice, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    wall_sizer->Add(wall_row, 0, wxEXPAND | wxALL, 5);
    material_sizer->Add(wall_sizer, 0, wxEXPAND | wxALL, 5);
    
    // Complexity setting
    wxStaticBoxSizer* complexity_sizer = new wxStaticBoxSizer(wxVERTICAL, material_panel, "Generation Complexity");
    wxFlexGridSizer* complexity_grid = new wxFlexGridSizer(1, 2, 5, 5);
    
    complexity_grid->Add(new wxStaticText(material_panel, wxID_ANY, "Complexity:"), 0, wxALIGN_CENTER_VERTICAL);
    complexity_text = new wxTextCtrl(material_panel, wxID_ANY, "0.5");
    complexity_text->SetToolTip("Controls how complex the dungeon layout will be (0.0 - 1.0)");
    complexity_grid->Add(complexity_text, 1, wxEXPAND);
    
    complexity_grid->AddGrowableCol(1);
    complexity_sizer->Add(complexity_grid, 0, wxEXPAND | wxALL, 5);
    material_sizer->Add(complexity_sizer, 0, wxEXPAND | wxALL, 5);
    
    material_panel->SetSizer(material_sizer);
    notebook->AddPage(material_panel, "Materials");

    // Main layout
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(notebook, 1, wxEXPAND | wxALL, 5);
    
    // Button panel
    wxPanel* button_panel = new wxPanel(this);
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    reset_defaults_button = new wxButton(button_panel, ID_DUNGEON_RESET_DEFAULTS, "Reset Defaults");
    preview_button = new wxButton(button_panel, ID_DUNGEON_PREVIEW, "Preview");
    generate_button = new wxButton(button_panel, ID_DUNGEON_GENERATE, "Generate");
    cancel_button = new wxButton(button_panel, ID_DUNGEON_CANCEL, "Cancel");
    
    button_sizer->Add(reset_defaults_button, 0, wxRIGHT, 5);
    button_sizer->AddStretchSpacer();
    button_sizer->Add(preview_button, 0, wxRIGHT, 5);
    button_sizer->Add(generate_button, 0, wxRIGHT, 5);
    button_sizer->Add(cancel_button, 0);
    
    button_panel->SetSizer(button_sizer);
    main_sizer->Add(button_panel, 0, wxEXPAND | wxALL, 5);
    
    // Progress bar
    progress = new wxGauge(this, wxID_ANY, 100);
    progress->Hide();
    main_sizer->Add(progress, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    SetSizer(main_sizer);
    
    // Initialize with default values
    start_position = Position(0, 0, 7);
    seed = "dungeon_seed";
    wall_id = 1775; // Default stone wall
    LoadDefaultSettings();
    UpdateWidgets();
}

DungeonGeneratorDialog::~DungeonGeneratorDialog() {
    // Clean up any allocated resources if needed
}

void DungeonGeneratorDialog::OnGenerateClick(wxCommandEvent& event) {
    GenerateDungeonMap();
}

void DungeonGeneratorDialog::OnCancelClick(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void DungeonGeneratorDialog::OnPreviewClick(wxCommandEvent& event) {
    UpdatePreview();
}

void DungeonGeneratorDialog::OnRandomizeSeed(wxCommandEvent& event) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    seed = wxString::Format("dungeon_%d", dis(gen));
    seed_input->SetValue(seed);
}

void DungeonGeneratorDialog::OnRoomTypeChange(wxCommandEvent& event) {
    // Handle room type change
}

void DungeonGeneratorDialog::OnBrushChange(wxCommandEvent& event) {
    // Handle brush selection change
    // Could update the preview button with the selected brush's default item
}

void DungeonGeneratorDialog::OnResetDefaults(wxCommandEvent& event) {
    LoadDefaultSettings();
    UpdateWidgets();
}

void DungeonGeneratorDialog::OnParameterChange(wxCommandEvent& event) {
    // Handle parameter changes
}

void DungeonGeneratorDialog::OnParameterSpin(wxSpinEvent& event) {
    // Handle spin control changes
}

void DungeonGeneratorDialog::OnWallClick(wxMouseEvent& event) {
    // Open item selection dialog for wall material
    // For now, just use a simple input dialog
    wxString wall_id_str = wxGetTextFromUser("Enter wall item ID:", "Select Wall Material", 
                                           wxString::Format("%d", wall_id));
    if (!wall_id_str.IsEmpty()) {
        long new_wall_id;
        if (wall_id_str.ToLong(&new_wall_id) && new_wall_id > 0 && new_wall_id <= 65535) {
            wall_id = static_cast<uint16_t>(new_wall_id);
            wall_button->SetItemId(wall_id);
        }
    }
}

void DungeonGeneratorDialog::UpdateWidgets() {
    seed_input->SetValue(seed);
    wall_button->SetItemId(wall_id);
    // Update other UI controls with current values
}

void DungeonGeneratorDialog::UpdatePreview() {
    // Generate a small preview of the dungeon
    // This would create a mini-map preview
}

void DungeonGeneratorDialog::GenerateDungeonMap() {
    if (!g_gui.IsEditorOpen()) {
        wxMessageBox("No map is currently open.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Build configuration
    config = BuildDungeonConfig();
    
    // Show progress
    progress->Show();
    progress->SetValue(0);
    Layout();
    
    try {
        // Create generator
        OTMapGeneratorDungeon generator;
        
        // Configure generator
        generator.SetSeed(std::string(seed.mb_str()));
        
        // Get current map
        Editor* editor = g_gui.GetCurrentEditor();
        if (!editor) {
            throw std::runtime_error("No editor available");
        }
        
        Map& map = editor->map;
        
        // Generate dungeon
        bool success = generator.Generate(map, config, start_position.x, start_position.y, start_position.z,
                                        width_spin->GetValue(), height_spin->GetValue());
        
        if (success) {
            progress->SetValue(100);
            wxMessageBox("Dungeon generated successfully!", "Success", wxOK | wxICON_INFORMATION);
            
            // Refresh view
            g_gui.RefreshView();
            g_gui.UpdateMinimap();
            
            EndModal(wxID_OK);
        } else {
            throw std::runtime_error("Failed to generate dungeon");
        }
    } catch (const std::exception& e) {
        progress->Hide();
        wxMessageBox(wxString::Format("Error generating dungeon: %s", e.what()), "Error", wxOK | wxICON_ERROR);
    }
    
    progress->Hide();
    Layout();
}

void DungeonGeneratorDialog::PopulateBrushChoices() {
    wall_brush_choice->Clear();
    wall_brush_choice->Append("Stone Wall");
    wall_brush_choice->Append("Cave Wall");
    wall_brush_choice->Append("Brick Wall");
    wall_brush_choice->Append("Wooden Wall");
    wall_brush_choice->SetSelection(0);
}

void DungeonGeneratorDialog::LoadDefaultSettings() {
    // Reset all settings to defaults
    room_count_spin->SetValue(8);
    room_min_size_spin->SetValue(4);
    room_max_size_spin->SetValue(12);
    circular_rooms_checkbox->SetValue(true);
    
    corridor_width_spin->SetValue(1);
    corridor_count_spin->SetValue(15);
    max_corridor_length_spin->SetValue(20);
    add_dead_ends_checkbox->SetValue(false);
    smart_pathfinding_checkbox->SetValue(true);
    
    triple_intersections_checkbox->SetValue(true);
    quad_intersections_checkbox->SetValue(true);
    intersection_count_spin->SetValue(5);
    intersection_size_spin->SetValue(2);
    intersection_probability_text->SetValue("0.3");
    prefer_intersections_checkbox->SetValue(true);
    
    complexity_text->SetValue("0.5");
    wall_id = 1775; // Stone wall
}

DungeonConfig DungeonGeneratorDialog::BuildDungeonConfig() {
    DungeonConfig cfg;
    
    // Basic settings
    cfg.width = width_spin->GetValue();
    cfg.height = height_spin->GetValue();
    
    // Room settings
    cfg.room_count = room_count_spin->GetValue();
    cfg.room_min_size = room_min_size_spin->GetValue();
    cfg.room_max_size = room_max_size_spin->GetValue();
    cfg.circular_rooms = circular_rooms_checkbox->GetValue();
    
    // Corridor settings
    cfg.corridor_width = corridor_width_spin->GetValue();
    cfg.corridor_count = corridor_count_spin->GetValue();
    cfg.max_corridor_length = max_corridor_length_spin->GetValue();
    cfg.add_dead_ends = add_dead_ends_checkbox->GetValue();
    cfg.smart_pathfinding = smart_pathfinding_checkbox->GetValue();
    
    // Intersection settings
    cfg.triple_intersections = triple_intersections_checkbox->GetValue();
    cfg.quad_intersections = quad_intersections_checkbox->GetValue();
    cfg.intersection_count = intersection_count_spin->GetValue();
    cfg.intersection_size = intersection_size_spin->GetValue();
    cfg.prefer_intersections = prefer_intersections_checkbox->GetValue();
    
    double intersection_prob;
    if (intersection_probability_text->GetValue().ToDouble(&intersection_prob)) {
        cfg.intersection_probability = static_cast<float>(intersection_prob);
    }
    
    // Complexity
    double complexity_val;
    if (complexity_text->GetValue().ToDouble(&complexity_val)) {
        cfg.complexity = static_cast<float>(complexity_val);
    }
    
    // Wall material
    cfg.wall_id = wall_id;
    
    return cfg;
} 