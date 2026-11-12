#include "CIncentivizedWindow.h"

#include "BaseDialog.h"
#include "CCareerWindow.h"
#include "CCheckBox.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "SScreenAdaptingConstant.h"

namespace {
const int FriendsPerPage = 255;
const int Accept = 0x17d51013;
const int Ignore = -0x6828c299;
const int Back = -0x68dfb87c;
const int CheckSelected = 0x0d96b6ad;
const int CheckDeselected = 0x5f58cb68;
const int SelectAll = 0x541a651f;
const int Next = -0x5d3797e6;
const int Previous = 0x541a681b;
const int ButtonFlags = Window::FLAG_VISIBLE | Window::FLAG_ENABLED;

const SScreenAdaptingConstant s_listTop(90, 180);
const SScreenAdaptingConstant s_listRight(50, 100);
const SScreenAdaptingConstant s_listBottom(85, 180);
const SScreenAdaptingConstant s_listLeft(75, 150);
const SScreenAdaptingConstant s_buttonsBottom(25, 50);
const SScreenAdaptingConstant s_sideButtonMargin(125, 250);
const SScreenAdaptingConstant s_extraButtonMargin(195, 390);
const SScreenAdaptingConstant s_pageButtonMargin(70, 140);
}

CIncentivizedWindow::CIncentivizedWindow(const XString& acceptCaption)
    : m_page(0), m_row(0), m_commandParameter(0), m_command(0), m_friendRows(NULL), m_next(NULL),
      m_previous(NULL), m_scroll(NULL), m_groups(NULL), m_singleSelection(false), m_eventReceiver(NULL)
{
    m_title->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    Window* list = new Window(false);
    list->SetOutsetSpacing(s_listTop(), s_listRight(), s_listBottom(), s_listLeft());
    AddToFront(list);

    m_friendRows = new Window(false);
    m_friendRows->SetLayoutType(LAYOUT_NONE);
    m_friendRows->SetWidthByContent(0, 0);
    m_friendRows->SetHeightByContent(0, 0);
    m_groups = new Window(false);
    m_groups->SetLayoutType(LAYOUT_NONE);
    m_groups->SetPercentWidth(100, 0, 0);
    m_groups->SetHeightByContent(0, 0);
    m_groups->AddToFront(m_friendRows, 0, 0);
    m_scroll = new ScrollingContainer();
    m_scroll->SetLayoutType(LAYOUT_NONE);
    m_scroll->SetPercentWidth(100, 0, 0);
    m_scroll->SetPercentHeight(100, 0, 0);
    m_scroll->AddToFront(m_groups);
    list->AddToFront(m_scroll, 0, 0);
    VertScrollerSimpleRed* scroller = new VertScrollerSimpleRed(m_scroll);
    scroller->SetOutsetSpacing(3, 5, 3, 4);
    list->AddToFront(scroller, 1, 0);

    CImageButton* accept = CreateButton(acceptCaption, "IDB_BUTTON_SET_BOUNTY", ALIGN_LEFT | ALIGN_BOTTOM, Accept);
    accept->SetOutsetSpacing(0, 0, s_buttonsBottom(), s_sideButtonMargin());
    CImageButton* ignore = CreateButton(ResString("IDS_DIALOG_BUTTON_IGNORE"), "IDB_DIALOG_IGNORE_BUTTON",
                                        ALIGN_RIGHT | ALIGN_BOTTOM, Ignore);
    ignore->SetOutsetSpacing(0, s_sideButtonMargin(), s_buttonsBottom(), 0);
}

CImageButton* CIncentivizedWindow::CreateButton(const XString& text, const char* image, int align, int command)
{
    CImageButton* button = new CImageButton();
    int textAlign = ALIGN_HCENTER | ALIGN_BOTTOM;
    if (image != NULL) {
        button->AddImage(image, 0, 0);
    } else {
        button->SetWidthByContent(0, 0);
        button->SetHeightByContent(0, 0);
        textAlign = ALIGN_HCENTER | ALIGN_VCENTER;
    }
    button->SetText(text, textAlign, NULL, 0, 0);
    button->SetLayoutType(LAYOUT_SIMPLE);
    button->SetAlign(align);
    button->m_command = command;
    AddToFront(button);
    return button;
}

void CIncentivizedWindow::AddFriend(const XString& name, int friendId)
{
    m_friendIds.addElement(friendId);
    m_friendNames.addElement(name);
    int index = m_friendIds.size() - 1;
    if (index / FriendsPerPage == m_page) {
        CCheckBox* box = CreateCheckBox(name, friendId, m_row++);
        m_checkBoxes.addElement(box);
    } else if (m_next == NULL) {
        m_next = CreateButton(XString("NEXT"), NULL, ALIGN_RIGHT | ALIGN_BOTTOM, Next);
        m_next->SetOutsetSpacing(0, s_pageButtonMargin(), s_pageButtonMargin(), 0);
        m_previous = CreateButton(XString("PREV"), NULL, ALIGN_LEFT | ALIGN_BOTTOM, Previous);
        m_previous->SetOutsetSpacing(0, 0, s_pageButtonMargin(), s_pageButtonMargin());
        m_previous->ClearFlags(ButtonFlags);
    }
    if (m_next != NULL && index / FriendsPerPage > m_page) {
        m_next->SetFlags(ButtonFlags);
    }
}

