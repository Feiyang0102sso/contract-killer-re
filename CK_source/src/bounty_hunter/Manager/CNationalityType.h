// CNationalityType.h - one nationality of nationalities.xml: random first names (man / woman) and surnames.
// Layout (0x34) from CNationalityType::CNationalityType and GetFirstName.
#ifndef CNATIONALITYTYPE_H
#define CNATIONALITYTYPE_H

#include "xString.h"
#include "Vector_ss.h"

class TiXmlNode;

class CNationalityType {
public:
    CNationalityType(TiXmlNode* node);

    void Parse(TiXmlNode* node);
    /** "<first name> <surname>" chosen at random. */
    XString GetRandomFullName(bool male) const;
    const XString& GetFirstName(bool male) const;
    /** Append the localized "name" of every `tag` child of `node` to `names` (does not use this). */
    void ParseName(Vector<XString>& names, const char* tag, TiXmlNode* node);

    XString m_name;                         // 0x00
    Vector<XString> m_firstNamesMan;        // 0x04 firstNameManData
    Vector<XString> m_firstNamesWoman;      // 0x14 firstNameWomanData
    Vector<XString> m_surnames;             // 0x24 surnameData
};

#endif
