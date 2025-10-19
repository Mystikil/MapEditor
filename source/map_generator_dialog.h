#pragma once

#include "generator/MapGenerator.h"

#include <vector>

#include <wx/dialog.h>

class wxCheckBox;
class wxChoice;
class wxSpinCtrl;
class wxTextCtrl;

class MapGeneratorDialog : public wxDialog {
public:
        explicit MapGeneratorDialog(wxWindow* parent);

private:
        void OnGenerate(wxCommandEvent& event);
        void OnCancel(wxCommandEvent& event);

        bool CollectOptions(GenOptions& outOptions) const;
        void PopulateBiomeChoices();

        wxTextCtrl* seedCtrl;
        wxSpinCtrl* widthCtrl;
        wxSpinCtrl* heightCtrl;
        wxSpinCtrl* overworldCtrl;
        wxSpinCtrl* caveCtrl;
        wxChoice* biomeChoice;
        wxCheckBox* riversCheck;
        wxCheckBox* cavesCheck;
        wxCheckBox* dungeonsCheck;
        wxCheckBox* townCheck;

        std::vector<std::string> biomeNames;

        wxDECLARE_EVENT_TABLE();
};

