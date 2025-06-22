#include "main.h"
#include "monster_attack_dialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <random>

wxBEGIN_EVENT_TABLE(MonsterAttackDialog, wxDialog)
    EVT_COMBOBOX(ID_ATTACK_NAME_COMBO, MonsterAttackDialog::OnAttackNameChanged)
    EVT_CHECKBOX(ID_ATTACK_ENABLE_RANGE, MonsterAttackDialog::OnEnableRangeToggle)
    EVT_CHECKBOX(ID_ATTACK_ENABLE_RADIUS, MonsterAttackDialog::OnEnableRadiusToggle)
    EVT_CHECKBOX(ID_ATTACK_ENABLE_BEAM, MonsterAttackDialog::OnEnableBeamToggle)
    EVT_CHECKBOX(ID_ATTACK_ENABLE_STATUS, MonsterAttackDialog::OnEnableStatusToggle)
    EVT_CHECKBOX(ID_ATTACK_ENABLE_VISUAL, MonsterAttackDialog::OnEnableVisualToggle)
    EVT_CHECKBOX(ID_ATTACK_ENABLE_CONDITION, MonsterAttackDialog::OnEnableConditionToggle)
    EVT_BUTTON(ID_ATTACK_DICE_ROLL, MonsterAttackDialog::OnDiceRoll)
    EVT_BUTTON(ID_ATTACK_OK, MonsterAttackDialog::OnOK)
    EVT_BUTTON(ID_ATTACK_CANCEL, MonsterAttackDialog::OnCancel)
wxEND_EVENT_TABLE()

MonsterAttackDialog::MonsterAttackDialog(wxWindow* parent, const AttackEntry* attack) :
    wxDialog(parent, wxID_ANY, "Advanced Attack Editor", wxDefaultPosition, wxSize(650, 750), 
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    CreateUI();
    PopulateAttackNames();
    PopulateEffects();
    
    if (attack) {
        SetAttackEntry(*attack);
    } else {
        // Set default values
        m_nameCombo->SetValue("melee");
        m_minDamageCtrl->SetValue(0);
        m_maxDamageCtrl->SetValue(-50);
        m_intervalCtrl->SetValue(2000);
        m_chanceCtrl->SetValue(100);
        m_rangeCtrl->SetValue(1);
        m_radiusCtrl->SetValue(0);
        m_targetCombo->SetSelection(0);
        m_lengthCtrl->SetValue(1);
        m_spreadCtrl->SetValue(0);
        m_speedChangeCtrl->SetValue(0);
        m_durationCtrl->SetValue(0);
    }
    
    UpdateControlStates();
}

MonsterAttackDialog::~MonsterAttackDialog() {
}

