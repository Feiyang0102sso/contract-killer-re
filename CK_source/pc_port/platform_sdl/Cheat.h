// Cheat codes: type the letters on the keyboard at any time once the game has loaded.
// This file holds the code texts and their numbers; Cheat.cpp applies them.
#pragma once

#include <SDL3/SDL.h>

namespace ck {

/** What a cheat code does; `amount` of the code says how much. */
enum class CheatEffect {
    AddLevels,     // raise the player level by `amount` (at most to the top level)
    MaxLevel,      // raise the player level to the top level of the level table (70)
    AddCurrencies, // add `amount` to the cash and set the gold to CheatGoldLimit
};

// The largest gold the map shows in full: its counter has room for five digits beside the icon.
constexpr int CheatGoldLimit = 99999;

struct CheatCode {
    const char* text; // lower-case letters only
    CheatEffect effect;
    int amount;
};

constexpr int CheatCurrencyAmount = 9999999;

// A code fires as soon as the last typed letters spell it, so no code may end with another code.
constexpr CheatCode CheatCodes[] = {
    { "chxp", CheatEffect::AddLevels, 1 },
    { "chxxp", CheatEffect::AddLevels, 10 },
    { "chxxxp", CheatEffect::MaxLevel, 0 },
    { "chm", CheatEffect::AddCurrencies, CheatCurrencyAmount },
};

// How many of the last typed letters are remembered; must be at least the longest code.
constexpr size_t CheatTypedLettersKept = 16;

/** Feed one pressed key; applies a cheat when the typed letters complete its code. */
void HandleCheatKey(SDL_Keycode key);

} // namespace ck
