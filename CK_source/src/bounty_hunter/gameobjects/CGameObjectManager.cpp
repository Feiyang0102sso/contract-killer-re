#include "CGameObjectManager.h"

#include "CExplosiveObject.h"
#include "CXmlHelper.h"

class IGameObjectCreator {
public:
    virtual IGameObjectCreator* Clone() = 0;
    virtual IGameObject* Create(const TiXmlNode* config) = 0;
    virtual ~IGameObjectCreator() {}
};

class CExplosiveObjectCreator : public IGameObjectCreator {
public:
    virtual IGameObjectCreator* Clone() { return new CExplosiveObjectCreator(); }
    virtual IGameObject* Create(const TiXmlNode* config) { return new CExplosiveObject(config); }
};

IGameObject* CGameObjectManager::CreateObjectByName(const XString& name)
{
    TiXmlNode* const* config = m_objects.Get(name);
    IGameObjectCreator* const* creator = m_creators.Get(name);
    if (config == NULL || creator == NULL) {
        return NULL;
    }
    return (*creator)->Create(*config);
}

CGameObjectManager::CGameObjectManager() : m_objects(8), m_creators(8)
{
    CExplosiveObjectCreator creator;
    ParseConfig(XString(L"res/common/configs/explosive_objects.xml"), &creator);
}

void CGameObjectManager::ParseConfig(const XString& resource, IGameObjectCreator* creator)
{
    const TiXmlDocument document = CXmlHelper::Parse(resource.c_str());
    const TiXmlNode* root = document.FirstChild("objects");
    if (root != NULL) {
        const TiXmlNode* node = root->FirstChild("object");
        while (node != NULL) {
            XString name = CXmlHelper::GetAttributeValue(node, "name");
            m_objects.Add(name, node->Clone());
            m_creators.Add(name, creator->Clone());
            node = node->NextSibling("object");
        }
    }
}

CGameObjectManager::~CGameObjectManager()
{
    m_objects.DeleteContents();
    m_creators.DeleteContents();
}

typedef char AssertGameObjectManagerSize[sizeof(CGameObjectManager) == 0x18 ? 1 : -1];
