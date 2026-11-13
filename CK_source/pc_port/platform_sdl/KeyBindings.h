// Desktop key map: the single place that decides which input triggers which game action.
// To rebind a control, edit the tables below; no other file needs to change.
#pragma once

#include <SDL3/SDL.h>

namespace ck {

/** Game actions a key or mouse button can trigger during a mission (each taps a HUD control). */
enum class GameAction {
    Reload,
    SwitchWeapon,
    Scope,      // in and out of the scope
    Steady,     // STEADY: hold breath for a steadier aim
    Cover,      // take cover / leave it
    Medkit,     // restore health (shown when hurt and a medkit is owned)
    Pill,       // slow motion pill (Reactix)
    Ultrasound, // ultrasound emitter: stuns the enemies
    Pause,
};

struct KeyBinding {
    SDL_Keycode key;
    GameAction action;
};

struct MouseButtonBinding {
    Uint8 button;
    GameAction action;
};

// During a mission the mouse is captured: moving it turns the view, like in any PC shooter.
// Game pixels of finger drag per mouse count; the game turns the unscoped view 0.1 degrees per pixel.
constexpr float MouseLookSensitivity = 1.0f;

// Held to fire during a mission (automatic weapons keep firing). In menus it is the pointer.
constexpr Uint8 FireButton = SDL_BUTTON_LEFT;

constexpr KeyBinding KeyBindings[] = {
    { SDLK_R, GameAction::Reload },
    { SDLK_Q, GameAction::SwitchWeapon },
    { SDLK_E, GameAction::SwitchWeapon },
    { SDLK_LSHIFT, GameAction::Steady },
    { SDLK_RSHIFT, GameAction::Steady },
    { SDLK_C, GameAction::Cover },
    { SDLK_H, GameAction::Medkit },
    { SDLK_F, GameAction::Pill },
    { SDLK_G, GameAction::Ultrasound },
    { SDLK_ESCAPE, GameAction::Pause },
};

constexpr MouseButtonBinding MouseButtonBindings[] = {
    { SDL_BUTTON_RIGHT, GameAction::Scope },
};

// The mouse wheel moves the scope's zoom slider (0: narrowest view, 1: widest) by this much per notch.
constexpr float ScopeZoomStep = 0.1f;

} // namespace ck
