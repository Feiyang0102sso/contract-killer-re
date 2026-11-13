#ifndef CWINDOWQUEUE_H
#define CWINDOWQUEUE_H

#include "WindowBase.h"

/** Queues pending dialogs and opens each in the application's modal layer. */
class CWindowQueue : public Window {
public:
    CWindowQueue();
    virtual ~CWindowQueue();
    virtual void Update();
    virtual void OnCommand(Event& event);

    bool Add(Window* window);
    bool ShowNext();
    bool Start();
    bool IsEmpty();

private:
    struct Entry {
        Window* window;
        Entry* next;
    };

    Window* m_active;
    Entry* m_first;
    Entry* m_last;
};

#endif
