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
#include "island_generator_dialog.h"
#include "gui.h"
#include "artprovider.h"
#include "items.h"
#include "brush.h"
#include "ground_brush.h"
#include <random>
#include <ctime>

BEGIN_EVENT_TABLE(IslandGeneratorDialog, wxDialog)
    EVT_BUTTON(ID_ISLAND_GENERATE_SINGLE, IslandGeneratorDialog::OnGenerateClick)
    EVT_BUTTON(ID_ISLAND_GENERATE_MULTIPLE, IslandGeneratorDialog::OnGenerateMultiple)
    EVT_BUTTON(ID_ISLAND_CANCEL, IslandGeneratorDialog::OnCancelClick)
    EVT_BUTTON(ID_ISLAND_RANDOM_SEED, IslandGeneratorDialog::OnRandomizeSeed)
    EVT_CHOICE(ID_ISLAND_SHAPE_SELECT, IslandGeneratorDialog::OnShapeSelect)
    EVT_SPINCTRL(ID_ISLAND_SIZE_CHANGE, IslandGeneratorDialog::OnSizeChange)
    EVT_SPINCTRL(ID_ISLAND_ROUGHNESS_CHANGE, IslandGeneratorDialog::OnRoughnessChange)
    EVT_TEXT(ID_ISLAND_SEED_TEXT, IslandGeneratorDialog::OnSeedText)
END_EVENT_TABLE()

IslandPreviewButton::IslandPreviewButton(wxWindow* parent) :
    DCButton(parent, wxID_ANY, wxDefaultPosition, DC_BTN_TOGGLE, RENDER_SIZE_32x32, 0),
    m_id(0) {
    ////
}

