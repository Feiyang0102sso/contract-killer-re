// ICGraphicsResource.h - base of every object a graphics device creates (created by b037x).
// CK initializes the resource type inline in each resource constructor.
#ifndef ICGRAPHICSRESOURCE_H
#define ICGRAPHICSRESOURCE_H

#include "ICGraphicsAbstractionManager.h"

                                                                         

/**
 * Base of the objects a device creates (surfaces, buffers, shaders, display programs). Every
 * resource registers itself in the resource registry of the current device for its lifetime, so
 * that the device can save and recreate them when the context is lost.
 */
class ICGraphicsResource {
public:
    explicit ICGraphicsResource(unsigned int resourceType) : m_resourceType(resourceType) {}
    virtual ~ICGraphicsResource() {}

    /** The device abstraction the resource belongs to. */
    virtual ICGraphicsAbstract::Abstraction GetAbstraction() = 0;

    unsigned int m_resourceType;    // 0x04 key of the resource kind (registry element class id)
};

       

#endif
