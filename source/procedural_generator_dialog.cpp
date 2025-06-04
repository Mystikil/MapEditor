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
#include "procedural_generator_dialog.h"
#include "gui.h"
#include "artprovider.h"
#include "items.h"
#include "brush.h"
#include "ground_brush.h"
#include "otmapgen_procedural.h"
#include <random>
#include <ctime>

BEGIN_EVENT_TABLE(ProceduralGeneratorDialog, wxDialog)
    EVT_BUTTON(ID_PROCEDURAL_GENERATE, ProceduralGeneratorDialog::OnGenerateClick)
    EVT_BUTTON(ID_PROCEDURAL_CANCEL, ProceduralGeneratorDialog::OnCancelClick)
    EVT_BUTTON(ID_PROCEDURAL_PREVIEW, ProceduralGeneratorDialog::OnPreviewClick)
    EVT_BUTTON(ID_PROCEDURAL_RANDOM_SEED, ProceduralGeneratorDialog::OnRandomizeSeed)
    EVT_BUTTON(ID_PROCEDURAL_LAYER_ADD, ProceduralGeneratorDialog::OnLayerAdd)
    EVT_BUTTON(ID_PROCEDURAL_LAYER_REMOVE, ProceduralGeneratorDialog::OnLayerRemove)
    EVT_BUTTON(ID_PROCEDURAL_LAYER_EDIT, ProceduralGeneratorDialog::OnLayerEdit)
    EVT_BUTTON(ID_PROCEDURAL_RESET_DEFAULTS, ProceduralGeneratorDialog::OnResetDefaults)
    EVT_LIST_ITEM_SELECTED(ID_PROCEDURAL_LAYER_SELECT, ProceduralGeneratorDialog::OnLayerSelect)
END_EVENT_TABLE()

ProceduralPreviewButton::ProceduralPreviewButton(wxWindow* parent) :
    DCButton(parent, wxID_ANY, wxDefaultPosition, DC_BTN_TOGGLE, RENDER_SIZE_32x32, 0),
    m_id(0) {
    ////
}

