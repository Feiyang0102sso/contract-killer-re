// CDH_ResourceManager.h - game resources: image surfaces cached by name, other data from the platform
// resource manager.
#ifndef CDH_RESOURCEMANAGER_H
#define CDH_RESOURCEMANAGER_H

#include <stddef.h>
#include "Vector_ss.h"
#include "CStrChar.h"
#include "CKeysetResource.h"

                                                                         
class ICRenderSurface;
       

/** Data of a KEYSET_* resource: a list of resource ids (layout from CDH_SoundManager::PlayRandomSound). */  // added by b048
typedef CKeysetResource CDH_KeySet;

class CDH_ResourceManager {
public:
    /** A cached image surface (24 bytes; implicit copy constructor). */
    struct TImage {
        ICRenderSurface* m_surface;   // 0x00
        CStrChar m_name;            // 0x04 resource name
        int m_orientation;                                          // 0x14 InitParam_Orientation (-1: none)
    };

    CDH_ResourceManager();
    virtual ~CDH_ResourceManager();

    /** Delete every cached surface. */
    void flush();
    /** Delete the cached surface `name`; false when it is not cached. */
    bool releaseImage(const char* name);
    /** Load (or take from the cache) the image `name`; `orientation` < 0: as stored. */
    ICRenderSurface* createImageSurface(const char* name, int orientation);
    /** The cached surface `name` (NULL when not cached). */
    ICRenderSurface* getFromCache(const char* name);
    /** Data of resource `name`: an image surface, or the resource data (for a key set a CDH_KeySet).
        `loading` is stored in DGHelper::m_loadingResource. */
    void* getResourceData(const char* name, bool loading);
    /** Release resource `name` (cached image or platform resource). */
    void releaseResource(const char* name);

    Vector<TImage> m_images;    // 0x04
};

#endif
