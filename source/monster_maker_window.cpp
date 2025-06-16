#include "main.h"
#include "monster_maker_window.h"
#include "gui.h"
#include "creature_sprite_manager.h"

#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>
#include <wx/sizer.h>

// MonsterPreviewPanel implementation
wxBEGIN_EVENT_TABLE(MonsterPreviewPanel, wxPanel)
    EVT_PAINT(MonsterPreviewPanel::OnPaint)
wxEND_EVENT_TABLE()

MonsterPreviewPanel::MonsterPreviewPanel(wxWindow* parent, MonsterMakerWindow* owner)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(80, 80)), m_owner(owner) {
    SetBackgroundColour(*wxLIGHT_GREY);
}

void MonsterPreviewPanel::OnPaint(wxPaintEvent& event) {
    if (m_owner) {
        m_owner->UpdatePreview();
    }
    event.Skip();
}

wxBEGIN_EVENT_TABLE(MonsterMakerWindow, wxFrame)
    EVT_CLOSE(MonsterMakerWindow::OnClose)
    EVT_BUTTON(ID_CLOSE_BUTTON, MonsterMakerWindow::OnCloseButton)
    EVT_BUTTON(ID_CREATE_MONSTER, MonsterMakerWindow::OnCreateMonster)
    EVT_BUTTON(ID_LOAD_MONSTER, MonsterMakerWindow::OnLoadMonster)
    EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK, MonsterMakerWindow::OnTabChange)
    EVT_TIMER(ID_PREVIEW_TIMER, MonsterMakerWindow::OnPreviewTimer)
wxEND_EVENT_TABLE()

MonsterMakerWindow::MonsterMakerWindow(wxWindow* parent) :
    wxFrame(parent, wxID_ANY, "Monster Maker", wxDefaultPosition, wxSize(650, 600), 
            wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT),
    m_previewTimer(nullptr), m_previewUpdatePending(false)
{
    SetIcon(wxNullIcon);
    
    // Initialize timer for live preview updates (150ms delay)
    m_previewTimer = new wxTimer(this, ID_PREVIEW_TIMER);
    
    // Create main sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create notebook for tabs
    m_notebook = new wxNotebook(this, ID_NOTEBOOK);
    
    // Create tabs
    CreateMonsterTab(m_notebook);
    CreateFlagsTab(m_notebook);
    CreateAttacksTab(m_notebook);
    CreateDefensesTab(m_notebook);
    CreateElementsTab(m_notebook);
    CreateImmunitiesTab(m_notebook);
    CreateSummonsTab(m_notebook);
    CreateVoicesTab(m_notebook);
    CreateLootTab(m_notebook);
    CreateIOTab(m_notebook);
    
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 5);
    
    // Create buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_createButton = new wxButton(this, ID_CREATE_MONSTER, "Create Monster");
    m_saveButton = new wxButton(this, ID_SAVE_MONSTER, "Save to File");
    wxButton* closeButton = new wxButton(this, ID_CLOSE_BUTTON, "Close");
    
    buttonSizer->Add(m_createButton, 0, wxRIGHT, 5);
    buttonSizer->Add(m_saveButton, 0, wxRIGHT, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(closeButton, 0, 0, 0);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
    
    // Bind spin control events for live preview updates
    if (m_lookType) m_lookType->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnLookTypeChange, this);
    if (m_head) m_head->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnColorChange, this);
    if (m_body) m_body->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnColorChange, this);
    if (m_legs) m_legs->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnColorChange, this);
    if (m_feet) m_feet->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnColorChange, this);
    
    // Set default values
    ClearAll();
    
    // Trigger initial preview update after a short delay
    if (m_previewTimer) {
        m_previewUpdatePending = true;
        m_previewTimer->Start(300, wxTIMER_ONE_SHOT);
    }
}

MonsterMakerWindow::~MonsterMakerWindow() {
    if (m_previewTimer) {
        m_previewTimer->Stop();
        delete m_previewTimer;
        m_previewTimer = nullptr;
    }
}