void IslandPreviewButton::SetItemId(uint16_t id) {
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

// Since we don't have FastNoise, let's create a simple noise function
class SimpleNoise {
public:
    SimpleNoise() : seed(0) {}
    void SetSeed(int s) { seed = s; }
    
    float GetNoise(float x, float y) {
        int n = seed + static_cast<int>(x * 1997) + static_cast<int>(y * 17931);
        n = (n << 13) ^ n;
        return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
    }
private:
    int seed;
};

IslandGeneratorDialog::IslandGeneratorDialog(wxWindow* parent) :
    wxDialog(parent, wxID_ANY, "Island Generator", wxDefaultPosition, wxSize(800, 700), 
             wxDEFAULT_DIALOG_STYLE) {
    
    // Create scrolled window for content
    wxScrolledWindow* scrolled = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, 
                                                     wxDefaultSize, wxVSCROLL | wxHSCROLL);
    scrolled->SetScrollRate(5, 5);

    // Main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

    // Ground and Water selection row
    wxBoxSizer* type_row = new wxBoxSizer(wxHORIZONTAL);

    // Ground selection with range input
    wxStaticBoxSizer* ground_sizer = new wxStaticBoxSizer(wxVERTICAL, scrolled, "Ground Type");
    ground_button = new IslandPreviewButton(scrolled);
    ground_button->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(IslandGeneratorDialog::OnGroundClick), NULL, this);
    
    wxBoxSizer* ground_input_row = new wxBoxSizer(wxHORIZONTAL);
    ground_input_row->Add(ground_button, 0, wxALL | wxALIGN_CENTER, 5);
    
    ground_range_input = new wxTextCtrl(scrolled, wxID_ANY, "", wxDefaultPosition, wxSize(100, -1));
    ground_range_input->SetToolTip("Enter ground ID or range (e.g., 100-105,200)");
    ground_range_input->Bind(wxEVT_TEXT, &IslandGeneratorDialog::OnIdInput, this);
    ground_input_row->Add(ground_range_input, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    ground_sizer->Add(ground_input_row, 0, wxEXPAND);
    type_row->Add(ground_sizer, 1, wxALL | wxEXPAND, 5);

    // Water selection with range input
    wxStaticBoxSizer* water_sizer = new wxStaticBoxSizer(wxVERTICAL, scrolled, "Water Type");
    water_button = new IslandPreviewButton(scrolled);
    water_button->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(IslandGeneratorDialog::OnWaterClick), NULL, this);
    
    wxBoxSizer* water_input_row = new wxBoxSizer(wxHORIZONTAL);
    water_input_row->Add(water_button, 0, wxALL | wxALIGN_CENTER, 5);
    
    water_range_input = new wxTextCtrl(scrolled, wxID_ANY, "", wxDefaultPosition, wxSize(100, -1));
    water_range_input->SetToolTip("Enter water ID or range (e.g., 100-105,200)");
    water_range_input->Bind(wxEVT_TEXT, &IslandGeneratorDialog::OnIdInput, this);
    water_input_row->Add(water_range_input, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    water_sizer->Add(water_input_row, 0, wxEXPAND);
    type_row->Add(water_sizer, 1, wxALL | wxEXPAND, 5);

    main_sizer->Add(type_row, 0, wxALL | wxEXPAND, 5);

    // Position and Shape row
    wxBoxSizer* pos_shape_row = new wxBoxSizer(wxHORIZONTAL);

    // Position controls
    wxStaticBoxSizer* pos_sizer = new wxStaticBoxSizer(wxVERTICAL, scrolled, "Starting Position");
    wxFlexGridSizer* pos_grid = new wxFlexGridSizer(3, 2, 5, 5);
    
    pos_grid->Add(new wxStaticText(scrolled, wxID_ANY, "X:"), 0, wxALIGN_CENTER_VERTICAL);
    pos_x_spin = new wxSpinCtrl(scrolled, wxID_ANY, "0", wxDefaultPosition, wxSize(100, -1), 
                               wxSP_ARROW_KEYS, 0, 65535, 0);
    pos_grid->Add(pos_x_spin);

    pos_grid->Add(new wxStaticText(scrolled, wxID_ANY, "Y:"), 0, wxALIGN_CENTER_VERTICAL);
    pos_y_spin = new wxSpinCtrl(scrolled, wxID_ANY, "0", wxDefaultPosition, wxSize(100, -1), 
                               wxSP_ARROW_KEYS, 0, 65535, 0);
    pos_grid->Add(pos_y_spin);

    pos_grid->Add(new wxStaticText(scrolled, wxID_ANY, "Z:"), 0, wxALIGN_CENTER_VERTICAL);
    pos_z_spin = new wxSpinCtrl(scrolled, wxID_ANY, "7", wxDefaultPosition, wxSize(100, -1), 
                               wxSP_ARROW_KEYS, 0, 15, 7);
    pos_grid->Add(pos_z_spin);

    pos_sizer->Add(pos_grid, 0, wxALL | wxEXPAND, 5);
    pos_shape_row->Add(pos_sizer, 1, wxALL | wxEXPAND, 5);

    // Shape selection
    wxStaticBoxSizer* shape_sizer = new wxStaticBoxSizer(wxVERTICAL, scrolled, "Island Shape");
    shape_choice = new wxChoice(scrolled, wxID_ANY);
    shape_choice->Append("Circular");
    shape_choice->Append("Square");
    shape_choice->Append("Irregular");
    shape_choice->SetSelection(0);
    shape_sizer->Add(shape_choice, 0, wxALL | wxEXPAND, 5);
    pos_shape_row->Add(shape_sizer, 1, wxALL | wxEXPAND, 5);

    main_sizer->Add(pos_shape_row, 0, wxALL | wxEXPAND, 5);

    // Size and Roughness row
    wxBoxSizer* size_rough_row = new wxBoxSizer(wxHORIZONTAL);

    // Size control
    wxStaticBoxSizer* size_sizer = new wxStaticBoxSizer(wxVERTICAL, scrolled, "Island Size");
    size_spin = new wxSpinCtrl(scrolled, wxID_ANY, "50", wxDefaultPosition, wxDefaultSize, 
                              wxSP_ARROW_KEYS, 10, 500, 50);
    size_sizer->Add(size_spin, 0, wxALL | wxEXPAND, 5);
    size_rough_row->Add(size_sizer, 1, wxALL | wxEXPAND, 5);

    // Roughness control
    wxStaticBoxSizer* roughness_sizer = new wxStaticBoxSizer(wxVERTICAL, scrolled, "Roughness");
    roughness_spin = new wxSpinCtrl(scrolled, wxID_ANY, "50", wxDefaultPosition, wxDefaultSize, 
                                   wxSP_ARROW_KEYS, 0, 100, 50);
    roughness_sizer->Add(roughness_spin, 0, wxALL | wxEXPAND, 5);
    size_rough_row->Add(roughness_sizer, 1, wxALL | wxEXPAND, 5);

    main_sizer->Add(size_rough_row, 0, wxALL | wxEXPAND, 5);

    // Multiple islands controls
    wxStaticBoxSizer* multi_sizer = new wxStaticBoxSizer(wxVERTICAL, scrolled, "Multiple Islands");
    wxFlexGridSizer* multi_grid = new wxFlexGridSizer(2, 2, 5, 5);

    multi_grid->Add(new wxStaticText(scrolled, wxID_ANY, "Count:"), 0, wxALIGN_CENTER_VERTICAL);
    islands_count_spin = new wxSpinCtrl(scrolled, wxID_ANY, "4", wxDefaultPosition, wxDefaultSize,
                                      wxSP_ARROW_KEYS, 1, 100, 4);
    multi_grid->Add(islands_count_spin);

    multi_grid->Add(new wxStaticText(scrolled, wxID_ANY, "Spacing:"), 0, wxALIGN_CENTER_VERTICAL);
    islands_spacing_spin = new wxSpinCtrl(scrolled, wxID_ANY, "10", wxDefaultPosition, wxDefaultSize,
                                        wxSP_ARROW_KEYS, 1, 100, 10);
    multi_grid->Add(islands_spacing_spin);

    multi_sizer->Add(multi_grid, 0, wxALL | wxEXPAND, 5);
    main_sizer->Add(multi_sizer, 0, wxALL | wxEXPAND, 5);

    // Seed input row
    wxStaticBoxSizer* seed_sizer = new wxStaticBoxSizer(wxHORIZONTAL, scrolled, "Generation Seed");
    seed_input = new wxTextCtrl(scrolled, wxID_ANY);
    random_seed_button = new wxButton(scrolled, wxID_ANY, "Random");
    seed_sizer->Add(seed_input, 1, wxALL | wxEXPAND, 5);
    seed_sizer->Add(random_seed_button, 0, wxALL, 5);
    main_sizer->Add(seed_sizer, 0, wxALL | wxEXPAND, 5);

    // Automagic checkbox
    use_automagic = new wxCheckBox(scrolled, wxID_ANY, "Use Automagic for Borders");
    use_automagic->SetValue(true);
    main_sizer->Add(use_automagic, 0, wxALL | wxEXPAND, 5);

    // Preview area
    wxStaticBoxSizer* preview_sizer = new wxStaticBoxSizer(wxVERTICAL, scrolled, "Preview");
    preview_bitmap = new wxStaticBitmap(scrolled, wxID_ANY, 
                                       wxBitmap(200, 200, wxBITMAP_SCREEN_DEPTH));
    preview_sizer->Add(preview_bitmap, 0, wxALL | wxEXPAND, 5);
    main_sizer->Add(preview_sizer, 1, wxALL | wxEXPAND, 5);

    // Progress bar
    progress = new wxGauge(scrolled, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 20));
    main_sizer->Add(progress, 0, wxALL | wxEXPAND, 5);

    // Buttons row
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    generate_button = new wxButton(scrolled, ID_ISLAND_GENERATE_SINGLE, "Generate Single");
    generate_multiple_button = new wxButton(scrolled, ID_ISLAND_GENERATE_MULTIPLE, "Generate Multiple");
    cancel_button = new wxButton(scrolled, ID_ISLAND_CANCEL, "Cancel");
    button_sizer->Add(generate_button, 1, wxALL, 5);
    button_sizer->Add(generate_multiple_button, 1, wxALL, 5);
    button_sizer->Add(cancel_button, 1, wxALL, 5);
    main_sizer->Add(button_sizer, 0, wxALL | wxEXPAND, 5);

    // Set sizer for scrolled window
    scrolled->SetSizer(main_sizer);

    // Dialog sizer
    wxBoxSizer* dialog_sizer = new wxBoxSizer(wxVERTICAL);
    dialog_sizer->Add(scrolled, 1, wxEXPAND | wxALL, 5);
    SetSizer(dialog_sizer);

    // Initialize values
    ground_id = 0;
    water_id = 0;
    island_size = 50;
    roughness = 50;
    selected_shape = "Circular";
    seed = wxString::Format("%d", wxGetLocalTime());
    seed_input->SetValue(seed);
    start_position = Position(0, 0, 7);

    // Initial state
    UpdateWidgets();
    Centre(wxBOTH);
}