void ProceduralPreviewButton::SetItemId(uint16_t id) {
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

ProceduralGeneratorDialog::ProceduralGeneratorDialog(wxWindow* parent) :
    wxDialog(parent, wxID_ANY, "Procedural Map Generator", wxDefaultPosition, wxSize(1200, 800), 
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
    seed_input = new wxTextCtrl(basic_panel, wxID_ANY, "default");
    random_seed_button = new wxButton(basic_panel, ID_PROCEDURAL_RANDOM_SEED, "Random", wxDefaultPosition, wxSize(80, -1));
    seed_row->Add(seed_input, 1, wxEXPAND | wxRIGHT, 5);
    seed_row->Add(random_seed_button, 0);
    gen_grid->Add(seed_row, 1, wxEXPAND);
    
    gen_grid->Add(new wxStaticText(basic_panel, wxID_ANY, "Width:"), 0, wxALIGN_CENTER_VERTICAL);
    width_spin = new wxSpinCtrl(basic_panel, wxID_ANY, "200", wxDefaultPosition, wxDefaultSize, 
                               wxSP_ARROW_KEYS, 50, 2000, 200);
    gen_grid->Add(width_spin, 1, wxEXPAND);

    gen_grid->Add(new wxStaticText(basic_panel, wxID_ANY, "Height:"), 0, wxALIGN_CENTER_VERTICAL);
    height_spin = new wxSpinCtrl(basic_panel, wxID_ANY, "200", wxDefaultPosition, wxDefaultSize, 
                                wxSP_ARROW_KEYS, 50, 2000, 200);
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

    // Terrain Layers Panel
    wxPanel* terrain_panel = new wxPanel(notebook);
    wxBoxSizer* terrain_main_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Terrain layer list
    wxStaticBoxSizer* layer_list_sizer = new wxStaticBoxSizer(wxVERTICAL, terrain_panel, "Terrain Layers");
    terrain_layer_list = new wxListCtrl(terrain_panel, ID_PROCEDURAL_LAYER_SELECT, wxDefaultPosition, wxSize(300, 200), 
                                        wxLC_REPORT | wxLC_SINGLE_SEL);
    terrain_layer_list->InsertColumn(0, "Layer", wxLIST_FORMAT_LEFT, 100);
    terrain_layer_list->InsertColumn(1, "Brush", wxLIST_FORMAT_LEFT, 100);
    terrain_layer_list->InsertColumn(2, "Height", wxLIST_FORMAT_LEFT, 80);
    layer_list_sizer->Add(terrain_layer_list, 1, wxEXPAND | wxALL, 5);
    
    // Layer management buttons
    wxBoxSizer* layer_buttons = new wxBoxSizer(wxHORIZONTAL);
    add_layer_button = new wxButton(terrain_panel, ID_PROCEDURAL_LAYER_ADD, "Add");
    remove_layer_button = new wxButton(terrain_panel, ID_PROCEDURAL_LAYER_REMOVE, "Remove");
    edit_layer_button = new wxButton(terrain_panel, ID_PROCEDURAL_LAYER_EDIT, "Edit");
    layer_buttons->Add(add_layer_button, 0, wxRIGHT, 5);
    layer_buttons->Add(remove_layer_button, 0, wxRIGHT, 5);
    layer_buttons->Add(edit_layer_button, 0);
    layer_list_sizer->Add(layer_buttons, 0, wxALIGN_CENTER | wxALL, 5);
    
    terrain_main_sizer->Add(layer_list_sizer, 1, wxEXPAND | wxALL, 5);
    
    // Layer configuration
    wxStaticBoxSizer* layer_config_sizer = new wxStaticBoxSizer(wxVERTICAL, terrain_panel, "Layer Configuration");
    wxFlexGridSizer* config_grid = new wxFlexGridSizer(7, 2, 5, 5);
    
    config_grid->Add(new wxStaticText(terrain_panel, wxID_ANY, "Brush:"), 0, wxALIGN_CENTER_VERTICAL);
    layer_brush_choice = new wxChoice(terrain_panel, wxID_ANY);
    PopulateBrushChoices();
    config_grid->Add(layer_brush_choice, 1, wxEXPAND);
    
    config_grid->Add(new wxStaticText(terrain_panel, wxID_ANY, "Item ID:"), 0, wxALIGN_CENTER_VERTICAL);
    layer_item_id_spin = new wxSpinCtrl(terrain_panel, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize, 
                                       wxSP_ARROW_KEYS, 1, 65535, 100);
    config_grid->Add(layer_item_id_spin, 1, wxEXPAND);
    
    config_grid->Add(new wxStaticText(terrain_panel, wxID_ANY, "Height Min:"), 0, wxALIGN_CENTER_VERTICAL);
    layer_height_min_text = new wxTextCtrl(terrain_panel, wxID_ANY, "0.0");
    config_grid->Add(layer_height_min_text, 1, wxEXPAND);
    
    config_grid->Add(new wxStaticText(terrain_panel, wxID_ANY, "Height Max:"), 0, wxALIGN_CENTER_VERTICAL);
    layer_height_max_text = new wxTextCtrl(terrain_panel, wxID_ANY, "1.0");
    config_grid->Add(layer_height_max_text, 1, wxEXPAND);
    
    config_grid->Add(new wxStaticText(terrain_panel, wxID_ANY, "Moisture Min:"), 0, wxALIGN_CENTER_VERTICAL);
    layer_moisture_min_text = new wxTextCtrl(terrain_panel, wxID_ANY, "0.0");
    config_grid->Add(layer_moisture_min_text, 1, wxEXPAND);
    
    config_grid->Add(new wxStaticText(terrain_panel, wxID_ANY, "Moisture Max:"), 0, wxALIGN_CENTER_VERTICAL);
    layer_moisture_max_text = new wxTextCtrl(terrain_panel, wxID_ANY, "1.0");
    config_grid->Add(layer_moisture_max_text, 1, wxEXPAND);
    
    config_grid->Add(new wxStaticText(terrain_panel, wxID_ANY, "Probability:"), 0, wxALIGN_CENTER_VERTICAL);
    layer_probability_text = new wxTextCtrl(terrain_panel, wxID_ANY, "1.0");
    config_grid->Add(layer_probability_text, 1, wxEXPAND);
    
    config_grid->AddGrowableCol(1);
    layer_config_sizer->Add(config_grid, 0, wxEXPAND | wxALL, 5);
    
    terrain_main_sizer->Add(layer_config_sizer, 1, wxEXPAND | wxALL, 5);
    terrain_panel->SetSizer(terrain_main_sizer);
    notebook->AddPage(terrain_panel, "Terrain Layers");

    // Noise Settings Panel
    wxPanel* noise_panel = new wxPanel(notebook);
    wxBoxSizer* noise_sizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticBoxSizer* noise_config_sizer = new wxStaticBoxSizer(wxVERTICAL, noise_panel, "Noise Configuration");
    wxFlexGridSizer* noise_grid = new wxFlexGridSizer(4, 2, 5, 5);
    
    noise_grid->Add(new wxStaticText(noise_panel, wxID_ANY, "Scale:"), 0, wxALIGN_CENTER_VERTICAL);
    noise_scale_text = new wxTextCtrl(noise_panel, wxID_ANY, "0.01");
    noise_grid->Add(noise_scale_text, 1, wxEXPAND);
    
    noise_grid->Add(new wxStaticText(noise_panel, wxID_ANY, "Octaves:"), 0, wxALIGN_CENTER_VERTICAL);
    noise_octaves_spin = new wxSpinCtrl(noise_panel, wxID_ANY, "6", wxDefaultPosition, wxDefaultSize, 
                                       wxSP_ARROW_KEYS, 1, 10, 6);
    noise_grid->Add(noise_octaves_spin, 1, wxEXPAND);
    
    noise_grid->Add(new wxStaticText(noise_panel, wxID_ANY, "Persistence:"), 0, wxALIGN_CENTER_VERTICAL);
    noise_persistence_text = new wxTextCtrl(noise_panel, wxID_ANY, "0.5");
    noise_grid->Add(noise_persistence_text, 1, wxEXPAND);
    
    noise_grid->Add(new wxStaticText(noise_panel, wxID_ANY, "Lacunarity:"), 0, wxALIGN_CENTER_VERTICAL);
    noise_lacunarity_text = new wxTextCtrl(noise_panel, wxID_ANY, "2.0");
    noise_grid->Add(noise_lacunarity_text, 1, wxEXPAND);
    
    noise_grid->AddGrowableCol(1);
    noise_config_sizer->Add(noise_grid, 0, wxEXPAND | wxALL, 5);
    noise_sizer->Add(noise_config_sizer, 0, wxEXPAND | wxALL, 5);
    
    // Cave Generation
    wxStaticBoxSizer* cave_sizer = new wxStaticBoxSizer(wxVERTICAL, noise_panel, "Cave Generation");
    enable_caves_checkbox = new wxCheckBox(noise_panel, wxID_ANY, "Enable cave generation");
    cave_sizer->Add(enable_caves_checkbox, 0, wxALL, 5);
    
    wxFlexGridSizer* cave_grid = new wxFlexGridSizer(4, 2, 5, 5);
    
    cave_grid->Add(new wxStaticText(noise_panel, wxID_ANY, "Probability:"), 0, wxALIGN_CENTER_VERTICAL);
    cave_probability_text = new wxTextCtrl(noise_panel, wxID_ANY, "0.1");
    cave_grid->Add(cave_probability_text, 1, wxEXPAND);
    
    cave_grid->Add(new wxStaticText(noise_panel, wxID_ANY, "Min Size:"), 0, wxALIGN_CENTER_VERTICAL);
    cave_min_size_spin = new wxSpinCtrl(noise_panel, wxID_ANY, "3", wxDefaultPosition, wxDefaultSize, 
                                       wxSP_ARROW_KEYS, 1, 20, 3);
    cave_grid->Add(cave_min_size_spin, 1, wxEXPAND);
    
    cave_grid->Add(new wxStaticText(noise_panel, wxID_ANY, "Max Size:"), 0, wxALIGN_CENTER_VERTICAL);
    cave_max_size_spin = new wxSpinCtrl(noise_panel, wxID_ANY, "10", wxDefaultPosition, wxDefaultSize, 
                                       wxSP_ARROW_KEYS, 1, 50, 10);
    cave_grid->Add(cave_max_size_spin, 1, wxEXPAND);
    
    cave_grid->Add(new wxStaticText(noise_panel, wxID_ANY, "Cave Brush:"), 0, wxALIGN_CENTER_VERTICAL);
    cave_brush_choice = new wxChoice(noise_panel, wxID_ANY);
    cave_brush_choice->Append("None");
    cave_grid->Add(cave_brush_choice, 1, wxEXPAND);
    
    cave_grid->AddGrowableCol(1);
    cave_sizer->Add(cave_grid, 0, wxEXPAND | wxALL, 5);
    noise_sizer->Add(cave_sizer, 0, wxEXPAND | wxALL, 5);
    
    noise_panel->SetSizer(noise_sizer);
    notebook->AddPage(noise_panel, "Noise & Caves");

    // Main layout
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(notebook, 1, wxEXPAND | wxALL, 5);
    
    // Button panel
    wxPanel* button_panel = new wxPanel(this);
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    reset_defaults_button = new wxButton(button_panel, ID_PROCEDURAL_RESET_DEFAULTS, "Reset Defaults");
    preview_button = new wxButton(button_panel, ID_PROCEDURAL_PREVIEW, "Preview");
    generate_button = new wxButton(button_panel, ID_PROCEDURAL_GENERATE, "Generate");
    cancel_button = new wxButton(button_panel, ID_PROCEDURAL_CANCEL, "Cancel");
    
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
    seed = "default";
    LoadDefaultTerrainLayers();
    UpdateWidgets();
}

ProceduralGeneratorDialog::~ProceduralGeneratorDialog() {
    // Clean up any allocated resources if needed
}

void ProceduralGeneratorDialog::OnGenerateClick(wxCommandEvent& event) {
    GenerateProceduralMap();
}

void ProceduralGeneratorDialog::OnCancelClick(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void ProceduralGeneratorDialog::OnPreviewClick(wxCommandEvent& event) {
    UpdatePreview();
}

void ProceduralGeneratorDialog::OnRandomizeSeed(wxCommandEvent& event) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    seed = wxString::Format("seed_%d", dis(gen));
    seed_input->SetValue(seed);
}

void ProceduralGeneratorDialog::OnLayerSelect(wxListEvent& event) {
    UpdateLayerControls();
}

void ProceduralGeneratorDialog::OnLayerAdd(wxCommandEvent& event) {
    // Add new terrain layer
    TerrainLayer layer;
    layer.name = "New Layer";
    layer.brush_name = "grass";
    layer.item_id = 100;
    layer.height_min = 0.0f;
    layer.height_max = 1.0f;
    layer.moisture_min = 0.0f;
    layer.moisture_max = 1.0f;
    layer.probability = 1.0f;
    
    terrain_layers.push_back(layer);
    PopulateTerrainLayerList();
}

void ProceduralGeneratorDialog::OnLayerRemove(wxCommandEvent& event) {
    long selected = terrain_layer_list->GetFirstSelected();
    if (selected != -1 && selected < (long)terrain_layers.size()) {
        terrain_layers.erase(terrain_layers.begin() + selected);
        PopulateTerrainLayerList();
    }
}

void ProceduralGeneratorDialog::OnLayerEdit(wxCommandEvent& event) {
    // Implementation for editing layer properties
    // This would open a detailed editing dialog
}

void ProceduralGeneratorDialog::OnResetDefaults(wxCommandEvent& event) {
    LoadDefaultTerrainLayers();
    UpdateWidgets();
}

void ProceduralGeneratorDialog::OnParameterChange(wxCommandEvent& event) {
    // Handle parameter changes
}

void ProceduralGeneratorDialog::OnParameterSpin(wxSpinEvent& event) {
    // Handle spin control changes
}

void ProceduralGeneratorDialog::UpdateWidgets() {
    seed_input->SetValue(seed);
    // Update other UI controls with current values
}

void ProceduralGeneratorDialog::UpdatePreview() {
    // Generate a small preview of the map
    // This would create a mini-map preview
}

void ProceduralGeneratorDialog::GenerateProceduralMap() {
    if (!g_gui.IsEditorOpen()) {
        wxMessageBox("No map is currently open.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Build configuration
    config = BuildProceduralConfig();
    
    // Show progress
    progress->Show();
    progress->SetValue(0);
    Layout();
    
    try {
        // Create generator
        OTMapGeneratorProcedural generator;
        
        // Configure generator
        generator.SetSeed(std::string(seed.mb_str()));
        
        // Get current map
        Editor* editor = g_gui.GetCurrentEditor();
        if (!editor) {
            throw std::runtime_error("No editor available");
        }
        
        Map& map = editor->map;
        
        // Generate map
        bool success = generator.Generate(map, config, start_position.x, start_position.y, start_position.z,
                                        width_spin->GetValue(), height_spin->GetValue());
        
        if (success) {
            progress->SetValue(100);
            wxMessageBox("Procedural map generated successfully!", "Success", wxOK | wxICON_INFORMATION);
            
            // Refresh view
            g_gui.RefreshView();
            g_gui.UpdateMinimap();
            
            EndModal(wxID_OK);
        } else {
            throw std::runtime_error("Failed to generate procedural map");
        }
    } catch (const std::exception& e) {
        progress->Hide();
        wxMessageBox(wxString::Format("Error generating map: %s", e.what()), "Error", wxOK | wxICON_ERROR);
    }
    
    progress->Hide();
    Layout();
}

void ProceduralGeneratorDialog::PopulateTerrainLayerList() {
    terrain_layer_list->DeleteAllItems();
    
    for (size_t i = 0; i < terrain_layers.size(); ++i) {
        const TerrainLayer& layer = terrain_layers[i];
        long item = terrain_layer_list->InsertItem(i, wxstr(layer.name));
        terrain_layer_list->SetItem(item, 1, wxstr(layer.brush_name));
        terrain_layer_list->SetItem(item, 2, wxString::Format("%.2f-%.2f", layer.height_min, layer.height_max));
    }
}

void ProceduralGeneratorDialog::PopulateBrushChoices() {
    layer_brush_choice->Clear();
    layer_brush_choice->Append("grass");
    layer_brush_choice->Append("dirt");
    layer_brush_choice->Append("stone");
    layer_brush_choice->Append("sand");
    layer_brush_choice->Append("water");
    layer_brush_choice->SetSelection(0);
}

void ProceduralGeneratorDialog::UpdateLayerControls() {
    long selected = terrain_layer_list->GetFirstSelected();
    if (selected != -1 && selected < (long)terrain_layers.size()) {
        const TerrainLayer& layer = terrain_layers[selected];
        
        // Update controls with selected layer values
        layer_item_id_spin->SetValue(layer.item_id);
        layer_height_min_text->SetValue(wxString::Format("%.3f", layer.height_min));
        layer_height_max_text->SetValue(wxString::Format("%.3f", layer.height_max));
        layer_moisture_min_text->SetValue(wxString::Format("%.3f", layer.moisture_min));
        layer_moisture_max_text->SetValue(wxString::Format("%.3f", layer.moisture_max));
        layer_probability_text->SetValue(wxString::Format("%.3f", layer.probability));
        
        // Find and select brush
        int brush_index = layer_brush_choice->FindString(wxstr(layer.brush_name));
        if (brush_index != wxNOT_FOUND) {
            layer_brush_choice->SetSelection(brush_index);
        }
    }
}

void ProceduralGeneratorDialog::LoadDefaultTerrainLayers() {
    terrain_layers.clear();
    
    // Water layer
    TerrainLayer water;
    water.name = "Water";
    water.brush_name = "water";
    water.item_id = 4820; // water tile ID
    water.height_min = 0.0f;
    water.height_max = 0.3f;
    water.moisture_min = 0.0f;
    water.moisture_max = 1.0f;
    water.probability = 1.0f;
    terrain_layers.push_back(water);
    
    // Sand/Beach layer
    TerrainLayer sand;
    sand.name = "Sand";
    sand.brush_name = "sand";
    sand.item_id = 231; // sand tile ID
    sand.height_min = 0.3f;
    sand.height_max = 0.4f;
    sand.moisture_min = 0.0f;
    sand.moisture_max = 1.0f;
    sand.probability = 1.0f;
    terrain_layers.push_back(sand);
    
    // Grass layer
    TerrainLayer grass;
    grass.name = "Grass";
    grass.brush_name = "grass";
    grass.item_id = 100; // grass tile ID
    grass.height_min = 0.4f;
    grass.height_max = 0.8f;
    grass.moisture_min = 0.3f;
    grass.moisture_max = 1.0f;
    grass.probability = 1.0f;
    terrain_layers.push_back(grass);
    
    // Mountain/Stone layer
    TerrainLayer stone;
    stone.name = "Stone";
    stone.brush_name = "stone";
    stone.item_id = 1791; // stone tile ID
    stone.height_min = 0.8f;
    stone.height_max = 1.0f;
    stone.moisture_min = 0.0f;
    stone.moisture_max = 1.0f;
    stone.probability = 1.0f;
    terrain_layers.push_back(stone);
    
    PopulateTerrainLayerList();
}

TerrainLayer* ProceduralGeneratorDialog::GetSelectedLayer() {
    long selected = terrain_layer_list->GetFirstSelected();
    if (selected != -1 && selected < (long)terrain_layers.size()) {
        return &terrain_layers[selected];
    }
    return nullptr;
}

void ProceduralGeneratorDialog::SetSelectedLayer(const TerrainLayer& layer) {
    TerrainLayer* selected_layer = GetSelectedLayer();
    if (selected_layer) {
        *selected_layer = layer;
        PopulateTerrainLayerList();
    }
}

ProceduralConfig ProceduralGeneratorDialog::BuildProceduralConfig() {
    ProceduralConfig cfg;
    
    // Basic settings
    cfg.width = width_spin->GetValue();
    cfg.height = height_spin->GetValue();
    
    // Noise settings
    double scale_val;
    if (noise_scale_text->GetValue().ToDouble(&scale_val)) {
        cfg.noise_scale = static_cast<float>(scale_val);
    }
    cfg.noise_octaves = noise_octaves_spin->GetValue();
    
    double persistence_val;
    if (noise_persistence_text->GetValue().ToDouble(&persistence_val)) {
        cfg.noise_persistence = static_cast<float>(persistence_val);
    }
    
    double lacunarity_val;
    if (noise_lacunarity_text->GetValue().ToDouble(&lacunarity_val)) {
        cfg.noise_lacunarity = static_cast<float>(lacunarity_val);
    }
    
    // Cave settings
    cfg.enable_caves = enable_caves_checkbox->GetValue();
    double cave_prob;
    if (cave_probability_text->GetValue().ToDouble(&cave_prob)) {
        cfg.cave_probability = static_cast<float>(cave_prob);
    }
    cfg.cave_min_size = cave_min_size_spin->GetValue();
    cfg.cave_max_size = cave_max_size_spin->GetValue();
    
    // Terrain layers
    cfg.terrain_layers = terrain_layers;
    
    return cfg;
} 