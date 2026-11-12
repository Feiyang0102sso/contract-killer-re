// CSwerve.cpp - application singleton owning the Swerve (M3G) runtime and its renderer.
// (CK version, written by ggn from the CK reference; CKZ's GL version check, error logging and
// user id hashing do not exist in CK's object)
#include "CSwerve.h"
#include "CSwerveGraphics3D.h"
#include "CApplet.h"
#include "CApp.h"
#include "CResourceManager.h"
#include "CResource.h"
#include "CVector.h"

/** Object kind of the M3G loader (ISwerveFunctions::CreateObject). */
static const int OBJECT_TYPE_LOADER = 0xe;

CSwerve* CSwerve::m_pSwerve;

CSwerve::CSwerve() : CSingleton(SingletonId)
{
    m_swerve = NULL;
    Init();
}

unsigned char CSwerve::Init()
{
    int error = swvNew(&m_swerve);
    if (error == 0) {
        error = m_swerve->m_functions->Initialize(m_swerve, 0, 0);
        if (error == 0) {
            m_graphics3D = new CSwerveGraphics3D();
            if (m_graphics3D == NULL) {
                error = 2;
            }
        }
    }
    return error == 0;
}

CSwerve::~CSwerve()
{
    Free();
    m_pSwerve = NULL;
}

void CSwerve::Free()
{
    if (m_graphics3D != NULL) {
        delete m_graphics3D;
        m_graphics3D = NULL;
    }
    if (m_swerve != NULL) {
        m_swerve->m_functions->Release(m_swerve);
    }
    m_swerve = NULL;
}

/** Decode an M3G file with `loader` and return its first object (NULL on failure). */
IObject3D* CSwerve::LoadM3GRoot(ILoader* loader, unsigned int size, const unsigned char* data)
{
    IObject3D* root = NULL;
    if (loader->m_functions->Load(loader, size, data, 0) == 0) {
        int count;
        loader->m_functions->GetObjectCount(loader, &count);
        int error = 0;
        for (int index = 0; index < count && error == 0 && root == NULL; index++) {
            IObject3D* object;
            error = loader->m_functions->GetObject(loader, index, &object);
            if (error == 0) {
                root = object;
            }
        }
    }
    return root;
}

/** Load the M3G resource `name` and return its first object (NULL on failure). */
IObject3D* CSwerve::LoadM3GResource(const char* name)
{
    IObject3D* root = NULL;
    ILoader* loader;
    int error = m_swerve->m_functions->CreateObject(OBJECT_TYPE_LOADER, (void**)&loader);
    if (error == 0) {
        CResourceManager* resourceManager = CApplet::m_pApp->m_resourceManager;
        CResource* resource = NULL;
        resourceManager->CreateResource(name, &resource);
        if (resource != NULL) {
            CVectorLegacy<unsigned char>* bytes = (CVectorLegacy<unsigned char>*)resource->GetData();
            root = LoadM3GRoot(loader, bytes->m_size, bytes->m_data);
            resourceManager->ReleaseResource(name);
        }
        loader->m_functions->Release(loader);
    }
    return root;
}
