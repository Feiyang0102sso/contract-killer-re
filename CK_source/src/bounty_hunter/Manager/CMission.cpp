#include "CMission.h"

#include "App.h"
#include "CXmlHelper.h"
#include "ResMap.h"

CMission::CMission(TiXmlNode* node) : m_estimatedWeaponId(-1), m_estimatedWeaponAmmoCount(-1)
{
    Parse(node);
}

void CMission::Parse(TiXmlNode* node)
{
    ParseInfo(node->FirstChild("info"));
    ParseReward(node->FirstChild("reward"));
    ParseBoss(node->FirstChild("boss"));
    ParseFakeBoss(node->FirstChild("fakeBoss"));
    ParseDialogs(node, XString("startDialog"), &m_startDialogs);
    ParseDialogs(node, XString("priorDialog"), &m_priorDialogs);
    ParseDialogs(node, XString("inMissionDialog"), &m_inMissionDialogs);
    ParseDialogs(node, XString("finalDialog"), &m_finalDialogs);
}

void CMission::ParseInfo(TiXmlNode* node)
{
    m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    m_screenName = CXmlHelper::GetResString(node, "screenName");
    m_objectiveTitle = CXmlHelper::GetResString(node, "objectiveTitle");
    m_objectiveGoal = CXmlHelper::GetResString(node, "objectiveGoal");
    m_fakeBoss = CXmlHelper::GetAttributeValueOptional(node, "fakeBossName");
    m_minDifficulty = CXmlHelper::GetAttributeValue(node, "diff_randomizer_from").ToFloat();
    m_maxDifficulty = CXmlHelper::GetAttributeValue(node, "diff_randomizer_to").ToFloat();
    m_energyCost = CXmlHelper::GetAttributeValue(node, "energyReduce").ToInt();
    m_civiliansCount = CXmlHelper::GetAttributeValue(node, "civiliansCount").ToInt();
    m_alertSeconds = CXmlHelper::GetAttributeValue(node, "timeSecAlert").ToInt();
    m_escapeSeconds = CXmlHelper::GetAttributeValue(node, "timeSecRunAway").ToInt();
    m_warTimer = CXmlHelper::GetAttributeValue(node, "isWarTimerEnabled").ToBool();
    m_bossFirst = CXmlHelper::GetAttributeValue(node, "isBossKillingFirst").ToBool();
    m_bossWins = CXmlHelper::GetAttributeValue(node, "isBossKillingWin").ToBool();
    m_tranquillize = CXmlHelper::GetAttributeValue(node, "isTranquillize").ToBool();
    m_bossSpawnsLast = CXmlHelper::GetAttributeValue(node, "isBossSpawnWhenAllDead").ToBool();
    m_silentKill = CXmlHelper::GetAttributeValueOptional(node, "silentKill").ToBool();
    m_scriptName = CXmlHelper::GetAttributeValue(node, "scriptName");
    m_minWeaponRange = CXmlHelper::GetAttributeValue(node, "minimalWeaponRange").ToFloat();
    ParsePermittedWeapon(node);
    ParseEstimatedWeapon(node);
}

void CMission::ParseReward(TiXmlNode* node)
{
    m_moneyReward = CXmlHelper::GetAttributeValue(node, "money").ToInt();
    m_experienceReward = CXmlHelper::GetAttributeValue(node, "xp").ToInt();
    m_weaponGiftId = CXmlHelper::GetAttributeValue(node, "weaponGiftId").ToInt();
}

void CMission::ParseBoss(TiXmlNode* node)
{
    m_boss = CXmlHelper::GetAttributeValue(node, "name");
    m_bossName = CXmlHelper::GetResString(node, "screenName");
}

void CMission::ParseFakeBoss(TiXmlNode* node)
{
    if (node != NULL) {
        m_fakeBoss = CXmlHelper::GetAttributeValue(node, "name");
        m_fakeBossName = CXmlHelper::GetResString(node, "screenName");
    }
}

void CMission::ParseDialogs(TiXmlNode* node, XString elementName, Vector<Dialog>* dialogs)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    XString::AnsiString name(elementName.c_str());
    for (TiXmlNode* child = node->FirstChild(name); child != NULL; child = child->NextSibling(name)) {
        Dialog dialog;
        dialog.m_id = CXmlHelper::GetAttributeValue(child, "id").ToInt();
        XString image = CXmlHelper::GetAttributeValue(child, "image");
        dialog.m_image = app->InitResMap()->getResourceID(image);
        dialog.m_title = CXmlHelper::GetResString(child, "title");
        dialog.m_text = CXmlHelper::GetResString(child, "text");
        dialog.m_condition = -1;
        if (CXmlHelper::HasAttribute(child, "condition")) {
            dialog.m_condition = CXmlHelper::GetAttributeValue(child, "condition").ToInt();
        }
        dialogs->addElement(dialog);
    }
}

void CMission::ParsePermittedWeapon(TiXmlNode* node)
{
    if (node == NULL || !CXmlHelper::HasAttribute(node, "permittedWeapon")) {
        return;
    }
    Vector<XString> ids;
    CXmlHelper::GetAttributeValue(node, "permittedWeapon").Split(ids, L",");
    for (int index = 0; index < ids.size(); ++index) {
        XString value = ids[index].Trim(true, true);
        if (value.Length() != 0) {
            m_permittedWeapons.addElement(value.ToInt());
        }
    }
}

void CMission::ParseEstimatedWeapon(TiXmlNode* node)
{
    if (node != NULL && CXmlHelper::HasAttribute(node, "estimatedWeaponID")) {
        m_estimatedWeaponId = CXmlHelper::GetAttributeValue(node, "estimatedWeaponID").ToInt();
        m_estimatedWeaponAmmoCount = CXmlHelper::GetAttributeValue(node, "estimatedWeaponAmmoCount").ToInt();
    }
}

typedef char AssertMissionSize[sizeof(CMission) == 0xac ? 1 : -1];
