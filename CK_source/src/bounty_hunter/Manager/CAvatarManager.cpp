// CAvatarManager.cpp - loads avatars.xml.
#include "CAvatarManager.h"
#include "CAvatar.h"
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"

CAvatarManager::CAvatarManager()
{
    char* text = ((App*)WindowApp::m_instance)->InitResMap()->loadText(XString(L"res/common/configs/avatars.xml"));
    TiXmlDocument document;
    document.Parse(text);
    if (text != NULL) {
        delete[] text;
    }
    for (TiXmlNode* node = document.FirstChild("avatar"); node != NULL; node = node->NextSibling("avatar")) {
        CAvatar* avatar = new CAvatar(node);
        m_avatars.addElement(avatar);
    }
}

CAvatar* CAvatarManager::GetAvatar(int id)
{
    for (int index = 0; index < m_avatars.size(); index++) {
        // The original indexes with `id`, not with the loop counter.
        if (m_avatars[id]->m_id == id) {
            return m_avatars[id];
        }
    }
    return NULL;
}

bool CAvatarManager::IsNewAvatarUnlocked(int level)
{
    for (int index = 0; index < m_avatars.size(); index++) {
        if (m_avatars[index]->m_unlockLvl == level) {
            return true;
        }
    }
    return false;
}

CAvatarManager::~CAvatarManager()
{
    SAFE_DELETE_VECTOR(m_avatars);
}
