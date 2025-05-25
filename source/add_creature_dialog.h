#ifndef RME_ADD_CREATURE_DIALOG_H_
#define RME_ADD_CREATURE_DIALOG_H_

#include <wx/dialog.h>
#include "creatures.h"

class wxSpinCtrl;
class wxTextCtrl;
class wxRadioBox;

class AddCreatureDialog : public wxDialog {
public:
    AddCreatureDialog(wxWindow* parent, const wxString& name);
    virtual ~AddCreatureDialog();

    CreatureType* GetCreatureType() const { return creature_type; }

protected:
    void OnClickOK(wxCommandEvent& event);
    void OnClickCancel(wxCommandEvent& event);

private:
    wxTextCtrl* name_field;
    wxSpinCtrl* looktype_field;
    wxSpinCtrl* lookhead_field;
    wxSpinCtrl* lookbody_field;
    wxSpinCtrl* looklegs_field;
    wxSpinCtrl* lookfeet_field;
    wxRadioBox* type_radio;

    CreatureType* creature_type;

    DECLARE_EVENT_TABLE()
};

#endif // RME_ADD_CREATURE_DIALOG_H_ 