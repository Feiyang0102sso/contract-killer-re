#include "CAchievementManager.h"

#include "App.h"
#include "CBH_Player.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CPopupController.h"
#include "CStdUtil_Cocoa.h"
#include "CTutorialManager.h"
#include "CXmlHelper.h"
#include "GameCenter_Facade.h"
#include "ResMap.h"

typedef char CKAchievementSize[sizeof(CAchievement) == 0x24 ? 1 : -1];
typedef char CKAchievementManagerSize[sizeof(CAchievementManager) == 0x28 ? 1 : -1];

CAchievementManager::CAchievementManager()
    : m_lastCheckTime(0), m_lastPostingTime(0), m_weaponAvailabilityStamp(0), m_achievements(8)
{
    char* text = static_cast<App*>(WindowApp::m_instance)->InitResMap()->loadText(XString(L"res/common/configs/achievements.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    ParseAchievements(document.FirstChild("achievements"));
}

CAchievementManager::~CAchievementManager()
{
    m_achievements.DeleteContents();
}

void CAchievementManager::ParseAchievements(TiXmlNode* node)
{
    for (TiXmlNode* child = node->FirstChild("achievement"); child != NULL; child = child->NextSibling("achievement")) {
        CAchievement* achievement = new CAchievement();
        achievement->m_name = CXmlHelper::GetAttributeValue(child, "name");
        achievement->m_value = CXmlHelper::GetAttributeValue(child, "value").ToInt();
        achievement->m_steps = CXmlHelper::GetAttributeValue(child, "steps").ToInt();
        achievement->m_imageId = CXmlHelper::GetAttributeValue(child, "imageId");
        achievement->m_screenName = CXmlHelper::GetResString(child, "screenName");
        achievement->m_description = CXmlHelper::GetResString(child, "description");
        achievement->m_completedDescription = CXmlHelper::GetResString(child, "completedDescription");
        achievement->m_hidden = CXmlHelper::GetAttributeValue(child, "hidden").ToBool();
        m_achievements.Set(achievement->m_name, achievement);
        m_names.addElement(achievement->m_name);
    }
}

void CAchievementManager::Save(TiXmlElement* root)
{
    TiXmlElement* list = CXmlHelper::ObtainElement(root, XString("achievements"));
    list->Clear();
    for (int index = 0; index < m_achievements.m_count; ++index) {
        CAchievement* achievement = *m_achievements.GetAt(index, NULL);
        TiXmlElement* element = new TiXmlElement("achievement");
        element->SetStringAttribute("name", XString::AnsiString(achievement->m_name.c_str()));
        element->SetAttribute("progress", achievement->m_progress);
        element->SetAttribute("synced", achievement->m_synced);
        list->InsertEndChild(*element);
        delete element;
    }
}

void CAchievementManager::Load(TiXmlElement* root)
{
    TiXmlElement* list = root->FirstChildElement("achievements");
    if (list == NULL) {
        return;
    }
    TiXmlElement* element = list->FirstChildElement("achievement");
    while (element != NULL) {
        XString name = CXmlHelper::GetAttributeValue(element, "name");
        int progress = CXmlHelper::GetAttributeValue(element, "progress").ToInt();
        bool synced = CXmlHelper::GetAttributeValue(element, "synced").ToBool();
        SetAchievementProgress(name, progress, synced);
        element = element->NextSiblingElement("achievement");
    }
}

void CAchievementManager::SetAchievementProgress(XString name, int progress, bool synced)
{
    CAchievement* missing = NULL;
    CAchievement** found = m_achievements.Get(name);
    // Select form, as in the original (it ne; movne).
    CAchievement* achievement = *(found != NULL ? found : &missing);
    if (achievement != NULL) {
        achievement->m_progress = progress;
        achievement->m_synced = synced;
    }
}

void CAchievementManager::CheckAllAchievements()
{
    CPlayerGameStats& stats = CBH_Player::GetInstance()->m_gameStats;
    CBH_Player* player = CBH_Player::GetInstance();
    CheckAchievement("BOUNTY_ACHIEVE_FINISH_TUTORIAL", !static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial());
    CheckAchievement("BOUNTY_ACHIEVE_KILL_3_BOSSES", stats.m_targetsKilled);
    CheckAchievement("BOUNTY_ACHIEVE_KILL_50_BOSSES", stats.m_targetsKilled);
    CheckAchievement("BOUNTY_ACHIEVE_KILL_100_BOSSES", stats.m_targetsKilled);
    CheckAchievement("BOUNTY_ACHIEVE_KILL_1_BOSSES", stats.m_targetsKilled);
    CheckAchievement("BOUNTY_ACHIEVE_KILL_10_ENEMIES", stats.m_enemyKilled);
    CheckAchievement("BOUNTY_ACHIEVE_KILL_50_ENEMIES", stats.m_enemyKilled);
    CheckAchievement("BOUNTY_ACHIEVE_KILL_100_ENEMIES", stats.m_enemyKilled);
    CheckAchievement("BOUNTY_ACHIEVE_KILL_1000_ENEMIES", stats.m_enemyKilled);
    CheckAchievement("BOUNTY_ACHIEVE_REACH_5_LEVEL", player->GetLevel());
    CheckAchievement("BOUNTY_ACHIEVE_REACH_10_LEVEL", player->GetLevel());
    CheckAchievement("BOUNTY_ACHIEVE_REACH_15_LEVEL", player->GetLevel());
    CheckAchievement("BOUNTY_ACHIEVE_REACH_20_LEVEL", player->GetLevel());
    CheckAchievement("BOUNTY_ACHIEVE_CRIMINALS_THREAT", player->GetLevel());
    CheckAchievement("BOUNTY_ACHIEVE_EXTERMINATOR", player->GetLevel());
    CheckAchievement("BOUNTY_ACHIEVE_5_HEADSHOTS", stats.m_headshotsNumber);
    CheckAchievement("BOUNTY_ACHIEVE_50_HEADSHOTS", stats.m_headshotsNumber);
    CheckAchievement("BOUNTY_ACHIEVE_100_HEADSHOTS", stats.m_headshotsNumber);
    CheckAchievement("BOUNTY_ACHIEVE_500_HEADSHOTS", stats.m_headshotsNumber);
    CheckAchievement("BOUNTY_ACHIEVE_5_TIMES_WOUNDED", stats.m_woundedFailBounties);
    CheckAchievement("BOUNTY_ACHIEVE_50_TIMES_WOUNDED", stats.m_woundedFailBounties);
    CheckAchievement("BOUNTY_ACHIEVE_2_KILL_ONE_SHOT", stats.m_killsWithOneShotBest);
    CheckAchievement("BOUNTY_ACHIEVE_3_KILL_ONE_SHOT", stats.m_killsWithOneShotBest);
    CheckAchievement("BOUNTY_ACHIEVE_FIRE_500_ROUNDS", stats.m_shotsNumber);
    CheckAchievement("BOUNTY_ACHIEVE_FIRE_1000_ROUNDS", stats.m_shotsNumber);
    CheckAchievement("BOUNTY_ACHIEVE_FIRE_5000_ROUNDS", stats.m_shotsNumber);
    CheckAchievement("BOUNTY_ACHIEVE_TRANQ_50_ENEMIES", stats.m_enemyTranquillized);
    CheckAchievement("BOUNTY_ACHIEVE_TRANQ_100_ENEMIES", stats.m_enemyTranquillized);
    CheckAchievement("BOUNTY_ACHIEVE_TRANQ_500_ENEMIES", stats.m_enemyTranquillized);
    CheckAchievement("BOUNTY_ACHIEVE_KILL_100_NO_MISSES", stats.m_killsWithoutMissesBest);
    CheckAchievement("BOUNTY_ACHIEVE_RUNNING_HEADSHOT", stats.m_headshotsInRunning);
    CheckAchievement("BOUNTY_ACHIEVE_15_X2_XP_MULT", stats.m_x2MultiplierKills);
    CheckAchievement("BOUNTY_ACHIEVE_25_X3_XP_MULT", stats.m_x3MultiplierKills);
    CheckAchievement("BOUNTY_ACHIEVE_USE_5_MEDKITS", stats.m_medkitsUsed);
    CheckAchievement("BOUNTY_ACHIEVE_USE_50_MEDKITS", stats.m_medkitsUsed);
    CheckAchievement("BOUNTY_ACHIEVE_USE_100_MEDKITS", stats.m_medkitsUsed);
    CheckAchievement("BOUNTY_ACHIEVE_USE_500_MEDKITS", stats.m_medkitsUsed);
    CheckAchievement("BOUNTY_ACHIEVE_COVER_100_TIMES", stats.m_coversUsed);
    CheckAchievement("BOUNTY_ACHIEVE_COVER_500_TIMES", stats.m_coversUsed);
    CheckAchievement("BOUNTY_ACHIEVE_COVER_1000_TIMES", stats.m_coversUsed);
    CheckAchievement("BOUNTY_ACHIEVE_KILL_13_IN_15_SEC", stats.m_killsIn15SecBest);
    CheckAchievement("BOUNTY_ACHIEVE_KILL_13_IN_25_SEC", stats.m_killsIn25SecBest);
    CheckAchievement("BOUNTY_ACHIEVE_KILL_13_IN_40_SEC", stats.m_killsIn40SecBest);
    CheckAchievement("BOUNTY_ACHIEVE_UNLOCK_3_INFORMANTS", stats.m_informantsUnlocked);
    CheckAchievement("BOUNTY_ACHIEVE_UNLOCK_6_INFORMANTS", stats.m_informantsUnlocked);
    CheckAchievement("BOUNTY_ACHIEVE_UNLOCK_10_INFORMANTS", stats.m_informantsUnlocked);
    CheckAchievement("BOUNTY_ACHIEVE_CHANGE_WEAPON_250_TIMES", stats.m_inMIssionWeaponChanges);
    CheckAchievement("BOUNTY_ACHIEVE_GET_ALL_DAILY_BONUS", player->GetDailyBonusesInRow());
    CheckAchievement("BOUNTY_ACHIEVE_BUY_5_WEAPONS", stats.m_weaponsEarned);
    CheckAchievement("BOUNTY_ACHIEVE_BUY_10_WEAPONS", stats.m_weaponsEarned);
    CheckAchievement("BOUNTY_ACHIEVE_OPEN_ALL_AMMO_TYPES", stats.m_allAmmoTypesOpen);
    CheckAchievement("BOUNTY_ACHIEVE_BUY_50_ENERGY", stats.m_energyBought);
    CheckAchievement("BOUNTY_ACHIEVE_BUY_100_ENERGY", stats.m_energyBought);
    CheckAchievement("BOUNTY_ACHIEVE_BUY_300_ENERGY", stats.m_energyBought);
    CheckAchievement("BOUNTY_ACHIEVE_5_NO_AMMO_FAIL", stats.m_noAmmoFailBounties);
    CheckAchievement("BOUNTY_ACHIEVE_25_BOSS_ESCAPE_FAIL", stats.m_bossEscapedFailBounties);
    CheckAchievement("BOUNTY_ACHIEVE_NO_ENERGY_100_TIMES", stats.m_outOfEnergyTimes);
    CheckAchievement("BOUNTY_ACHIEVE_50_PISTOL_PERSON", stats.m_pistolHitsOnePersonBest);
    CheckAchievement("BOUNTY_ACHIEVE_100_PISTOL_HEADSHOTS", stats.m_pistolHeadshots);
    CheckAchievement("BOUNTY_ACHIEVE_NO_KILLS_TILL_TIMER_END", stats.m_pacifistMissions);
    CheckAchievement("BOUNTY_ACHIEVE_DEMO_MANIAC", stats.m_maxKillsCountByOneExplosion);
    CheckAchievement("BOUNTY_ACHIEVE_PRECISE_SHOT", stats.m_outOfRangeKillsCount);
    CheckAchievement("BOUNTY_ACHIEVE_WHERE_ARE_YOU_GOING", stats.m_escapingKillsCount);
    CheckAchievement("BOUNTY_ACHIEVE_LIGHTNING_REACTIONS", stats.m_pillUsedTimeMS / 1000);
    CheckAchievement("BOUNTY_ACHIEVE_BOOM", stats.m_totalKillsCountByExplosion);
    CheckAchievement("BOUNTY_ACHIEVE_KABOOM", stats.m_totalKillsCountByExplosion);
    CheckAchievement("BOUNTY_ACHIEVE_TOUGH_GUY", stats.m_armorConsumeDamage / 1000);
    CheckAchievement("BOUNTY_ACHIEVE_STEEL_CHEST", stats.m_armorConsumeDamage / 1000);
    CheckAchievement("BOUNTY_ACHIEVE_SHAKING_SIGHT", stats.m_onBoatHeadshotsCount);
    if (CDH_Weapon::g_availabilityChangesCount != m_weaponAvailabilityStamp) {
        m_weaponAvailabilityStamp = CDH_Weapon::g_availabilityChangesCount;
        CheckAchievement("BOUNTY_ACHIEVE_SPICY_STYLE", player->WeaponsWithCategoryCount(CDH_Weapon::Category_Exotic));
    }
    CheckAchievement("BOUNTY_ACHIEVE_PYROMANIAC", stats.m_incendiaryAmmoKills);
    CheckAchievement("BOUNTY_ACHIEVE_VICIOUSLY_VOLATILE", stats.m_explosiveAmmoKills);
    CheckAchievement("BOUNTY_ACHIEVE_HITCHHIKER", stats.m_movingMissionsCompleted);
    CheckAchievement("BOUNTY_ACHIEVE_EXPLORER", stats.CompletedMissionsAtEveryLocations());
    CheckAchievement("BOUNTY_ACHIEVE_COOPERATIVE_KILLER", stats.m_coopsCompleted);
    CheckAchievement("BOUNTY_ACHIEVE_TECHIE", stats.HasPurchasedVisorAndCyberScope());
    CheckAchievement("BOUNTY_ACHIEVE_TO_HELL_AND_BACK", stats.m_resurrects);
}

void CAchievementManager::CheckAchievement(XString name, int value)
{
    if (value < 1) {
        return;
    }
    CAchievement** found = m_achievements.Get(name);
    if (found == NULL) {
        return;
    }
    CAchievement* achievement = *found;
    if (achievement->m_progress < achievement->m_steps) {
        int stepValue = achievement->m_value / achievement->m_steps;
        if (stepValue < 1) {
            return;
        }
        if (stepValue * (achievement->m_progress + 1) <= value) {
            int reached = value / stepValue;
            int progress = 0;
            if (reached >= 0) {
                if (reached > achievement->m_steps) {
                    reached = achievement->m_steps;
                }
                progress = reached;
            }
            if (progress <= achievement->m_progress) {
                return;
            }
            GiveAchievement(name, progress);
        }
    }
    if (!achievement->m_synced) {
        if (GameCenter_Facade::GetInstance()->isGameCenterSupported() == true) {
            float percent = (float)(achievement->m_progress * 100) / (float)achievement->m_steps;
            GameCenter_Facade::GetInstance()->StartPostingAchievement(XString::AnsiString(achievement->m_name.c_str()), percent);
            m_lastPostingTime = CStdUtil_Cocoa::GetUpTimeMS();
            achievement->m_synced = true;
        }
    }
}

bool CAchievementManager::GiveAchievement(XString name, int progress)
{
    CAchievement** found = m_achievements.Get(name);
    if (found == NULL) {
        return false;
    }
    CAchievement* achievement = *found;
    if (achievement->m_progress >= progress) {
        return false;
    }
    if (achievement->m_steps < progress) {
        return false;
    }
    achievement->m_progress = progress;
    achievement->m_synced = false;
    static_cast<App*>(WindowApp::m_instance)->m_popupController->AddAchievementPopup(achievement);
    CBH_Player::GetInstance()->Save();
    return true;
}

void CAchievementManager::Update()
{
    if (CStdUtil_Cocoa::GetUpTimeMS() - m_lastCheckTime > 2000) {
        m_lastCheckTime = CStdUtil_Cocoa::GetUpTimeMS();
        return;
    }
    if (CStdUtil_Cocoa::GetUpTimeMS() - m_lastCheckTime > 1000) {
        m_lastCheckTime = CStdUtil_Cocoa::GetUpTimeMS();
        CheckAllAchievements();
    }
    if (m_lastPostingTime == 0) {
        m_lastPostingTime = CStdUtil_Cocoa::GetUpTimeMS();
    }
    if (CStdUtil_Cocoa::GetUpTimeMS() - m_lastPostingTime > 60000) {
        m_lastPostingTime = CStdUtil_Cocoa::GetUpTimeMS();
        if (GameCenter_Facade::GetInstance()->isGameCenterSupported() == true) {
            GameCenter_Facade::GetInstance()->FetchPosting();
        }
    }
}