void MonsterMakerWindow::CreateMonsterTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Monster");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Basic info section
    wxStaticBoxSizer* basicSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Basic Information");
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(4, 2, 5, 5);
    gridSizer->AddGrowableCol(1);
    
    // Name
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
    m_name = new wxTextCtrl(panel, wxID_ANY);
    gridSizer->Add(m_name, 1, wxEXPAND);
    
    // Name Description
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Description:"), 0, wxALIGN_CENTER_VERTICAL);
    m_nameDescription = new wxTextCtrl(panel, wxID_ANY);
    gridSizer->Add(m_nameDescription, 1, wxEXPAND);
    
    // Race
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Race:"), 0, wxALIGN_CENTER_VERTICAL);
    m_race = new wxComboBox(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN);
    m_race->Append("blood");
    m_race->Append("venom");
    m_race->Append("undead");
    m_race->Append("fire");
    m_race->Append("energy");
    gridSizer->Add(m_race, 1, wxEXPAND);
    
    // Experience
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Experience:"), 0, wxALIGN_CENTER_VERTICAL);
    m_experience = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999999, 1000);
    gridSizer->Add(m_experience, 1, wxEXPAND);
    
    basicSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(basicSizer, 0, wxEXPAND | wxALL, 5);
    
    // Health and Speed section
    wxStaticBoxSizer* statsSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Stats");
    
    wxFlexGridSizer* statGridSizer = new wxFlexGridSizer(3, 2, 5, 5);
    statGridSizer->AddGrowableCol(1);
    
    // Speed
    statGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Speed:"), 0, wxALIGN_CENTER_VERTICAL);
    m_speed = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9999, 100);
    statGridSizer->Add(m_speed, 1, wxEXPAND);
    
    // Health Max
    statGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Health Max:"), 0, wxALIGN_CENTER_VERTICAL);
    m_healthMax = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999999, 500);
    statGridSizer->Add(m_healthMax, 1, wxEXPAND);
    
    // Mana Cost
    statGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Mana Cost:"), 0, wxALIGN_CENTER_VERTICAL);
    m_manacost = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 9999, 0);
    statGridSizer->Add(m_manacost, 1, wxEXPAND);
    
    statsSizer->Add(statGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(statsSizer, 0, wxEXPAND | wxALL, 5);
    
    // Look section
    wxStaticBoxSizer* lookSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Appearance");
    
    wxFlexGridSizer* lookGridSizer = new wxFlexGridSizer(5, 2, 5, 5);
    lookGridSizer->AddGrowableCol(1);
    
    // Look Type
    lookGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Look Type:"), 0, wxALIGN_CENTER_VERTICAL);
    m_lookType = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9999, 1);
    lookGridSizer->Add(m_lookType, 1, wxEXPAND);
    
    // Head Color
    lookGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Head Color:"), 0, wxALIGN_CENTER_VERTICAL);
    m_head = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 132, 0);
    lookGridSizer->Add(m_head, 1, wxEXPAND);
    
    // Body Color
    lookGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Body Color:"), 0, wxALIGN_CENTER_VERTICAL);
    m_body = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 132, 0);
    lookGridSizer->Add(m_body, 1, wxEXPAND);
    
    // Legs Color
    lookGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Legs Color:"), 0, wxALIGN_CENTER_VERTICAL);
    m_legs = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 132, 0);
    lookGridSizer->Add(m_legs, 1, wxEXPAND);
    
    // Feet Color
    lookGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Feet Color:"), 0, wxALIGN_CENTER_VERTICAL);
    m_feet = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 132, 0);
    lookGridSizer->Add(m_feet, 1, wxEXPAND);
    
    lookSizer->Add(lookGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(lookSizer, 0, wxEXPAND | wxALL, 5);
    
    // Preview section
    wxStaticBoxSizer* previewSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Preview");
    m_previewPanel = new MonsterPreviewPanel(panel, this);
    
    previewSizer->Add(m_previewPanel, 0, wxALIGN_CENTER | wxALL, 5);
    mainSizer->Add(previewSizer, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(mainSizer);
}

void MonsterMakerWindow::CreateFlagsTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Flags");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Behavior flags
    wxStaticBoxSizer* behaviorSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Behavior");
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(3, 3, 5, 5);
    
    m_summonable = new wxCheckBox(panel, wxID_ANY, "Summonable");
    m_attackable = new wxCheckBox(panel, wxID_ANY, "Attackable");
    m_hostile = new wxCheckBox(panel, wxID_ANY, "Hostile");
    m_illusionable = new wxCheckBox(panel, wxID_ANY, "Illusionable");
    m_convinceable = new wxCheckBox(panel, wxID_ANY, "Convinceable");
    m_pushable = new wxCheckBox(panel, wxID_ANY, "Pushable");
    m_canpushitems = new wxCheckBox(panel, wxID_ANY, "Can Push Items");
    m_canpushcreatures = new wxCheckBox(panel, wxID_ANY, "Can Push Creatures");
    m_hidehealth = new wxCheckBox(panel, wxID_ANY, "Hide Health");
    
    gridSizer->Add(m_summonable, 0, 0, 0);
    gridSizer->Add(m_attackable, 0, 0, 0);
    gridSizer->Add(m_hostile, 0, 0, 0);
    gridSizer->Add(m_illusionable, 0, 0, 0);
    gridSizer->Add(m_convinceable, 0, 0, 0);
    gridSizer->Add(m_pushable, 0, 0, 0);
    gridSizer->Add(m_canpushitems, 0, 0, 0);
    gridSizer->Add(m_canpushcreatures, 0, 0, 0);
    gridSizer->Add(m_hidehealth, 0, 0, 0);
    
    behaviorSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(behaviorSizer, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(mainSizer);
}

void MonsterMakerWindow::CreateAttacksTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Attacks");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    m_attacksList = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
    m_attacksList->AppendColumn("Name", wxLIST_FORMAT_LEFT, 100);
    m_attacksList->AppendColumn("Type", wxLIST_FORMAT_LEFT, 80);
    m_attacksList->AppendColumn("Damage", wxLIST_FORMAT_LEFT, 80);
    
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Attacks (Coming Soon)"), 0, wxALL, 5);
    sizer->Add(m_attacksList, 1, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateDefensesTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Defenses");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    m_defensesList = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
    m_defensesList->AppendColumn("Name", wxLIST_FORMAT_LEFT, 100);
    m_defensesList->AppendColumn("Type", wxLIST_FORMAT_LEFT, 80);
    
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Defenses (Coming Soon)"), 0, wxALL, 5);
    sizer->Add(m_defensesList, 1, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateElementsTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Elements");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Element resistances (Coming Soon)"), 0, wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateImmunitiesTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Immunities");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Immunities (Coming Soon)"), 0, wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateSummonsTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Summons");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Summons (Coming Soon)"), 0, wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateVoicesTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Voices");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Monster voices (Coming Soon)"), 0, wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateLootTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Loot");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Loot table (Coming Soon)"), 0, wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateIOTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Load/Save");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Monsters list
    wxStaticBoxSizer* listSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Available Monsters");
    
    m_monstersList = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    m_monstersList->AppendColumn("Name", wxLIST_FORMAT_LEFT, 200);
    m_monstersList->AppendColumn("File", wxLIST_FORMAT_LEFT, 250);
    
    // Populate with loaded monsters
    if (g_gui.monster_manager.isLoaded()) {
        const auto& monsters = g_gui.monster_manager.getAllMonsters();
        int index = 0;
        for (const auto& entry : monsters) {
            long itemIndex = m_monstersList->InsertItem(index, wxstr(entry.second.name));
            m_monstersList->SetItem(itemIndex, 1, wxstr(entry.second.filename));
            index++;
        }
    }
    
    listSizer->Add(m_monstersList, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(listSizer, 1, wxEXPAND | wxALL, 5);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_loadButton = new wxButton(panel, ID_LOAD_MONSTER, "Load Selected");
    buttonSizer->Add(m_loadButton, 0, wxRIGHT, 5);
    
    buttonSizer->Add(new wxButton(panel, wxID_ANY, "Refresh List"), 0, wxRIGHT, 5);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(mainSizer);
}

