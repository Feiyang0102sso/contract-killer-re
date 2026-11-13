// BaseDialog.h - basic building blocks of the dialogs: an image window, a multi-line text
// window and a container whose children can be scrolled by dragging (with inertia).
#ifndef BASEDIALOG_H
#define BASEDIALOG_H

#include "WindowBase.h"
#include "Vector_ss.h"
#include "xString.h"

                                                                         
class CFont;
       

/** Shows one image, optionally tiled, scaled or blinking. */
class ImageWindow : public Window {
public:
    /** How the image fills the window. */
    enum EFillType {
        FILL_NONE = 0,          // image size, placed by the alignment
        FILL_HORIZONTAL = 1,    // tiled across the width
        FILL_VERTICAL = 2,      // tiled across the height
        FILL_BOTH = 3           // tiled over the whole window
    };

    ImageWindow();
    ImageWindow(EFillType fillType, const ImageRes& image);
    virtual ~ImageWindow();

    virtual void Paint(ICGraphics2d& graphics);

    void SetFillType(EFillType fillType);
    void SetScale(int x, int y);
    /** Use the shared surface `id` of the surface manager. */
    void SetImageById(const char* id);
    void SetImage(const ImageRes& image);
    void SetImage(ICRenderSurface* surface);

    ImageRes m_image;                                       // 0xa4
    ICRenderSurface* m_surface;                              // 0xac drawn surface
    EFillType m_fillType;                                    // 0xb0
    // CKZ used a command to switch blinking. CK stores two draw offsets instead.
    int m_offsetX;                                           // 0xb4
    int m_offsetY;                                           // 0xb8
    bool m_blinking;                                         // 0xbc
    int m_scaleX;                                            // 0xc0 16.16, <= 0: no scaling
    int m_scaleY;                                            // 0xc4 16.16
    XString m_surfaceId;                                    // 0xc8 id passed to SetImageById
};

/** A tiled horizontal image with optional end caps. */
class HorizImageWindow : public WindowTransparent {
public:
    HorizImageWindow(const ImageRes& left, const ImageRes& middle, const ImageRes& right, bool fill);
    virtual ~HorizImageWindow() {}
};

/** Nine-image frame around a window's content. */
class Filler : public Window {
public:
    // CK: both inline (weak copies in ServiceWindows.o and here).
    Filler() : Window(false) {}
    // CK: implicit virtual destructor (D1 holds the whole body, as in the original).
    virtual void Paint(ICGraphics2d& graphics);
    void DisableTop();

protected:
    enum FramePart { Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight, Center, FrameCount };
    void SetFrame(const char* const* resources);
    // CK constructs and destroys the nine images one by one (separate members); kept as an array
    // because CBaseScreen.cpp (other owner) indexes m_frame. See notes/2026-11-11-ui.md.
    ImageRes m_frame[FrameCount];
};

class InsetPanel : public Filler {
public:
    InsetPanel();
};

/** Text broken into lines, drawn with one of two fonts. */
class TextWindow : public Window {
public:
    TextWindow();
    TextWindow(const XString& text, CFont* font);
    TextWindow(const wchar_t* text, CFont* font);
    TextWindow(CFont* font);

    virtual ~TextWindow()
    {
    }

    virtual void SetRect(int x, int y, int width, int height, bool force);
    virtual void Paint(ICGraphics2d& graphics);

    void SetText(const XString& text, CFont* font);
    void SetText(const XString& text, CFont* font,
                 CFont* alternateFont, int fontIndex);
    void SetFont(CFont* font);
    void SetText(const XString& text);

    CFont* m_fonts[2];    // 0xac
    int m_fontIndex;                                    // 0xb4 font in use
    XString m_text;                                     // 0xb8
    Vector<XString> m_lines;                            // 0xbc
    int m_textX;                                        // 0xcc offset of the text
    int m_textY;                                        // 0xd0
    int m_textWidth;                                    // 0xd4 widest line (0: not measured)
};

/** Children are scrolled by dragging the pointer; the scrolling continues with inertia. */
class ScrollingContainer : public WindowTransparent {
public:
    /** Scroll bar drawn next to a scrolling container. */
    class VertScroller : public Window {
    public:
        // b081j: Career original 155188 weak default constructor, only base class/six ImageRes; does not initialize container.
        VertScroller() : Window(false) {}
        VertScroller(ScrollingContainer* container);

        virtual void OnPointerEvent(Event& event);
        virtual void Paint(ICGraphics2d& graphics);

        /** Images of m_images. */
        enum {
            IMAGE_BAR_TOP = 0,
            IMAGE_BAR_CENTER = 1,
            IMAGE_BAR_BOTTOM = 2,
            IMAGE_ARROW_UP = 3,
            IMAGE_ARROW_DOWN = 4,
            IMAGE_COUNT = 5         // CK: five images (no marker, unlike CKZ)
        };

        ScrollingContainer* m_container;    // 0xa4
        ImageRes m_images[IMAGE_COUNT];     // 0xa8
    };

    ScrollingContainer();

    virtual void OnCommand(Event& event);
    virtual void OnUnhandledEvent(Event& event);
    virtual void OnPointerPressed(Event& event);
    virtual void OnPointerDragged(Event& event);
    virtual void OnPointerReleased(Event& event);
    virtual void Update();
    virtual void OnChildInitialized(Window* child);
    virtual void OnChildRectChanged(Window* child);

    /** The whole content fits in the window. */
    bool IsFullVisible() const;
    /** Scroll position 0..1 (1 when everything is visible). */
    float ScrollPos() const;
    /** Move the content so that its top is at `top` (0 or negative). */
    void SetContentTop(int top);

    bool m_contentDirty;        // 0xac content rectangle must be recomputed
    bool m_dragging;            // 0xad
    bool m_movingChildren;      // 0xae children are being moved by the container itself
    int m_contentX;             // 0xb0 content rectangle relative to the window
    int m_contentY;             // 0xb4
    int m_contentWidth;         // 0xb8
    int m_contentHeight;        // 0xbc
    float m_velocityX;          // 0xc0 pixels per second
    float m_velocityY;          // 0xc4
};

#endif
