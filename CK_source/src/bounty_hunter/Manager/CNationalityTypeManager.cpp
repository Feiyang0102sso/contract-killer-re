// CNationalityTypeManager.cpp - owns nationality records populated by the game.
#include "CNationalityTypeManager.h"

#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"

CNationalityTypeManager::CNationalityTypeManager()
    : m_types(8)
{
    // CK reads sibling nationality records from nationality.xml.
    App* app = static_cast<App*>(WindowApp::m_instance);
    char* text = app->InitResMap()->loadText(XString(L"res/common/configs/nationality.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    for (TiXmlNode* node = document.FirstChild("nationality"); node != NULL; node = node->NextSibling("nationality")) {
        CNationalityType* type = new CNationalityType(node);
        m_types.Set(type->m_name, type);
    }
}

CNationalityTypeManager::~CNationalityTypeManager()
{
    m_types.DeleteContents();
}

CNationalityType* CNationalityTypeManager::GetType(const XString& name) const
{
    CNationalityType* const* type = m_types.Get(name);
    if (type != NULL) {
        return *type;
    }
    return NULL;
}
