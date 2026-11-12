// CK localized strings and UI input-map registrations.
#include "CUtility.h"
#include "CApplet.h"
#include "CApp.h"
#include "CAppProperties.h"
#include "CResourceManager.h"
#include "CResource.h"
#include "CKeyInputMapping.h"
#include "CStrWChar.h"


extern const char JADKEY__SOFTKEY_REVERSE[];

void CUtility::GetString(CStrWChar& text, const char* id)
{
    text.ReleaseMemory();
    text.Concatenate("");
    AppendString(text, id);
}
void CUtility::AppendString(CStrWChar& text, const char* id)
{
    CResourceManager* resources = (CResourceManager*)CApplet::m_pApp->m_resourceManager;
    CResource* resource = NULL;
    resources->CreateResource(id, &resource);
    if (resource != NULL) {
        CStrWChar* value = (CStrWChar*)resource->GetData();
        if (value != NULL) {
            text.Concatenate(value->GetWCharPtr());
        }
        resources->ReleaseResource(id);
    }
}
CStrWChar CUtility::GetString(const char* id)
{
    CStrWChar text("");
    AppendString(text, id);
    return text;
}
void CUtility::RegisterUIKeys()
{
    CKeyInputMapping::Register(0x92484fb2, 0x3f972120, (eKeyCode)4, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x972028c1, (eKeyCode)5, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x9720a3cb, (eKeyCode)6, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x7214ff0d, (eKeyCode)7, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x2074d009, (eKeyCode)11, 1, 0, 0, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x97204784, (eKeyCode)12, 1, 0, 0, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x3f972120, (eKeyCode)0x32, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x972028c1, (eKeyCode)0x38, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x9720a3cb, (eKeyCode)0x34, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x7214ff0d, (eKeyCode)0x36, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x2074d009, (eKeyCode)0x35, 1, 0, 0, 1);
}
void CUtility::RegisterUISoftkeys()
{
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(CAppProperties::SingletonId, &found);
    CAppProperties* instance = (CAppProperties*)found;
    if (instance == NULL) {
        instance = new CAppProperties();
    }
    unsigned int rightAction;
    if (instance->m_properties.GetBool(JADKEY__SOFTKEY_REVERSE) == 0) {
        CKeyInputMapping::Register(0x92484fb2, 0x5f61a84a, (eKeyCode)15, 1, 0, 0, 1);
        rightAction = 0xd45f52de;
    } else {
        CKeyInputMapping::Register(0x92484fb2, 0xd45f52de, (eKeyCode)15, 1, 0, 0, 1);
        rightAction = 0x5f61a84a;
    }
    CKeyInputMapping::Register(0x92484fb2, rightAction, (eKeyCode)16, 1, 0, 0, 1);
}
void CUtility::UnregisterUIKeys()
{
    // The original repeats the first five actions and does not unregister the release mappings here.
    CKeyInputMapping::Unregister(0x92484fb2, 0x3f972120, (eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x972028c1, (eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x9720a3cb, (eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x7214ff0d, (eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x2074d009, (eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x97204784, (eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x3f972120, (eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x972028c1, (eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x9720a3cb, (eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x7214ff0d, (eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x2074d009, (eKeyCode)0);
}
void CUtility::UnregisterUISoftkeys()
{
    CKeyInputMapping::Unregister(0x92484fb2, 0x5f61a84a, (eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0xd45f52de, (eKeyCode)0);
}
