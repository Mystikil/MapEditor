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
#include "otmapgen_island.h"
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>

// Control IDs
enum {
    ID_ISLAND_SEED_TEXT = wxID_HIGHEST + 1000,
    ID_ISLAND_WIDTH_SPIN,
    ID_ISLAND_HEIGHT_SPIN,
    ID_ISLAND_VERSION_CHOICE,
    ID_ISLAND_NOISE_SCALE,
    ID_ISLAND_NOISE_OCTAVES,
    ID_ISLAND_NOISE_PERSISTENCE,
    ID_ISLAND_NOISE_LACUNARITY,
    ID_ISLAND_SIZE,
    ID_ISLAND_FALLOFF,
    ID_ISLAND_THRESHOLD,
    ID_ISLAND_WATER_LEVEL,
    ID_ISLAND_WATER_ID,
    ID_ISLAND_GROUND_ID,
    ID_ISLAND_ENABLE_CLEANUP,
    ID_ISLAND_MIN_PATCH_SIZE,
    ID_ISLAND_MAX_HOLE_SIZE,
    ID_ISLAND_SMOOTHING_PASSES,
    ID_ISLAND_ROLL_DICE,
    ID_ISLAND_PREVIEW,
    ID_ISLAND_GENERATE,
    ID_ISLAND_PRESET_CHOICE,
    ID_ISLAND_PRESET_SAVE,
    ID_ISLAND_PRESET_DELETE
};

// Preset structure for island generation
struct IslandPreset {
    std::string name;
    IslandConfig config;
    int width;
    int height;
    std::string version;
    std::string seed;
};

class IslandGeneratorDialog : public wxDialog {
public:
    IslandGeneratorDialog(wxWindow* parent);
    virtual ~IslandGeneratorDialog();
    
    // Event handlers
    void OnGenerate(wxCommandEvent& event);
    void OnPreview(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnRollDice(wxCommandEvent& event);
    void OnParameterChange(wxCommandEvent& event);
    void OnParameterSpin(wxSpinEvent& event);
    void OnPresetLoad(wxCommandEvent& event);
    void OnPresetSave(wxCommandEvent& event);
    void OnPresetDelete(wxCommandEvent& event);
    
private:
    // UI Controls
    wxTextCtrl* seed_text_ctrl;
    wxSpinCtrl* width_spin_ctrl;
    wxSpinCtrl* height_spin_ctrl;
    wxChoice* version_choice;
    
    // Noise parameters
    wxTextCtrl* noise_scale_text;
    wxSpinCtrl* noise_octaves_spin;
    wxTextCtrl* noise_persistence_text;
    wxTextCtrl* noise_lacunarity_text;
    
    // Island shape parameters
    wxTextCtrl* island_size_text;
    wxTextCtrl* island_falloff_text;
    wxTextCtrl* island_threshold_text;
    wxTextCtrl* water_level_text;
    
    // Tile configuration
    wxSpinCtrl* water_id_spin;
    wxSpinCtrl* ground_id_spin;
    
    // Cleanup settings
    wxCheckBox* enable_cleanup_checkbox;
    wxSpinCtrl* min_patch_size_spin;
    wxSpinCtrl* max_hole_size_spin;
    wxSpinCtrl* smoothing_passes_spin;
    
    // Preview
    wxStaticBitmap* preview_bitmap;
    wxBitmap* current_preview;
    
    // Buttons
    wxButton* roll_dice_button;
    wxButton* preview_button;
    wxButton* generate_button;
    wxButton* cancel_button;
    
    // Preset management
    wxChoice* preset_choice;
    wxTextCtrl* preset_name_text;
    wxButton* preset_save_button;
    wxButton* preset_delete_button;
    
    // Helper methods
    IslandConfig BuildIslandConfig();
    bool GenerateIslandMap();
    void UpdatePreview();
    void RollIslandDice();
    void LoadPresets();
    void SavePresets();
    void LoadPreset(const std::string& presetName);
    void SaveCurrentAsPreset(const std::string& presetName);
    void GetTilePreviewColor(uint16_t tileId, unsigned char& r, unsigned char& g, unsigned char& b);
    
    // Data
    std::vector<IslandPreset> presets;
    std::string presets_file_path;
    
    DECLARE_EVENT_TABLE()
};

#endif 