IslandGeneratorDialog::~IslandGeneratorDialog() {
    ground_button->Disconnect(wxEVT_LEFT_DOWN, wxMouseEventHandler(IslandGeneratorDialog::OnGroundClick), NULL, this);
    water_button->Disconnect(wxEVT_LEFT_DOWN, wxMouseEventHandler(IslandGeneratorDialog::OnWaterClick), NULL, this);
}

void IslandGeneratorDialog::OnGroundClick(wxMouseEvent& event) {
    const Brush* brush = g_gui.GetCurrentBrush();
    if (brush && brush->isGround()) {
        const GroundBrush* ground = dynamic_cast<const GroundBrush*>(brush);
        if (ground) {
            uint16_t id = ground->getID();
            ground_button->SetItemId(id);
            ground_id = id;
            ground_range_input->SetValue(wxString::Format("%d", id));
            UpdatePreview();
        }
    }
    UpdateWidgets();
}

void IslandGeneratorDialog::OnWaterClick(wxMouseEvent& event) {
    const Brush* brush = g_gui.GetCurrentBrush();
    if (brush && brush->isGround()) {
        const GroundBrush* ground = dynamic_cast<const GroundBrush*>(brush);
        if (ground) {
            uint16_t id = ground->getID();
            water_button->SetItemId(id);
            water_id = id;
            water_range_input->SetValue(wxString::Format("%d", id));
            UpdatePreview();
        }
    }
    UpdateWidgets();
}

