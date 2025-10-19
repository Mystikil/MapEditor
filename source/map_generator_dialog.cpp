#include "map_generator_dialog.h"

#include "generator/Biomes.h"

#include "client_version.h"
#include "editor.h"
#include "gui.h"
#include "map.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <memory>

wxBEGIN_EVENT_TABLE(MapGeneratorDialog, wxDialog)
        EVT_BUTTON(wxID_OK, MapGeneratorDialog::OnGenerate)
        EVT_BUTTON(wxID_CANCEL, MapGeneratorDialog::OnCancel)
wxEND_EVENT_TABLE()

MapGeneratorDialog::MapGeneratorDialog(wxWindow* parent) :
        wxDialog(parent, wxID_ANY, "Generate Map...", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
        wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

        wxFlexGridSizer* grid = new wxFlexGridSizer(2, 10, 10);
        grid->AddGrowableCol(1, 1);

        grid->Add(new wxStaticText(this, wxID_ANY, "Seed"), 0, wxALIGN_CENTER_VERTICAL);
        seedCtrl = new wxTextCtrl(this, wxID_ANY, "1337");
        grid->Add(seedCtrl, 1, wxEXPAND);

        grid->Add(new wxStaticText(this, wxID_ANY, "Width"), 0, wxALIGN_CENTER_VERTICAL);
        widthCtrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 64, 8192, 1024);
        grid->Add(widthCtrl, 1, wxEXPAND);

        grid->Add(new wxStaticText(this, wxID_ANY, "Height"), 0, wxALIGN_CENTER_VERTICAL);
        heightCtrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 64, 8192, 1024);
        grid->Add(heightCtrl, 1, wxEXPAND);

        grid->Add(new wxStaticText(this, wxID_ANY, "Overworld Z"), 0, wxALIGN_CENTER_VERTICAL);
        overworldCtrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 15, 7);
        grid->Add(overworldCtrl, 1, wxEXPAND);

        grid->Add(new wxStaticText(this, wxID_ANY, "Cave Z"), 0, wxALIGN_CENTER_VERTICAL);
        caveCtrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 15, 8);
        grid->Add(caveCtrl, 1, wxEXPAND);

        grid->Add(new wxStaticText(this, wxID_ANY, "Biome"), 0, wxALIGN_CENTER_VERTICAL);
        biomeChoice = new wxChoice(this, wxID_ANY);
        grid->Add(biomeChoice, 1, wxEXPAND);

        riversCheck = new wxCheckBox(this, wxID_ANY, "Generate Rivers");
        riversCheck->SetValue(true);
        grid->Add(riversCheck, 0, wxALIGN_LEFT);
        cavesCheck = new wxCheckBox(this, wxID_ANY, "Generate Caves");
        cavesCheck->SetValue(true);
        grid->Add(cavesCheck, 0, wxALIGN_LEFT);

        dungeonsCheck = new wxCheckBox(this, wxID_ANY, "Generate Dungeons");
        dungeonsCheck->SetValue(true);
        grid->Add(dungeonsCheck, 0, wxALIGN_LEFT);
        townCheck = new wxCheckBox(this, wxID_ANY, "Place Starter Town");
        townCheck->SetValue(true);
        grid->Add(townCheck, 0, wxALIGN_LEFT);

        rootSizer->Add(grid, 0, wxALL | wxEXPAND, 12);

        wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
        buttons->AddButton(new wxButton(this, wxID_OK, "Generate"));
        buttons->AddButton(new wxButton(this, wxID_CANCEL, "Cancel"));
        buttons->Realize();
        rootSizer->Add(buttons, 0, wxALL | wxALIGN_RIGHT, 12);

        SetSizerAndFit(rootSizer);
        CentreOnParent();

        PopulateBiomeChoices();
}

