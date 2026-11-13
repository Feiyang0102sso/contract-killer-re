#include "CVisualObject.h"

#include "App.h"
#include "DGHelper.h"

CVisualObject::CVisualObject() : m_copyCount(0)
{
}

CVisualObject::~CVisualObject()
{
    // the original releases the template model explicitly and clears it inside the check
    if (m_model.m_interface != NULL) {
        m_model.m_interface->Release();
        m_model.m_interface = NULL;
    }
}

SwerveHelper::Group CVisualObject::GetModelCopy(const char* const& resource, const char* root)
{
    if (m_model == NULL) {
        LoadModelImpl(resource, root);
    }
    ++m_copyCount;
    if (m_model == NULL) {
        return SwerveHelper::Group();
    }
    SwerveHelper::Group copy(m_model.duplicate());
    return SwerveHelper::FullObjectDuplicate(copy, 0);
}

void CVisualObject::LoadModelImpl(const char* const& resource, const char* root)
{
    SwerveHelper::Object3D object = App::LoadObject3D(resource);
    if (object != NULL) {
        m_model = object.find(DGHelper::getSwerveID(root));
    }
}

void CVisualObject::ReleaseModelCopy()
{
    --m_copyCount;
    if (m_copyCount == 0) {
        m_model.Clear();
    }
}

typedef char AssertVisualObjectSize[sizeof(CVisualObject) == 0xc ? 1 : -1];
