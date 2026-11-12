// ButtonWindow.h - image/text button window, first declared for App::NewSoftkeyButton.
// Size 0x124 (App::NewSoftkeyButton allocates 0x124); base Window (ButtonWindow C2 calls Window(false)).
#ifndef BUTTONWINDOW_H
#define BUTTONWINDOW_H

#include "WindowBase.h"
#include "xString.h"

class CFont;

class ButtonWindow : public Window {
public:
    ButtonWindow();
    virtual ~ButtonWindow() {}

    void SetSleeped(bool sleeped);
    void SetImage(const ImageRes& image);
    void SetImages(const ImageRes& image, const ImageRes& selected);
    void SetImages(const ImageRes& left, const ImageRes& tile, const ImageRes& right,
                   const ImageRes& selectedLeft, const ImageRes& selectedTile, const ImageRes& selectedRight);
    void SetBackgroundOpacity(float opacity);
    void SetName(const XString& name, CFont* font);
    void SetSounds(const MediaRes& pressed, const MediaRes& released);
    void SetCommands(int command, int secondCommand);
    void SetCommand(int command);

    virtual void Shutdown();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void OnCommand(Event& event);
    virtual void OnPointerPressed(Event& event);
    virtual void OnPointerDragged(Event& event);
    virtual void OnPointerReleased(Event& event);
    virtual void OnPressed() {}
    virtual void OnUnpressed() {}

    // Members at 0xa4..0x124, recovered from ButtonWindow::ButtonWindow() and its handlers.
    enum Background { Solid, CenteredImage, TiledImage };
    Background m_background;           // 0xa4
    ImageRes m_images[3];               // 0xa8
    ImageRes m_selectedImages[3];       // 0xc0
    XString m_name;                     // 0xd8
    XString m_selectedName;             // 0xdc
    CFont* m_font;                      // 0xe0
    CFont* m_selectedFont;              // 0xe4
    MediaRes m_pressSound;               // 0xe8
    MediaRes m_releaseSound;             // 0xf0
    MediaRes m_secondarySound;           // 0xf8
    int m_command;                      // 0x100
    int m_secondCommand;                // 0x104
    int m_labelX;                       // 0x108
    int m_labelY;                       // 0x10c
    int m_lastActivationTime;           // 0x110
    int m_reserved114;                  // 0x114
    float m_backgroundOpacity;          // 0x118
    bool m_pressed;                     // 0x11c
    bool m_softkey;                     // 0x11d
    bool m_sleeped;                     // 0x11e
    bool m_selected;                    // 0x11f
    int m_commandParam;                 // 0x120, assigned by callers that send a payload
};

typedef char ButtonWindowSizeCheck[sizeof(ButtonWindow) == 0x124 ? 1 : -1];

#endif
