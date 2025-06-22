#ifndef RME_MONSTER_ATTACK_DIALOG_H_
#define RME_MONSTER_ATTACK_DIALOG_H_

#include "main.h"
#include "monster_manager.h"

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>

class MonsterAttackDialog : public wxDialog {
public:
    MonsterAttackDialog(wxWindow* parent, const AttackEntry* attack = nullptr);
    ~MonsterAttackDialog();
    
    // Get the configured attack entry
    AttackEntry GetAttackEntry() const;
    
    // Set the attack entry for editing
    void SetAttackEntry(const AttackEntry& attack);

private:
    void CreateUI();
    void PopulateAttackNames();
    void PopulateEffects();
    void UpdateControlStates();
    void OnDiceRollAttack();
    
    // Event handlers
    void OnAttackNameChanged(wxCommandEvent& event);
    void OnEnableRangeToggle(wxCommandEvent& event);
    void OnEnableRadiusToggle(wxCommandEvent& event);
    void OnEnableBeamToggle(wxCommandEvent& event);
    void OnEnableStatusToggle(wxCommandEvent& event);
    void OnEnableVisualToggle(wxCommandEvent& event);
    void OnEnableConditionToggle(wxCommandEvent& event);
    void OnDiceRoll(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    
    // Basic Controls
    wxComboBox* m_nameCombo;
    wxSpinCtrl* m_minDamageCtrl;
    wxSpinCtrl* m_maxDamageCtrl;
    wxSpinCtrl* m_intervalCtrl;
    wxSpinCtrl* m_chanceCtrl;
    
    // Advanced Properties
    wxCheckBox* m_enableRange;
    wxSpinCtrl* m_rangeCtrl;
    wxCheckBox* m_enableRadius;
    wxSpinCtrl* m_radiusCtrl;
    wxComboBox* m_targetCombo;
    
    // Beam Properties
    wxCheckBox* m_enableBeam;
    wxSpinCtrl* m_lengthCtrl;
    wxSpinCtrl* m_spreadCtrl;
    
    // Status Effects
    wxCheckBox* m_enableStatusEffect;
    wxSpinCtrl* m_speedChangeCtrl;
    wxSpinCtrl* m_durationCtrl;
    
    // Visual Effects
    wxCheckBox* m_enableVisualEffects;
    wxComboBox* m_shootEffectCombo;
    wxComboBox* m_areaEffectCombo;
    
    // Condition
    wxCheckBox* m_enableCondition;
    wxComboBox* m_conditionCombo;
    
    // Dice roll button
    wxButton* m_diceRollBtn;
    
    DECLARE_EVENT_TABLE()
};

enum {
    ID_ATTACK_NAME_COMBO = wxID_HIGHEST + 1,
    ID_ATTACK_ENABLE_RANGE,
    ID_ATTACK_ENABLE_RADIUS,
    ID_ATTACK_ENABLE_BEAM,
    ID_ATTACK_ENABLE_STATUS,
    ID_ATTACK_ENABLE_VISUAL,
    ID_ATTACK_ENABLE_CONDITION,
    ID_ATTACK_DICE_ROLL,
    ID_ATTACK_OK,
    ID_ATTACK_CANCEL
};

#endif // RME_MONSTER_ATTACK_DIALOG_H_ 