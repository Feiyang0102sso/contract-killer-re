#include "CDH_BaseScreen.h"

#include "BaseDialog.h"
#include "CDH_ButtonWidget.h"
#include "CDH_SpacerWidget.h"
#include "CUtility.h"

CDH_BaseScreen::CDH_BaseScreen() : m_leftButton(NULL), m_rightButton(NULL)
{
    SetFlags(FLAG_MODAL);
    SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_selector[0].Create("SUR_DLG_TITLE_SELECTOR_LEFT");
    m_selector[1].Create("SUR_DLG_TITLE_SELECTOR_MIDDLE");
    m_selector[2].Create("SUR_DLG_TITLE_SELECTOR_RIGHT");
    m_splitter[0].Create("SUR_DLG_ITEM_SPLITTER_LEFT");
    m_splitter[1].Create("SUR_DLG_ITEM_SPLITTER_MIDDLE");
    m_splitter[2].Create("SUR_DLG_ITEM_SPLITTER_RIGHT");
    m_buttons = new Window(false);
    setLayoutWidget(m_buttons, Footer);
}

CDH_BaseScreen::~CDH_BaseScreen()
{
}

HorizImageWindow* CDH_BaseScreen::createLineSelector(int column, int row)
{
    HorizImageWindow* line = new HorizImageWindow(m_selector[0], m_selector[1], m_selector[2], false);
    if ((column | row) >= 0) {
        line->SetCellPos(column, row, 1, 1);
    }
    line->SetDesiredHeight(20);
    return line;
}

void CDH_BaseScreen::setButtons(const char* left, const char* right)
{
    if (left != NULL) {
        CStrWChar text = CUtility::GetString(left);
        m_leftText = XString(text.GetWCharPtr());
    }
    if (right != NULL) {
        CStrWChar text = CUtility::GetString(right);
        m_rightText = XString(text.GetWCharPtr());
    }
}

void CDH_BaseScreen::Init()
{
    CDH_WidgetScreen::Init();
    if (m_leftText.Length() == 0 && m_rightText.Length() == 0) {
        return;
    }
    if (m_leftText.Length() != 0) {
        m_leftButton = new CDH_ButtonWidget();
        m_leftButton->SetAlign(ALIGN_LEFT);
        m_leftButton->m_text = m_leftText;
        m_leftButton->m_command = 0x21204844;
        m_buttons->AddToFront(m_leftButton, 0, 0);
    }

    m_buttons->AddToFront(new CDH_SpacerWidget(1, 0, 100, 100));
    m_buttons->AddToFront(new CDH_SpacerWidget(50, 50), 2, 0);

    if (m_rightText.Length() != 0) {
        m_rightButton = new CDH_ButtonWidget();
        m_rightButton->SetAlign(ALIGN_RIGHT);
        m_rightButton->m_text = m_rightText;
        m_rightButton->m_command = 0x07b18a47;
        m_buttons->AddToFront(m_rightButton, 3, 0);
    }
    m_buttons->SetWidthByContent(0, 0);
    m_buttons->SetHeightByContent(0, 0);
}

void CDH_BaseScreen::OnCommand(Event& event)
{
    if (event.m_id == 0x07b18a47) {
        SetInterrupt(1);
    } else if (event.m_id == 0x21204844 || event.m_id == (int)0x97204784) {
        SetInterrupt(2);
    } else {
        return;
    }
    event.Clear();
}

void CDH_BaseScreen::OnLostFocus()
{
    ClearFlags(FLAG_VISIBLE);
}

void CDH_BaseScreen::OnGetFocus()
{
    SetFlags(FLAG_VISIBLE);
}

typedef char AssertBaseScreenSize[sizeof(CDH_BaseScreen) == 0x188 ? 1 : -1];
