#ifndef CGIFTRECIEVEDNEWSWINDOW_H
#define CGIFTRECIEVEDNEWSWINDOW_H

#include "WindowBase.h"
#include "Vector_ss.h"

struct SGiftReceivedMessage;
class CItem;
class StretchImageButton;

class CGiftRecievedNewsWindow : public Window {
public:
    explicit CGiftRecievedNewsWindow(const SGiftReceivedMessage* message);
    virtual void OnCommand(Event& event);

private:
    void CreateUserInterface(const XString& sender, const XString& item, bool fromFriend);

    CItem* m_item;                           // 0xa4
    int m_fieldA8;                           // 0xa8
    int m_messageId;                         // 0xac
    const SGiftReceivedMessage* m_message;   // 0xb0
    int m_fieldB4;                           // 0xb4
    StretchImageButton* m_sendButton;         // 0xb8
    Vector<int> m_items;                     // 0xbc, unused by the original UI
};

typedef char GiftRecievedNewsWindowSizeCheck[sizeof(CGiftRecievedNewsWindow) == 0xcc ? 1 : -1];

#endif
