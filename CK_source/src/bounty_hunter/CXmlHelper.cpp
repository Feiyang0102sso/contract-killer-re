// CXmlHelper.cpp: original 122650..12299a, keeps the TinyXML virtual calls and the temporary string lifetimes.
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"
#include <stdlib.h>

bool CXmlHelper::HasAttribute(const TiXmlNode* node, const char* name)
{
    if (node == NULL || name == NULL) {
        return false;
    }
    // the original does not check the element
    return node->ToElement()->Attribute(name) != NULL;
}

const char* CXmlHelper::GetCharAttributeValueOptional(const TiXmlElement& element, const char* name)
{
    return element.Attribute(name);
}

XString CXmlHelper::GetAttributeValue(const TiXmlElement& element, const char* name)
{
    return XString(element.Attribute(name)).Replace(L"\\n", L"\n", true);
}

XString CXmlHelper::GetAttributeValueOptional(const TiXmlElement& element, const char* name)
{
    const char* value = element.Attribute(name);
    if (value != NULL) {
        return XString(value).Replace(L"\\n", L"\n", true);
    }
    return XString();
}

XString CXmlHelper::GetAttributeValueOrUseDefault(const TiXmlElement& element, const char* name,
                                                 const XString& defaultValue)
{
    const char* value = element.Attribute(name);
    if (value != NULL) {
        return XString(value).Replace(L"\\n", L"\n", true);
    }
    return defaultValue;
}

const char* CXmlHelper::GetCharAttributeValue(const TiXmlNode* node, const char* name)
{
    return node->ToElement()->Attribute(name);
}

XString CXmlHelper::GetResString(const TiXmlNode* node, const char* name)
{
    return GetResString(*node->ToElement(), name);
}

XString CXmlHelper::GetResString(const TiXmlElement& element, const char* name)
{
    XString id = GetAttributeValueOrUseDefault(element, name, XString(""));
    if (id.ByteSize() < sizeof(wchar_t)) {
        return id;
    }
    XString::AnsiString key(id.c_str());
    return Window::ResString(key);
}

XString CXmlHelper::GetAttributeValue(const TiXmlNode* node, const char* name)
{
    return XString(node->ToElement()->Attribute(name)).Replace(L"\\n", L"\n", true);
}

XString CXmlHelper::GetAttributeValueOptional(const TiXmlNode* node, const char* name)
{
    const char* value = node->ToElement()->Attribute(name);
    if (value == NULL) {
        return XString();
    }
    return XString(value).Replace(L"\\n", L"\n", true);
}

XString CXmlHelper::GetAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, const char* defaultValue)
{
    return GetAttributeValueOrUseDefault(node, name, XString(defaultValue));
}

XString CXmlHelper::GetAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, XString defaultValue)
{
    const char* value = node->ToElement()->Attribute(name);
    if (value == NULL) {
        return defaultValue;
    }
    return XString(value).Replace(L"\\n", L"\n", true);
}

int CXmlHelper::GetIntAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, int defaultValue)
{
    if (node == NULL) {
        return defaultValue;
    }
    const char* value = node->ToElement()->Attribute(name);
    if (value == NULL) {
        return defaultValue;
    }
    return XString(value).Replace(L"\\n", L"\n", true).ToInt();
}

void CXmlHelper::WriteHashtable(const HashTable<HashTableKey<int>, int>& table, TiXmlElement* parent, const XString& name)
{
    // the original builds a fresh section and inserts a copy of it (callers clear the parent first)
    TiXmlElement* section = new TiXmlElement(XString::AnsiString(name.c_str()));
    for (int index = 0; index < table.m_count; ++index) {
        HashTableKey<int>* key = NULL;
        int value = *table.GetAt(index, &key);
        TiXmlElement* entry = new TiXmlElement("element");
        entry->SetAttribute("key", key->m_value);
        entry->SetAttribute("value", value);
        section->InsertEndChild(*entry);
        delete entry;
    }
    parent->InsertEndChild(*section);
    delete section;
}

void CXmlHelper::WriteHashtable(const HashTable<XString, int>& table, TiXmlElement* parent, const XString& name)
{
    TiXmlElement* section = new TiXmlElement(XString::AnsiString(name.c_str()));
    for (int index = 0; index < table.m_count; ++index) {
        XString* key = NULL;
        int value = *table.GetAt(index, &key);
        TiXmlElement* entry = new TiXmlElement("element");
        entry->SetAttribute("key", XString::AnsiString(key->c_str()));
        entry->SetAttribute("value", value);
        section->InsertEndChild(*entry);
        delete entry;
    }
    parent->InsertEndChild(*section);
    delete section;
}

void CXmlHelper::ReadHashtable(HashTable<HashTableKey<int>, int>& table, TiXmlElement* parent, const XString& name)
{
    // the original looks the section up inline and always calls Add
    TiXmlElement* section = parent->FirstChildElement(XString::AnsiString(name.c_str()));
    if (section == NULL) {
        return;
    }
    TiXmlElement* entry = section->FirstChildElement("element");
    while (entry != NULL) {
        int key = GetAttributeValue(entry, "key").ToInt();
        int value = GetAttributeValue(entry, "value").ToInt();
        table.Add(HashTableKey<int>(key), value);
        entry = entry->NextSiblingElement("element");
    }
}

void CXmlHelper::ReadHashtable(HashTable<XString, int>& table, TiXmlElement* parent, const XString& name)
{
    TiXmlElement* section = parent->FirstChildElement(XString::AnsiString(name.c_str()));
    if (section == NULL) {
        return;
    }
    TiXmlElement* entry = section->FirstChildElement("element");
    while (entry != NULL) {
        // the original releases the key only after stepping to the next sibling
        XString key = GetAttributeValue(entry, "key");
        int value = GetAttributeValue(entry, "value").ToInt();
        table.Add(key, value);
        entry = entry->NextSiblingElement("element");
    }
}

bool CXmlHelper::GetBoolAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, bool defaultValue)
{
    if (node == NULL) {
        return defaultValue;
    }
    const char* value = node->ToElement()->Attribute(name);
    if (value == NULL) {
        return defaultValue;
    }
    return XString(value).Replace(L"\\n", L"\n", true).ToBool();
}

TiXmlElement* CXmlHelper::ObtainElement(TiXmlElement* parent, const XString& name)
{
    TiXmlNode* found;
    {
        XString::AnsiString ansi(name.c_str());
        found = parent->FirstChild(ansi);
    }
    if (found == NULL) {
        // The temporary Ansi is destroyed first at the end of the construction statement; element is destroyed only after it is cloned.
        TiXmlElement element((const char*)XString::AnsiString(name.c_str()));
        return parent->InsertEndChild(element)->ToElement();
    }
    // The original still repeats the lookup on a hit; do not cache found in place of the second call.
    XString::AnsiString ansi(name.c_str());
    return parent->FirstChild(ansi)->ToElement();
}

TiXmlElement* CXmlHelper::GetElement(TiXmlElement* parent, const XString& name)
{
    XString::AnsiString ansi(name.c_str());
    return parent->FirstChildElement(ansi);
}

TiXmlDocument CXmlHelper::Parse(const wchar_t* resourceName)
{
    char* text = static_cast<App*>(WindowApp::m_instance)->InitResMap()->loadText(XString(resourceName));
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    return document;
}
