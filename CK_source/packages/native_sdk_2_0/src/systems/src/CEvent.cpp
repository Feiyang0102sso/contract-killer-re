// CEvent.cpp - event delivery to the registered CEventListener objects.
#include "CEvent.h"

#include "CApplet.h"
#include "CApp.h"
#include "CEventListener.h"
#include "CRegistry.h"
#include "CRegistryItr.h"

                                                                        

CEvent::CEvent()
{
    m_classId = ClassId;
    m_eventId = 0;
    m_deleteWhenDone = 1;
    m_sender = NULL;
    m_param = 0;
    m_runCount = OneShot;
    m_wasEverHandled = 0;
    m_wasHandled = 0;
}

CEvent::CEvent(CClass* sender, unsigned int eventId, unsigned int param, int interval, unsigned int runCount)
{
    m_classId = ClassId;
    m_eventId = eventId;
    m_deleteWhenDone = 1;
    m_sender = sender;
    m_param = param;
    m_interval = interval;
    m_runCount = runCount;
    m_wasEverHandled = 0;
    m_wasHandled = 0;
}

void CEvent::OnExecute()
{
    CSystem* rootSystem = (CSystem*)CApplet::m_pApp->m_subsystem14;
    m_wasHandled = 0;

    CSystemElement* element;
    if (!rootSystem->Query(0, kEventSystemHandle, &element)) {
        return;
    }
    CRegistry* eventSystem = (CRegistry*)element->m_object;
    if (!eventSystem->Query(0, m_eventId, &element)) {
        return;
    }

    CEventCore* core = CEventCore::GetInstance();
    const unsigned char alreadyDispatching = core->m_isDispatching;
    core->m_isDispatching = 1;
    CRegistry* listeners = (CRegistry*)element->m_object;
    if (m_param != 0) {
        // only the listener registered under m_param
        if (listeners->Query(0, m_param, &element)) {
            CEventListener* listener = (CEventListener*)element->m_object;
            if (listener->m_callback(*this, listener)) {
                if (m_wasHandled == 0) {
                    m_wasHandled = 1;
                }
                if (m_wasEverHandled == 0) {
                    m_wasEverHandled = 1;
                }
            }
        }
    } else {
        // broadcast to every listener of the event
        for (CRegistryItr itr(*listeners); !itr.IsDone(); itr++) {
            CEventListener* listener = (CEventListener*)(*itr).m_object;
            if (listener != NULL && listener->m_callback(*this, listener)) {
                if (m_wasHandled == 0) {
                    m_wasHandled = 1;
                }
                if (m_wasEverHandled == 0) {
                    m_wasEverHandled = 1;
                }
            }
        }
    }
    core->m_isDispatching = alreadyDispatching;
    if (alreadyDispatching || core->m_pendingRemovals.Size() == 0) {
        return;
    }

    // All iterators have returned. Release the removed nodes and any empty event registry.
    for (int index = 0; index < core->m_pendingRemovals.Size(); index++) {
        delete core->m_pendingRemovals[index];
    }
    core->m_pendingRemovals.Clear();
    CRegistryItr itr(*eventSystem);
    while (!itr.IsDone()) {
        CSystemElement* entry = &*itr;
        CRegistry* eventListeners = (CRegistry*)entry->m_object;
        itr++;
        if (eventListeners->IsEmpty()) {
            eventSystem->Remove(*entry);
            delete eventListeners;
            delete entry;
        }
    }
}

       
