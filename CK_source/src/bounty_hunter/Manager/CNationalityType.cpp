// CNationalityType.cpp - names of one nationality.
#include "CNationalityType.h"
#include "CXmlHelper.h"
#include "CRandGen.h"

CNationalityType::CNationalityType(TiXmlNode* node)
{
    Parse(node);
}

void CNationalityType::Parse(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    ParseName(m_firstNamesMan, "firstNameManData", node);
    ParseName(m_firstNamesWoman, "firstNameWomanData", node);
    ParseName(m_surnames, "surnameData", node);
}

XString CNationalityType::GetRandomFullName(bool male) const
{
    const XString& firstName = GetFirstName(male);
    int index = CRandGen::GetInstance()->GetRandRange(0, m_surnames.size() - 1);
    return firstName + XString(" ") + m_surnames[index];
}

const XString& CNationalityType::GetFirstName(bool male) const
{
    CRandGen* random = CRandGen::GetInstance();
    if (male) {
        return m_firstNamesMan[random->GetRandRange(0, m_firstNamesMan.size() - 1)];
    }
    return m_firstNamesWoman[random->GetRandRange(0, m_firstNamesWoman.size() - 1)];
}

void CNationalityType::ParseName(Vector<XString>& names, const char* tag, TiXmlNode* node)
{
    for (TiXmlNode* child = node->FirstChild(tag); child != NULL; child = child->NextSibling(tag)) {
        names.addElement(CXmlHelper::GetResString(child, "name"));
    }
}
