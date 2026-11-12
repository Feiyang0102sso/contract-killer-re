// Templates for scene objects with independently animated instances.
#ifndef CVISUALOBJECT_H
#define CVISUALOBJECT_H

#include "SwerveHelper.h"

class CVisualObject {
public:
    CVisualObject();
    virtual ~CVisualObject();
    SwerveHelper::Group GetModelCopy(const char* const& resource, const char* root);
    void LoadModelImpl(const char* const& resource, const char* root);
    void ReleaseModelCopy();

private:
    SwerveHelper::Object3D m_model;
    int m_copyCount;
};

#endif
