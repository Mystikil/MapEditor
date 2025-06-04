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

#ifndef RME_DUNGEON_GENERATOR_DIALOG_H_
#define RME_DUNGEON_GENERATOR_DIALOG_H_

#include "main.h"
#include "common_windows.h"
#include "dcbutton.h"
#include "editor.h"
#include "brush.h"
#include "ground_brush.h"
#include "otmapgen_dungeon.h"

// Event IDs
enum {
    ID_DUNGEON_GENERATE = 36000,
    ID_DUNGEON_CANCEL,
    ID_DUNGEON_PREVIEW,
    ID_DUNGEON_RANDOM_SEED,
    ID_DUNGEON_ROOM_TYPE_CHANGE,
    ID_DUNGEON_BRUSH_CHANGE,
    ID_DUNGEON_RESET_DEFAULTS
};

class DungeonPreviewButton : public DCButton {
public:
    DungeonPreviewButton(wxWindow* parent);
    ~DungeonPreviewButton() { }

    uint16_t GetItemId() const { return m_id; }
    void SetItemId(uint16_t id);

private:
    uint16_t m_id;
};

class DungeonGeneratorDialog : public wxDialog {
public:
    DungeonGeneratorDialog(wxWindow* parent);
    ~DungeonGeneratorDialog();

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
    void OnRoomTypeChange(wxCommandEvent& event);
    void OnBrushChange(wxCommandEvent& event);
    void OnResetDefaults(wxCommandEvent& event);
    void OnParameterChange(wxCommandEvent& event);
    void OnParameterSpin(wxSpinEvent& event);
    void OnWallClick(wxMouseEvent& event);

private:
    void UpdateWidgets();
    void UpdatePreview();
    void GenerateDungeonMap();
    void PopulateBrushChoices();
    void LoadDefaultSettings();
    DungeonConfig BuildDungeonConfig();

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
    
    // Room Configuration
    wxSpinCtrl* room_count_spin;
    wxSpinCtrl* room_min_size_spin;
    wxSpinCtrl* room_max_size_spin;
    wxCheckBox* circular_rooms_checkbox;
    
    // Corridor Configuration
    wxSpinCtrl* corridor_width_spin;
    wxSpinCtrl* corridor_count_spin;
    wxSpinCtrl* max_corridor_length_spin;
    wxCheckBox* add_dead_ends_checkbox;
    wxCheckBox* smart_pathfinding_checkbox;
    
    // Intersection Configuration
    wxCheckBox* triple_intersections_checkbox;
    wxCheckBox* quad_intersections_checkbox;
    wxSpinCtrl* intersection_count_spin;
    wxSpinCtrl* intersection_size_spin;
    wxTextCtrl* intersection_probability_text;
    wxCheckBox* prefer_intersections_checkbox;
    
    // Wall and Brush Configuration
    DungeonPreviewButton* wall_button;
    wxChoice* wall_brush_choice;
    wxTextCtrl* complexity_text;
    
    // Generation Controls
    wxButton* generate_button;
    wxButton* preview_button;
    wxButton* cancel_button;
    wxButton* reset_defaults_button;
    
    // Progress and Preview
    wxGauge* progress;
    wxStaticBitmap* preview_bitmap;
    
    // Generation parameters and data
    DungeonConfig config;
    Position start_position;
    wxString seed;
    uint16_t wall_id;

    DECLARE_EVENT_TABLE()
};

#endif // RME_DUNGEON_GENERATOR_DIALOG_H_ 