void MonsterMakerWindow::RefreshMonsterList() {
    if (m_monstersList && g_gui.monster_manager.isLoaded()) {
        m_monstersList->DeleteAllItems();
        const auto& monsters = g_gui.monster_manager.getAllMonsters();
        int index = 0;
        for (const auto& entry : monsters) {
            long itemIndex = m_monstersList->InsertItem(index, wxstr(entry.second.name));
            m_monstersList->SetItem(itemIndex, 1, wxstr(entry.second.filename));
            index++;
        }
    }
}

void MonsterMakerWindow::LoadMonster(const MonsterEntry& monster) {
    m_name->SetValue(wxstr(monster.name));
    m_race->SetValue(wxstr(monster.race));
    m_experience->SetValue(monster.experience);
    m_speed->SetValue(monster.speed);
    m_manacost->SetValue(monster.manacost);
    m_healthMax->SetValue(monster.health_max);
    m_lookType->SetValue(monster.look_type);
    m_head->SetValue(monster.look_head);
    m_body->SetValue(monster.look_body);
    m_legs->SetValue(monster.look_legs);
    
    UpdatePreview();
}

void MonsterMakerWindow::ClearAll() {
    m_name->SetValue("");
    m_nameDescription->SetValue("");
    m_race->SetValue("blood");
    m_experience->SetValue(1000);
    m_speed->SetValue(100);
    m_manacost->SetValue(0);
    m_healthMax->SetValue(500);
    m_lookType->SetValue(1);
    m_head->SetValue(0);
    m_body->SetValue(0);
    m_legs->SetValue(0);
    m_feet->SetValue(0);
    
    // Clear flags
    m_summonable->SetValue(false);
    m_attackable->SetValue(true);
    m_hostile->SetValue(true);
    m_illusionable->SetValue(false);
    m_convinceable->SetValue(false);
    m_pushable->SetValue(false);
    m_canpushitems->SetValue(false);
    m_canpushcreatures->SetValue(false);
    m_hidehealth->SetValue(false);
    
    UpdatePreview();
}

