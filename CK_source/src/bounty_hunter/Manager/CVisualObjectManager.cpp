#include "CVisualObjectManager.h"

CVisualObjectManager::CVisualObjectManager() : m_objects(20)
{
}

CVisualObjectManager::~CVisualObjectManager()
{
    m_objects.DeleteContents();
}

SwerveHelper::Group CVisualObjectManager::Create(const char* const& resource, const char* root)
{
    CVisualObject* object = NULL;
    CVisualObject* const* existing = m_objects.Get(XString(resource));
    if (existing == NULL) {
        object = new CVisualObject();
        m_objects.Set(XString(resource), object);
    } else {
        object = *m_objects.Get(XString(resource));
    }
    if (object == NULL) {
        return SwerveHelper::Group();
    }
    return object->GetModelCopy(resource, root);
}

void CVisualObjectManager::Release(const char* const& resource)
{
    CVisualObject* const* existing = m_objects.Get(XString(resource));
    if (existing != NULL) {
        CVisualObject* object = *m_objects.Get(XString(resource));
        if (object != NULL) {
            object->ReleaseModelCopy();
        }
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertVisualObjectManagerSize[sizeof(CVisualObjectManager) == 0x10 ? 1 : -1];
#endif
