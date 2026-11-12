#ifndef CVISUALOBJECTMANAGER_H
#define CVISUALOBJECTMANAGER_H

#include "CVisualObject.h"
#include "HashTable.h"
#include "xString.h"

class CVisualObjectManager {
public:
    CVisualObjectManager();
    virtual ~CVisualObjectManager();
    SwerveHelper::Group Create(const char* const& resource, const char* root);
    void Release(const char* const& resource);

private:
    HashTable<XString, CVisualObject*> m_objects;
};

#endif
