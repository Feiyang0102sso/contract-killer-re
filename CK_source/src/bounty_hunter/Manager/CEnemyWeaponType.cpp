#include "CEnemyWeaponType.h"

#include "App.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "ResMap.h"
#include "SwerveHelper.h"

CEnemyWeaponType::CEnemyWeaponType(TiXmlNode* node)
    : m_baseDamage(0), m_effectiveRange(0), m_animType(PERSON_ANIM_TYPE_DEFAULT),
      m_modelResource(NULL), m_shootSound(NULL), m_modelCopies(0)
{
    Parse(node);
}

CEnemyWeaponType::~CEnemyWeaponType()
{
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
    m_animType = static_cast<EPersonAnimType>(CXmlHelper::GetAttributeValue(node, "characterAnimationType").ToInt());
    m_dummyName = CXmlHelper::GetAttributeValue(node, "dummy");
    m_effectiveRange = CXmlHelper::GetAttributeValue(node, "effectiveRange").ToInt();
}

void CEnemyWeaponType::ParseModel(TiXmlNode* node)
{
    if (node != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        m_modelResource = app->InitResMap()->getResourceID(CXmlHelper::GetAttributeValue(node, "m3g"));
    }
}

void CEnemyWeaponType::ParseSounds(TiXmlNode* node)
{
    if (node != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        m_shootSound = app->InitResMap()->getResourceID(CXmlHelper::GetAttributeValue(node, "shoot"));
    }
}

void CEnemyWeaponType::LoadModel() const
{
    if (m_modelResource == NULL) {
        return;
    }
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
    if (m_model == NULL) {
        return SwerveHelper::Group();
    }
    ++m_modelCopies;
    return SwerveHelper::FullObjectDuplicate(SwerveHelper::Group(m_model), 0);
}

void CEnemyWeaponType::ReleaseModelCopy() const
{
    if (m_modelCopies > 0 && --m_modelCopies == 0) {
        m_model = SwerveHelper::Object3D();
    }
}

typedef char AssertEnemyWeaponTypeSize[sizeof(CEnemyWeaponType) == 0x24 ? 1 : -1];
