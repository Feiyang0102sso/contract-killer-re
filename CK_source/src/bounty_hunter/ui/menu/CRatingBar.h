// CRatingBar.h - a two-sided rating meter with a neutral center.
#ifndef CRATINGBAR_H
#define CRATINGBAR_H

#include "WindowBase.h"

class CRatingBar : public Window {
public:
    CRatingBar();
    virtual ~CRatingBar() {}
    virtual void Paint(ICGraphics2d& graphics);

    void SetLimitValues(int negativeLimit, int positiveLimit);
    void SetValue(int value) { m_value = value; }

private:
    int m_negativeLimit;
    int m_positiveLimit;
    int m_value;
    ImageRes m_side;
    ImageRes m_center;
    ImageRes m_meter;
    ImageRes m_green;
    ImageRes m_red;
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char RatingBarSizeCheck[sizeof(CRatingBar) == 0xd8 ? 1 : -1];
#endif

#endif