void MonsterMakerWindow::UpdatePreview() {
    if (!m_previewPanel) return;
    
    // Get current look values
    int lookType = m_lookType->GetValue();
    int headColor = m_head->GetValue();
    int bodyColor = m_body->GetValue();
    int legsColor = m_legs->GetValue();
    int feetColor = m_feet ? m_feet->GetValue() : 0;
    
    // Validate look type and use fallback if needed
    if (lookType <= 0) {
        lookType = 5; // Fallback to look type 5
        if (m_lookType->GetValue() != lookType) {
            m_lookType->SetValue(lookType);
        }
    }
    
    // Create a client device context for drawing
    wxClientDC dc(m_previewPanel);
    
    // Clear the panel with gray background
    wxSize panelSize = m_previewPanel->GetSize();
    dc.SetBrush(wxBrush(wxColour(192, 192, 192))); // Light gray
    dc.SetPen(wxPen(wxColour(128, 128, 128))); // Gray border
    dc.DrawRectangle(0, 0, panelSize.x, panelSize.y);
    
    // Try to render the monster sprite using the creature sprite manager
    wxBitmap* spriteBitmap = nullptr;
    
    // Use creature sprite manager to get the rendered sprite
    if (headColor > 0 || bodyColor > 0 || legsColor > 0 || feetColor > 0) {
        spriteBitmap = g_creature_sprites.getSpriteBitmap(
            lookType, headColor, bodyColor, legsColor, feetColor, 64, 64);
    } else {
        spriteBitmap = g_creature_sprites.getSpriteBitmap(lookType, 64, 64);
    }
    
    if (spriteBitmap) {
        // Calculate center position for the sprite
        int x = (panelSize.x - spriteBitmap->GetWidth()) / 2;
        int y = (panelSize.y - spriteBitmap->GetHeight()) / 2;
        
        // Draw the sprite
        dc.DrawBitmap(*spriteBitmap, x, y, true);
    } else {
        // Fallback: draw a simple colored representation
        DrawFallbackPreview(dc, panelSize, lookType, headColor, bodyColor, legsColor);
    }
}

