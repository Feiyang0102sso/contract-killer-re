// SimpleDialog.h - list dialog: a title, a text, a list of selectable items (with optional
// sub items chosen by left / right arrows) and a frame drawn from border images.
// Owner: src/general/SimpleDialog.cpp (b058; first draft by b057x).
// Slot order from the vtables of SimpleDialog / SimpleDialog::ItemsWindow, fields from their
// constructors and from SimpleDialog::Item::Item.
#ifndef SIMPLEDIALOG_H
#define SIMPLEDIALOG_H

#include <stddef.h>
#include "CDH_SoftkeysScreen.h"
#include "SimpleTextArea.h"
#include "Vector_ss.h"
#include "xString.h"

                                                                         
class CFont;
class ICGraphics2d;
       

class SimpleDialog : public CDH_SoftkeysScreen {
public:
    /** Dialog kinds (stored, not read by SimpleDialog itself). */
    enum EDialogType {
        DIALOG_TYPE_0 = 0,
        DIALOG_TYPE_1 = 1
    };

    /** Item kinds (Item::m_type). */
    enum EItemType {
        ITEM_NORMAL = 0,
        ITEM_DISABLED = 1,      // shown, cannot be selected
        ITEM_HIDDEN = 2         // not shown, cannot be selected
    };

    /** One choice of an item (chosen with left / right). */
    struct SubItem {
        /** Signature guessed: no sub item is created in SimpleDialog.cpp (a user constructor must exist,
            otherwise the copies in Vector<SubItem> would be zero-filled first). */
        SubItem(const XString& text, int field04, int id)
            : m_text(text), m_field04(field04), m_id(id)
        {
        }

        XString m_text;     // 0x00
        int m_field04;      // 0x04 copied, never read here
        int m_id;           // 0x08 passed to SelectedSubitemChanged / ClickSubitem
    };

    /** One list entry. */
    struct Item {
        Item(int id, const XString& text, int type, const char* iconName, bool enabled)
            : m_text(text), m_type(type), m_id(id), m_enabled(enabled), m_subIndex(0)
        {
            if (iconName != NULL) {
                m_pressedIcon.Create(iconName);
                m_icon.Create(iconName);
            } else {
                m_pressedIcon.Create("SUR_ITEM_STATUS_SEL_PURCHASED");
                m_icon.Create("SUR_ITEM_STATUS_PURCHASED");
            }
            ResetLayout();
        }

        /** Forget the painted position (set again by ItemsWindow::Paint). */
        void ResetLayout()
        {
            m_y = 0;
            m_height = 0;
            m_arrowX[0] = 0;
            m_arrowX[1] = 0;
            m_arrowWidth[0] = 0;
            m_arrowWidth[1] = 0;
        }

        XString m_text;             // 0x00
        int m_type;                 // 0x04 EItemType
        int m_id;                   // 0x08 passed to SelectedItemChanged / ClickItem
        bool m_enabled;             // 0x0c can be selected
        ImageRes m_pressedIcon;     // 0x10 icon while the pointer presses the item
        ImageRes m_icon;            // 0x18
        Vector<SubItem> m_subItems; // 0x20
        int m_subIndex;             // 0x30 selected sub item
        short m_y;                  // 0x34 painted position in the items window
        short m_height;             // 0x36
        short m_arrowX[2];          // 0x38 left / right arrow of the sub items
        short m_arrowWidth[2];      // 0x3c
    };

    /** Fonts of an item: while the pointer presses it, and normally. */
    struct ItemFonts {
        ItemFonts(CFont* pressedFont, CFont* font)
        {
            m_pressedFont = pressedFont;
            m_font = font;
        }

        CFont* m_pressedFont;     // 0x00
        CFont* m_font;            // 0x04
    };

    /** The scrolling list of items. */
    class ItemsWindow : public Window {
    public:
        ItemsWindow();

        inline virtual ~ItemsWindow()
        {
        }

        virtual void SetRect(int x, int y, int width, int height, bool force);
        virtual void OnCommand(Event& event);
        virtual void OnPointerEvent(Event& event);
        virtual void Paint(ICGraphics2d& graphics);
        /** Draw the text of item `index` and the separator line below it. */
        virtual void PaintItem(ICGraphics2d& graphics, int index, int x, int y,
                               int height, bool centered);
        /** Draw the selected sub item of item `index` with its arrows. */
        virtual void PaintSubItem(ICGraphics2d& graphics, int index, int x, int y);

        /** Select item `index`, skipping the disabled ones in the direction of the move. */
        void SetSelectedIndex(int index);
        void SetSelectedKey(int id);
        /** Scroll so that the selected item is visible. */
        void UpdateTopIndex();
        void SetSubitemSelectedIndex(int index, int subIndex);
        int MaxIconWidth() const;
        void AddItem(int id, const XString& text, const char* iconName, int type);
        CFont* ItemFontByIndex(int index) const;
        int VirtualWidth() const;
        int VirtualHeight() const;
        void UpdateScrollYByTopIndex();
        /** Scroll position 0..1 (1 when every item is visible). */
        float ScrollPos() const;

