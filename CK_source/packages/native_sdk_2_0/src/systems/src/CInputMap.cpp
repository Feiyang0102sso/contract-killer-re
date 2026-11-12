// CInputMap.cpp - executable registry of input mappings.
#include "CInputMap.h"

#include "CRegistryItr.h"

                                                                        

CInputMap::CInputMap()
{
    m_classId = ClassId;
}

CInputMap::~CInputMap()
{
    UnregisterAll();
}

void CInputMap::UnregisterAll()
{
    CRegistryItr itr(*this);
    while (!itr.IsDone()) {
        CSystemElement* element = &*itr;
        CClass* mapping = (CClass*)element->m_object;
        if (mapping != NULL) {
            delete mapping;
        }
        itr++;
        Remove(*element);
        if (element != NULL) {
            delete element;
        }
    }
}

       
