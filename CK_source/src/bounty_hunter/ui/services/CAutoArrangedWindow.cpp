// CAutoArrangedWindow.cpp - compute capacity before changing the child tree.
#include "CAutoArrangedWindow.h"

#include "SScreenAdaptingConstant.h"

CAutoArrangedWindow::CAutoArrangedWindow(int width, int height)
    : Window(false), m_oneLine(false), m_maxRows(0), m_maxColumns(0),
      m_fixedSpacing(false), m_spacing(0)
{
    SetLayoutType(LAYOUT_NONE);
    SetSize(width, height);
    SetDesiredWidth(width);
    SetDesiredHeight(height);
}

CAutoArrangedWindow::~CAutoArrangedWindow()
{
}

bool CAutoArrangedWindow::UpdateContentLayout()
{
    if (m_items.size() == 0) {
        return true;
    }

    Window* lastItem = m_items[m_items.size() - 1];
    int itemWidth = lastItem->m_width;
    int itemHeight = lastItem->m_height;
    int columns = m_width / itemWidth;
    if (m_maxColumns != 0 && m_maxColumns < columns) {
        columns = m_maxColumns;
    }
    int rows = 1;
    if (!m_oneLine) {
        rows = m_height / itemHeight;
    }
    if (m_maxRows != 0 && m_maxRows < rows) {
        rows = m_maxRows;
    }
    if (rows <= 0 || columns <= 0 || m_items.size() > rows * columns) {
        return false;
    }

    int horizontalGap = m_spacing;
    if (!m_fixedSpacing) {
        horizontalGap = (m_width - columns * itemWidth) / (columns + 1);
    }
    int verticalGap = 0;
    if (!m_oneLine) {
        verticalGap = (m_height - rows * itemHeight) / (rows + 1);
    }
    for (int index = 0; index < m_items.size(); ++index) {
        Window* item = m_items[index];
        if (IsChildExisting(item)) {
            Remove(item);
        }
    }

    int x = horizontalGap;
    int y = verticalGap + SScreenAdaptingConstant(4, 3, 10, -1)();
    int row = 0;
    int column = 0;
    for (int index = 0; index < m_items.size(); ++index) {
        Window* item = m_items[index];
        if (m_maxRows != 0 && m_maxRows <= row) {
            return false;
        }
        if (m_maxColumns != 0 && m_maxColumns < column) {
            return false;
        }
        AddToFront(item);
        item->SetLayoutType(LAYOUT_NONE);
        ++column;
        item->SetCorner(x, y);
        if (column == columns) {
            ++row;
            y += itemHeight + verticalGap;
            column = 0;
            x = horizontalGap;
        } else {
            x += itemWidth + horizontalGap;
        }
    }
    return true;
}

bool CAutoArrangedWindow::Add(Window* window)
{
    m_items.addElement(window);
    bool arranged = UpdateContentLayout();
    if (!arranged) {
        m_items.removeElementAt(m_items.size() - 1);
    }
    return arranged;
}

void CAutoArrangedWindow::SetOneLineMode(bool oneLine)
{
    m_oneLine = oneLine;
    UpdateContentLayout();
}

void CAutoArrangedWindow::SetItemSpacing(int spacing)
{
    m_spacing = spacing;
    m_fixedSpacing = true;
}
