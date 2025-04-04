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

#ifndef RME_ISLAND_GENERATOR_DIALOG_H_
#define RME_ISLAND_GENERATOR_DIALOG_H_

#include "main.h"
#include "common_windows.h"
#include "dcbutton.h"
#include "editor.h"
#include "brush.h"
#include "ground_brush.h"

// Event IDs
enum {
    ID_ISLAND_GENERATE_SINGLE = 34000,
    ID_ISLAND_GENERATE_MULTIPLE,
    ID_ISLAND_CANCEL,
    ID_ISLAND_RANDOM_SEED,
    ID_ISLAND_SHAPE_SELECT,
    ID_ISLAND_SIZE_CHANGE,
    ID_ISLAND_ROUGHNESS_CHANGE,
    ID_ISLAND_SEED_TEXT,
    ID_ISLAND_BORDER_SELECT,
    ID_ISLAND_BORDER_PREVIEW
};

class IslandPreviewButton : public DCButton {
public:
    IslandPreviewButton(wxWindow* parent);
    ~IslandPreviewButton() { }

    uint16_t GetItemId() const { return m_id; }
    void SetItemId(uint16_t id);

private:
    uint16_t m_id;
};

class IslandGeneratorDialog : public wxDialog {
public:
    IslandGeneratorDialog(wxWindow* parent);
    ~IslandGeneratorDialog();

    void SetStartPosition(const Position& pos) {
        start_position = pos;
        pos_x_spin->SetValue(pos.x);
        pos_y_spin->SetValue(pos.y);
        pos_z_spin->SetValue(pos.z);
    }

    // UI Event Handlers
    void OnGroundClick(wxMouseEvent& event);
    void OnWaterClick(wxMouseEvent& event);
    void OnGenerateClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);
    void OnShapeSelect(wxCommandEvent& event);
    void OnSeedText(wxCommandEvent& event);
    void OnSizeChange(wxSpinEvent& event);
    void OnRoughnessChange(wxSpinEvent& event);
    void OnRandomizeSeed(wxCommandEvent& event);
    void OnPreviewUpdate(wxCommandEvent& event);
    void OnGenerateMultiple(wxCommandEvent& event);
    void OnBorderSelect(wxCommandEvent& event);
    void OnBorderPreview(wxCommandEvent& event);

private:
    void UpdateWidgets();
    void UpdatePreview();
    void GenerateIsland();
    void GenerateMultipleIslands(int count, int spacing);
    wxString GetDataDirectoryForVersion(const wxString& versionStr);
    std::vector<std::pair<uint16_t, uint16_t>> ParseRangeString(const wxString& input);
    void OnIdInput(wxCommandEvent& event);
    void LoadBorderChoices();
    void UpdateBorderPreview();

    struct BorderData {
        wxString name;
        int id;
        std::vector<uint16_t> items;
        wxBitmap preview;
    };

    // UI Controls
    IslandPreviewButton* ground_button;
    IslandPreviewButton* water_button;
    wxTextCtrl* ground_range_input;
    wxTextCtrl* water_range_input;
    wxChoice* shape_choice;
    wxSpinCtrl* size_spin;
    wxSpinCtrl* roughness_spin;
    wxTextCtrl* seed_input;
    wxButton* random_seed_button;
    wxButton* generate_button;
    wxButton* generate_multiple_button;
    wxButton* cancel_button;
    wxGauge* progress;
    wxStaticBitmap* preview_bitmap;
    wxCheckBox* use_automagic;

    // Border controls
    wxListBox* border_list;
    wxStaticBitmap* border_preview;
    std::vector<BorderData> border_data;
    int selected_border_id;

    // Position controls
    wxSpinCtrl* pos_x_spin;
    wxSpinCtrl* pos_y_spin;
    wxSpinCtrl* pos_z_spin;

    // Multiple islands controls
    wxSpinCtrl* islands_count_spin;
    wxSpinCtrl* islands_spacing_spin;

    // Generation parameters
    uint16_t ground_id;
    uint16_t water_id;
    int island_size;
    int roughness;
    wxString seed;
    wxString selected_shape;
    Position start_position;

    DECLARE_EVENT_TABLE()
};

#endif // RME_ISLAND_GENERATOR_DIALOG_H_ 