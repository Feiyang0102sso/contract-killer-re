// CVisualCharacterTypeManager.cpp - visual_characters.xml and common animations.
#include "CVisualCharacterTypeManager.h"
#include "CVisualCharacterType.h"
#include "App.h"
#include "ResMap.h"
#include "SwerveHelper.h"
#include "tinyxml.h"

CVisualCharacterTypeManager::CVisualCharacterTypeManager()
    : m_types(8)
{
    char* text = ((App*)WindowApp::m_instance)->InitResMap()->loadText(XString(L"res/common/configs/visual_characters.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    TiXmlNode* common = document.FirstChild("commonSection");
    CVisualCharacterType::LoadCommonInfo(common);
    CVisualCharacterType::LoadCommonAnimation(common);
    for (TiXmlNode* node = document.FirstChild("visualCharacter"); node != NULL; node = node->NextSibling("visualCharacter")) {
        CVisualCharacterType* type = new CVisualCharacterType(node);
        m_types.Set(type->m_name, type);
    }
}

CVisualCharacterTypeManager::~CVisualCharacterTypeManager()
{
    for (int index = 0; index < m_types.m_count; index++) {
        CVisualCharacterType* type = *m_types.GetAt(index, NULL);
        if (type != NULL) {
            // Original 0x13fd26 calls this static cleanup once for every non-null entry.
            type->DestroyCommonAnimation();
        }
    }
    m_types.DeleteContents();
}

CVisualCharacterType* CVisualCharacterTypeManager::GetType(const XString& name) const
{
    return *m_types.Get(name);
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertVisualTypeManagerSize[sizeof(CVisualCharacterTypeManager) == 0x10 ? 1 : -1];
#endif

void CVisualCharacterTypeManager::CleanupResources() const
{
    for (int index = 0; index < m_types.m_count; index++) {
        (*m_types.GetAt(index, NULL))->CleanupResources();
    }
}
