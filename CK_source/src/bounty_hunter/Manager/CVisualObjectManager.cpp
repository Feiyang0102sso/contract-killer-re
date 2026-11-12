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
    XString name(resource);
    CVisualObject* object;
    CVisualObject* const* existing = m_objects.Get(name);
    if (existing != NULL) {
        object = *existing;
    } else {
        object = new CVisualObject();
        m_objects.Set(name, object);
    }
    return object->GetModelCopy(resource, root);
}

void CVisualObjectManager::Release(const char* const& resource)
{
    CVisualObject* const* object = m_objects.Get(XString(resource));
    if (object != NULL) {
        (*object)->ReleaseModelCopy();
    }
}

typedef char AssertVisualObjectManagerSize[sizeof(CVisualObjectManager) == 0x10 ? 1 : -1];