void IslandGeneratorDialog::OnGenerateClick(wxCommandEvent& event) {
    if (ground_id == 0 || water_id == 0) {
        wxMessageBox("Please select both ground and water types!", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Update start position from controls
    start_position.x = pos_x_spin->GetValue();
    start_position.y = pos_y_spin->GetValue();
    start_position.z = pos_z_spin->GetValue();

    GenerateIsland();
}

void IslandGeneratorDialog::OnGenerateMultiple(wxCommandEvent& event) {
    if (ground_id == 0 || water_id == 0) {
        wxMessageBox("Please select both ground and water types!", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Update start position from controls
    start_position.x = pos_x_spin->GetValue();
    start_position.y = pos_y_spin->GetValue();
    start_position.z = pos_z_spin->GetValue();

    int count = islands_count_spin->GetValue();
    int spacing = islands_spacing_spin->GetValue();

    GenerateMultipleIslands(count, spacing);
}

void IslandGeneratorDialog::OnCancelClick(wxCommandEvent& event) {
    Close();
}

void IslandGeneratorDialog::OnShapeSelect(wxCommandEvent& event) {
    selected_shape = event.GetString();
    UpdatePreview();
}

void IslandGeneratorDialog::OnSeedText(wxCommandEvent& event) {
    seed = event.GetString();
    UpdatePreview();
}

void IslandGeneratorDialog::OnSizeChange(wxSpinEvent& event) {
    island_size = event.GetValue();
    UpdatePreview();
}

void IslandGeneratorDialog::OnRoughnessChange(wxSpinEvent& event) {
    roughness = event.GetValue();
    UpdatePreview();
}

void IslandGeneratorDialog::OnRandomizeSeed(wxCommandEvent& event) {
    seed = wxString::Format("%d", wxGetLocalTime());
    seed_input->SetValue(seed);
    UpdatePreview();
}

void IslandGeneratorDialog::UpdateWidgets() {
    generate_button->Enable(ground_id != 0 && water_id != 0);
}

void IslandGeneratorDialog::UpdatePreview() {
    // TODO: Generate a small preview of the island shape
    // This will be implemented in the next step
}

void IslandGeneratorDialog::GenerateIsland() {
    Editor* editor = g_gui.GetCurrentEditor();
    if (!editor) return;

    // Create action to hold all changes
    Action* action = editor->actionQueue->createAction(ACTION_DRAW);
    
    // Calculate island dimensions based on size
    int radius = island_size / 2;
    int start_x = start_position.x - radius;
    int start_y = start_position.y - radius;
    int end_x = start_position.x + radius;
    int end_y = start_position.y + radius;

    // Initialize random number generator with seed
    std::mt19937 rng(std::hash<std::string>{}(nstr(seed)));

    // Create noise generator
    SimpleNoise noise;
    noise.SetSeed(rng());

    // Generate the island
    progress->SetValue(0);
    int total_tiles = (end_x - start_x + 1) * (end_y - start_y + 1);
    int current_tile = 0;

    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            // Update progress
            current_tile++;
            progress->SetValue((current_tile * 100) / total_tiles);

            // Calculate distance from center for circular/square shapes
            float center_x = start_position.x;
            float center_y = start_position.y;
            float dx = (x - center_x) / radius;
            float dy = (y - center_y) / radius;

            bool should_place = false;
            
            if (selected_shape == "Circular") {
                // For circular islands, use distance from center
                float distance = std::sqrt(dx * dx + dy * dy);
                should_place = distance <= 1.0f;
                
                // Add some roughness to the edge
                if (std::abs(distance - 1.0f) < (roughness / 100.0f)) {
                    should_place = (rng() % 100) < (100 - roughness);
                }
            }
            else if (selected_shape == "Square") {
                // For square islands, use max of x/y distance
                float distance = std::max(std::abs(dx), std::abs(dy));
                should_place = distance <= 1.0f;
                
                // Add some roughness to the edge
                if (std::abs(distance - 1.0f) < (roughness / 100.0f)) {
                    should_place = (rng() % 100) < (100 - roughness);
                }
            }
            else if (selected_shape == "Irregular") {
                // For irregular islands, combine noise with base shape
                float distance = std::sqrt(dx * dx + dy * dy);
                float noise_val = (noise.GetNoise(x * 0.1f, y * 0.1f) + 1.0f) * 0.5f;
                noise_val = noise_val * (roughness / 100.0f);
                should_place = distance <= (1.0f + noise_val);
            }

            if (should_place) {
                // Create or get the tile
                Tile* tile = editor->map.getTile(x, y, start_position.z);
                if (!tile) {
                    tile = editor->map.createTile(x, y, start_position.z);
                }

                // Create a copy for the action
                Tile* new_tile = tile->deepCopy(editor->map);

                // Set the ground
                if (ground_id > 0) {
                    Item* ground_item = Item::Create(ground_id);
                    new_tile->addItem(ground_item);
                }

                // Add the change to the action
                action->addChange(newd Change(new_tile));
            } else {
                // Place water around the island
                Tile* tile = editor->map.getTile(x, y, start_position.z);
                if (!tile) {
                    tile = editor->map.createTile(x, y, start_position.z);
                }

                Tile* new_tile = tile->deepCopy(editor->map);
                
                if (water_id > 0) {
                    Item* water_item = Item::Create(water_id);
                    new_tile->addItem(water_item);
                }

                action->addChange(newd Change(new_tile));
            }
        }
    }

    // Add the action to the editor
    editor->addAction(action);

    // Apply borders if automagic is enabled
    if (use_automagic->GetValue()) {
        editor->borderizeSelection();
    }

    // Refresh the view
    g_gui.RefreshView();
}

void IslandGeneratorDialog::GenerateMultipleIslands(int count, int spacing) {
    Editor* editor = g_gui.GetCurrentEditor();
    if (!editor) return;

    // Calculate grid dimensions
    size_t grid_size = static_cast<size_t>(std::ceil(std::sqrt(static_cast<double>(count))));
    size_t spacing_tiles = static_cast<size_t>(spacing * TileSize); // Convert spacing to tiles
    size_t island_size_t = static_cast<size_t>(island_size);

    // Store original position
    Position original_pos = start_position;

    // Generate islands in a grid pattern
    size_t islands_created = 0;
    progress->SetValue(0);

    for (size_t row = 0; row < grid_size && islands_created < static_cast<size_t>(count); ++row) {
        for (size_t col = 0; col < grid_size && islands_created < static_cast<size_t>(count); ++col) {
            // Calculate new start position for this island
            start_position.x = original_pos.x + static_cast<int>(col * (island_size_t + spacing_tiles));
            start_position.y = original_pos.y + static_cast<int>(row * (island_size_t + spacing_tiles));

            // Update progress
            progress->SetValue(static_cast<int>((islands_created * 100) / static_cast<size_t>(count)));

            // Generate the island
            GenerateIsland();

            islands_created++;
        }
    }

    // Restore original position
    start_position = original_pos;

    // Final progress update
    progress->SetValue(100);

    // Refresh view
    g_gui.RefreshView();
}

wxString IslandGeneratorDialog::GetDataDirectoryForVersion(const wxString& versionStr) {
    pugi::xml_document clientsDoc;
    wxString clientsPath = g_gui.GetDataDirectory() + "/clients.xml";
    if(clientsDoc.load_file(clientsPath.mb_str())) {
        for(pugi::xml_node clientNode = clientsDoc.child("client_config").child("clients").child("client"); 
            clientNode; clientNode = clientNode.next_sibling("client")) {
            if(versionStr == clientNode.attribute("name").value()) {
                return wxString(clientNode.attribute("data_directory").value());
            }
        }
    }
    return wxString();
}

std::vector<std::pair<uint16_t, uint16_t>> IslandGeneratorDialog::ParseRangeString(const wxString& input) {
    std::vector<std::pair<uint16_t, uint16_t>> ranges;
    wxStringTokenizer tokenizer(input, ",");
    
    while (tokenizer.HasMoreTokens()) {
        wxString token = tokenizer.GetNextToken().Trim();
        
        if (token.Contains("-")) {
            // Handle range (e.g., "100-105")
            long start, end;
            wxString startStr = token.Before('-').Trim();
            wxString endStr = token.After('-').Trim();
            
            if (startStr.ToLong(&start) && endStr.ToLong(&end) && 
                start > 0 && end > 0 && start <= end && end <= 65535) {
                ranges.push_back({static_cast<uint16_t>(start), static_cast<uint16_t>(end)});
            }
        } else {
            // Handle single number
            long id;
            if (token.ToLong(&id) && id > 0 && id <= 65535) {
                ranges.push_back({static_cast<uint16_t>(id), static_cast<uint16_t>(id)});
            }
        }
    }
    
    return ranges;
}

void IslandGeneratorDialog::OnIdInput(wxCommandEvent& event) {
    wxTextCtrl* input = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
    if (!input) return;

    wxString value = input->GetValue().Trim();
    if (value.IsEmpty()) {
        if (input == ground_range_input) {
            ground_id = 0;
            ground_button->SetItemId(0);
        } else if (input == water_range_input) {
            water_id = 0;
            water_button->SetItemId(0);
        }
        UpdateWidgets();
        return;
    }

    auto ranges = ParseRangeString(value);
    if (!ranges.empty()) {
        // Use the first ID in the range
        uint16_t id = ranges[0].first;
        const ItemType& type = g_items.getItemType(id);
        
        if (type.id != 0) {
            if (input == ground_range_input) {
                ground_id = id;
                ground_button->SetItemId(id);
            } else if (input == water_range_input) {
                water_id = id;
                water_button->SetItemId(id);
            }
        }
    }
    
    UpdateWidgets();
} 
