#ifndef CCHANGEAVATARWINDOW_H
#define CCHANGEAVATARWINDOW_H

#include "CBaseScreen.h"

class CImageButton;

class CChangeAvatarWindow : public CBaseScreen {
public:
    enum { AvatarChanged = 0x01234eee };

    CChangeAvatarWindow();
    virtual void OnCommand(Event& event);
    virtual void Update();

private:
    int m_selectedId;
    ImageWindow* m_selectedFrame;
    CImageButton* m_selectedButton;
};

#endif
