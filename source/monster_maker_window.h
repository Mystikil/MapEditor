#ifndef RME_MONSTER_MAKER_WINDOW_H_
#define RME_MONSTER_MAKER_WINDOW_H_

#include "main.h"
#include "monster_manager.h"

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include <wx/listctrl.h>
#include <wx/timer.h>

// Forward declaration
class MonsterMakerWindow;

// Custom preview panel that handles its own painting
class MonsterPreviewPanel : public wxPanel {
public:
    MonsterPreviewPanel(wxWindow* parent, MonsterMakerWindow* owner);
    
    void OnPaint(wxPaintEvent& event);
    
    DECLARE_EVENT_TABLE()

private:
    MonsterMakerWindow* m_owner;
};

class MonsterMakerWindow : public wxFrame {
public:
    MonsterMakerWindow(wxWindow* parent);
    ~MonsterMakerWindow();

    // Set monster data from existing monster
    void LoadMonster(const MonsterEntry& monster);
    
    // Clear all fields
    void ClearAll();
    
    // Refresh the monsters list
    void RefreshMonsterList();
    
    // Preview methods
    void UpdatePreview();
    void DrawFallbackPreview(wxDC& dc, const wxSize& size, int lookType, int headColor, int bodyColor, int legsColor);

private:
    // Event handlers
    void OnClose(wxCloseEvent& event);
    void OnCloseButton(wxCommandEvent& event);
    void OnCreateMonster(wxCommandEvent& event);
    void OnLoadMonster(wxCommandEvent& event);
    void OnPreviewUpdate(wxCommandEvent& event);
    void OnTabChange(wxNotebookEvent& event);
    void OnLookTypeChange(wxCommandEvent& event);
    void OnColorChange(wxCommandEvent& event);
    void OnPreviewTimer(wxTimerEvent& event);
    
    // UI Creation
    void CreateMonsterTab(wxNotebook* notebook);
    void CreateFlagsTab(wxNotebook* notebook);
    void CreateAttacksTab(wxNotebook* notebook);
    void CreateDefensesTab(wxNotebook* notebook);
    void CreateElementsTab(wxNotebook* notebook);
    void CreateImmunitiesTab(wxNotebook* notebook);
    void CreateSummonsTab(wxNotebook* notebook);
    void CreateVoicesTab(wxNotebook* notebook);
    void CreateLootTab(wxNotebook* notebook);
    void CreateIOTab(wxNotebook* notebook);
    
    void SaveToFile(const wxString& filename);
    bool LoadFromFile(const wxString& filename);
    
    // UI Components
    wxNotebook* m_notebook;
    
    // Monster Tab
    wxTextCtrl* m_name;
    wxTextCtrl* m_nameDescription;
    wxComboBox* m_race;
    wxSpinCtrl* m_experience;
    wxComboBox* m_skull;
    wxSpinCtrl* m_speed;
    wxSpinCtrl* m_manacost;
    wxSpinCtrl* m_healthNow;
    wxSpinCtrl* m_healthMax;
    
    // Look settings
    wxRadioButton* m_lookTypeCheck;
    wxRadioButton* m_lookTypeExCheck;
    wxSpinCtrl* m_lookType;
    wxSpinCtrl* m_head;
    wxSpinCtrl* m_body;
    wxSpinCtrl* m_legs;
    wxSpinCtrl* m_feet;
    wxSpinCtrl* m_addons;
    wxSpinCtrl* m_mount;
    wxSpinCtrl* m_corpse;
    
    // Combat settings
    wxSpinCtrl* m_interval;
    wxSpinCtrl* m_chance;
    wxCheckBox* m_strategyCheck;
    wxSlider* m_strategy;
    
    // Preview
    MonsterPreviewPanel* m_previewPanel;
    wxTimer* m_previewTimer;
    bool m_previewUpdatePending;
    
    // Flags Tab
    wxCheckBox* m_summonable;
    wxCheckBox* m_attackable;
    wxCheckBox* m_hostile;
    wxCheckBox* m_illusionable;
    wxCheckBox* m_convinceable;
    wxCheckBox* m_pushable;
    wxCheckBox* m_canpushitems;
    wxCheckBox* m_canpushcreatures;
    wxSpinCtrl* m_targetdistance;
    wxSlider* m_staticattack;
    wxCheckBox* m_hidehealth;
    wxSpinCtrl* m_lightcolor;
    wxSpinCtrl* m_lightlevel;
    wxSlider* m_runonhealth;
    
    // Attacks, Defenses, etc. (to be expanded)
    wxListCtrl* m_attacksList;
    wxListCtrl* m_defensesList;
    wxListCtrl* m_elementsList;
    wxListCtrl* m_immunitiesList;
    wxListCtrl* m_summonsList;
    wxListCtrl* m_voicesList;
    wxListCtrl* m_lootList;
    
    // IO Tab
    wxListCtrl* m_monstersList;
    wxButton* m_loadButton;
    wxButton* m_saveButton;
    wxButton* m_createButton;
    
    DECLARE_EVENT_TABLE()
};

enum {
    ID_NOTEBOOK = wxID_HIGHEST + 1,
    ID_CREATE_MONSTER,
    ID_LOAD_MONSTER,
    ID_SAVE_MONSTER,
    ID_CLOSE_BUTTON,
    ID_PREVIEW_TIMER
};

#endif // RME_MONSTER_MAKER_WINDOW_H_ 