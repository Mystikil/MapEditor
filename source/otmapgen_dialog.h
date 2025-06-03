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
#include <wx/listctrl.h>

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
	void OnParameterChangeText(wxCommandEvent& event);
	void OnMountainTypeChange(wxCommandEvent& event);
	void OnFloorUp(wxCommandEvent& event);
	void OnFloorDown(wxCommandEvent& event);
	void OnZoomIn(wxCommandEvent& event);
	void OnZoomOut(wxCommandEvent& event);
	
	// Terrain layer management events
	void OnTerrainLayerSelect(wxListEvent& event);
	void OnTerrainLayerAdd(wxCommandEvent& event);
	void OnTerrainLayerRemove(wxCommandEvent& event);
	void OnTerrainLayerMoveUp(wxCommandEvent& event);
	void OnTerrainLayerMoveDown(wxCommandEvent& event);
	void OnTerrainLayerEdit(wxCommandEvent& event);
	void OnBrushChoice(wxCommandEvent& event);
	void OnItemIdChange(wxCommandEvent& event);

	// Helper functions
	bool GenerateMap();
	void UpdatePreview();
	void UpdatePreviewFloor();
	void UpdateFloorLabel();
	void UpdateZoomLabel();
	GenerationConfig BuildGenerationConfig();
	void GetTilePreviewColor(uint16_t tileId, unsigned char& r, unsigned char& g, unsigned char& b);
	
	// Terrain layer management helpers
	void PopulateTerrainLayerList();
	void PopulateBrushChoices();
	void UpdateLayerControls();
	TerrainLayer* GetSelectedLayer();
	void SetSelectedLayer(const TerrainLayer& layer);

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
	wxTextCtrl* noise_increment_text;
	wxTextCtrl* island_distance_text;
	wxTextCtrl* cave_depth_text;
	wxTextCtrl* cave_roughness_text;
	wxTextCtrl* cave_chance_text;
	wxTextCtrl* water_level_text;
	wxTextCtrl* exponent_text;
	wxTextCtrl* linear_text;
	
	// Layout design controls (new advanced tab)
	wxListCtrl* terrain_layer_list;
	wxButton* add_layer_button;
	wxButton* remove_layer_button;
	wxButton* move_up_button;
	wxButton* move_down_button;
	wxButton* edit_layer_button;
	
	// Layer properties panel
	wxPanel* layer_properties_panel;
	wxTextCtrl* layer_name_text;
	wxChoice* layer_brush_choice;
	wxSpinCtrl* layer_item_id_spin;
	wxTextCtrl* height_min_text;
	wxTextCtrl* height_max_text;
	wxTextCtrl* moisture_min_text;
	wxTextCtrl* moisture_max_text;
	wxTextCtrl* noise_scale_text;
	wxTextCtrl* coverage_text;
	wxCheckBox* use_borders_checkbox;
	wxCheckBox* layer_enabled_checkbox;
	wxSpinCtrl* z_order_spin;
	
	// Cave and water configuration
	wxChoice* cave_brush_choice;
	wxSpinCtrl* cave_item_id_spin;
	wxChoice* water_brush_choice;
	wxSpinCtrl* water_item_id_spin;
	
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
	
	// Terrain layer management
	std::vector<TerrainLayer> working_terrain_layers;
	std::vector<std::string> available_brushes;

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
	ID_NOISE_INCREMENT_TEXT,
	ID_ISLAND_DISTANCE_TEXT,
	ID_CAVE_DEPTH_TEXT,
	ID_CAVE_ROUGHNESS_TEXT,
	ID_CAVE_CHANCE_TEXT,
	ID_WATER_LEVEL_TEXT,
	ID_EXPONENT_TEXT,
	ID_LINEAR_TEXT,
	ID_FLOOR_UP,
	ID_FLOOR_DOWN,
	ID_ZOOM_IN,
	ID_ZOOM_OUT,
	
	// Terrain layer management IDs
	ID_TERRAIN_LAYER_LIST,
	ID_ADD_LAYER,
	ID_REMOVE_LAYER,
	ID_MOVE_UP_LAYER,
	ID_MOVE_DOWN_LAYER,
	ID_EDIT_LAYER,
	ID_LAYER_BRUSH_CHOICE,
	ID_LAYER_ITEM_ID_SPIN,
	ID_CAVE_BRUSH_CHOICE,
	ID_CAVE_ITEM_ID_SPIN,
	ID_WATER_BRUSH_CHOICE,
	ID_WATER_ITEM_ID_SPIN
};

#endif 