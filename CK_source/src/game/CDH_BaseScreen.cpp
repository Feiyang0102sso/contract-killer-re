#include "CDH_BaseScreen.h"

#include "BaseDialog.h"
#include "CDH_ButtonWidget.h"
#include "CDH_SpacerWidget.h"
#include "CFontMgr.h"
#include "CUtility.h"

CDH_BaseScreen::CDH_BaseScreen()
{
    SetFlags(FLAG_MODAL);
    SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    m_selectorLeft.Create("SUR_DLG_TITLE_SELECTOR_LEFT");
    m_selectorMiddle.Create("SUR_DLG_TITLE_SELECTOR_MIDDLE");
    m_selectorRight.Create("SUR_DLG_TITLE_SELECTOR_RIGHT");
    m_splitterLeft.Create("SUR_DLG_ITEM_SPLITTER_LEFT");
    m_splitterMiddle.Create("SUR_DLG_ITEM_SPLITTER_MIDDLE");
    m_splitterRight.Create("SUR_DLG_ITEM_SPLITTER_RIGHT");
    m_buttons = new Window(false);
}

CDH_BaseScreen::~CDH_BaseScreen()
{
}

HorizImageWindow* CDH_BaseScreen::createLineSelector(int column, int row)
{
    HorizImageWindow* line = new HorizImageWindow(m_selectorLeft, m_selectorMiddle, m_selectorRight, false);
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
        m_leftText = text.GetWCharPtr();
    }
    if (right != NULL) {
        CStrWChar text = CUtility::GetString(right);
        m_rightText = text.GetWCharPtr();
    }
}

void CDH_BaseScreen::Init()
{
    CDH_WidgetScreen::Init();
    // Original fetches font 0 and discards it
    CFontMgr::GetInstance()->GetFont(0);
    if (m_leftText.ByteSize() != 0 || m_rightText.ByteSize() != 0) {
        if (m_leftText.ByteSize() != 0) {
            m_leftButton = new CDH_ButtonWidget();
            m_leftButton->SetAlign(ALIGN_LEFT);
            m_leftButton->m_text = XString(m_leftText);
            m_leftButton->m_command = 0x21204844;
            m_leftButton->SetCellPos(0, 0, 1, 1);
            m_buttons->AddToFront(m_leftButton);
        }

        CDH_SpacerWidget* spacer = new CDH_SpacerWidget(1, 0, 100, 100);
        m_buttons->AddToFront(spacer);
        spacer = new CDH_SpacerWidget(50, 50);
        spacer->SetCellPos(2, 0, 1, 1);
        m_buttons->AddToFront(spacer);

        if (m_rightText.ByteSize() != 0) {
            m_rightButton = new CDH_ButtonWidget();
            m_rightButton->SetAlign(ALIGN_RIGHT);
            m_rightButton->m_text = XString(m_rightText);
            m_rightButton->m_command = 0x07b18a47;
            m_rightButton->SetCellPos(3, 0, 1, 1);
            m_buttons->AddToFront(m_rightButton);
        }
        m_buttons->SetWidthByContent(0, 0);
        m_buttons->SetHeightByContent(0, 0);
    }
    setLayoutWidget(m_buttons, Footer);
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
