// Translate desktop input to the touches of the iOS mission HUD (GamePlayHUD).
// Which input maps to which action lives in KeyBindings.h.
//
// The mission turns the view only with the first finger (pointer 0, CSwerveGame::HandleTouchPressed),
// and the game hands out pointer slots in the order the fingers go down. So during a mission one
// look finger stays down all the time and keeps slot 0; fire and the HUD taps use later slots.
#include "Host.h"
#include <stdio.h>
#include "App.h"
#include "AppSettings.h"
#include "CApplet.h"
#include "CBH_TriggerButton.h"
#include "CDH_GameSoftkeyWidget.h"
#include "CImageButton.h"
#include "CSwerveGame.h"
#include "CZoomSliderControl.h"
#include "Collision.h"
#include "DGHelper.h"
#include "GamePlayHUD.h"

namespace ck {
namespace {

// Each kind of synthetic finger has its own touch id (the menu pointer is touch 1).
constexpr unsigned int LookTouchId = 2;   // the view drag, kept down during the mission
constexpr unsigned int ActionTouchId = 3; // one-shot taps on HUD controls
constexpr unsigned int FireTouchId = 4;   // the held fire button
// The look finger is put back to the centre before it gets this close to the screen edge.
constexpr int LookEdgeMargin = 64;

struct LookFinger {
    bool down = false;
    int x = 0;
    int y = 0;
    float pendingX = 0.0f; // mouse movement not yet sent (below one game pixel)
    float pendingY = 0.0f;
};

LookFinger look;
bool fireHeld;
unsigned int firePosition;

/** Pack a game pixel position the way touch events carry it. */
unsigned int EncodePosition(int x, int y)
{
    return (x & 0x3fff) | ((y & 0x3fff) << 14);
}

/** The running mission, or nullptr outside of one. */
CSwerveGame* GetMission()
{
    auto* app = static_cast<App*>(WindowApp::m_instance);
    if (app == nullptr || app->m_helper == nullptr) {
        return nullptr;
    }
    CSwerveGame* game = app->m_helper->m_swerveGame;
    if (game == nullptr || !game->m_sceneReady || game->m_hud == nullptr) {
        return nullptr;
    }
    return game;
}

/** True when a visible modal window (pause menu, dialog, shop) lies in front of `window`. */
bool IsCoveredByModal(Window* window)
{
    while (window->m_parent != nullptr) {
        for (Window* sibling = window->m_parent->m_firstChild; sibling != window; sibling = sibling->m_nextSibling) {
            const bool modal = (sibling->m_flags & Window::FLAG_MODAL) != 0;
            const bool visible = (sibling->m_flags & Window::FLAG_VISIBLE) != 0;
            if (modal && visible) {
                return true;
            }
        }
        window = window->m_parent;
    }
    return false;
}

/** The HUD of the mission while it takes input, else nullptr. */
GamePlayHUD* GetActiveHud()
{
    CSwerveGame* game = GetMission();
    if (game == nullptr || game->IsGamePaused() || !game->m_hud->m_enabled || IsCoveredByModal(game)) {
        return nullptr;
    }
    return game->m_hud;
}

/** Tap a game pixel with the action finger. */
void Tap(CApplet& applet, int x, int y)
{
    const unsigned int position = EncodePosition(x, y);
    applet.QueueSystemEvent(TouchBegan, ActionTouchId, position | (1u << 28));
    applet.QueueSystemEvent(TouchEnded, ActionTouchId, position);
}

/** Tap the centre of a HUD hit area. */
void TapArea(CApplet& applet, const Collision& area)
{
    const CRectangle& bounds = area.m_bounds;
    Tap(applet, bounds.m_x + bounds.m_width / 2, bounds.m_y + bounds.m_height / 2);
}

/** Tap the centre of a HUD window when it is shown; false when it is not. */
bool TapWindow(CApplet& applet, Window* window)
{
    if (window == nullptr || (window->m_flags & Window::FLAG_VISIBLE) == 0) {
        return false;
    }
    Tap(applet, window->LocalToGlobalX(window->m_width / 2), window->LocalToGlobalY(window->m_height / 2));
    return true;
}

/** Put the look finger down at the screen centre. */
void PressLook(CApplet& applet)
{
    look.x = WindowWidth / 2;
    look.y = WindowHeight / 2;
    applet.QueueSystemEvent(TouchBegan, LookTouchId, EncodePosition(look.x, look.y) | (1u << 28));
    look.down = true;
}

/** Lift the look finger where it is. */
void ReleaseLook(CApplet& applet)
{
    if (!look.down) {
        return;
    }
    applet.QueueSystemEvent(TouchEnded, LookTouchId, EncodePosition(look.x, look.y));
    look.down = false;
}

} // namespace

bool TriggerAction(CApplet& applet, GameAction action)
{
    GamePlayHUD* hud = GetActiveHud();
    if (hud == nullptr) {
        return false;
    }
    // Each control is tapped only while the HUD shows it; the HUD itself checks the rest
    // (ammo, item counts, weapon ready), as for a finger.
    switch (action) {
    case GameAction::Reload:
        TapArea(applet, hud->m_reloadButtonArea);
        return true;
    case GameAction::SwitchWeapon:
        if (!hud->m_hasAlternateWeapon) {
            return false;
        }
        TapArea(applet, hud->m_weaponSwitchArea);
        return true;
    case GameAction::Scope:
        if (!hud->m_scopeButtonShown) {
            return false;
        }
        TapArea(applet, hud->m_scopeArea);
        return true;
    case GameAction::Steady:
        if (hud->m_steadyRemainingMs < 0 || hud->m_steadyActive) {
            return false;
        }
        TapArea(applet, hud->m_steadyArea);
        return true;
    case GameAction::Cover:
        return TapWindow(applet, hud->m_coverButton);
    case GameAction::Medkit:
        if (!hud->m_showMedkit) {
            return false;
        }
        TapArea(applet, hud->m_medkitArea);
        return true;
    case GameAction::Pill:
        TapArea(applet, hud->m_tabletArea);
        return true;
    case GameAction::Ultrasound:
        if (!hud->m_showUltrasound) {
            return false;
        }
        TapArea(applet, hud->m_ultrasoundArea);
        return true;
    case GameAction::Pause:
        return TapWindow(applet, hud->m_pauseButton);
    case GameAction::Shop:
        return TapWindow(applet, hud->m_ingameShop);
    }
    return false;
}

void HandleKeyDown(CApplet& applet, SDL_Keycode key)
{
    for (const KeyBinding& binding : KeyBindings) {
        if (binding.key == key) {
            TriggerAction(applet, binding.action);
            return;
        }
    }
}

bool HandleMouseButtonDown(CApplet& applet, Uint8 button)
{
    for (const MouseButtonBinding& binding : MouseButtonBindings) {
        if (binding.button == button) {
            TriggerAction(applet, binding.action);
            return true;
        }
    }
    return false;
}

void AdjustScopeZoom(float wheelY)
{
    GamePlayHUD* hud = GetActiveHud();
    if (hud == nullptr || !hud->m_scoped) {
        return;
    }
    CZoomSliderControl* slider = hud->m_zoomSliderControl;
    // The slider is enabled only while looking through a scope that can zoom.
    if (slider == nullptr || (slider->m_flags & Window::FLAG_ENABLED) == 0) {
        return;
    }
    // Wheel up zooms in: towards 0, the narrowest field of view.
    slider->SetProgress(slider->GetValue() - wheelY * ScopeZoomStep);
}

void ApplyDesktopFireMode()
{
    auto* app = static_cast<App*>(WindowApp::m_instance);
    if (app == nullptr || app->m_settings == nullptr) {
        return;
    }
    // With the HUD fire button on, holding it fires and a look drag never shoots on release,
    // which is what the left mouse button and the look finger need.
    if (!app->m_settings->IsFireButtonEnabled()) {
        app->m_settings->SetFireButtonEnabled(true);
    }
}

bool IsMissionInputActive()
{
    return GetActiveHud() != nullptr;
}

void TurnView(CApplet& applet, float mouseX, float mouseY)
{
    CSwerveGame* game = GetMission();
    if (game == nullptr) {
        return;
    }
    // Keep the look finger down in slot 0. The game drops it while tutorial hints are shown.
    if (!look.down) {
        PressLook(applet);
        return;
    }
    if (!game->m_pointerDown[0]) {
        ReleaseLook(applet);
        PressLook(applet);
        return;
    }

    // Whole pixels go to the game now, the fraction waits for the next frame.
    look.pendingX += mouseX * MouseLookSensitivity;
    look.pendingY += mouseY * MouseLookSensitivity;
    int moveX = static_cast<int>(look.pendingX);
    int moveY = static_cast<int>(look.pendingY);
    if (moveX == 0 && moveY == 0) {
        return;
    }
    look.pendingX -= static_cast<float>(moveX);
    look.pendingY -= static_cast<float>(moveY);

    int x = look.x + moveX;
    int y = look.y + moveY;
    const bool leavesX = x < LookEdgeMargin || x > WindowWidth - LookEdgeMargin;
    const bool leavesY = y < LookEdgeMargin || y > WindowHeight - LookEdgeMargin;
    if (leavesX || leavesY) {
        // Lift the finger and put it down again at the centre, then drag from there.
        ReleaseLook(applet);
        PressLook(applet);
        x = SDL_clamp(look.x + moveX, LookEdgeMargin, WindowWidth - LookEdgeMargin);
        y = SDL_clamp(look.y + moveY, LookEdgeMargin, WindowHeight - LookEdgeMargin);
    }
    applet.QueueSystemEvent(TouchMoved, LookTouchId, EncodePosition(x, y));
    look.x = x;
    look.y = y;
}

void EndViewDrag(CApplet& applet)
{
    ReleaseLook(applet);
    look.pendingX = 0.0f;
    look.pendingY = 0.0f;
}

void SetFireHeld(CApplet& applet, bool held)
{
    if (held == fireHeld) {
        return;
    }
    if (!held) {
        applet.QueueSystemEvent(TouchEnded, FireTouchId, firePosition);
        fireHeld = false;
        return;
    }
    GamePlayHUD* hud = GetActiveHud();
    if (hud == nullptr) {
        return;
    }
    // The look finger must own slot 0 before the fire finger takes a slot.
    if (!look.down) {
        PressLook(applet);
    }
    const CRectangle& bounds = hud->m_fireArea.m_bounds;
    firePosition = EncodePosition(bounds.m_x + bounds.m_width / 2, bounds.m_y + bounds.m_height / 2);
    applet.QueueSystemEvent(TouchBegan, FireTouchId, firePosition | (1u << 28));
    fireHeld = true;
}

} // namespace ck
