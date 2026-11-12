// CResourceManager.cpp - resource manager interface and the common manager base.
// plat 2026-11-10: CK 1.5.2 form - one base class whose list/name operations are empty
// defaults (return 0); the handle hash is the only state it manages itself.
#include "CResourceManager.h"

#include "CResource.h"



/** Game-specific factories (implemented by the game, src/CGameApp.cpp). */
// Declaration now comes from the common CAppFactory.h; the original comment is retained above.
// (CK 1.5.2 has no CAppFactory: CGameApp constructs the CResourceManager_v2 itself.)



const unsigned long CResourceManager::DefaultTableSize = 1500;
const unsigned long CResourceManager::DefaultNumOfElementsInInitialAlloc = 150;

CResourceManager::~CResourceManager()
{
}

void CResourceManager::Destroy()
{
    m_resources.Destroy();
    m_router = NULL;
}

unsigned char CResourceManager::AddResource(CResource& resource)
{
    m_resources.Insert(resource.m_handle, &resource);
    return 1;
}

unsigned char CResourceManager::RemoveResource(unsigned int handle)
{
    m_resources.Remove(handle);
    return 1;
}

unsigned char CResourceManager::RemoveResource(const char* name)
{
    return 0;
}

unsigned char CResourceManager::RemoveResource(const wchar_t* name)
{
    return 0;
}

unsigned char CResourceManager::RemoveResource(CResource& resource)
{
    return RemoveResource(resource.m_handle);
}

unsigned int CResourceManager::CreateResource(unsigned int handle, CResource** resource)
{
    return 0;
}

unsigned char CResourceManager::CreateResources(const unsigned int* handles)
{
    return 0;
}

unsigned int CResourceManager::CreateResource(const CreationInfoHnd& info, CResource** resource)
{
    return 0;
}

unsigned char CResourceManager::CreateResources(const CreationInfoHnd* infos)
{
    return 0;
}

unsigned int CResourceManager::CreateResource(const char* name, CResource** resource)
{
    return 0;
}

unsigned char CResourceManager::CreateResources(const char** names)
{
    return 0;
}

unsigned int CResourceManager::CreateResource(const CreationInfoStr& info, CResource** resource)
{
    return 0;
}

unsigned char CResourceManager::CreateResources(const CreationInfoStr* infos)
{
    return 0;
}

unsigned int CResourceManager::CreateResource(const wchar_t* name, CResource** resource)
{
    return 0;
}

unsigned char CResourceManager::CreateResources(const wchar_t** names)
{
    return 0;
}

unsigned int CResourceManager::CreateResource(const CreationInfoWStr& info, CResource** resource)
{
    return 0;
}

unsigned char CResourceManager::CreateResources(const CreationInfoWStr* infos)
{
    return 0;
}

unsigned char CResourceManager::GetResource(unsigned int handle, CResource** resource)
{
    *resource = NULL;
    if (m_resources.Find(handle, (void**)resource)) {
        return 1;
    }
    return 0;
}

unsigned char CResourceManager::GetResource(const char* name, CResource** resource)
{
    return 0;
}

unsigned char CResourceManager::GetResource(const wchar_t* name, CResource** resource)
{
    return 0;
}

unsigned char CResourceManager::ReleaseResource(unsigned int handle)
{
    return 0;
}

unsigned char CResourceManager::ReleaseResource(const char* name)
{
    return 0;
}

unsigned char CResourceManager::ReleaseResources(const char** names)
{
    return 0;
}

unsigned char CResourceManager::ReleaseResource(const wchar_t* name)
{
    return 0;
}

unsigned char CResourceManager::ReleaseResources(const wchar_t** names)
{
    return 0;
}

unsigned char CResourceManager::ReleaseResource(CResource** resource)
{
    return 0;
}

unsigned char CResourceManager::ReleaseResources(const unsigned int* handles)
{
    return 0;
}

unsigned char CResourceManager::DestroyResource(unsigned int handle)
{
    return 0;
}

unsigned char CResourceManager::DestroyResource(const char* name)
{
    return 0;
}

unsigned char CResourceManager::DestroyResources(const char** names)
{
    return 0;
}

unsigned char CResourceManager::DestroyResource(const wchar_t* name)
{
    return 0;
}

unsigned char CResourceManager::DestroyResources(const wchar_t** names)
{
    return 0;
}

unsigned char CResourceManager::DestroyResources(const unsigned int* handles)
{
    return 0;
}

unsigned char CResourceManager::DestroyResources()
{
    return 0;
}