        /** The dialog that owns the list. */
        SimpleDialog* Dialog() const
        {
            return (SimpleDialog*)m_parent;
        }

        Vector<Item> m_items;       // 0xa4
        int m_selectedIndex;        // 0xb4
        ImageRes m_arrowLeft;       // 0xb8 sub item arrows
        ImageRes m_arrowRight;      // 0xc0
        ImageRes m_lineLeft;        // 0xc8 separator line between the items
        ImageRes m_lineMiddle;      // 0xd0
        ImageRes m_lineRight;       // 0xd8
        int m_topIndex;             // 0xe0 first visible item
        int m_scrollY;              // 0xe4
        bool m_dragging;            // 0xe8 the list is dragged with the pointer
    };

    SimpleDialog(EDialogType type);
    virtual ~SimpleDialog();

    virtual void OnEasyStarted();
    virtual void SetRect(int x, int y, int width, int height, bool force);
    virtual void Init();
    virtual void Shutdown();
    virtual void OnGetFocus();
    virtual void OnLostFocus();
    virtual void OnLayout();
    virtual void OnCommand(Event& event);
    virtual void Paint(ICGraphics2d& graphics);

    virtual ItemsWindow* CreateItemInstance();
    virtual ItemFonts ItemFontsByIndex(int index) const;
    virtual void SetText(const XString& text);
    virtual void SetTextFont(CFont* font);
    virtual void SetTitle(const XString& title);
    virtual void SetTitleFont(CFont* font);
    virtual void PaintBackground(ICGraphics2d& graphics);
    virtual void PaintScrollbar(ICGraphics2d& graphics);
    virtual void PaintItemIcon(ICGraphics2d& graphics, int index, bool selected,
                               Item* item, int x, int y);

    virtual void SelectedItemChanged(int id)
    {
    }

    virtual void ClickItem(int id, bool byPointer)
    {
    }

    virtual void SelectedSubitemChanged(int id, int subId)
    {
    }

    virtual void ClickSubitem(int id, int subId, bool byPointer)
    {
    }

    virtual void CancelDialog();
    /** Size and place the dialog around its content inside the given rectangle. */
    virtual void AdjustByRect(int x, int y, int width, int height);
    virtual void AdjustByDefRect();
    // b118: InitCenter is introduced by the concrete dialog; the original SimpleDialog ends at AdjustByDefRect.

    /** Add an item to the list (the list window is created with the first item). */
    void AddItem(int id, const XString& text, int type);
    void AddResourceItem(const char* text, int id, int type);
    void AddResourceItem(const char* text, const ImageRes& pressed, const ImageRes& image, int id, int type);
    Item* ItemByKey(int id) const;
    void SetSelectedKey(int id);

    /** Height of one item: the item font plus a margin. */
    static int getItemHeight();

    /** Indices of m_margins. */
    enum {
        MARGIN_SIDE = 0,        // left and right
        MARGIN_TOP = 1,
        MARGIN_BOTTOM = 2,
        MARGIN_ITEMS = 3,       // between the text and the items
        MARGIN_ITEM_GAP = 4,    // between two items
        MARGIN_TITLE = 5        // below the title
    };

    EDialogType m_type;                 // 0xa4
    signed char m_margins[6];           // 0xa8
    ImageRes m_borderLeft;              // 0xb0 frame images
    ImageRes m_borderRight;             // 0xb8
    ImageRes m_borderTop;               // 0xc0
    ImageRes m_borderBottom;            // 0xc8
    ImageRes m_cornerTopLeft;           // 0xd0
    ImageRes m_cornerTopRight;          // 0xd8
    ImageRes m_cornerBottomLeft;        // 0xe0
    ImageRes m_cornerBottomRight;       // 0xe8
    ImageRes m_imageF8;                 // 0xf0 never drawn here
    ImageRes m_background;              // 0xf8 fills the frame
    ImageRes m_titleLineLeft;           // 0x100 line below the title
    ImageRes m_titleLineMiddle;         // 0x108
    ImageRes m_titleLineRight;          // 0x110
    ImageRes m_scrollTop;               // 0x118 scroll bar images
    ImageRes m_scrollBottom;            // 0x120
    ImageRes m_scrollTrack;             // 0x128
    ImageRes m_scrollThumb;             // 0x130
    ImageRes m_scrollThumbTop;          // 0x138
    ImageRes m_scrollThumbBottom;       // 0x140
    MediaRes m_soundSelect;             // 0x148 played at the start of an easing animation
    MediaRes m_soundClick;              // 0x150
    SimpleTextArea* m_textArea;         // 0x158
    ItemsWindow* m_itemsWindow;         // 0x15c NULL until the first item is added
    XString m_title;                    // 0x160
    CFont* m_titleFont;  // 0x164
    bool m_registerKeys;                // 0x168 register the UI keys while focused
    int m_frameStyle;                   // 0x16c 1: frame and title are drawn and laid out
};

#endif
