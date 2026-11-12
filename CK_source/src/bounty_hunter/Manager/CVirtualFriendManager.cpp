// CVirtualFriendManager.cpp - load the virtual friends from the save file.
#include "CVirtualFriendManager.h"
#include "App.h"
#include "CSaveManager.h"
#include "CXmlHelper.h"
#include "tinyxml.h"

CVirtualFriendManager::CVirtualFriendManager()
{
    Load();
}

void CVirtualFriendManager::Load()
{
    TiXmlElement* root = static_cast<App*>(WindowApp::m_instance)->m_saveManager->GetRootElement();
    TiXmlElement* list = root->FirstChildElement("virtual_friend_list");
    if (list == NULL) {
        return;
    }
    TiXmlElement* node = list->FirstChildElement("virtual_friend");
    if (node == NULL) {
        return;
    }
    do {
        SVirtualFriendInfo info;
        info.m_avatarId = XString(CXmlHelper::GetCharAttributeValue(node, "avatarId"));
        info.m_name = CXmlHelper::GetAttributeValue(node, "name");
        info.m_level = CXmlHelper::GetAttributeValue(node, "level").ToInt();
        m_friends.addElement(info);
        node = node->NextSiblingElement("virtual_friend");
    } while (node != NULL);
}
