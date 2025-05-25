#include "main.h"
#include "add_creature_dialog.h"
#include "materials.h"
#include "creature_brush.h"

#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/radiobox.h>

BEGIN_EVENT_TABLE(AddCreatureDialog, wxDialog)
    EVT_BUTTON(wxID_OK, AddCreatureDialog::OnClickOK)
    EVT_BUTTON(wxID_CANCEL, AddCreatureDialog::OnClickCancel)
END_EVENT_TABLE()

AddCreatureDialog::AddCreatureDialog(wxWindow* parent, const wxString& name) :
    wxDialog(parent, wxID_ANY, "Add New Creature", wxDefaultPosition, wxDefaultSize),
    creature_type(nullptr)
{
    // Create sizers
    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* form_sizer = new wxBoxSizer(wxVERTICAL);

    // Create form fields
    wxStaticBoxSizer* name_box = new wxStaticBoxSizer(wxVERTICAL, this, "Name");
    name_field = new wxTextCtrl(this, wxID_ANY, name);
    name_box->Add(name_field, 0, wxEXPAND | wxALL, 5);
    form_sizer->Add(name_box, 0, wxEXPAND | wxALL, 5);

    // Type selection
    wxArrayString type_choices;
    type_choices.Add("Monster");
    type_choices.Add("NPC");
    type_radio = new wxRadioBox(this, wxID_ANY, "Creature Type", 
                               wxDefaultPosition, wxDefaultSize,
                               type_choices, 1, wxRA_SPECIFY_ROWS);
    form_sizer->Add(type_radio, 0, wxEXPAND | wxALL, 5);

    // Outfit settings
    wxStaticBoxSizer* outfit_box = new wxStaticBoxSizer(wxVERTICAL, this, "Outfit");
    wxFlexGridSizer* outfit_sizer = new wxFlexGridSizer(2, 5, 5);

    outfit_sizer->Add(new wxStaticText(this, wxID_ANY, "Look Type:"));
    looktype_field = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 1500, 1);
    outfit_sizer->Add(looktype_field);

    outfit_sizer->Add(new wxStaticText(this, wxID_ANY, "Head:"));
    lookhead_field = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 0);
    outfit_sizer->Add(lookhead_field);

    outfit_sizer->Add(new wxStaticText(this, wxID_ANY, "Body:"));
    lookbody_field = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 0);
    outfit_sizer->Add(lookbody_field);

    outfit_sizer->Add(new wxStaticText(this, wxID_ANY, "Legs:"));
    looklegs_field = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 0);
    outfit_sizer->Add(looklegs_field);

    outfit_sizer->Add(new wxStaticText(this, wxID_ANY, "Feet:"));
    lookfeet_field = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 0);
    outfit_sizer->Add(lookfeet_field);

    outfit_box->Add(outfit_sizer, 0, wxEXPAND | wxALL, 5);
    form_sizer->Add(outfit_box, 0, wxEXPAND | wxALL, 5);

    // Add buttons
    wxSizer* button_sizer = CreateButtonSizer(wxOK | wxCANCEL);
    
    // Add everything to the dialog
    topsizer->Add(form_sizer, 1, wxEXPAND | wxALL, 5);
    topsizer->Add(button_sizer, 0, wxEXPAND | wxALL, 5);

    SetSizerAndFit(topsizer);
}

AddCreatureDialog::~AddCreatureDialog()
{
    // Clean up if needed
}

void AddCreatureDialog::OnClickOK(wxCommandEvent& event)
{
    // Get values from form
    std::string name = name_field->GetValue().ToStdString();
    bool isNpc = (type_radio->GetSelection() == 1);
    
    // Create outfit
    Outfit outfit;
    outfit.lookType = looktype_field->GetValue();
    outfit.lookHead = lookhead_field->GetValue();
    outfit.lookBody = lookbody_field->GetValue();
    outfit.lookLegs = looklegs_field->GetValue();
    outfit.lookFeet = lookfeet_field->GetValue();

    // Create new creature type
    creature_type = g_creatures.addCreatureType(name, isNpc, outfit);

    if (creature_type) {
        // Create brush and add to appropriate tileset
        Tileset* tileset = nullptr;
        if (isNpc) {
            tileset = g_materials.tilesets["NPCs"];
        } else {
            tileset = g_materials.tilesets["Others"];
        }

        if (tileset) {
            Brush* brush = newd CreatureBrush(creature_type);
            g_brushes.addBrush(brush);

            TilesetCategory* category = tileset->getCategory(TILESET_CREATURE);
            if (category) {
                category->brushlist.push_back(brush);
            }
        }

        EndModal(wxID_OK);
    } else {
        wxMessageBox("Failed to create creature. The name might already be in use.", 
                    "Error", wxOK | wxICON_ERROR, this);
    }
}

void AddCreatureDialog::OnClickCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
} 