// CExecutableRegistry.cpp - runs the registered executables once per OnExecute().
#include "CExecutableRegistry.h"

#include "CRegistryItr.h"
#include "ICResourceManager.h"
#include "CResource.h"

                                                                        

void CExecutableRegistry::OnExecute()
{
    ICResourceManager* resourceManager = ICResourceManager::GetInstance();
    CRegistryItr itr(*this);
    while (!itr.IsDone()) {
        CSystemElement* element = &*itr;
        CExecutable* executable = (CExecutable*)element->m_object;
        if (executable == NULL) {
            // the element names a resource whose data is the executable
            CResource* resource;
            if (!resourceManager->GetResource(element->m_handle, &resource)) {
                resourceManager->CreateResource(element->m_handle, &resource);
            }
            if (resource == NULL) {
                continue;
            }
            executable = (CExecutable*)resource->GetData();
        }
        if (executable->Run() == CExecutable::RunResult_Deleted) {
            // the executable deleted itself: drop its element
            CSystemElement* next = element->m_next;
            Remove(*element);
            if (next != NULL) {
                itr.m_current = next;
            }
            if (element != NULL) {
                delete element;
            }
            continue;
        }
        itr++;
    }
}

       
