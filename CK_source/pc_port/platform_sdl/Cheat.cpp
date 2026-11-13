// Recognise the cheat codes of Cheat.h in the typed letters and apply them to the player.
#include "Cheat.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include "App.h"
#include "CBH_Player.h"
#include "CProgressSystem.h"

namespace ck {
namespace {

// The last typed letters, oldest first.
std::string typedLetters;

/**
 * Raise the player level to `targetLevel` (at most the top level) through the game's own
 * experience path, so its level-up handling (energy, health, save) runs.
 */
void RaiseLevel(int targetLevel)
{
    auto* app = static_cast<App*>(WindowApp::m_instance);
    CProgressSystem* progress = app->InitProgressSystem();
    CBH_Player* player = CBH_Player::GetInstance();
    const int oldLevel = player->GetLevel();
    const int maxLevel = progress->GetMaxLevel();
    if (targetLevel > maxLevel) {
        targetLevel = maxLevel;
    }
    // A level is reached at its experience threshold: give exactly what is missing.
    const int missingXp = progress->GetMaxXPForLevel(targetLevel) - player->m_experience;
    if (missingXp > 0) {
        player->AddXp(missingXp, GAIN_SOURCE_IAP); // IAP: no mission multiplier on top
    }
    fprintf(stderr, "Cheat: level %d -> %d (max %d), experience %d\n", oldLevel, player->GetLevel(), maxLevel,
            player->m_experience);
}

void AddCurrencies(int amount)
{
    CBH_Player* player = CBH_Player::GetInstance();
    player->AddMoney(amount, GAIN_SOURCE_IAP); // IAP: no mission multiplier on top
    // The map's gold counter has room for five digits and cuts off the leading digits of a larger value
    // (10,000,005 shows as "000005"), so the gold is set to the largest value it shows in full.
    player->AddCredits(CheatGoldLimit - player->m_credits);
    fprintf(stderr, "Cheat: cash %d, gold %d\n", player->m_cash, player->m_credits);
}

void ApplyCheat(const CheatCode& code)
{
    // The player and the level table only exist once the game has loaded.
    if (!App::IsInitialized()) {
        fprintf(stderr, "Cheat %s ignored: game not loaded yet\n", code.text);
        return;
    }
    switch (code.effect) {
    case CheatEffect::AddLevels:
        RaiseLevel(CBH_Player::GetInstance()->GetLevel() + code.amount);
        break;
    case CheatEffect::MaxLevel: {
        auto* app = static_cast<App*>(WindowApp::m_instance);
        RaiseLevel(app->InitProgressSystem()->GetMaxLevel());
        break;
    }
    case CheatEffect::AddCurrencies:
        AddCurrencies(code.amount);
        break;
    }
}

/** True when the typed letters end with the code text. */
bool TypedLettersEndWith(const char* text)
{
    const size_t length = strlen(text);
    if (typedLetters.size() < length) {
        return false;
    }
    return typedLetters.compare(typedLetters.size() - length, length, text) == 0;
}

} // namespace

void HandleCheatKey(SDL_Keycode key)
{
    // Codes are letters only; any other key starts over.
    if (key < SDLK_A || key > SDLK_Z) {
        typedLetters.clear();
        return;
    }
    typedLetters.push_back(static_cast<char>(key));
    if (typedLetters.size() > CheatTypedLettersKept) {
        typedLetters.erase(0, typedLetters.size() - CheatTypedLettersKept);
    }
    for (const CheatCode& code : CheatCodes) {
        if (TypedLettersEndWith(code.text)) {
            typedLetters.clear();
            ApplyCheat(code);
            return;
        }
    }
}

} // namespace ck
