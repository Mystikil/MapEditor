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

#ifndef RME_OTMAPGEN_DIALOG_H_
#define RME_OTMAPGEN_DIALOG_H_

#include "main.h"
#include "otmapgen.h"

class OTMapGenDialog : public wxDialog {
public:
	OTMapGenDialog(wxWindow* parent);
	~OTMapGenDialog();

	// Event handlers
	void OnGenerate(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnPreview(wxCommandEvent& event);
	void OnSeedChange(wxCommandEvent& event);
	void OnParameterChange(wxSpinEvent& event);
	void OnParameterChange(wxScrollEvent& event);
	void OnParameterChangeText(wxCommandEvent& event);
	void OnMountainTypeChange(wxCommandEvent& event);
	void OnFloorUp(wxCommandEvent& event);
	void OnFloorDown(wxCommandEvent& event);
	void OnZoomIn(wxCommandEvent& event);
	void OnZoomOut(wxCommandEvent& event);

	// Helper functions
	bool GenerateMap();
	void UpdatePreview();
	void UpdatePreviewFloor();
	void UpdateFloorLabel();
	void UpdateZoomLabel();
	GenerationConfig BuildGenerationConfig();
	void GetTilePreviewColor(uint16_t tileId, unsigned char& r, unsigned char& g, unsigned char& b);

protected:
	// Dialog controls
	wxNotebook* notebook;
	
	// Basic settings
	wxTextCtrl* seed_text_ctrl;
	wxSpinCtrl* width_spin_ctrl;
	wxSpinCtrl* height_spin_ctrl;
	wxChoice* version_choice;
	wxChoice* mountain_type_choice;
	wxCheckBox* terrain_only_checkbox;
	wxCheckBox* sand_biome_checkbox;
	wxCheckBox* smooth_coastline_checkbox;
	wxCheckBox* add_caves_checkbox;
	
	// Advanced settings
	wxSlider* noise_increment_slider;
	wxSlider* island_distance_slider;
	wxSlider* cave_depth_slider;
	wxSlider* cave_roughness_slider;
	wxSlider* cave_chance_slider;
	wxSlider* water_level_slider;
	wxSlider* exponent_slider;
	wxSlider* linear_slider;
	
	// Preview
	wxStaticBitmap* preview_bitmap;
	wxButton* preview_button;
	wxButton* floor_up_button;
	wxButton* floor_down_button;
	wxStaticText* floor_label;
	wxButton* zoom_in_button;
	wxButton* zoom_out_button;
	wxStaticText* zoom_label;
	
	// Buttons
	wxButton* generate_button;
	wxButton* cancel_button;
	
	// Preview state
	wxBitmap* current_preview;
	std::vector<std::vector<uint16_t>> current_layers;
	int current_preview_floor;
	double current_zoom;
	int preview_offset_x;
	int preview_offset_y;

	DECLARE_EVENT_TABLE()
};

enum {
	ID_GENERATE = 1000,
	ID_PREVIEW,
	ID_SEED_TEXT,
	ID_WIDTH_SPIN,
	ID_HEIGHT_SPIN,
	ID_VERSION_CHOICE,
	ID_MOUNTAIN_TYPE_CHOICE,
	ID_NOISE_INCREMENT_SLIDER,
	ID_ISLAND_DISTANCE_SLIDER,
	ID_CAVE_DEPTH_SLIDER,
	ID_CAVE_ROUGHNESS_SLIDER,
	ID_CAVE_CHANCE_SLIDER,
	ID_WATER_LEVEL_SLIDER,
	ID_EXPONENT_SLIDER,
	ID_LINEAR_SLIDER,
	ID_FLOOR_UP,
	ID_FLOOR_DOWN,
	ID_ZOOM_IN,
	ID_ZOOM_OUT
};

#endif 