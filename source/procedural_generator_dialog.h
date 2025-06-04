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

#ifndef RME_PROCEDURAL_GENERATOR_DIALOG_H_
#define RME_PROCEDURAL_GENERATOR_DIALOG_H_

#include "main.h"
#include "common_windows.h"
#include "dcbutton.h"
#include "editor.h"
#include "brush.h"
#include "ground_brush.h"
#include "otmapgen_procedural.h"

// Event IDs
enum {
    ID_PROCEDURAL_GENERATE = 35000,
    ID_PROCEDURAL_CANCEL,
    ID_PROCEDURAL_PREVIEW,
    ID_PROCEDURAL_RANDOM_SEED,
    ID_PROCEDURAL_LAYER_SELECT,
    ID_PROCEDURAL_LAYER_ADD,
    ID_PROCEDURAL_LAYER_REMOVE,
    ID_PROCEDURAL_LAYER_EDIT,
    ID_PROCEDURAL_RESET_DEFAULTS
};

class ProceduralPreviewButton : public DCButton {
public:
    ProceduralPreviewButton(wxWindow* parent);
    ~ProceduralPreviewButton() { }

    uint16_t GetItemId() const { return m_id; }
    void SetItemId(uint16_t id);

private:
    uint16_t m_id;
};

class ProceduralGeneratorDialog : public wxDialog {
public:
    ProceduralGeneratorDialog(wxWindow* parent);
    ~ProceduralGeneratorDialog();

    void SetStartPosition(const Position& pos) {
        start_position = pos;
        pos_x_spin->SetValue(pos.x);
        pos_y_spin->SetValue(pos.y);
        pos_z_spin->SetValue(pos.z);
    }

    // UI Event Handlers
    void OnGenerateClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);
    void OnPreviewClick(wxCommandEvent& event);
    void OnRandomizeSeed(wxCommandEvent& event);
    void OnLayerSelect(wxListEvent& event);
    void OnLayerAdd(wxCommandEvent& event);
    void OnLayerRemove(wxCommandEvent& event);
    void OnLayerEdit(wxCommandEvent& event);
    void OnResetDefaults(wxCommandEvent& event);
    void OnParameterChange(wxCommandEvent& event);
    void OnParameterSpin(wxSpinEvent& event);

private:
    void UpdateWidgets();
    void UpdatePreview();
    void GenerateProceduralMap();
    void PopulateTerrainLayerList();
    void PopulateBrushChoices();
    void UpdateLayerControls();
    void LoadDefaultTerrainLayers();
    TerrainLayer* GetSelectedLayer();
    void SetSelectedLayer(const TerrainLayer& layer);
    ProceduralConfig BuildProceduralConfig();

    // UI Controls - Basic Settings
    wxTextCtrl* seed_input;
    wxButton* random_seed_button;
    wxSpinCtrl* width_spin;
    wxSpinCtrl* height_spin;
    wxChoice* version_choice;
    
    // Position controls
    wxSpinCtrl* pos_x_spin;
    wxSpinCtrl* pos_y_spin;
    wxSpinCtrl* pos_z_spin;
    
    // Terrain Layer Management
    wxListCtrl* terrain_layer_list;
    wxButton* add_layer_button;
    wxButton* remove_layer_button;
    wxButton* edit_layer_button;
    
    // Layer Configuration
    wxChoice* layer_brush_choice;
    wxSpinCtrl* layer_item_id_spin;
    wxTextCtrl* layer_height_min_text;
    wxTextCtrl* layer_height_max_text;
    wxTextCtrl* layer_moisture_min_text;
    wxTextCtrl* layer_moisture_max_text;
    wxTextCtrl* layer_probability_text;
    
    // Noise Configuration
    wxTextCtrl* noise_scale_text;
    wxSpinCtrl* noise_octaves_spin;
    wxTextCtrl* noise_persistence_text;
    wxTextCtrl* noise_lacunarity_text;
    
    // Cave Generation
    wxCheckBox* enable_caves_checkbox;
    wxTextCtrl* cave_probability_text;
    wxSpinCtrl* cave_min_size_spin;
    wxSpinCtrl* cave_max_size_spin;
    wxChoice* cave_brush_choice;
    
    // Generation Controls
    wxButton* generate_button;
    wxButton* preview_button;
    wxButton* cancel_button;
    wxButton* reset_defaults_button;
    
    // Progress and Preview
    wxGauge* progress;
    wxStaticBitmap* preview_bitmap;
    
    // Generation parameters and data
    ProceduralConfig config;
    std::vector<TerrainLayer> terrain_layers;
    Position start_position;
    wxString seed;

    DECLARE_EVENT_TABLE()
};

#endif // RME_PROCEDURAL_GENERATOR_DIALOG_H_ 