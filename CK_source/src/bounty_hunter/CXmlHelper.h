// PROVISIONAL (created by b077): owner src/bounty_hunter/CXmlHelper.cpp not written yet
// CXmlHelper.h - attribute / element helpers over TinyXML used by the game configs.
// Signatures from the symbol names; return types from the callers (sret XString, element pointers).
// b074i: full recovery of the 9 ranges; the PROVISIONAL above is kept as history.
#ifndef CXMLHELPER_H
#define CXMLHELPER_H

#include "xString.h"
#include "tinyxml.h"
#include "HashTable.h"

class CXmlHelper {
public:
    /** Attribute `name` of `node` (empty when missing). */
    static XString GetAttributeValue(const TiXmlNode* node, const char* name);
    /** Attribute `name` of `node`, empty without complaining when missing. */
    static XString GetAttributeValueOptional(const TiXmlNode* node, const char* name);
    static XString GetAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, const char* defaultValue);
    static XString GetAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, XString defaultValue);
    static int GetIntAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, int defaultValue);
    static bool GetBoolAttributeValueOrUseDefault(const TiXmlNode* node, const char* name, bool defaultValue);
    /** Child element `name` of `parent`, created when missing. */
    static TiXmlElement* ObtainElement(TiXmlElement* parent, const XString& name);
    /** Child element `name` of `parent` (NULL when missing). */
    static TiXmlElement* GetElement(TiXmlElement* parent, const XString& name);
    /** Original 122930 returns the full document in place via the hidden r0; the resource buffer is released after parsing. */
    static TiXmlDocument Parse(const wchar_t* resourceName);
    // CK overloads from the original symbol list (0x1078f0..0x107fcc); used by the Manager units.  // added by game1
    static bool HasAttribute(const TiXmlNode* node, const char* name);  // added by game1
    static const char* GetCharAttributeValueOptional(const TiXmlElement& element, const char* name);  // added by game1
    static XString GetAttributeValue(const TiXmlElement& element, const char* name);  // added by game1
    static XString GetAttributeValueOptional(const TiXmlElement& element, const char* name);  // added by game1
    static XString GetAttributeValueOrUseDefault(const TiXmlElement& element, const char* name, const XString& defaultValue);  // added by game1
    static const char* GetCharAttributeValue(const TiXmlNode* node, const char* name);  // added by game1
    /** Localized string of the resource named by attribute `name`. */  // added by game1
    static XString GetResString(const TiXmlNode* node, const char* name);  // added by game1
    static XString GetResString(const TiXmlElement& element, const char* name);  // added by game1
    static void WriteHashtable(const HashTable<HashTableKey<int>, int>& table, TiXmlElement* parent, const XString& name);
    static void WriteHashtable(const HashTable<XString, int>& table, TiXmlElement* parent, const XString& name);
    static void ReadHashtable(HashTable<HashTableKey<int>, int>& table, TiXmlElement* parent, const XString& name);
    static void ReadHashtable(HashTable<XString, int>& table, TiXmlElement* parent, const XString& name);
};

#endif
