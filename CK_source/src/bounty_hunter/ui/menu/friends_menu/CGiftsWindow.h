#ifndef CGIFTSWINDOW_H
#define CGIFTSWINDOW_H

#include "Vector_ss.h"
#include "WindowBase.h"

struct SGift;

class CGiftsWindow : public Window {
public:
    CGiftsWindow();

private:
    void GetGifts(Vector<const SGift*>& gifts);
    void CreateUserInterface(const Vector<const SGift*>& gifts);
    Window* CreateGiftButton(int catalogIndex);
};

#endif
