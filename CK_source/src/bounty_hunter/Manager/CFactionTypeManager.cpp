#include "CFactionTypeManager.h"

#include "App.h"
#include "CFactionType.h"
#include "CRandGen.h"
#include "ResMap.h"
#include "tinyxml.h"

CFactionTypeManager::CFactionTypeManager() : m_factions(8)
{
    char* config = static_cast<App*>(WindowApp::m_instance)->InitResMap()->loadText(XString(L"res/common/configs/factions.xml"));
    TiXmlDocument document;
    document.Parse(config);
    if (config != NULL) {
        delete[] config;
    }
    for (TiXmlNode* node = document.FirstChild("faction"); node != NULL; node = node->NextSibling("faction")) {
        CFactionType* faction = new CFactionType(node);
        m_factions.Set(faction->m_name, faction);
    }
}

CFactionTypeManager::~CFactionTypeManager()
{
    m_factions.DeleteContents();
}

CFactionType* CFactionTypeManager::GetType(const XString& name) const
{
    // the original does not check the lookup result
    return *m_factions.Get(name);
}

CFactionType* CFactionTypeManager::GetRandomFaction() const
{
    // the original reads the entry directly (no range check on the random index)
    int count = m_factions.m_count;
    if (count > 0) {
        return *m_factions.GetAt(CRandGen::GetInstance()->GetRandRange(0, count - 1), NULL);
    }
    return NULL;
}

CFactionType* CFactionTypeManager::GetFactionAtIndex(int index) const
{
    if (index >= 0 && index < m_factions.m_count) {
        return *m_factions.GetAt(index, NULL);
    }
    return NULL;
}

void CFactionTypeManager::GetFactionTypeNames(Vector<XString*>& names)
{
    names.setSize(m_factions.m_count);
    for (int index = 0; index < m_factions.m_count; ++index) {
        m_factions.GetAt(index, &names[index]);
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertFactionManagerSize[sizeof(CFactionTypeManager) == 0x10 ? 1 : -1];
#endif
