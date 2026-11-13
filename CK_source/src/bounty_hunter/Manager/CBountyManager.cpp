#include "CBountyManager.h"

#include "App.h"
#include "CRandGen.h"
#include "CScriptManager.h"
#include "CScriptType.h"
#include "CXmlHelper.h"
#include "ResMap.h"

CBountyManager::CBountyManager()
{
    ParseBountyConfig();
    ParseTargettingConfig();
}

void CBountyManager::ParseBountyConfig()
{
    char* text = static_cast<App*>(WindowApp::m_instance)->InitResMap()->loadText(XString(L"res/common/configs/bounty.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    for (TiXmlNode* node = document.FirstChild("bountyTypes")->FirstChild("bountyType"); node != NULL;
         node = node->NextSibling("bountyType")) {
        ParseBounty(node);
    }
}

void CBountyManager::ParseTargettingConfig()
{
    char* text = static_cast<App*>(WindowApp::m_instance)->InitResMap()->loadText(XString(L"res/common/configs/target_missions.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    for (TiXmlNode* node = document.FirstChild("mission_texts")->FirstChild("text"); node != NULL;
         node = node->NextSibling("text")) {
        XString title = CXmlHelper::GetResString(node, "title");
        XString message = CXmlHelper::GetResString(node, "message");
        SMessage* targeting = new SMessage(XString(""), title, message);
        m_targetingMessages.addElement(targeting);
    }
}

CBountyManager::~CBountyManager()
{
    SAFE_DELETE_VECTOR(m_targetingMessages);
}

CBountyManager::SBountyGeneralInfo* CBountyManager::GetInfo(int id)
{
    for (int index = 0; index < m_types.size(); ++index) {
        if (m_types[index].m_id == id) {
            return &m_types[index];
        }
    }
    // Unknown type falls back to the first entry (no empty check in the original).
    return &m_types[0];
}

CBountyManager::SMessage* CBountyManager::GetRandomMessageWithSeed(int id, const XString& haunt, unsigned int seed,
                                                                 EBoutnyMessageType type)
{
    SBountyGeneralInfo* info = GetInfo(id);
    int hauntCount = info->m_haunts.size();
    Vector<SMessage*> messages;
    switch (type) {
    case BountyMessageDefault:
        for (int index = 0; index < hauntCount; ++index) {
            if (haunt == info->m_haunts[index].m_name) {
                HauntInfo& entry = info->m_haunts[index];
                int messageCount = entry.m_messages.size();
                for (int message = 0; message < messageCount; ++message) {
                    messages.addElement(&entry.m_messages[message]);
                }
            }
        }
        for (int index = 0; index < info->m_messages.size(); ++index) {
            messages.addElement(&info->m_messages[index]);
        }
        break;
    case BountyMessageWeaponGated:
        for (int index = 0; index < info->m_weaponGatedMessages.size(); ++index) {
            messages.addElement(&info->m_weaponGatedMessages[index]);
        }
        break;
    case BountyMessageMoving:
        for (int index = 0; index < info->m_movingMessages.size(); ++index) {
            messages.addElement(&info->m_movingMessages[index]);
        }
        break;
    case BountyMessageRandomWeapon:
        for (int index = 0; index < info->m_randomWeaponMessages.size(); ++index) {
            messages.addElement(&info->m_randomWeaponMessages[index]);
        }
        break;
    }
    SMessage* result;
    if (messages.size() <= 0) {
        // Leaked placeholder message, as in the original.
        result = new SMessage(XString("error"), XString("error"), XString("error"));
    } else {
        result = messages[seed % messages.size()];
    }
    return result;
}

XString CBountyManager::GetRandomScriptWithSeed(const SBountyGeneralInfo& info, int, const XString& location,
                                               int camera, int playerLevel, unsigned int seed)
{
    int scriptCount = info.m_scripts.size();
    Vector<XString*> scripts;
    int threshold = (int)(info.m_noScriptProbability * 100.0f);
    if (CRandGen::GetInstance()->GetRand(100) < threshold) {
        return XString("");
    }
    for (int index = 0; index < scriptCount; ++index) {
        CScriptType* script = static_cast<App*>(WindowApp::m_instance)->m_scriptManager->GetType(info.m_scripts[index]);
        if (script->m_location == location && script->m_cameraId == camera && script->m_minLevel <= playerLevel
            && script->m_maxLevel >= playerLevel) {
            scripts.addElement(const_cast<XString*>(&info.m_scripts[index]));
        }
    }
    if (scripts.size() == 0) {
        return XString("");
    }
    return *scripts[seed % scripts.size()];
}

void CBountyManager::ParseBounty(TiXmlNode* node)
{
    SBountyGeneralInfo info;
    info.m_id = CXmlHelper::GetAttributeValue(node, "typeId").ToInt();
    info.m_usedForAmbush = CXmlHelper::GetAttributeValue(node, "usedForAmbush").ToBool();
    ParseInfo(node->FirstChild("info"), info);
    ParseReward(node->FirstChild("baseReward"), info);
    ParsePunishment(node->FirstChild("punishment"), info);
    ParseRefuse(node->FirstChild("refuse"), info);
    ParsePossibleScripts(node->FirstChild("scripts"), info);
    ParseHaunts(node, info);
    ParseMessagesImpl(node, "message", info.m_messages);
    ParseMessagesImpl(node, "weaponGatedMessage", info.m_weaponGatedMessages);
    ParseMessagesImpl(node, "movingCameraMessage", info.m_movingMessages);
    info.m_randomWeaponMessages.addElement(SMessage(Window::ResString("IDS_RANDOM_WEAPON_GOAL"),
                                                    Window::ResString("IDS_RANDOM_WEAPON_TITLE"),
                                                    Window::ResString("IDS_RANDOM_WEAPON_TEXT")));
    m_types.addElement(info);
}

void CBountyManager::ParseInfo(TiXmlNode* node, SBountyGeneralInfo& info)
{
    info.m_difficultyMin = CXmlHelper::GetAttributeValue(node, "diff_randomizer_from").ToFloat();
    info.m_difficultyMax = CXmlHelper::GetAttributeValue(node, "diff_randomizer_to").ToFloat();
    info.m_name = CXmlHelper::GetAttributeValue(node, "name");
    info.m_energy = CXmlHelper::GetAttributeValue(node, "baseEnergyReduce").ToInt();
    info.m_alertSeconds = CXmlHelper::GetAttributeValue(node, "baseTimeSecAlert").ToInt();
    info.m_escapeSeconds = CXmlHelper::GetAttributeValue(node, "baseTimeSecRunAway").ToInt();
    info.m_bossFirst = CXmlHelper::GetAttributeValue(node, "isBossKillingFirst").ToBool();
    info.m_bossWins = CXmlHelper::GetAttributeValue(node, "isBossKillingWin").ToBool();
    info.m_tranquillize = CXmlHelper::GetAttributeValue(node, "isTranquillize").ToBool();
    info.m_bossSpawnsLast = CXmlHelper::GetAttributeValue(node, "isBossSpawnWhenAllDead").ToBool();
    info.m_silentKill = CXmlHelper::GetAttributeValueOptional(node, "silentKill").ToBool();
    info.m_minWeaponRange = CXmlHelper::GetAttributeValue(node, "minimalWeaponRange").ToFloat();
}

void CBountyManager::ParseReward(TiXmlNode* node, SBountyGeneralInfo& info)
{
    info.m_money = CXmlHelper::GetAttributeValue(node, "money").ToInt();
    info.m_xp = CXmlHelper::GetAttributeValue(node, "xp").ToInt();
}

void CBountyManager::ParsePunishment(TiXmlNode* node, SBountyGeneralInfo& info)
{
    info.m_moneyPenalty = CXmlHelper::GetAttributeValue(node, "money").ToInt();
    info.m_xpPenalty = CXmlHelper::GetAttributeValue(node, "xp").ToInt();
}

void CBountyManager::ParseRefuse(TiXmlNode* node, SBountyGeneralInfo& info)
{
    if (node == NULL) {
        info.m_moneyRefusal = 0;
        info.m_xpRefusal = 0;
        return;
    }
    info.m_moneyRefusal = CXmlHelper::GetAttributeValue(node, "money").ToInt();
    info.m_xpRefusal = CXmlHelper::GetAttributeValue(node, "xp").ToInt();
}

void CBountyManager::ParsePossibleScripts(TiXmlNode* node, SBountyGeneralInfo& info)
{
    if (node == NULL) {
        return;
    }
    info.m_noScriptProbability = CXmlHelper::GetAttributeValueOptional(node, "noScriptProb").ToFloat();
    for (TiXmlNode* script = node->FirstChild("script"); script != NULL; script = script->NextSibling("script")) {
        info.m_scripts.addElement(CXmlHelper::GetAttributeValue(script, "name"));
    }
}

void CBountyManager::ParseHaunts(TiXmlNode* node, SBountyGeneralInfo& info)
{
    for (TiXmlNode* child = node->FirstChild("haunt"); child != NULL; child = child->NextSibling("haunt")) {
        HauntInfo haunt;
        haunt.m_name = CXmlHelper::GetAttributeValue(child, "name");
        for (TiXmlNode* message = child->FirstChild("message"); message != NULL; message = message->NextSibling("message")) {
            XString text = CXmlHelper::GetAttributeValue(message, "text");
            XString title = CXmlHelper::GetAttributeValue(message, "title");
            XString goal = CXmlHelper::GetAttributeValue(message, "goal");
            haunt.m_messages.addElement(SMessage(goal, title, text));
        }
        info.m_haunts.addElement(haunt);
    }
}

void CBountyManager::ParseMessagesImpl(TiXmlNode* node, const char* name, Vector<SMessage>& messages)
{
    if (node == NULL || name == NULL) {
        return;
    }
    for (TiXmlNode* child = node->FirstChild(name); child != NULL; child = child->NextSibling(name)) {
        XString text = CXmlHelper::GetResString(child, "text");
        XString title = CXmlHelper::GetResString(child, "title");
        XString goal = CXmlHelper::GetResString(child, "goal");
        messages.addElement(SMessage(goal, title, text));
    }
}

int CBountyManager::GetRandomAmbushTypeId()
{
    Vector<SBountyGeneralInfo*> candidates;
    for (int index = 0; index < m_types.size(); ++index) {
        if (m_types[index].m_usedForAmbush) {
            candidates.addElement(&m_types[index]);
        }
    }
    if (candidates.size() > 0) {
        return candidates[CRandGen::GetInstance()->GetRand(candidates.size())]->m_id;
    }
    return 0;
}

CBountyManager::SMessage* CBountyManager::GetRandomTargettingMessage()
{
    return m_targetingMessages[CRandGen::GetInstance()->GetRandRange(0, m_targetingMessages.size() - 1)];
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertBountyManagerSize[sizeof(CBountyManager) == 0x20 ? 1 : -1];
#endif
#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertBountyInfoSize[sizeof(CBountyManager::SBountyGeneralInfo) == 0xa8 ? 1 : -1];
#endif
