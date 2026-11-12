// CDH_SpacerWidget.h - fixed or percentage-sized empty layout cell.
#ifndef CDH_SPACERWIDGET_H
#define CDH_SPACERWIDGET_H

#include "WindowBase.h"

class CDH_SpacerWidget : public Window {
public:
    CDH_SpacerWidget(int width, int height);
    CDH_SpacerWidget(int column, int row, int widthPercent, int heightPercent);
    virtual ~CDH_SpacerWidget();
};

#endif
