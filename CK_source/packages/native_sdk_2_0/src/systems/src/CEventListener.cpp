// CEventListener.cpp - registration of listeners in the event system.
#include "CEventListener.h"

#include "CApplet.h"
#include "CApp.h"
#include "CEvent.h"
#include "CRegistry.h"
#include "CRegistryItr.h"
#include "CRegistryAccelerateHandleQuery.h"

/**
 * Game hook (src/CGameApp.cpp): whether the listener registry of `eventId` gets a handle hash,
 * and its bucket count and node pool size.
 */
unsigned char AccelerateEventSpecificListenerQuery(unsigned int eventId, int& bucketCount, int& poolSize);

                                                                        

unsigned char CEventListener::Register(unsigned int eventId, int priority)
{
    CSystem* rootSystem = (CSystem*)CApplet::m_pApp->m_subsystem14;
    unsigned char registered = 0;
    CSystemElement* element;
    if (rootSystem->Query(0, kEventSystemHandle, &element)) {
        registered = 1;
        RegisterIdSpecfic(eventId, priority, *(CSystem*)element->m_object);
    }
    return registered;
}

void CEventListener::RegisterIdSpecfic(unsigned int eventId, int priority, CSystem& eventSystem)
{
    CSystemElement* element;
    CRegistry* listeners;
    if (!eventSystem.Query(0, eventId, &element)) {
        // first listener of this event: create its registry
        listeners = new CRegistry();
        int bucketCount;
        int poolSize;
        if (AccelerateEventSpecificListenerQuery(eventId, bucketCount, poolSize)) {
            listeners->SetAccelerator(new CRegistryAccelerateHandleQuery(bucketCount, poolSize));
        }
        eventSystem.Add(*eventSystem.CreateSystemElement(eventId, listeners, 0, DefaultPriority));
    } else {
        listeners = (CRegistry*)element->m_object;
    }
    listeners->Add(*listeners->CreateSystemElement(m_handle, this, m_classId, priority));
}

unsigned char CEventListener::Register(const unsigned int* eventIds, int priority)
{
    CSystem* rootSystem = (CSystem*)CApplet::m_pApp->m_subsystem14;
    unsigned char registered = 0;
    CSystemElement* element;
    if (rootSystem->Query(0, kEventSystemHandle, &element)) {
        registered = 1;
        do {
            RegisterIdSpecfic(*eventIds, priority, *(CSystem*)element->m_object);
            eventIds++;
        } while (*eventIds != 0);
    }
    return registered;
}

void CEventListener::UnregisterAll()
{
    if (m_handle == 0) {
        return;
    }
    CSystem* rootSystem = (CSystem*)CApplet::m_pApp->m_subsystem14;
    CSystemElement* element;
    if (!rootSystem->Query(0, kEventSystemHandle, &element)) {
        return;
    }

    CEventCore* core = CEventCore::GetInstance();
    CRegistry* eventSystem = (CRegistry*)element->m_object;
    CRegistryItr itr(*eventSystem);
    while (!itr.IsDone()) {
        CSystemElement* eventElement = &*itr;
        CRegistry* listeners = (CRegistry*)eventElement->m_object;
        CSystemElement* listenerElement;
        if (listeners->Query(0, m_handle, &listenerElement)) {
            listeners->Remove(*listenerElement);
            if (core->m_isDispatching) {
                // An active iterator can still hold this entry. Skip its callback and free it later.
                listenerElement->m_object = NULL;
                core->m_pendingRemovals.Add(listenerElement);
            } else {
                if (listenerElement != NULL) {
                    delete listenerElement;
                }
                if (listeners->IsEmpty()) {
                    // last listener gone: drop the registry of the event
                    itr++;
                    eventSystem->Remove(*eventElement);
                    if (listeners != NULL) {
                        delete listeners;
                    }
                    if (eventElement != NULL) {
                        delete eventElement;
                    }
                    continue;
                }
            }
        }
        itr++;
    }
}

       