void MonsterMakerWindow::DrawFallbackPreview(wxDC& dc, const wxSize& size, int lookType, int headColor, int bodyColor, int legsColor) {
    // Draw a simple colored representation
    int centerX = size.x / 2;
    int centerY = size.y / 2;
    
    // Draw a simple creature representation with color zones
    dc.SetBrush(wxBrush(wxColour(150 + (headColor * 2), 100, 100)));
    dc.SetPen(wxPen(wxColour(50, 50, 50)));
    dc.DrawCircle(centerX, centerY - 8, 8); // Head
    
    dc.SetBrush(wxBrush(wxColour(100, 150 + (bodyColor * 2), 100)));
    dc.DrawRectangle(centerX - 6, centerY - 2, 12, 10); // Body
    
    dc.SetBrush(wxBrush(wxColour(100, 100, 150 + (legsColor * 2))));
    dc.DrawRectangle(centerX - 4, centerY + 6, 8, 6); // Legs
    
    // Draw look type number
    dc.SetTextForeground(wxColour(0, 0, 0));
    dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    wxString lookText = wxString::Format("Type: %d", lookType);
    wxSize textSize = dc.GetTextExtent(lookText);
    dc.DrawText(lookText, (size.x - textSize.x) / 2, size.y - textSize.y - 2);
}

void MonsterMakerWindow::OnClose(wxCloseEvent& event) {
    Hide();
}

void MonsterMakerWindow::OnCloseButton(wxCommandEvent& event) {
    Hide();
}

void MonsterMakerWindow::OnCreateMonster(wxCommandEvent& event) {
    // Create monster entry from current UI values
    MonsterEntry entry;
    entry.name = nstr(m_name->GetValue());
    entry.race = nstr(m_race->GetValue());
    entry.experience = m_experience->GetValue();
    entry.speed = m_speed->GetValue();
    entry.manacost = m_manacost->GetValue();
    entry.health_max = m_healthMax->GetValue();
    entry.look_type = m_lookType->GetValue();
    entry.look_head = m_head->GetValue();
    entry.look_body = m_body->GetValue();
    entry.look_legs = m_legs->GetValue();
    
    // Ask for save location
    wxFileDialog saveDialog(this, "Save Monster XML", "", entry.name + ".xml", 
                           "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (saveDialog.ShowModal() == wxID_OK) {
        wxString path = saveDialog.GetPath();
        if (g_gui.monster_manager.createMonsterXML(entry, nstr(path))) {
            wxMessageBox("Monster created successfully!", "Success", wxOK | wxICON_INFORMATION);
        } else {
            wxMessageBox("Failed to create monster file!", "Error", wxOK | wxICON_ERROR);
        }
    }
}

void MonsterMakerWindow::OnLoadMonster(wxCommandEvent& event) {
    long selectedIndex = m_monstersList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selectedIndex == -1) {
        wxMessageBox("Please select a monster to load.", "No Selection", wxOK | wxICON_WARNING);
        return;
    }
    
    wxString monsterName = m_monstersList->GetItemText(selectedIndex, 0);
    MonsterEntry* entry = g_gui.monster_manager.findByName(nstr(monsterName));
    
    if (entry) {
        // Load details if not already loaded
        if (!entry->loaded) {
            g_gui.monster_manager.loadMonsterDetails(*entry);
        }
        LoadMonster(*entry);
        m_notebook->SetSelection(0); // Switch to Monster tab
    } else {
        wxMessageBox("Monster not found!", "Error", wxOK | wxICON_ERROR);
    }
}

void MonsterMakerWindow::OnPreviewUpdate(wxCommandEvent& event) {
    UpdatePreview();
}

void MonsterMakerWindow::OnTabChange(wxNotebookEvent& event) {
    // Handle tab changes if needed
    event.Skip();
}

void MonsterMakerWindow::OnLookTypeChange(wxCommandEvent& event) {
    // Start timer for delayed preview update (150ms delay)
    if (m_previewTimer && !m_previewTimer->IsRunning()) {
        m_previewUpdatePending = true;
        m_previewTimer->Start(150, wxTIMER_ONE_SHOT);
    }
    event.Skip();
}

void MonsterMakerWindow::OnColorChange(wxCommandEvent& event) {
    // Start timer for delayed preview update (150ms delay)
    if (m_previewTimer && !m_previewTimer->IsRunning()) {
        m_previewUpdatePending = true;
        m_previewTimer->Start(150, wxTIMER_ONE_SHOT);
    }
    event.Skip();
}

void MonsterMakerWindow::OnPreviewTimer(wxTimerEvent& event) {
    if (m_previewUpdatePending) {
        UpdatePreview();
        m_previewUpdatePending = false;
    }
} 