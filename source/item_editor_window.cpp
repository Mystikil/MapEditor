#include "main.h"
#include "item_editor_window.h"
#include "gui.h"
#include "items.h"
#include "graphics.h"
#include "filehandle.h"
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/progdlg.h>
#include <wx/textdlg.h>

// Event table
wxBEGIN_EVENT_TABLE(ItemEditorWindow, wxDialog)
	EVT_BUTTON(wxID_OK, ItemEditorWindow::OnOK)
	EVT_BUTTON(wxID_CANCEL, ItemEditorWindow::OnCancel)
	EVT_LIST_ITEM_SELECTED(ID_ITEM_LIST, ItemEditorWindow::OnItemSelected)
	EVT_BUTTON(ID_SAVE_OTB, ItemEditorWindow::OnSaveOTB)
	EVT_BUTTON(ID_RELOAD_OTB, ItemEditorWindow::OnReloadOTB)
	EVT_BUTTON(ID_GENERATE_MISSING, ItemEditorWindow::OnGenerateMissing)
	EVT_BUTTON(ID_CREATE_ITEM, ItemEditorWindow::OnCreateItem)
	EVT_BUTTON(ID_DUPLICATE_ITEM, ItemEditorWindow::OnDuplicateItem)
	EVT_BUTTON(ID_DELETE_ITEM, ItemEditorWindow::OnDeleteItem)
	EVT_BUTTON(ID_FIND_ITEM, ItemEditorWindow::OnFindItem)
	EVT_CHOICE(ID_GROUP_CHOICE, ItemEditorWindow::OnGroupChanged)
	EVT_CHOICE(ID_TYPE_CHOICE, ItemEditorWindow::OnTypeChanged)
wxEND_EVENT_TABLE()

ItemEditorWindow::ItemEditorWindow(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "Item Editor", wxDefaultPosition, wxSize(1000, 700), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX)
	, m_itemList(nullptr)
	, m_notebook(nullptr)
	, m_currentItemId(0)
	, m_itemsModified(false)
{
	SetIcon(wxICON(editor_icon));
	CreateControls();
	LoadItemList();
	EnablePropertyControls(false);
}

ItemEditorWindow::~ItemEditorWindow()
{
	// Destructor - cleanup is handled automatically by wxWidgets
}

void ItemEditorWindow::CreateControls()
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Create splitter window for item list and properties
	wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxSP_LIVE_UPDATE);
	
	// Left panel - Item list
	wxPanel* leftPanel = new wxPanel(splitter);
	CreateItemList(leftPanel);
	
	// Right panel - Properties
	wxPanel* rightPanel = new wxPanel(splitter);
	CreatePropertiesPanel(rightPanel);
	
	splitter->SplitVertically(leftPanel, rightPanel, 350);
	splitter->SetMinimumPaneSize(200);
	
	mainSizer->Add(splitter, 1, wxEXPAND | wxALL, 5);
	
	// Button panel
	CreateButtonPanel();
	mainSizer->Add(m_buttonPanel, 0, wxEXPAND | wxALL, 5);
	
	// OK/Cancel buttons
	wxStdDialogButtonSizer* buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
	mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
	
	SetSizer(mainSizer);
}

void ItemEditorWindow::CreateItemList(wxPanel* parent)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	
	wxStaticText* label = new wxStaticText(parent, wxID_ANY, "Items:");
	sizer->Add(label, 0, wxALL, 5);
	
	m_itemList = new wxListCtrl(parent, ID_ITEM_LIST, wxDefaultPosition, wxDefaultSize, 
		wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
	
	// Add columns
	m_itemList->InsertColumn(0, "ID", wxLIST_FORMAT_LEFT, 80);
	m_itemList->InsertColumn(1, "Client ID", wxLIST_FORMAT_LEFT, 80);
	m_itemList->InsertColumn(2, "Name", wxLIST_FORMAT_LEFT, 180);
	m_itemList->InsertColumn(3, "Group", wxLIST_FORMAT_LEFT, 100);
	
	sizer->Add(m_itemList, 1, wxEXPAND | wxALL, 5);
	parent->SetSizer(sizer);
}

void ItemEditorWindow::CreatePropertiesPanel(wxPanel* parent)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	
	wxStaticText* label = new wxStaticText(parent, wxID_ANY, "Item Properties:");
	sizer->Add(label, 0, wxALL, 5);
	
	m_notebook = new wxNotebook(parent, ID_PROPERTIES_NOTEBOOK);
	
	// General properties tab
	m_generalPanel = new wxPanel(m_notebook);
	CreateGeneralTab();
	m_notebook->AddPage(m_generalPanel, "General");
	
	// Flags tab
	m_flagsPanel = new wxPanel(m_notebook);
	CreateFlagsTab();
	m_notebook->AddPage(m_flagsPanel, "Flags");
	
	// Attributes tab
	m_attributesPanel = new wxPanel(m_notebook);
	CreateAttributesTab();
	m_notebook->AddPage(m_attributesPanel, "Attributes");
	
	sizer->Add(m_notebook, 1, wxEXPAND | wxALL, 5);
	parent->SetSizer(sizer);
}