void MonsterAttackDialog::CreateUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* title = new wxStaticText(this, wxID_ANY, "Advanced Attack Configuration");
    wxFont titleFont = title->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 2);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    mainSizer->Add(title, 0, wxALIGN_CENTER | wxALL, 10);
    
    // Basic properties section
    wxStaticBoxSizer* basicSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Basic Properties");
    
    wxFlexGridSizer* basicGridSizer = new wxFlexGridSizer(5, 2, 5, 5);
    basicGridSizer->AddGrowableCol(1);
    
    // Attack Name
    basicGridSizer->Add(new wxStaticText(this, wxID_ANY, "Attack Name:"), 0, wxALIGN_CENTER_VERTICAL);
    m_nameCombo = new wxComboBox(this, ID_ATTACK_NAME_COMBO, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN);
    basicGridSizer->Add(m_nameCombo, 1, wxEXPAND);
    
    // Min Damage
    basicGridSizer->Add(new wxStaticText(this, wxID_ANY, "Min Damage:"), 0, wxALIGN_CENTER_VERTICAL);
    m_minDamageCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, 0);
    basicGridSizer->Add(m_minDamageCtrl, 1, wxEXPAND);
    
    // Max Damage
    basicGridSizer->Add(new wxStaticText(this, wxID_ANY, "Max Damage:"), 0, wxALIGN_CENTER_VERTICAL);
    m_maxDamageCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000, 10000, -50);
    basicGridSizer->Add(m_maxDamageCtrl, 1, wxEXPAND);
    
    // Interval
    basicGridSizer->Add(new wxStaticText(this, wxID_ANY, "Interval (ms):"), 0, wxALIGN_CENTER_VERTICAL);
    m_intervalCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 500, 60000, 2000);
    basicGridSizer->Add(m_intervalCtrl, 1, wxEXPAND);
    
    // Chance
    basicGridSizer->Add(new wxStaticText(this, wxID_ANY, "Chance (%):"), 0, wxALIGN_CENTER_VERTICAL);
    m_chanceCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 100);
    basicGridSizer->Add(m_chanceCtrl, 1, wxEXPAND);
    
    basicSizer->Add(basicGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(basicSizer, 0, wxEXPAND | wxALL, 5);
    
    // Advanced properties section
    wxStaticBoxSizer* advancedSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Advanced Properties");
    
    // Range & Radius
    wxFlexGridSizer* rangeGridSizer = new wxFlexGridSizer(3, 3, 5, 5);
    rangeGridSizer->AddGrowableCol(1);
    
    m_enableRange = new wxCheckBox(this, ID_ATTACK_ENABLE_RANGE, "Range:");
    rangeGridSizer->Add(m_enableRange, 0, wxALIGN_CENTER_VERTICAL);
    m_rangeCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 20, 1);
    rangeGridSizer->Add(m_rangeCtrl, 1, wxEXPAND);
    rangeGridSizer->Add(new wxStaticText(this, wxID_ANY, "tiles"), 0, wxALIGN_CENTER_VERTICAL);
    
    m_enableRadius = new wxCheckBox(this, ID_ATTACK_ENABLE_RADIUS, "Radius:");
    rangeGridSizer->Add(m_enableRadius, 0, wxALIGN_CENTER_VERTICAL);
    m_radiusCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 20, 0);
    rangeGridSizer->Add(m_radiusCtrl, 1, wxEXPAND);
    rangeGridSizer->Add(new wxStaticText(this, wxID_ANY, "tiles"), 0, wxALIGN_CENTER_VERTICAL);
    
    rangeGridSizer->Add(new wxStaticText(this, wxID_ANY, "Target:"), 0, wxALIGN_CENTER_VERTICAL);
    m_targetCombo = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
    m_targetCombo->Append("Area around target (0)");
    m_targetCombo->Append("Target position (1)");
    m_targetCombo->SetSelection(0);
    rangeGridSizer->Add(m_targetCombo, 1, wxEXPAND);
    rangeGridSizer->Add(new wxStaticText(this, wxID_ANY, ""), 0, 0, 0);
    
    advancedSizer->Add(rangeGridSizer, 0, wxEXPAND | wxALL, 5);
    
    // Beam properties
    wxFlexGridSizer* beamGridSizer = new wxFlexGridSizer(2, 3, 5, 5);
    beamGridSizer->AddGrowableCol(1);
    
    m_enableBeam = new wxCheckBox(this, ID_ATTACK_ENABLE_BEAM, "Length:");
    beamGridSizer->Add(m_enableBeam, 0, wxALIGN_CENTER_VERTICAL);
    m_lengthCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 20, 1);
    beamGridSizer->Add(m_lengthCtrl, 1, wxEXPAND);
    beamGridSizer->Add(new wxStaticText(this, wxID_ANY, "tiles"), 0, wxALIGN_CENTER_VERTICAL);
    
    beamGridSizer->Add(new wxStaticText(this, wxID_ANY, "Spread:"), 0, wxALIGN_CENTER_VERTICAL);
    m_spreadCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0);
    beamGridSizer->Add(m_spreadCtrl, 1, wxEXPAND);
    beamGridSizer->Add(new wxStaticText(this, wxID_ANY, "tiles"), 0, wxALIGN_CENTER_VERTICAL);
    
    advancedSizer->Add(beamGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(advancedSizer, 0, wxEXPAND | wxALL, 5);
    
    // Status effects section
    wxStaticBoxSizer* statusSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Status Effects");
    
    wxFlexGridSizer* statusGridSizer = new wxFlexGridSizer(2, 3, 5, 5);
    statusGridSizer->AddGrowableCol(1);
    
    m_enableStatusEffect = new wxCheckBox(this, ID_ATTACK_ENABLE_STATUS, "Speed Change:");
    statusGridSizer->Add(m_enableStatusEffect, 0, wxALIGN_CENTER_VERTICAL);
    m_speedChangeCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -1000, 1000, 0);
    statusGridSizer->Add(m_speedChangeCtrl, 1, wxEXPAND);
    statusGridSizer->Add(new wxStaticText(this, wxID_ANY, "units"), 0, wxALIGN_CENTER_VERTICAL);
    
    statusGridSizer->Add(new wxStaticText(this, wxID_ANY, "Duration:"), 0, wxALIGN_CENTER_VERTICAL);
    m_durationCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 300000, 0);
    statusGridSizer->Add(m_durationCtrl, 1, wxEXPAND);
    statusGridSizer->Add(new wxStaticText(this, wxID_ANY, "ms"), 0, wxALIGN_CENTER_VERTICAL);
    
    statusSizer->Add(statusGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(statusSizer, 0, wxEXPAND | wxALL, 5);
    
    // Visual effects section
    wxStaticBoxSizer* visualSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Visual Effects");
    
    wxFlexGridSizer* visualGridSizer = new wxFlexGridSizer(2, 2, 5, 5);
    visualGridSizer->AddGrowableCol(1);
    
    m_enableVisualEffects = new wxCheckBox(this, ID_ATTACK_ENABLE_VISUAL, "Shoot Effect:");
    visualGridSizer->Add(m_enableVisualEffects, 0, wxALIGN_CENTER_VERTICAL);
    m_shootEffectCombo = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN);
    visualGridSizer->Add(m_shootEffectCombo, 1, wxEXPAND);
    
    visualGridSizer->Add(new wxStaticText(this, wxID_ANY, "Area Effect:"), 0, wxALIGN_CENTER_VERTICAL);
    m_areaEffectCombo = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN);
    visualGridSizer->Add(m_areaEffectCombo, 1, wxEXPAND);
    
    visualSizer->Add(visualGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(visualSizer, 0, wxEXPAND | wxALL, 5);
    
    // Condition section
    wxStaticBoxSizer* conditionSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Condition");
    
    wxFlexGridSizer* conditionGridSizer = new wxFlexGridSizer(1, 2, 5, 5);
    conditionGridSizer->AddGrowableCol(1);
    
    m_enableCondition = new wxCheckBox(this, ID_ATTACK_ENABLE_CONDITION, "Condition Type:");
    conditionGridSizer->Add(m_enableCondition, 0, wxALIGN_CENTER_VERTICAL);
    m_conditionCombo = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN);
    conditionGridSizer->Add(m_conditionCombo, 1, wxEXPAND);
    
    conditionSizer->Add(conditionGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(conditionSizer, 0, wxEXPAND | wxALL, 5);
    
    // Dice roll section
    wxBoxSizer* diceSizer = new wxBoxSizer(wxHORIZONTAL);
    m_diceRollBtn = new wxButton(this, ID_ATTACK_DICE_ROLL, "🎲 Random Attack Generator");
    m_diceRollBtn->SetToolTip("Generate a random attack with realistic values based on monster examples");
    m_diceRollBtn->SetBackgroundColour(wxColour(240, 255, 240));
    diceSizer->Add(m_diceRollBtn, 0, wxALL, 5);
    diceSizer->AddStretchSpacer();
    mainSizer->Add(diceSizer, 0, wxEXPAND | wxALL, 5);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(new wxButton(this, ID_ATTACK_OK, "OK"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(this, ID_ATTACK_CANCEL, "Cancel"), 0, 0, 0);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
}

void MonsterAttackDialog::PopulateAttackNames() {
    // Common attack names from the monster examples
    m_nameCombo->Append("melee");
    m_nameCombo->Append("fire");
    m_nameCombo->Append("lifedrain");
    m_nameCombo->Append("manadrain");
    m_nameCombo->Append("energy");
    m_nameCombo->Append("earth");
    m_nameCombo->Append("ice");
    m_nameCombo->Append("holy");
    m_nameCombo->Append("death");
    m_nameCombo->Append("speed");
    m_nameCombo->Append("firefield");
    m_nameCombo->Append("poisonfield");
    m_nameCombo->Append("energyfield");
    m_nameCombo->Append("poisoncondition");
    m_nameCombo->Append("burncondition");
    m_nameCombo->Append("freezecondition");
    m_nameCombo->Append("dazzlecondition");
    m_nameCombo->Append("cursecondition");
    m_nameCombo->Append("outfit");
    m_nameCombo->Append("drunk");
    m_nameCombo->Append("invisible");
}

void MonsterAttackDialog::PopulateEffects() {
    // Shoot effects from monster examples
    m_shootEffectCombo->Append("fire");
    m_shootEffectCombo->Append("energy");
    m_shootEffectCombo->Append("earth");
    m_shootEffectCombo->Append("ice");
    m_shootEffectCombo->Append("holy");
    m_shootEffectCombo->Append("death");
    m_shootEffectCombo->Append("smallearth");
    m_shootEffectCombo->Append("largerock");
    m_shootEffectCombo->Append("spear");
    m_shootEffectCombo->Append("bolt");
    m_shootEffectCombo->Append("arrow");
    m_shootEffectCombo->Append("poisonarrow");
    m_shootEffectCombo->Append("burstarrow");
    m_shootEffectCombo->Append("throwingstar");
    m_shootEffectCombo->Append("throwingknife");
    m_shootEffectCombo->Append("smallstone");
    m_shootEffectCombo->Append("suddendeath");
    m_shootEffectCombo->Append("largeholy");
    m_shootEffectCombo->Append("icicle");
    
    // Area effects from monster examples
    m_areaEffectCombo->Append("firearea");
    m_areaEffectCombo->Append("energyarea");
    m_areaEffectCombo->Append("poisonarea");
    m_areaEffectCombo->Append("icearea");
    m_areaEffectCombo->Append("holyarea");
    m_areaEffectCombo->Append("deatharea");
    m_areaEffectCombo->Append("blueshimmer");
    m_areaEffectCombo->Append("redshimmer");
    m_areaEffectCombo->Append("greenshimmer");
    m_areaEffectCombo->Append("blackspark");
    m_areaEffectCombo->Append("teleport");
    m_areaEffectCombo->Append("purpleenergy");
    m_areaEffectCombo->Append("yellowenergy");
    m_areaEffectCombo->Append("greenspark");
    m_areaEffectCombo->Append("mortarea");
    m_areaEffectCombo->Append("poff");
    m_areaEffectCombo->Append("blockhit");
    
    // Conditions
    m_conditionCombo->Append("poison");
    m_conditionCombo->Append("burn");
    m_conditionCombo->Append("freeze");
    m_conditionCombo->Append("dazzle");
    m_conditionCombo->Append("curse");
    m_conditionCombo->Append("paralyze");
    m_conditionCombo->Append("drunk");
    m_conditionCombo->Append("invisible");
    m_conditionCombo->Append("outfit");
    m_conditionCombo->Append("lifedrain");
    m_conditionCombo->Append("manadrain");
}

void MonsterAttackDialog::UpdateControlStates() {
    // Enable/disable controls based on checkboxes
    m_rangeCtrl->Enable(m_enableRange->GetValue());
    m_radiusCtrl->Enable(m_enableRadius->GetValue());
    m_targetCombo->Enable(m_enableRadius->GetValue());
    
    m_lengthCtrl->Enable(m_enableBeam->GetValue());
    m_spreadCtrl->Enable(m_enableBeam->GetValue());
    
    m_speedChangeCtrl->Enable(m_enableStatusEffect->GetValue());
    m_durationCtrl->Enable(m_enableStatusEffect->GetValue());
    
    m_shootEffectCombo->Enable(m_enableVisualEffects->GetValue());
    m_areaEffectCombo->Enable(m_enableVisualEffects->GetValue());
    
    m_conditionCombo->Enable(m_enableCondition->GetValue());
}

void MonsterAttackDialog::OnDiceRollAttack() {
    // Advanced random attack generator based on real monster examples
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Attack types weighted by frequency from actual monsters
    std::vector<std::string> attackTypes = {
        "melee", "melee", "melee",  // Most common (always present)
        "fire", "energy", "lifedrain", "manadrain",  // Common elemental
        "speed", "poisoncondition", "firefield",  // Status effects
        "ice", "holy", "death", "earth"  // Less common elementals
    };
    
    std::uniform_int_distribution<> typeDist(0, attackTypes.size() - 1);
    std::string selectedType = attackTypes[typeDist(gen)];
    
    m_nameCombo->SetValue(selectedType);
    
    // Clear all checkboxes first
    m_enableRange->SetValue(false);
    m_enableRadius->SetValue(false);
    m_enableBeam->SetValue(false);
    m_enableStatusEffect->SetValue(false);
    m_enableVisualEffects->SetValue(false);
    m_enableCondition->SetValue(false);
    
    // Generate realistic damage and properties based on attack type
    if (selectedType == "melee") {
        // Melee attacks: 0 to -500 damage, always 100% chance
        std::uniform_int_distribution<> damageDist(50, 500);
        int maxDamage = damageDist(gen);
        m_minDamageCtrl->SetValue(0);
        m_maxDamageCtrl->SetValue(-maxDamage);
        m_intervalCtrl->SetValue(2000);
        m_chanceCtrl->SetValue(100);
    } else {
        // Spell attacks: varied damage, lower chance, intervals
        std::uniform_int_distribution<> damageDist(20, 400);
        std::uniform_int_distribution<> chanceDist(5, 25);
        std::uniform_int_distribution<> intervalDist(1500, 3000);
        
        int maxDamage = damageDist(gen);
        m_minDamageCtrl->SetValue(-maxDamage/3);
        m_maxDamageCtrl->SetValue(-maxDamage);
        m_intervalCtrl->SetValue(intervalDist(gen));
        m_chanceCtrl->SetValue(chanceDist(gen));
        
        // Random advanced properties based on attack type
        std::uniform_int_distribution<> boolDist(0, 1);
        std::uniform_int_distribution<> rangeDist(1, 7);
        std::uniform_int_distribution<> radiusDist(1, 7);
        
        // Range for projectile attacks (75% chance for non-melee)
        if (boolDist(gen) || boolDist(gen) || boolDist(gen)) {  // 87.5% chance
            m_enableRange->SetValue(true);
            m_rangeCtrl->SetValue(rangeDist(gen));
        }
        
        // Radius for area attacks (40% chance)
        if (boolDist(gen) && boolDist(gen) && !boolDist(gen)) {  
            m_enableRadius->SetValue(true);
            m_radiusCtrl->SetValue(radiusDist(gen));
            m_targetCombo->SetSelection(boolDist(gen));
        }
        
        // Beam attacks for lifedrain and poison
        if (selectedType == "lifedrain" || selectedType == "poisoncondition") {
            if (boolDist(gen)) {  // 50% chance
                m_enableBeam->SetValue(true);
                m_lengthCtrl->SetValue(rangeDist(gen));
                m_spreadCtrl->SetValue(std::uniform_int_distribution<>(0, 2)(gen));
            }
        }
        
        // Speed effects
        if (selectedType == "speed") {
            m_enableStatusEffect->SetValue(true);
            m_speedChangeCtrl->SetValue(std::uniform_int_distribution<>(-700, 300)(gen));
            m_durationCtrl->SetValue(std::uniform_int_distribution<>(5000, 30000)(gen));
        }
        
        // Visual effects (80% chance for spell attacks)
        if (boolDist(gen) || boolDist(gen) || boolDist(gen) || boolDist(gen)) {
            m_enableVisualEffects->SetValue(true);
            if (selectedType == "fire") {
                m_shootEffectCombo->SetValue("fire");
                m_areaEffectCombo->SetValue("firearea");
            } else if (selectedType == "energy") {
                m_shootEffectCombo->SetValue("energy");
                m_areaEffectCombo->SetValue("energyarea");
            } else if (selectedType == "lifedrain") {
                m_areaEffectCombo->SetValue("purpleenergy");
            } else if (selectedType == "speed") {
                m_areaEffectCombo->SetValue("redshimmer");
            } else if (selectedType == "ice") {
                m_shootEffectCombo->SetValue("ice");
                m_areaEffectCombo->SetValue("icearea");
            } else if (selectedType == "holy") {
                m_shootEffectCombo->SetValue("largeholy");
                m_areaEffectCombo->SetValue("holyarea");
            } else if (selectedType == "death") {
                m_shootEffectCombo->SetValue("suddendeath");
                m_areaEffectCombo->SetValue("deatharea");
            } else if (selectedType == "earth") {
                m_shootEffectCombo->SetValue("smallearth");
                m_areaEffectCombo->SetValue("poisonarea");
            }
        }
        
        // Conditions for condition attacks
        if (selectedType.find("condition") != std::string::npos) {
            m_enableCondition->SetValue(true);
            if (selectedType == "poisoncondition") {
                m_conditionCombo->SetValue("poison");
            } else if (selectedType == "burncondition") {
                m_conditionCombo->SetValue("burn");
            } else if (selectedType == "freezecondition") {
                m_conditionCombo->SetValue("freeze");
            }
        }
    }
    
    UpdateControlStates();
    
    wxMessageBox(wxString::Format("🎲 Random %s attack generated!\n\nDamage: %d to %d\nInterval: %dms\nChance: %d%%", 
                                 selectedType, m_minDamageCtrl->GetValue(), m_maxDamageCtrl->GetValue(),
                                 m_intervalCtrl->GetValue(), m_chanceCtrl->GetValue()), 
                "Dice Roll Complete", wxOK | wxICON_INFORMATION);
}

AttackEntry MonsterAttackDialog::GetAttackEntry() const {
    AttackEntry attack;
    
    attack.name = nstr(m_nameCombo->GetValue());
    attack.minDamage = m_minDamageCtrl->GetValue();
    attack.maxDamage = m_maxDamageCtrl->GetValue();
    attack.interval = m_intervalCtrl->GetValue();
    attack.chance = m_chanceCtrl->GetValue();
    
    attack.range = m_enableRange->GetValue() ? m_rangeCtrl->GetValue() : 0;
    attack.radius = m_enableRadius->GetValue() ? m_radiusCtrl->GetValue() : 0;
    attack.target = m_targetCombo->GetSelection();
    
    attack.length = m_enableBeam->GetValue() ? m_lengthCtrl->GetValue() : 0;
    attack.spread = m_enableBeam->GetValue() ? m_spreadCtrl->GetValue() : 0;
    
    attack.speedChange = m_enableStatusEffect->GetValue() ? m_speedChangeCtrl->GetValue() : 0;
    attack.duration = m_enableStatusEffect->GetValue() ? m_durationCtrl->GetValue() : 0;
    
    if (m_enableVisualEffects->GetValue()) {
        attack.shootEffect = nstr(m_shootEffectCombo->GetValue());
        attack.areaEffect = nstr(m_areaEffectCombo->GetValue());
    }
    
    if (m_enableCondition->GetValue()) {
        attack.conditionType = nstr(m_conditionCombo->GetValue());
    }
    
    return attack;
}

void MonsterAttackDialog::SetAttackEntry(const AttackEntry& attack) {
    m_nameCombo->SetValue(wxstr(attack.name));
    m_minDamageCtrl->SetValue(attack.minDamage);
    m_maxDamageCtrl->SetValue(attack.maxDamage);
    m_intervalCtrl->SetValue(attack.interval);
    m_chanceCtrl->SetValue(attack.chance);
    
    m_enableRange->SetValue(attack.range > 0);
    m_rangeCtrl->SetValue(attack.range);
    
    m_enableRadius->SetValue(attack.radius > 0);
    m_radiusCtrl->SetValue(attack.radius);
    m_targetCombo->SetSelection(attack.target);
    
    m_enableBeam->SetValue(attack.length > 0);
    m_lengthCtrl->SetValue(attack.length);
    m_spreadCtrl->SetValue(attack.spread);
    
    m_enableStatusEffect->SetValue(attack.speedChange != 0 || attack.duration > 0);
    m_speedChangeCtrl->SetValue(attack.speedChange);
    m_durationCtrl->SetValue(attack.duration);
    
    bool hasVisualEffects = !attack.shootEffect.empty() || !attack.areaEffect.empty();
    m_enableVisualEffects->SetValue(hasVisualEffects);
    m_shootEffectCombo->SetValue(wxstr(attack.shootEffect));
    m_areaEffectCombo->SetValue(wxstr(attack.areaEffect));
    
    m_enableCondition->SetValue(!attack.conditionType.empty());
    m_conditionCombo->SetValue(wxstr(attack.conditionType));
    
    UpdateControlStates();
}

// Event handlers
void MonsterAttackDialog::OnAttackNameChanged(wxCommandEvent& event) {
    // Auto-suggest properties based on attack name
    wxString name = m_nameCombo->GetValue().Lower();
    
    if (name == "fire") {
        m_shootEffectCombo->SetValue("fire");
        m_areaEffectCombo->SetValue("firearea");
        m_enableVisualEffects->SetValue(true);
    } else if (name == "energy") {
        m_shootEffectCombo->SetValue("energy");
        m_areaEffectCombo->SetValue("energyarea");
        m_enableVisualEffects->SetValue(true);
    } else if (name == "lifedrain") {
        m_areaEffectCombo->SetValue("purpleenergy");
        m_enableVisualEffects->SetValue(true);
    } else if (name == "speed") {
        m_areaEffectCombo->SetValue("redshimmer");
        m_enableVisualEffects->SetValue(true);
        m_enableStatusEffect->SetValue(true);
    }
    
    UpdateControlStates();
}

void MonsterAttackDialog::OnEnableRangeToggle(wxCommandEvent& event) { UpdateControlStates(); }
void MonsterAttackDialog::OnEnableRadiusToggle(wxCommandEvent& event) { UpdateControlStates(); }
void MonsterAttackDialog::OnEnableBeamToggle(wxCommandEvent& event) { UpdateControlStates(); }
void MonsterAttackDialog::OnEnableStatusToggle(wxCommandEvent& event) { UpdateControlStates(); }
void MonsterAttackDialog::OnEnableVisualToggle(wxCommandEvent& event) { UpdateControlStates(); }
void MonsterAttackDialog::OnEnableConditionToggle(wxCommandEvent& event) { UpdateControlStates(); }

void MonsterAttackDialog::OnDiceRoll(wxCommandEvent& event) {
    OnDiceRollAttack();
}

void MonsterAttackDialog::OnOK(wxCommandEvent& event) {
    if (m_nameCombo->GetValue().IsEmpty()) {
        wxMessageBox("Please enter an attack name.", "Validation Error", wxOK | wxICON_WARNING);
        return;
    }
    
    EndModal(wxID_OK);
}

void MonsterAttackDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
} 