CCheckBox* CIncentivizedWindow::CreateCheckBox(const XString& name, int friendId, int row)
{
    Window* entry = new Window(false);
    entry->SetLayoutType(LAYOUT_NONE);
    entry->SetWidthByContent(0, 0);
    entry->SetHeightByContent(0, 0);
    CCheckBox* box = new CCheckBox(friendId);
    box->SetAlign(ALIGN_LEFT);
    box->SetPercentWidth(15, 0, 0);
    entry->AddToFront(box, 0, 0);
    XString caption = name.SubString(0, 40);
    TextWindow* label = new TextWindow(caption, CFontMgr::GetInstance()->GetFont(15));
    label->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    entry->AddToFront(label, 1, 0);
    m_friendRows->AddToFront(entry, 0, row);
    return box;
}

void CIncentivizedWindow::OnCommand(Event& event)
{
    switch (event.m_id) {
    case Accept:
        HandleAcceptPressed();
        break;
    case Ignore:
    case Back:
        Close();
        break;
    case CheckSelected:
        HandleCheckBoxSelected(event.m_param1);
        break;
    case CheckDeselected:
        HandleCheckBoxDeselected(event.m_param1);
        break;
    case Next:
        ShowPage(m_page + 1);
        break;
    case Previous:
        ShowPage(m_page - 1);
        break;
    case SelectAll:
        for (int index = 0; index < m_checkBoxes.size(); ++index) {
            SelectCheckBox(index);
        }
        break;
    default:
        CTextDialog::OnCommand(event);
        return;
    }
    event.Clear();
}

void CIncentivizedWindow::HandleAcceptPressed()
{
    if (m_command == 0) {
        return;
    }
    int sender = reinterpret_cast<int>(this);
    if (m_eventReceiver == NULL) {
        Window* handler = WindowApp::HandleTunnelCommand(m_command, sender, m_commandParameter, 0);
        if (handler == NULL && m_commandParameter != 0) {
            Close();
        }
    } else {
        Event event(Event::COMMAND, m_command, sender, m_commandParameter, 0);
        m_eventReceiver->OnCommand(event);
    }
}

void CIncentivizedWindow::HandleCheckBoxSelected(int friendId)
{
    if (m_singleSelection) {
        m_selectedIds.removeAllElements();
        DeselectAllCheckBoxes();
        for (int index = 0; index < m_checkBoxes.size(); ++index) {
            if (m_checkBoxes[index]->m_commandParam == friendId) {
                SelectCheckBox(index);
                return;
            }
        }
    }
    if (m_selectedIds.indexOf(friendId) < 0) {
        m_selectedIds.addElement(friendId);
    }
}

void CIncentivizedWindow::HandleCheckBoxDeselected(int friendId)
{
    int index = m_selectedIds.indexOf(friendId);
    if (index >= 0) {
        m_selectedIds.removeElementAt(index);
    }
}

void CIncentivizedWindow::SetEventReciever(Window* receiver)
{
    m_eventReceiver = receiver;
}

Vector<int> CIncentivizedWindow::GetSelectedIds() const
{
    return m_selectedIds;
}

void CIncentivizedWindow::SetCommand(unsigned int command)
{
    m_command = command;
}

void CIncentivizedWindow::DeselectAllCheckBoxes()
{
    for (int index = 0; index < m_checkBoxes.size(); ++index) {
        m_checkBoxes[index]->SetChecked(false);
    }
}

void CIncentivizedWindow::SelectCheckBox(int index)
{
    CCheckBox* box = m_checkBoxes[index];
    int friendId = box->m_commandParam;
    if (m_selectedIds.indexOf(friendId) < 0) {
        m_selectedIds.addElement(friendId);
    }
    box->SetChecked(true);
}

void CIncentivizedWindow::SelectFriend(int friendId)
{
    HandleCheckBoxSelected(friendId);
    for (int index = 0; index < m_checkBoxes.size(); ++index) {
        if (m_checkBoxes[index]->m_commandParam == friendId) {
            m_checkBoxes[index]->SetChecked(true);
            return;
        }
    }
}

void CIncentivizedWindow::ShowPage(int page)
{
    int lastPage = (m_friendIds.size() - 1) / FriendsPerPage;
    if (page < 0 || page > lastPage || page == m_page) {
        return;
    }
    m_page = page;
    if (page > 0) {
        m_previous->SetFlags(ButtonFlags);
    } else {
        m_previous->ClearFlags(ButtonFlags);
    }
    if (page < lastPage) {
        m_next->SetFlags(ButtonFlags);
    } else {
        m_next->ClearFlags(ButtonFlags);
    }

    m_scroll->SetContentTop(0);
    m_friendRows->CloseChilds();
    m_checkBoxes.removeAllElements();
    m_row = 0;
    int start = page * FriendsPerPage;
    for (int index = start; index < m_friendIds.size() && index < start + FriendsPerPage; ++index) {
        int friendId = m_friendIds[index];
        CCheckBox* box = CreateCheckBox(m_friendNames[index], friendId, m_row++);
        box->SetChecked(m_selectedIds.indexOf(friendId) >= 0);
        m_checkBoxes.addElement(box);
    }
}

void CIncentivizedWindow::SetExtraButton(const XString& text, const char* image, unsigned int command)
{
    CImageButton* button = CreateButton(text, image, ALIGN_RIGHT | ALIGN_BOTTOM, command);
    button->SetOutsetSpacing(0, s_extraButtonMargin(), s_buttonsBottom(), 0);
}

void CIncentivizedWindow::ShowSelectAllButton()
{
    SetExtraButton(XString("Select All"), "IDB_BUTTON_SET_BOUNTY", SelectAll);
}