void ItemEditorWindow::CreateGeneralTab()
{
	wxFlexGridSizer* sizer = new wxFlexGridSizer(2, 5, 5);
	sizer->AddGrowableCol(1, 1);
	
	// Server ID
	sizer->Add(new wxStaticText(m_generalPanel, wxID_ANY, "Server ID:"), 0, wxALIGN_CENTER_VERTICAL);
	m_serverIdCtrl = new wxSpinCtrl(m_generalPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 65535);
	sizer->Add(m_serverIdCtrl, 1, wxEXPAND);
	
	// Client ID
	sizer->Add(new wxStaticText(m_generalPanel, wxID_ANY, "Client ID:"), 0, wxALIGN_CENTER_VERTICAL);
	m_clientIdCtrl = new wxSpinCtrl(m_generalPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 65535);
	sizer->Add(m_clientIdCtrl, 1, wxEXPAND);
	
	// Name
	sizer->Add(new wxStaticText(m_generalPanel, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
	m_nameCtrl = new wxTextCtrl(m_generalPanel, wxID_ANY);
	sizer->Add(m_nameCtrl, 1, wxEXPAND);
	
	// Description
	sizer->Add(new wxStaticText(m_generalPanel, wxID_ANY, "Description:"), 0, wxALIGN_CENTER_VERTICAL);
	m_descCtrl = new wxTextCtrl(m_generalPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	sizer->Add(m_descCtrl, 1, wxEXPAND);
	
	// Group
	sizer->Add(new wxStaticText(m_generalPanel, wxID_ANY, "Group:"), 0, wxALIGN_CENTER_VERTICAL);
	m_groupChoice = new wxChoice(m_generalPanel, ID_GROUP_CHOICE);
	m_groupChoice->Append("None");
	m_groupChoice->Append("Ground");
	m_groupChoice->Append("Container");
	m_groupChoice->Append("Weapon");
	m_groupChoice->Append("Ammunition");
	m_groupChoice->Append("Armor");
	m_groupChoice->Append("Rune");
	m_groupChoice->Append("Teleport");
	m_groupChoice->Append("Magic Field");
	m_groupChoice->Append("Writeable");
	m_groupChoice->Append("Key");
	m_groupChoice->Append("Splash");
	m_groupChoice->Append("Fluid");
	m_groupChoice->Append("Door");
	m_groupChoice->Append("Podium");
	sizer->Add(m_groupChoice, 1, wxEXPAND);
	
	// Type
	sizer->Add(new wxStaticText(m_generalPanel, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL);
	m_typeChoice = new wxChoice(m_generalPanel, ID_TYPE_CHOICE);
	m_typeChoice->Append("None");
	m_typeChoice->Append("Depot");
	m_typeChoice->Append("Mailbox");
	m_typeChoice->Append("Trash Holder");
	m_typeChoice->Append("Container");
	m_typeChoice->Append("Door");
	m_typeChoice->Append("Magic Field");
	m_typeChoice->Append("Teleport");
	m_typeChoice->Append("Bed");
	m_typeChoice->Append("Key");
	m_typeChoice->Append("Podium");
	sizer->Add(m_typeChoice, 1, wxEXPAND);
	
	// Weight
	sizer->Add(new wxStaticText(m_generalPanel, wxID_ANY, "Weight:"), 0, wxALIGN_CENTER_VERTICAL);
	m_weightCtrl = new wxTextCtrl(m_generalPanel, wxID_ANY, "0.0");
	sizer->Add(m_weightCtrl, 1, wxEXPAND);
	
	m_generalPanel->SetSizer(sizer);
}

void ItemEditorWindow::CreateFlagsTab()
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	
	// Movement flags
	wxStaticBoxSizer* movementBox = new wxStaticBoxSizer(wxVERTICAL, m_flagsPanel, "Movement");
	m_unpassableFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Unpassable");
	m_blockMissilesFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Block Missiles");
	m_blockPathfinderFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Block Pathfinder");
	m_hasElevationFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Has Elevation");
	movementBox->Add(m_unpassableFlag, 0, wxALL, 2);
	movementBox->Add(m_blockMissilesFlag, 0, wxALL, 2);
	movementBox->Add(m_blockPathfinderFlag, 0, wxALL, 2);
	movementBox->Add(m_hasElevationFlag, 0, wxALL, 2);
	mainSizer->Add(movementBox, 0, wxEXPAND | wxALL, 5);
	
	// Item flags
	wxStaticBoxSizer* itemBox = new wxStaticBoxSizer(wxVERTICAL, m_flagsPanel, "Item Properties");
	m_pickupableFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Pickupable");
	m_moveableFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Moveable");
	m_stackableFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Stackable");
	m_readableFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Readable");
	m_rotableFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Rotatable");
	itemBox->Add(m_pickupableFlag, 0, wxALL, 2);
	itemBox->Add(m_moveableFlag, 0, wxALL, 2);
	itemBox->Add(m_stackableFlag, 0, wxALL, 2);
	itemBox->Add(m_readableFlag, 0, wxALL, 2);
	itemBox->Add(m_rotableFlag, 0, wxALL, 2);
	mainSizer->Add(itemBox, 0, wxEXPAND | wxALL, 5);
	
	// Hook flags
	wxStaticBoxSizer* hookBox = new wxStaticBoxSizer(wxVERTICAL, m_flagsPanel, "Hooks");
	m_hangableFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Hangable");
	m_hookEastFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Hook East");
	m_hookSouthFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Hook South");
	m_allowDistReadFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Allow Distance Read");
	hookBox->Add(m_hangableFlag, 0, wxALL, 2);
	hookBox->Add(m_hookEastFlag, 0, wxALL, 2);
	hookBox->Add(m_hookSouthFlag, 0, wxALL, 2);
	hookBox->Add(m_allowDistReadFlag, 0, wxALL, 2);
	mainSizer->Add(hookBox, 0, wxEXPAND | wxALL, 5);
	
	// Floor change flags
	wxStaticBoxSizer* floorBox = new wxStaticBoxSizer(wxVERTICAL, m_flagsPanel, "Floor Changes");
	m_floorChangeDownFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Floor Change Down");
	m_floorChangeNorthFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Floor Change North");
	m_floorChangeSouthFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Floor Change South");
	m_floorChangeEastFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Floor Change East");
	m_floorChangeWestFlag = new wxCheckBox(m_flagsPanel, wxID_ANY, "Floor Change West");
	floorBox->Add(m_floorChangeDownFlag, 0, wxALL, 2);
	floorBox->Add(m_floorChangeNorthFlag, 0, wxALL, 2);
	floorBox->Add(m_floorChangeSouthFlag, 0, wxALL, 2);
	floorBox->Add(m_floorChangeEastFlag, 0, wxALL, 2);
	floorBox->Add(m_floorChangeWestFlag, 0, wxALL, 2);
	mainSizer->Add(floorBox, 0, wxEXPAND | wxALL, 5);
	
	m_flagsPanel->SetSizer(mainSizer);
}

void ItemEditorWindow::CreateAttributesTab()
{
	wxFlexGridSizer* sizer = new wxFlexGridSizer(2, 5, 5);
	sizer->AddGrowableCol(1, 1);
	
	// Light Level
	sizer->Add(new wxStaticText(m_attributesPanel, wxID_ANY, "Light Level:"), 0, wxALIGN_CENTER_VERTICAL);
	m_lightLevelCtrl = new wxSpinCtrl(m_attributesPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255);
	sizer->Add(m_lightLevelCtrl, 1, wxEXPAND);
	
	// Light Color
	sizer->Add(new wxStaticText(m_attributesPanel, wxID_ANY, "Light Color:"), 0, wxALIGN_CENTER_VERTICAL);
	m_lightColorCtrl = new wxSpinCtrl(m_attributesPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255);
	sizer->Add(m_lightColorCtrl, 1, wxEXPAND);
	
	// Minimap Color
	sizer->Add(new wxStaticText(m_attributesPanel, wxID_ANY, "Minimap Color:"), 0, wxALIGN_CENTER_VERTICAL);
	m_minimapColorCtrl = new wxSpinCtrl(m_attributesPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255);
	sizer->Add(m_minimapColorCtrl, 1, wxEXPAND);
	
	// Max Text Length
	sizer->Add(new wxStaticText(m_attributesPanel, wxID_ANY, "Max Text Length:"), 0, wxALIGN_CENTER_VERTICAL);
	m_maxTextLenCtrl = new wxSpinCtrl(m_attributesPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535);
	sizer->Add(m_maxTextLenCtrl, 1, wxEXPAND);
	
	// Rotate To
	sizer->Add(new wxStaticText(m_attributesPanel, wxID_ANY, "Rotate To:"), 0, wxALIGN_CENTER_VERTICAL);
	m_rotateToCtrl = new wxSpinCtrl(m_attributesPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535);
	sizer->Add(m_rotateToCtrl, 1, wxEXPAND);
	
	// Volume
	sizer->Add(new wxStaticText(m_attributesPanel, wxID_ANY, "Volume:"), 0, wxALIGN_CENTER_VERTICAL);
	m_volumeCtrl = new wxSpinCtrl(m_attributesPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535);
	sizer->Add(m_volumeCtrl, 1, wxEXPAND);
	
	m_attributesPanel->SetSizer(sizer);
}

void ItemEditorWindow::CreateButtonPanel()
{
	m_buttonPanel = new wxPanel(this);
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	
	m_saveButton = new wxButton(m_buttonPanel, ID_SAVE_OTB, "Save OTB");
	m_reloadButton = new wxButton(m_buttonPanel, ID_RELOAD_OTB, "Reload OTB");
	m_generateButton = new wxButton(m_buttonPanel, ID_GENERATE_MISSING, "Generate Missing");
	
	sizer->Add(m_saveButton, 0, wxALL, 5);
	sizer->Add(m_reloadButton, 0, wxALL, 5);
	sizer->Add(m_generateButton, 0, wxALL, 5);
	sizer->AddStretchSpacer(1);
	
	m_createButton = new wxButton(m_buttonPanel, ID_CREATE_ITEM, "Create Item");
	m_duplicateButton = new wxButton(m_buttonPanel, ID_DUPLICATE_ITEM, "Duplicate");
	m_deleteButton = new wxButton(m_buttonPanel, ID_DELETE_ITEM, "Delete");
	m_findButton = new wxButton(m_buttonPanel, ID_FIND_ITEM, "Find");
	
	sizer->Add(m_createButton, 0, wxALL, 5);
	sizer->Add(m_duplicateButton, 0, wxALL, 5);
	sizer->Add(m_deleteButton, 0, wxALL, 5);
	sizer->Add(m_findButton, 0, wxALL, 5);
	
	m_buttonPanel->SetSizer(sizer);
}

void ItemEditorWindow::LoadItemList()
{
	m_itemList->DeleteAllItems();
	
	for (uint16_t id = 100; id <= g_items.getMaxID(); ++id) {
		if (g_items.typeExists(id)) {
			const ItemType& item = g_items[id];
			
			long index = m_itemList->InsertItem(m_itemList->GetItemCount(), wxString::Format("%d", id));
			m_itemList->SetItem(index, 1, wxString::Format("%d", item.clientID));
			m_itemList->SetItem(index, 2, wxString(item.name.c_str(), wxConvUTF8));
			
			wxString groupName;
			switch (item.group) {
				case ITEM_GROUP_NONE: groupName = "None"; break;
				case ITEM_GROUP_GROUND: groupName = "Ground"; break;
				case ITEM_GROUP_CONTAINER: groupName = "Container"; break;
				case ITEM_GROUP_WEAPON: groupName = "Weapon"; break;
				case ITEM_GROUP_AMMUNITION: groupName = "Ammunition"; break;
				case ITEM_GROUP_ARMOR: groupName = "Armor"; break;
				case ITEM_GROUP_RUNE: groupName = "Rune"; break;
				case ITEM_GROUP_TELEPORT: groupName = "Teleport"; break;
				case ITEM_GROUP_MAGICFIELD: groupName = "Magic Field"; break;
				case ITEM_GROUP_WRITEABLE: groupName = "Writeable"; break;
				case ITEM_GROUP_KEY: groupName = "Key"; break;
				case ITEM_GROUP_SPLASH: groupName = "Splash"; break;
				case ITEM_GROUP_FLUID: groupName = "Fluid"; break;
				case ITEM_GROUP_DOOR: groupName = "Door"; break;
				case ITEM_GROUP_PODIUM: groupName = "Podium"; break;
				default: groupName = "Unknown"; break;
			}
			m_itemList->SetItem(index, 3, groupName);
			
			// Store the item ID as data
			m_itemList->SetItemData(index, id);
		}
	}
}

void ItemEditorWindow::LoadItemProperties(uint16_t itemId)
{
	if (!g_items.typeExists(itemId)) {
		EnablePropertyControls(false);
		return;
	}
	
	const ItemType& item = g_items[itemId];
	
	// General properties
	m_serverIdCtrl->SetValue(itemId);
	m_clientIdCtrl->SetValue(item.clientID);
	m_nameCtrl->SetValue(wxString(item.name.c_str(), wxConvUTF8));
	m_descCtrl->SetValue(wxString(item.description.c_str(), wxConvUTF8));
	m_groupChoice->SetSelection(item.group);
	m_typeChoice->SetSelection(item.type);
	m_weightCtrl->SetValue(wxString::Format("%.2f", item.weight));
	
	// Flags
	m_unpassableFlag->SetValue(item.unpassable);
	m_blockMissilesFlag->SetValue(item.blockMissiles);
	m_blockPathfinderFlag->SetValue(item.blockPathfinder);
	m_hasElevationFlag->SetValue(item.hasElevation);
	m_pickupableFlag->SetValue(item.pickupable);
	m_moveableFlag->SetValue(item.moveable);
	m_stackableFlag->SetValue(item.stackable);
	m_readableFlag->SetValue(item.canReadText);
	m_rotableFlag->SetValue(item.rotable);
	m_hangableFlag->SetValue(item.isHangable);
	m_hookEastFlag->SetValue(item.hookEast);
	m_hookSouthFlag->SetValue(item.hookSouth);
	m_allowDistReadFlag->SetValue(item.allowDistRead);
	m_floorChangeDownFlag->SetValue(item.floorChangeDown);
	m_floorChangeNorthFlag->SetValue(item.floorChangeNorth);
	m_floorChangeSouthFlag->SetValue(item.floorChangeSouth);
	m_floorChangeEastFlag->SetValue(item.floorChangeEast);
	m_floorChangeWestFlag->SetValue(item.floorChangeWest);
	
	// Attributes
	m_maxTextLenCtrl->SetValue(item.maxTextLen);
	m_rotateToCtrl->SetValue(item.rotateTo);
	m_volumeCtrl->SetValue(item.volume);
	
	// Enable controls
	EnablePropertyControls(true);
	m_currentItemId = itemId;
}

void ItemEditorWindow::SaveItemProperties()
{
	if (m_currentItemId == 0 || !g_items.typeExists(m_currentItemId)) {
		return;
	}
	
	ItemType& item = g_items.getItemType(m_currentItemId);
	ApplyChangesToItem(&item);
	m_itemsModified = true;
}

void ItemEditorWindow::ApplyChangesToItem(ItemType* item)
{
	if (!item) return;
	
	// General properties
	item->clientID = m_clientIdCtrl->GetValue();
	item->name = m_nameCtrl->GetValue().ToStdString();
	item->description = m_descCtrl->GetValue().ToStdString();
	item->group = static_cast<ItemGroup_t>(m_groupChoice->GetSelection());
	item->type = static_cast<ItemTypes_t>(m_typeChoice->GetSelection());
	
	double weight;
	if (m_weightCtrl->GetValue().ToDouble(&weight)) {
		item->weight = weight;
	}
	
	// Flags
	item->unpassable = m_unpassableFlag->GetValue();
	item->blockMissiles = m_blockMissilesFlag->GetValue();
	item->blockPathfinder = m_blockPathfinderFlag->GetValue();
	item->hasElevation = m_hasElevationFlag->GetValue();
	item->pickupable = m_pickupableFlag->GetValue();
	item->moveable = m_moveableFlag->GetValue();
	item->stackable = m_stackableFlag->GetValue();
	item->canReadText = m_readableFlag->GetValue();
	item->rotable = m_rotableFlag->GetValue();
	item->isHangable = m_hangableFlag->GetValue();
	item->hookEast = m_hookEastFlag->GetValue();
	item->hookSouth = m_hookSouthFlag->GetValue();
	item->allowDistRead = m_allowDistReadFlag->GetValue();
	item->floorChangeDown = m_floorChangeDownFlag->GetValue();
	item->floorChangeNorth = m_floorChangeNorthFlag->GetValue();
	item->floorChangeSouth = m_floorChangeSouthFlag->GetValue();
	item->floorChangeEast = m_floorChangeEastFlag->GetValue();
	item->floorChangeWest = m_floorChangeWestFlag->GetValue();
	
	// Update floor change flag
	item->floorChange = item->floorChangeDown || item->floorChangeNorth || 
		item->floorChangeSouth || item->floorChangeEast || item->floorChangeWest;
	
	// Attributes
	item->maxTextLen = m_maxTextLenCtrl->GetValue();
	item->rotateTo = m_rotateToCtrl->GetValue();
	item->volume = m_volumeCtrl->GetValue();
}

void ItemEditorWindow::EnablePropertyControls(bool enable)
{
	// General tab
	m_clientIdCtrl->Enable(enable);
	m_nameCtrl->Enable(enable);
	m_descCtrl->Enable(enable);
	m_groupChoice->Enable(enable);
	m_typeChoice->Enable(enable);
	m_weightCtrl->Enable(enable);
	
	// Flags tab
	m_unpassableFlag->Enable(enable);
	m_blockMissilesFlag->Enable(enable);
	m_blockPathfinderFlag->Enable(enable);
	m_hasElevationFlag->Enable(enable);
	m_pickupableFlag->Enable(enable);
	m_moveableFlag->Enable(enable);
	m_stackableFlag->Enable(enable);
	m_readableFlag->Enable(enable);
	m_rotableFlag->Enable(enable);
	m_hangableFlag->Enable(enable);
	m_hookEastFlag->Enable(enable);
	m_hookSouthFlag->Enable(enable);
	m_allowDistReadFlag->Enable(enable);
	m_floorChangeDownFlag->Enable(enable);
	m_floorChangeNorthFlag->Enable(enable);
	m_floorChangeSouthFlag->Enable(enable);
	m_floorChangeEastFlag->Enable(enable);
	m_floorChangeWestFlag->Enable(enable);
	
	// Attributes tab
	m_lightLevelCtrl->Enable(enable);
	m_lightColorCtrl->Enable(enable);
	m_minimapColorCtrl->Enable(enable);
	m_maxTextLenCtrl->Enable(enable);
	m_rotateToCtrl->Enable(enable);
	m_volumeCtrl->Enable(enable);
	
	// Buttons
	m_duplicateButton->Enable(enable);
	m_deleteButton->Enable(enable);
}

// Event handlers
void ItemEditorWindow::OnOK(wxCommandEvent& event)
{
	if (m_itemsModified) {
		SaveItemProperties();
		
		int response = wxMessageBox("Save changes to items.otb?", "Save Changes", 
			wxYES_NO | wxCANCEL | wxICON_QUESTION, this);
		
		if (response == wxYES) {
			if (!SaveOTBFile()) {
				return; // Don't close if save failed
			}
		} else if (response == wxCANCEL) {
			return; // Don't close
		}
	}
	
	EndModal(wxID_OK);
}

void ItemEditorWindow::OnCancel(wxCommandEvent& event)
{
	if (m_itemsModified) {
		int response = wxMessageBox("Discard changes to items?", "Confirm", 
			wxYES_NO | wxICON_QUESTION, this);
		
		if (response != wxYES) {
			return; // Don't close
		}
	}
	
	EndModal(wxID_CANCEL);
}

void ItemEditorWindow::OnItemSelected(wxListEvent& event)
{
	SaveItemProperties(); // Save current item first
	
	long index = event.GetIndex();
	if (index >= 0) {
		uint16_t itemId = static_cast<uint16_t>(m_itemList->GetItemData(index));
		LoadItemProperties(itemId);
	}
}

void ItemEditorWindow::OnSaveOTB(wxCommandEvent& event)
{
	SaveItemProperties();
	SaveOTBFile();
}

void ItemEditorWindow::OnReloadOTB(wxCommandEvent& event)
{
	if (m_itemsModified) {
		int response = wxMessageBox("Discard unsaved changes and reload items.otb?", "Confirm", 
			wxYES_NO | wxICON_QUESTION, this);
		
		if (response != wxYES) {
			return;
		}
	}
	
	// Reload items.otb
	wxString error;
	wxArrayString warnings;
	
	// Get current OTB path
	ClientVersion* version = g_gui.getLoadedVersion();
	if (!version) {
		wxMessageBox("No version loaded.", "Error", wxOK | wxICON_ERROR, this);
		return;
	}
	
	FileName data_path = version->getDataPath();
	wxString otb_path = data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "items.otb";
	
	if (!g_items.loadFromOtb(otb_path, error, warnings)) {
		wxMessageBox("Failed to reload items.otb: " + error, "Error", wxOK | wxICON_ERROR, this);
		return;
	}
	
	// Refresh the UI
	LoadItemList();
	EnablePropertyControls(false);
	m_currentItemId = 0;
	m_itemsModified = false;
	
	wxMessageBox("Items.otb reloaded successfully.", "Success", wxOK | wxICON_INFORMATION, this);
}

void ItemEditorWindow::OnGenerateMissing(wxCommandEvent& event)
{
	GenerateMissingItems();
}

void ItemEditorWindow::OnCreateItem(wxCommandEvent& event)
{
	// Find next available ID
	uint16_t newId = g_items.getMaxID() + 1;
	
	// Create new item type
	ItemType* newItem = new ItemType();
	newItem->id = newId;
	newItem->clientID = 100; // Default client ID
	newItem->name = wxString::Format("New Item %d", newId).ToStdString();
	newItem->group = ITEM_GROUP_NONE;
	newItem->type = ITEM_TYPE_NONE;
	
	// Add to items database
	g_items.items.set(newId, newItem);
	
	// Refresh list and select new item
	LoadItemList();
	
	// Find and select the new item in the list
	for (int i = 0; i < m_itemList->GetItemCount(); ++i) {
		if (m_itemList->GetItemData(i) == newId) {
			m_itemList->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
			m_itemList->EnsureVisible(i);
			LoadItemProperties(newId);
			break;
		}
	}
	
	m_itemsModified = true;
}

void ItemEditorWindow::OnDuplicateItem(wxCommandEvent& event)
{
	if (m_currentItemId == 0) {
		wxMessageBox("No item selected.", "Error", wxOK | wxICON_ERROR, this);
		return;
	}
	
	SaveItemProperties(); // Save current changes
	
	// Find next available ID
	uint16_t newId = g_items.getMaxID() + 1;
	
	// Copy current item properties
	const ItemType& sourceItem = g_items[m_currentItemId];
	ItemType* newItem = new ItemType();
	
	// Copy all properties manually
	newItem->id = newId;
	newItem->clientID = sourceItem.clientID;
	newItem->name = sourceItem.name + " (Copy)";
	newItem->description = sourceItem.description;
	newItem->group = sourceItem.group;
	newItem->type = sourceItem.type;
	newItem->weight = sourceItem.weight;
	newItem->volume = sourceItem.volume;
	newItem->maxTextLen = sourceItem.maxTextLen;
	newItem->rotateTo = sourceItem.rotateTo;
	
	// Copy flags
	newItem->unpassable = sourceItem.unpassable;
	newItem->blockMissiles = sourceItem.blockMissiles;
	newItem->blockPathfinder = sourceItem.blockPathfinder;
	newItem->hasElevation = sourceItem.hasElevation;
	newItem->pickupable = sourceItem.pickupable;
	newItem->moveable = sourceItem.moveable;
	newItem->stackable = sourceItem.stackable;
	newItem->floorChangeDown = sourceItem.floorChangeDown;
	newItem->floorChangeNorth = sourceItem.floorChangeNorth;
	newItem->floorChangeEast = sourceItem.floorChangeEast;
	newItem->floorChangeSouth = sourceItem.floorChangeSouth;
	newItem->floorChangeWest = sourceItem.floorChangeWest;
	newItem->floorChange = sourceItem.floorChange;
	newItem->alwaysOnBottom = sourceItem.alwaysOnBottom;
	newItem->isHangable = sourceItem.isHangable;
	newItem->hookEast = sourceItem.hookEast;
	newItem->hookSouth = sourceItem.hookSouth;
	newItem->allowDistRead = sourceItem.allowDistRead;
	newItem->rotable = sourceItem.rotable;
	newItem->canReadText = sourceItem.canReadText;
	newItem->canWriteText = sourceItem.canWriteText;
	newItem->replaceable = sourceItem.replaceable;
	newItem->decays = sourceItem.decays;
	newItem->alwaysOnTopOrder = sourceItem.alwaysOnTopOrder;
	newItem->hasLight = sourceItem.hasLight;
	
	// Add to items database
	g_items.items.set(newId, newItem);
	
	// Refresh list and select new item
	LoadItemList();
	
	// Find and select the new item in the list
	for (int i = 0; i < m_itemList->GetItemCount(); ++i) {
		if (m_itemList->GetItemData(i) == newId) {
			m_itemList->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
			m_itemList->EnsureVisible(i);
			LoadItemProperties(newId);
			break;
		}
	}
	
	m_itemsModified = true;
}

void ItemEditorWindow::OnDeleteItem(wxCommandEvent& event)
{
	if (m_currentItemId == 0) {
		wxMessageBox("No item selected.", "Error", wxOK | wxICON_ERROR, this);
		return;
	}
	
	int response = wxMessageBox(wxString::Format("Delete item %d?", m_currentItemId), 
		"Confirm Delete", wxYES_NO | wxICON_QUESTION, this);
	
	if (response == wxYES) {
		// Remove from items database
		delete g_items.items[m_currentItemId];
		g_items.items.set(m_currentItemId, nullptr);
		
		// Refresh list
		LoadItemList();
		EnablePropertyControls(false);
		m_currentItemId = 0;
		m_itemsModified = true;
	}
}

void ItemEditorWindow::OnFindItem(wxCommandEvent& event)
{
	wxTextEntryDialog dialog(this, "Enter item ID to find:", "Find Item");
	
	if (dialog.ShowModal() == wxID_OK) {
		long itemId;
		if (dialog.GetValue().ToLong(&itemId) && itemId >= 100 && itemId <= 65535) {
			// Find item in list
			for (int i = 0; i < m_itemList->GetItemCount(); ++i) {
				if (m_itemList->GetItemData(i) == itemId) {
					m_itemList->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
					m_itemList->EnsureVisible(i);
					LoadItemProperties(itemId);
					return;
				}
			}
			wxMessageBox("Item not found.", "Find Item", wxOK | wxICON_INFORMATION, this);
		} else {
			wxMessageBox("Invalid item ID.", "Error", wxOK | wxICON_ERROR, this);
		}
	}
}

void ItemEditorWindow::OnPropertyChanged(wxCommandEvent& event)
{
	// Mark as modified when any property changes
	m_itemsModified = true;
}

void ItemEditorWindow::OnGroupChanged(wxCommandEvent& event)
{
	// Update type choices based on group
	int group = m_groupChoice->GetSelection();
	m_typeChoice->Clear();
	
	switch (group) {
		case ITEM_GROUP_CONTAINER:
			m_typeChoice->Append("Container");
			m_typeChoice->SetSelection(0);
			break;
		case ITEM_GROUP_DOOR:
			m_typeChoice->Append("Door");
			m_typeChoice->SetSelection(0);
			break;
		case ITEM_GROUP_TELEPORT:
			m_typeChoice->Append("Teleport");
			m_typeChoice->SetSelection(0);
			break;
		case ITEM_GROUP_MAGICFIELD:
			m_typeChoice->Append("Magic Field");
			m_typeChoice->SetSelection(0);
			break;
		case ITEM_GROUP_KEY:
			m_typeChoice->Append("Key");
			m_typeChoice->SetSelection(0);
			break;
		case ITEM_GROUP_PODIUM:
			m_typeChoice->Append("Podium");
			m_typeChoice->SetSelection(0);
			break;
		default:
			m_typeChoice->Append("None");
			m_typeChoice->SetSelection(0);
			break;
	}
	
	m_itemsModified = true;
}

void ItemEditorWindow::OnTypeChanged(wxCommandEvent& event)
{
	m_itemsModified = true;
}

bool ItemEditorWindow::SaveOTBFile(const wxString& filename)
{
	try {
		// Create backup of original file
		wxString backupFile = filename + ".backup";
		if (wxFileExists(filename)) {
			if (!wxCopyFile(filename, backupFile, true)) {
				wxMessageBox("Failed to create backup file.", "Error", wxOK | wxICON_ERROR, this);
				return false;
			}
		}

		DiskNodeFileWriteHandle writer(nstr(filename), "OTBI");
		if (!writer.isOk()) {
			wxMessageBox("Could not open file for writing: " + filename, "Error", wxOK | wxICON_ERROR, this);
			return false;
		}

		// Write file signature
		writer.addRAW("OTBI");

		// Write root node
		writer.addNode(0); // Root node type

		// Write root flags (unused)
		writer.addU32(0);

		// Write version information
		writer.addU8(ROOT_ATTR_VERSION);
		writer.addU16(4 + 4 + 4 + 128); // Data length

		// Write version data
		writer.addU32(g_items.MajorVersion);  // OTB format version
		writer.addU32(g_items.MinorVersion);  // Client version
		writer.addU32(g_items.BuildNumber);   // Build number

		// Write CSD version string (128 bytes)
		std::string csdVersion = "RME OTB Editor";
		csdVersion.resize(128, '\0');
		writer.addRAW(csdVersion);

		// Write all items
		for (uint32_t id = 100; id <= g_items.getMaxID(); ++id) {
			const ItemType* item = g_items.items[id];
			if (!item) continue;

			// Create item node
			writer.addNode(item->group);

			// Calculate and write flags
			uint32_t flags = 0;
			if (item->unpassable) flags |= FLAG_UNPASSABLE;
			if (item->blockMissiles) flags |= FLAG_BLOCK_MISSILES;
			if (item->blockPathfinder) flags |= FLAG_BLOCK_PATHFINDER;
			if (item->hasElevation) flags |= FLAG_HAS_ELEVATION;
			if (item->pickupable) flags |= FLAG_PICKUPABLE;
			if (item->moveable) flags |= FLAG_MOVEABLE;
			if (item->stackable) flags |= FLAG_STACKABLE;
			if (item->floorChangeDown) flags |= FLAG_FLOORCHANGEDOWN;
			if (item->floorChangeNorth) flags |= FLAG_FLOORCHANGENORTH;
			if (item->floorChangeEast) flags |= FLAG_FLOORCHANGEEAST;
			if (item->floorChangeSouth) flags |= FLAG_FLOORCHANGESOUTH;
			if (item->floorChangeWest) flags |= FLAG_FLOORCHANGEWEST;
			if (item->alwaysOnBottom) flags |= FLAG_ALWAYSONTOP; // Confusing naming
			if (item->isHangable) flags |= FLAG_HANGABLE;
			if (item->hookEast) flags |= FLAG_HOOK_EAST;
			if (item->hookSouth) flags |= FLAG_HOOK_SOUTH;
			if (item->allowDistRead) flags |= FLAG_ALLOWDISTREAD;
			if (item->rotable) flags |= FLAG_ROTABLE;
			if (item->canReadText) flags |= FLAG_READABLE;

			writer.addU32(flags);

			// Write item attributes
			// Server ID (required)
			writer.addU8(ITEM_ATTR_SERVERID);
			writer.addU16(2); // Data length
			writer.addU16(item->id);

			// Client ID (required)
			writer.addU8(ITEM_ATTR_CLIENTID);
			writer.addU16(2); // Data length
			writer.addU16(item->clientID);

			// Optional attributes
			if (item->hasLight) {
				writer.addU8(ITEM_ATTR_LIGHT2);
				writer.addU16(4); // Data length
				writer.addU16(0); // Light level (not stored in current implementation)
				writer.addU16(0); // Light color (not stored in current implementation)
			}

			if (item->alwaysOnTopOrder != 0) {
				writer.addU8(ITEM_ATTR_TOPORDER);
				writer.addU16(1); // Data length
				writer.addU8(item->alwaysOnTopOrder);
			}

			// End item node
			writer.endNode();
		}

		// End root node
		writer.endNode();

		writer.close();

		wxMessageBox("Items.otb saved successfully to: " + filename, "Success", 
			wxOK | wxICON_INFORMATION, this);
		
		// Reset modified flag
		m_itemsModified = false;
		
		return true;

	} catch (const std::exception& e) {
		wxMessageBox("Error saving OTB file: " + wxString(e.what()), "Error", 
			wxOK | wxICON_ERROR, this);
		return false;
	} catch (...) {
		wxMessageBox("Unknown error occurred while saving OTB file.", "Error", 
			wxOK | wxICON_ERROR, this);
		return false;
	}
}

bool ItemEditorWindow::GenerateMissingItems()
{
	if (g_gui.getLoadedVersion() == nullptr) {
        //get loaded version
		wxMessageBox("No sprites loaded. Please load a client version first.", "Error", 
			wxOK | wxICON_ERROR, this);
		return false;
	}

	wxString message = "This will generate missing item entries for all sprite IDs that exist in the loaded DAT/SPR files but don't have corresponding items in the OTB.\n\n";
	message += "This operation will add new items with default properties. You can edit them afterwards.\n\n";
	message += "Continue?";

	int response = wxMessageBox(message, "Generate Missing Items", wxYES_NO | wxICON_QUESTION, this);
	if (response != wxYES) {
		return false;
	}

	int itemsGenerated = 0;
	uint16_t maxClientId = g_gui.gfx.getItemSpriteMaxID();
	uint16_t nextServerId = g_items.getMaxID() + 1;

	// Create a set of existing client IDs for quick lookup
	std::set<uint16_t> existingClientIds;
	for (uint32_t id = 100; id <= g_items.getMaxID(); ++id) {
		const ItemType* item = g_items.items[id];
		if (item) {
			existingClientIds.insert(item->clientID);
		}
	}

	// Generate items for missing client IDs
	for (uint16_t clientId = 100; clientId <= maxClientId; ++clientId) {
		// Skip if item already exists with this client ID
		if (existingClientIds.find(clientId) != existingClientIds.end()) {
			continue;
		}

		// Check if sprite actually exists
		if (!g_gui.gfx.getSprite(clientId)) {
			continue;
		}

		// Create new item
		ItemType* newItem = new ItemType();
		newItem->id = nextServerId++;
		newItem->clientID = clientId;
		newItem->name = wxString::Format("Item %d", clientId).ToStdString();
		newItem->group = ITEM_GROUP_NONE;
		newItem->type = ITEM_TYPE_NONE;
		
		// Set some reasonable defaults
		newItem->stackable = false;
		newItem->pickupable = true;
		newItem->moveable = true;
		newItem->unpassable = false;
		newItem->blockMissiles = false;
		newItem->blockPathfinder = false;
		newItem->hasElevation = false;
		newItem->weight = 1.0f;

		// Add to items database
		g_items.items.set(newItem->id, newItem);
		itemsGenerated++;
	}

	// Refresh the item list
	LoadItemList();
	
	// Mark as modified
	m_itemsModified = true;

	wxString resultMessage = wxString::Format("Generated %d missing items.\n\n", itemsGenerated);
	resultMessage += "Items have been created with default properties. You can now edit them as needed.";
	
	wxMessageBox(resultMessage, "Generation Complete", wxOK | wxICON_INFORMATION, this);
	
	return itemsGenerated > 0;
}

int ItemEditorWindow::ShowModal()
{
	return wxDialog::ShowModal();
} 