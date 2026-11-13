#ifndef CINCENTIVIZEDWINDOW_H
#define CINCENTIVIZEDWINDOW_H

#include "CDialogWindow.h"
#include "Vector_ss.h"

class CCheckBox;
class CImageButton;
class ScrollingContainer;

/** Friend picker shared by the social invitation dialogs. */
class CIncentivizedWindow : public CTextDialog {
public:
    explicit CIncentivizedWindow(const XString& acceptCaption);
    virtual void OnCommand(Event& event);

    void AddFriend(const XString& name, int friendId);
    void SelectFriend(int friendId);
    void SetSingleSelection() { m_singleSelection = true; }
    void SetEventReciever(Window* receiver);
    Vector<int> GetSelectedIds() const;
    void SetCommand(unsigned int command);
    void SetCommandParameter(int parameter) { m_commandParameter = parameter; }
    void SetExtraButton(const XString& text, const char* image, unsigned int command);
    void ShowSelectAllButton();
    friend class CFriendsWindow;  // original CFriendsWindow::ShowSelectGiftRecipient calls SelectCheckBox

private:
    CCheckBox* CreateCheckBox(const XString& name, int friendId, int row);
    void HandleAcceptPressed();
    void HandleCheckBoxSelected(int friendId);
    void HandleCheckBoxDeselected(int friendId);
    void DeselectAllCheckBoxes();
    void SelectCheckBox(int index);
    void ShowPage(int page);
    CImageButton* CreateButton(const XString& text, const char* image, int align, int command);

    int m_page;
    int m_row;
    int m_commandParameter;
    int m_command;
    Window* m_friendRows;
    CImageButton* m_next;
    CImageButton* m_previous;
    ScrollingContainer* m_scroll;
    Vector<CCheckBox*> m_checkBoxes;
    Vector<int> m_friendIds;
    Vector<XString> m_friendNames;
    Vector<int> m_selectedIds;
    Window* m_groups;
    bool m_singleSelection;
    Window* m_eventReceiver;
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char IncentivizedWindowSizeCheck[sizeof(CIncentivizedWindow) == 0x18c ? 1 : -1];
#endif

#endif
