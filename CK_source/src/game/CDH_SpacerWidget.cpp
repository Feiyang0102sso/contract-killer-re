// CDH_SpacerWidget.cpp - spacer constructors used by the screen footer.
#include "CDH_SpacerWidget.h"

CDH_SpacerWidget::CDH_SpacerWidget(int width, int height) : Window(false)
{
    SetDesiredWidth(width);
    SetDesiredHeight(height);
}

CDH_SpacerWidget::CDH_SpacerWidget(int column, int row, int widthPercent, int heightPercent)
    : Window(false)
{
    SetPercentWidth(widthPercent, 0, 0);
    SetPercentHeight(heightPercent, 0, 0);
    SetCellPos(column, row, 1, 1);
}

CDH_SpacerWidget::~CDH_SpacerWidget()
{
}
