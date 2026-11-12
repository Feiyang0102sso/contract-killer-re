#ifndef CGAMEOBJECTMANAGER_H
#define CGAMEOBJECTMANAGER_H

#include "HashTable.h"
#include "xString.h"

class IGameObject;
class IGameObjectCreator;
class TiXmlNode;

class CGameObjectManager {
public:
    CGameObjectManager();
    ~CGameObjectManager();
    IGameObject* CreateObjectByName(const XString& name);
    void ParseConfig(const XString& resource, IGameObjectCreator* creator);

private:
    HashTable<XString, TiXmlNode*> m_objects;
    HashTable<XString, IGameObjectCreator*> m_creators;
};

#endif
