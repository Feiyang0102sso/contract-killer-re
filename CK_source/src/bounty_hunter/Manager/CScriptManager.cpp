#include "CScriptManager.h"

#include "App.h"
#include "CScriptType.h"
#include "ResMap.h"
#include "tinyxml.h"

CScriptManager::CScriptManager() : m_scripts(8)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* text = app->InitResMap()->loadText(XString(L"res/common/configs/scripts.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    for (TiXmlNode* node = document.FirstChild("script"); node != NULL; node = node->NextSibling("script")) {
        CScriptType* script = new CScriptType(node);
        m_scripts.Set(script->m_name, script);
    }
}

CScriptManager::~CScriptManager()
{
    m_scripts.DeleteContents();
}

CScriptType* CScriptManager::GetType(const XString& name) const
{
    // The original dereferences the lookup result without a NULL check.
    CScriptType* const* script = m_scripts.Get(name);
    return *script;
}

typedef char AssertScriptManagerSize[sizeof(CScriptManager) == 0x10 ? 1 : -1];