void MapGeneratorDialog::PopulateBiomeChoices() {
        biomeNames = Biomes::ListPresets(GUI::GetDataDirectory().ToStdString());
        if (biomeNames.empty()) {
                biomeNames.push_back("temperate");
        }

        biomeChoice->Clear();
        for (const std::string& name : biomeNames) {
                biomeChoice->Append(wxString::FromUTF8(name.c_str()));
        }

        int selection = 0;
        for (size_t i = 0; i < biomeNames.size(); ++i) {
                if (biomeNames[i] == "temperate") {
                        selection = static_cast<int>(i);
                        break;
                }
        }
        biomeChoice->SetSelection(selection);
}

bool MapGeneratorDialog::CollectOptions(GenOptions& outOptions) const {
        std::string seedText = seedCtrl->GetValue().ToStdString();
        if (seedText.empty()) {
                seedText = "0";
        }
        try {
                outOptions.seed = std::stoull(seedText);
        } catch (...) {
                wxMessageBox("Seed must be an unsigned integer.", "Invalid Seed", wxOK | wxICON_WARNING, this);
                return false;
        }

        outOptions.width = widthCtrl->GetValue();
        outOptions.height = heightCtrl->GetValue();
        outOptions.overworldZ = overworldCtrl->GetValue();
        outOptions.caveZ = caveCtrl->GetValue();

        int selection = biomeChoice->GetSelection();
        if (selection == wxNOT_FOUND || selection >= static_cast<int>(biomeNames.size())) {
                wxMessageBox("Please select a biome preset.", "Missing Biome", wxOK | wxICON_WARNING, this);
                return false;
        }
        outOptions.biomePreset = biomeNames[selection];

        outOptions.makeRivers = riversCheck->GetValue();
        outOptions.makeCaves = cavesCheck->GetValue();
        outOptions.makeDungeons = dungeonsCheck->GetValue();
        outOptions.placeStarterTown = townCheck->GetValue();

        return true;
}

void MapGeneratorDialog::OnGenerate(wxCommandEvent& event) {
        GenOptions options;
        if (!CollectOptions(options)) {
                return;
        }

        bool cancelled = false;
        bool loadBarCreated = false;
        std::unique_ptr<Map> generated;

        try {
                g_gui.CreateLoadBar("Generating map...", true);
                loadBarCreated = true;
                generated = MapGenerator::Generate(options, [&](int done, int total) {
                        const int percent = total > 0 ? static_cast<int>((done * 100) / total) : 100;
                        const bool abort = g_gui.SetLoadDone(percent, wxString::Format("Generating (%d/%d)", done, total));
                        return !abort;
                });
        } catch (const std::runtime_error& e) {
                if (std::string(e.what()) == "generation_cancelled") {
                        cancelled = true;
                } else {
                        wxMessageBox(wxString::Format("Failed to generate map: %s", wxString::FromUTF8(e.what())), "Generation Error", wxOK | wxICON_ERROR, this);
                }
        } catch (...) {
                wxMessageBox("Unknown error during generation.", "Generation Error", wxOK | wxICON_ERROR, this);
        }

        if (loadBarCreated) {
                g_gui.DestroyLoadBar();
        }

        if (cancelled) {
                wxMessageBox("Generation cancelled.", "Cancelled", wxOK | wxICON_INFORMATION, this);
                return;
        }

        if (!generated) {
                return;
        }

        MapVersion version;
        version.otbm = g_gui.GetCurrentVersion().getPrefferedMapVersionID();
        version.client = g_gui.GetCurrentVersionID();
        generated->convert(version);

        g_gui.NewMap();
        Editor* editor = g_gui.GetCurrentEditor();
        if (!editor) {
                wxMessageBox("Unable to open a new editor tab.", "Error", wxOK | wxICON_ERROR, this);
                return;
        }

        editor->LoadGeneratedMap(*generated);

        g_gui.RefreshView();
        g_gui.UpdateMinimap();
        g_gui.RefreshPalettes();
        g_gui.UpdateTitle();
        g_gui.FitViewToMap();
        g_gui.SetStatusText(wxString::Format("Generated %dx%d map using %s preset.", options.width, options.height, wxString::FromUTF8(options.biomePreset.c_str())));

        EndModal(wxID_OK);
}

void MapGeneratorDialog::OnCancel(wxCommandEvent& event) {
        EndModal(wxID_CANCEL);
}

