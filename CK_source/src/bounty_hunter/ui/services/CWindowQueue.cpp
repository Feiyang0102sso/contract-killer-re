#include "CWindowQueue.h"

#include "App.h"

CWindowQueue::CWindowQueue() : Window(false), m_active(NULL), m_first(NULL), m_last(NULL)
{
}

CWindowQueue::~CWindowQueue()
{
    while (m_first != NULL) {
        Entry* entry = m_first;
        m_first = entry->next;
        delete entry->window;
        delete entry;
    }
}

bool CWindowQueue::Add(Window* window)
{
    Entry* entry = new Entry();
    entry->window = window;
    entry->next = NULL;
    if (m_first == NULL) {
        m_first = entry;
    } else {
        m_last->next = entry;
    }
    m_last = entry;
    return true;
}

void CWindowQueue::OnCommand(Event& event)
{
    if (event.m_id == (int)0xbb03fc55) {
        ShowNext();
    }
}

bool CWindowQueue::ShowNext()
{
    if (m_first == NULL) {
        m_active = NULL;
    } else {
        Entry* entry = m_first;
        m_active = entry->window;
        m_first = entry->next;
        if (m_first == NULL) {
            m_last = NULL;
        }
        delete entry;
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_mainLayer->AddModal(m_active);
    }
    return false;
}

void CWindowQueue::Update()
{
    if (m_active == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!app->m_mainLayer->IsChildExisting(m_active) || (m_active->m_flags & FLAG_SHUT_DOWN) != 0) {
        ShowNext();
    }
}

bool CWindowQueue::Start()
{
    if (m_active == NULL) {
        ShowNext();
    }
    return false;
}

bool CWindowQueue::IsEmpty()
{
    return m_active == NULL;
}

typedef char AssertWindowQueueSize[sizeof(CWindowQueue) == 0xb0 ? 1 : -1];
