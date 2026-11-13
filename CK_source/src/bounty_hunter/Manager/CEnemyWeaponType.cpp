#include "CEnemyWeaponType.h"

#include "App.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "ResMap.h"
#include "SwerveHelper.h"

CEnemyWeaponType::CEnemyWeaponType(TiXmlNode* node)
    : m_baseDamage(0), m_animType(PERSON_ANIM_TYPE_DEFAULT), m_modelCopies(0)
{
    m_model.Clear();
    Parse(node);
}

CEnemyWeaponType::~CEnemyWeaponType()
{
    m_model.Clear();
}

void CEnemyWeaponType::Parse(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    ParseInfo(node->FirstChild("info"));
    ParseModel(node->FirstChild("model"));
    ParseSounds(node->FirstChild("sounds"));
}

void CEnemyWeaponType::ParseInfo(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    m_baseDamage = CXmlHelper::GetAttributeValue(node, "base_damage").ToInt();
    int animationType = CXmlHelper::GetAttributeValue(node, "characterAnimationType").ToInt();
    m_animType = static_cast<EPersonAnimType>(animationType);
    m_dummyName = CXmlHelper::GetAttributeValue(node, "dummy");
    m_effectiveRange = CXmlHelper::GetAttributeValue(node, "effectiveRange").ToInt();
}

void CEnemyWeaponType::ParseModel(TiXmlNode* node)
{
    if (node != NULL) {
        // the original reads the attribute before asking for the resource map
        XString value = CXmlHelper::GetAttributeValue(node, "m3g");
        m_modelResource = static_cast<App*>(WindowApp::m_instance)->InitResMap()->getResourceID(value);
    }
}

void CEnemyWeaponType::ParseSounds(TiXmlNode* node)
{
    if (node != NULL) {
        // the original reads the attribute before asking for the resource map
        XString value = CXmlHelper::GetAttributeValue(node, "shoot");
        m_shootSound = static_cast<App*>(WindowApp::m_instance)->InitResMap()->getResourceID(value);
    }
}

void CEnemyWeaponType::LoadModel() const
{
    // the original does not check the resource name
    SwerveHelper::Object3D model = App::LoadObject3D(m_modelResource);
    if (model != NULL) {
        XString::AnsiString dummyName(m_dummyName.c_str());
        m_model = model.find(DGHelper::getSwerveID(dummyName));
    }
}

SwerveHelper::Group CEnemyWeaponType::GetModelCopy() const
{
    if (m_model == NULL) {
        LoadModel();
    }
    // the original does not check the model after loading
    ++m_modelCopies;
    return SwerveHelper::FullObjectDuplicate(SwerveHelper::Group(m_model), 0);
}

void CEnemyWeaponType::ReleaseModelCopy() const
{
    // the original does not test for a positive count first
    if (--m_modelCopies == 0) {
        m_model.Clear();
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertEnemyWeaponTypeSize[sizeof(CEnemyWeaponType) == 0x24 ? 1 : -1];
#endif
