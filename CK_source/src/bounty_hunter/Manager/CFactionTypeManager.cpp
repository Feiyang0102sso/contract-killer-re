#include "CFactionTypeManager.h"

#include "CFactionType.h"
#include "CRandGen.h"
#include "CXmlHelper.h"

CFactionTypeManager::CFactionTypeManager() : m_factions(8)
{
    TiXmlDocument document = CXmlHelper::Parse(L"res/common/configs/factions.xml");
    for (TiXmlNode* node = document.FirstChild("faction"); node != NULL; node = node->NextSibling("faction")) {
        CFactionType* faction = new CFactionType(node);
        CFactionType** previous = m_factions.Get(faction->m_name);
        if (previous != NULL) {
            delete *previous;
        }
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
    for (int index = 0; index < names.size(); ++index) {
        m_factions.GetAt(index, &names[index]);
    }
}

typedef char AssertFactionManagerSize[sizeof(CFactionTypeManager) == 0x10 ? 1 : -1];
