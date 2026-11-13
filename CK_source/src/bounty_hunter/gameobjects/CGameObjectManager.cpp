#include "CGameObjectManager.h"

#include "App.h"
#include "CExplosiveObject.h"
#include "CXmlHelper.h"
#include "ResMap.h"

class IGameObjectCreator {
public:
    virtual IGameObjectCreator* Clone() = 0;
    virtual IGameObject* Create(const TiXmlNode* config) = 0;
};

class CExplosiveObjectCreator : public IGameObjectCreator {
public:
    virtual IGameObjectCreator* Clone() { return new CExplosiveObjectCreator(); }
    virtual IGameObject* Create(const TiXmlNode* config) { return new CExplosiveObject(config); }
};

template <>
void HashTable<XString, IGameObjectCreator*>::DeleteContents()
{
    for (int index = 0; index < static_cast<int>(m_bucketCount); ++index) {
        Entry* entry = m_buckets[index];
        m_buckets[index] = NULL;
        while (entry != NULL) {
            Entry* next = entry->m_next;
            // The creator ABI has only Clone and Create. This table owns explosive creators.
            delete static_cast<CExplosiveObjectCreator*>(entry->m_value);
            delete entry;
            entry = next;
        }
    }
    m_count = 0;
}

IGameObject* CGameObjectManager::CreateObjectByName(const XString& name)
{
    // the original does not check either lookup
    TiXmlNode* config = *m_objects.Get(name);
    return (*m_creators.Get(name))->Create(config);
}

CGameObjectManager::CGameObjectManager() : m_objects(8), m_creators(8)
{
    ParseConfig(XString("res/common/configs/explosive_objects.xml"), new CExplosiveObjectCreator());
}

void CGameObjectManager::ParseConfig(const XString& resource, IGameObjectCreator* creator)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* xml = app->InitResMap()->loadText(XString(resource.c_str()));
    TiXmlDocument document;
    document.Parse(xml);
    if (xml != NULL) {
        delete[] xml;
    }

    const TiXmlNode* root = document.FirstChild("objects");
    const TiXmlNode* node = root->FirstChild("object");
    while (node != NULL) {
        XString name = CXmlHelper::GetAttributeValue(node, "name");
        m_objects.Add(name, node->Clone());
        m_creators.Add(name, creator->Clone());
        node = node->NextSibling("object");
    }
    delete static_cast<CExplosiveObjectCreator*>(creator);
}

CGameObjectManager::~CGameObjectManager()
{
    m_objects.DeleteContents();
    m_creators.DeleteContents();
}

typedef char AssertGameObjectManagerSize[sizeof(CGameObjectManager) == 0x18 ? 1 : -1];
