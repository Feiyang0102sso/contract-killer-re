// Desktop key hints over the mission HUD: at a corner of every HUD control that a key or mouse
// button triggers (KeyBindings.h), a blood splat badge shows that key (pixels: KeyHintArt.cpp).
//
// The badges are a child window put in front of the other HUD children by the GamePlayHUD
// constructor. So they are drawn through the game's own 2D renderer (no GL state of ours), over
// the cover and pause buttons, fade with the HUD and lie under the pause menu and dialogs.
// [game] KeyHints in BountyHunter.cfg turns them off.
#include "Host.h"
#include "Config.h"
#include "KeyHintArt.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include "App.h"
#include "AppSettings.h"
#include "CBH_Player.h"
#include "CBH_SurfaceManager.h"
#include "CBH_TriggerButton.h"
#include "CDH_BasicGameData.h"
#include "CDH_GameSoftkeyWidget.h"
#include "CDH_Weapon.h"
#include "CImageButton.h"
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "GamePlayHUD.h"
#include "ICRenderSurface.h"
#include "zlib.h"

namespace ck {
namespace {

/** HUD controls that get a badge. */
enum class HudControl {
    Fire,
    Scope,
    SwitchWeapon,
    Reload,
    Steady,
    Cover,
    Medkit,
    Pill,
    Ultrasound,
    Pause,
    Shop,
};

/** The corner of the control that the badge centre sits on. */
enum class BadgeCorner {
    TopLeft,
    TopRight,
    BottomRight,
    LeftMiddle, // left of the control, half way down
};

struct BadgeSpec {
    HudControl control;
    BadgeCorner corner;
    int offsetX; // from the corner to the badge centre, in game pixels (+ is right / down)
    int offsetY;
};

// Placement as in the approved preview (v3). The controls along the bottom edge and the two
// at the top left carry their badge at the bottom right, so the timers drawn above the pill and
// ultrasound buttons stay readable; the controls at the right screen edge carry it on the left.
// The offsets were measured on dumped frames: several HUD images have transparent margins (the
// cover image ~30 px at the top, the bottom row's hit areas reach ~18 px below the visible buttons).
const BadgeSpec BadgeSpecs[] = {
    { HudControl::Pause, BadgeCorner::BottomRight, -3, -8 },
    { HudControl::Shop, BadgeCorner::BottomRight, -3, -8 },
    { HudControl::Cover, BadgeCorner::TopRight, -3, 34 },
    { HudControl::Scope, BadgeCorner::TopRight, -3, 4 },
    { HudControl::Medkit, BadgeCorner::BottomRight, -6, -24 },
    { HudControl::Pill, BadgeCorner::BottomRight, -6, -24 },
    { HudControl::Ultrasound, BadgeCorner::BottomRight, -6, -24 },
    { HudControl::Steady, BadgeCorner::BottomRight, -6, -24 },
    { HudControl::SwitchWeapon, BadgeCorner::TopLeft, 2, 4 },
    { HudControl::Fire, BadgeCorner::TopLeft, 2, 4 },
    { HudControl::Reload, BadgeCorner::LeftMiddle, -15, 0 },
};
// Each badge gets its own splat shape from this seed and its index.
constexpr int SplatSeedBase = 17;
constexpr int SplatSeedStep = 7919;
// Value of CBH_Player::GetAvailableAmmo for unlimited ammo (no reload button then).
constexpr int UnlimitedAmmo = 0xfffffff;
// Mime type key of PNG for CBH_SurfaceManager::CreateSurface (ImageType_PNG of CRenderSurfaceBuffer).
constexpr unsigned int PngMimeType = 0xb7178678;

/** Short names for the keys whose SDL names are long. */
struct ShortKeyName {
    SDL_Keycode key;
    const char* name;
};

const ShortKeyName ShortKeyNames[] = {
    { SDLK_LSHIFT, "SHIFT" },
    { SDLK_RSHIFT, "SHIFT" },
    { SDLK_LCTRL, "CTRL" },
    { SDLK_RCTRL, "CTRL" },
    { SDLK_LALT, "ALT" },
    { SDLK_RALT, "ALT" },
    { SDLK_ESCAPE, "ESC" },
    { SDLK_SPACE, "SPACE" },
    { SDLK_RETURN, "ENTER" },
    { SDLK_TAB, "TAB" },
    { SDLK_BACKSPACE, "BKSP" },
};

/** The key or mouse button bound to a control. */
struct BoundInput {
    bool mouse;
    Uint8 button;    // when mouse
    SDL_Keycode key; // otherwise
};

/** The first binding of `action` in KeyBindings.h (keys before mouse buttons). */
bool FindBinding(GameAction action, BoundInput& input)
{
    for (const KeyBinding& binding : KeyBindings) {
        if (binding.action == action) {
            input.mouse = false;
            input.key = binding.key;
            return true;
        }
    }
    for (const MouseButtonBinding& binding : MouseButtonBindings) {
        if (binding.action == action) {
            input.mouse = true;
            input.button = binding.button;
            return true;
        }
    }
    return false;
}

/** The input that triggers `control`; false when nothing is bound to it. */
bool FindInput(HudControl control, BoundInput& input)
{
    switch (control) {
    case HudControl::Fire:
        input.mouse = true;
        input.button = FireButton;
        return true;
    case HudControl::Scope:
        return FindBinding(GameAction::Scope, input);
    case HudControl::SwitchWeapon:
        return FindBinding(GameAction::SwitchWeapon, input);
    case HudControl::Reload:
        return FindBinding(GameAction::Reload, input);
    case HudControl::Steady:
        return FindBinding(GameAction::Steady, input);
    case HudControl::Cover:
        return FindBinding(GameAction::Cover, input);
    case HudControl::Medkit:
        return FindBinding(GameAction::Medkit, input);
    case HudControl::Pill:
        return FindBinding(GameAction::Pill, input);
    case HudControl::Ultrasound:
        return FindBinding(GameAction::Ultrasound, input);
    case HudControl::Pause:
        return FindBinding(GameAction::Pause, input);
    case HudControl::Shop:
        return FindBinding(GameAction::Shop, input);
    }
    return false;
}

/** The name printed on the badge: a short name, else the SDL key name in capitals. */
std::string KeyLabel(SDL_Keycode key)
{
    for (const ShortKeyName& shortName : ShortKeyNames) {
        if (shortName.key == key) {
            return shortName.name;
        }
    }
    std::string name = SDL_GetKeyName(key);
    for (char& letter : name) {
        letter = static_cast<char>(toupper(static_cast<unsigned char>(letter)));
    }
    return name;
}

std::vector<unsigned char> MakeBadgePixels(const BoundInput& input, int seed)
{
    if (!input.mouse) {
        return MakeKeyBadge(KeyLabel(input.key), seed);
    }
    if (input.button == SDL_BUTTON_LEFT) {
        return MakeMouseBadge(false, seed);
    }
    if (input.button == SDL_BUTTON_RIGHT) {
        return MakeMouseBadge(true, seed);
    }
    // Other mouse buttons have no icon: M3, M4, ...
    return MakeKeyBadge("M" + std::to_string(input.button), seed);
}

void AppendBigEndian(std::vector<unsigned char>& bytes, unsigned int value)
{
    bytes.push_back(static_cast<unsigned char>(value >> 24));
    bytes.push_back(static_cast<unsigned char>(value >> 16));
    bytes.push_back(static_cast<unsigned char>(value >> 8));
    bytes.push_back(static_cast<unsigned char>(value));
}

/** Append one PNG chunk: length, type, data and the CRC of type and data. */
void AppendPngChunk(std::vector<unsigned char>& png, const char* type, const std::vector<unsigned char>& data)
{
    AppendBigEndian(png, static_cast<unsigned int>(data.size()));
    const size_t typeStart = png.size();
    png.insert(png.end(), type, type + 4);
    png.insert(png.end(), data.begin(), data.end());
    const uLong crc = crc32(0, &png[typeStart], static_cast<uInt>(png.size() - typeStart));
    AppendBigEndian(png, static_cast<unsigned int>(crc));
}

/**
 * RGBA pixels (top row first) as a PNG file in memory. The engine's hardware surfaces cannot be
 * locked for writing, but they load PNG through its own libpng, as every game image does. The game
 * has no deflate, so the image data goes into uncompressed ("stored") deflate blocks.
 */
std::vector<unsigned char> EncodePng(const std::vector<unsigned char>& pixels, int width, int height)
{
    // Each row starts with filter type 0 (none).
    std::vector<unsigned char> raw;
    const size_t rowBytes = static_cast<size_t>(width) * 4;
    for (int row = 0; row < height; row++) {
        raw.push_back(0);
        raw.insert(raw.end(), pixels.begin() + row * rowBytes, pixels.begin() + (row + 1) * rowBytes);
    }

    // zlib stream: header, stored blocks of at most 65535 bytes, Adler-32 of the raw data.
    constexpr size_t MaxStoredBlock = 65535;
    std::vector<unsigned char> zlibData = { 0x78, 0x01 };
    size_t offset = 0;
    while (offset < raw.size()) {
        size_t blockSize = raw.size() - offset;
        if (blockSize > MaxStoredBlock) {
            blockSize = MaxStoredBlock;
        }
        // Block header byte: bit 0 BFINAL (last block), bits 1-2 BTYPE 00 (stored).
        unsigned char blockHeader = 0;
        if (offset + blockSize == raw.size()) {
            blockHeader = 1;
        }
        zlibData.push_back(blockHeader);
        zlibData.push_back(static_cast<unsigned char>(blockSize));
        zlibData.push_back(static_cast<unsigned char>(blockSize >> 8));
        zlibData.push_back(static_cast<unsigned char>(~blockSize));
        zlibData.push_back(static_cast<unsigned char>(~blockSize >> 8));
        zlibData.insert(zlibData.end(), raw.begin() + offset, raw.begin() + offset + blockSize);
        offset += blockSize;
    }
    AppendBigEndian(zlibData, static_cast<unsigned int>(adler32(1, raw.data(), static_cast<uInt>(raw.size()))));

    // Header: width, height, 8 bits per channel, colour type 6 (RGBA), deflate, no filter set, no interlace.
    std::vector<unsigned char> header;
    AppendBigEndian(header, static_cast<unsigned int>(width));
    AppendBigEndian(header, static_cast<unsigned int>(height));
    header.push_back(8);
    header.push_back(6);
    header.push_back(0);
    header.push_back(0);
    header.push_back(0);

    std::vector<unsigned char> png = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' };
    AppendPngChunk(png, "IHDR", header);
    AppendPngChunk(png, "IDAT", zlibData);
    AppendPngChunk(png, "IEND", std::vector<unsigned char>());
    return png;
}

/** A surface of the game's 2D renderer holding KeyBadgeSize x KeyBadgeSize RGBA pixels. */
ICRenderSurface* CreateBadgeSurface(const std::vector<unsigned char>& pixels)
{
    const std::vector<unsigned char> png = EncodePng(pixels, KeyBadgeSize, KeyBadgeSize);
    CBH_SurfaceManager* surfaces = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
    ICRenderSurface* surface = surfaces->CreateSurface(png.data(), static_cast<unsigned int>(png.size()),
                                                       PngMimeType);
    unsigned int width = 0;
    unsigned int height = 0;
    if (surface != nullptr) {
        surface->GetWidthAndHeight(width, height);
    }
    if (width != KeyBadgeSize || height != KeyBadgeSize) {
        fprintf(stderr, "Key hints: badge surface could not be created (%ux%u)\n", width, height);
        delete surface;
        return nullptr;
    }
    return surface;
}

/** Screen rectangle of a HUD child window; false while it is hidden. */
bool WindowRect(Window* window, CRectangle& rect)
{
    if (window == nullptr || (window->m_flags & Window::FLAG_VISIBLE) == 0) {
        return false;
    }
    rect = CRectangle(window->LocalToGlobalX(0), window->LocalToGlobalY(0), window->m_width, window->m_height);
    return true;
}

/**
 * Where the HUD shows `control` this frame; false when it does not show it. The conditions are those
 * of GamePlayHUD::Paint and its Draw* functions; keep them in step.
 */
bool GetControlRect(GamePlayHUD* hud, HudControl control, CRectangle& rect)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CBH_Player* player = CBH_Player::GetInstance();
    switch (control) {
    case HudControl::Fire:
        if (!app->m_settings->IsFireButtonEnabled()) {
            return false;
        }
        rect = hud->m_fireArea.m_bounds;
        return true;
    case HudControl::Scope:
        // DrawScopeButton: none with a special weapon; m_scopeButtonShown is set there each frame.
        if (app->m_helper->m_swerveGame->m_specialWeapon != nullptr || !hud->m_scopeButtonShown) {
            return false;
        }
        rect = hud->m_scopeArea.m_bounds;
        return true;
    case HudControl::SwitchWeapon:
        if (!hud->m_hasAlternateWeapon || hud->m_scoped) {
            return false;
        }
        rect = hud->m_weaponSwitchArea.m_bounds;
        return true;
    case HudControl::Reload: {
        // DrawReloadButton; its position is moved by DrawWeaponButton every frame.
        CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
        if (weapon == nullptr || hud->m_scoped) {
            return false;
        }
        if (player->GetAvailableAmmo(weapon->GetDefaultAmmo(), true) == UnlimitedAmmo) {
            return false;
        }
        rect = hud->m_reloadButtonArea.m_bounds;
        return true;
    }
    case HudControl::Steady:
        if (hud->m_steadyRemainingMs < 0 || hud->m_modelAnimationState != 0) {
            return false;
        }
        rect = hud->m_steadyArea.m_bounds;
        return true;
    case HudControl::Cover:
        return WindowRect(hud->m_coverButton, rect);
    case HudControl::Medkit:
        if (!hud->m_showMedkit) {
            return false;
        }
        rect = hud->m_medkitArea.m_bounds;
        return true;
    case HudControl::Pill:
        if (!player->IsSpeedModeAvailable() && !player->IsSpeedMode()) {
            return false;
        }
        rect = hud->m_tabletArea.m_bounds;
        return true;
    case HudControl::Ultrasound:
        if (!hud->m_showUltrasound) {
            return false;
        }
        rect = hud->m_ultrasoundArea.m_bounds;
        return true;
    case HudControl::Pause:
        return WindowRect(hud->m_pauseButton, rect);
    case HudControl::Shop:
        return WindowRect(hud->m_ingameShop, rect);
    }
    return false;
}

/** The point of `rect` named by `corner`. */
void CornerPoint(const CRectangle& rect, BadgeCorner corner, int& x, int& y)
{
    const int left = rect.m_x;
    const int top = rect.m_y;
    const int right = rect.m_x + rect.m_width;
    const int bottom = rect.m_y + rect.m_height;
    switch (corner) {
    case BadgeCorner::TopLeft:
        x = left;
        y = top;
        return;
    case BadgeCorner::TopRight:
        x = right;
        y = top;
        return;
    case BadgeCorner::BottomRight:
        x = right;
        y = bottom;
        return;
    case BadgeCorner::LeftMiddle:
        x = left;
        y = top + rect.m_height / 2;
        return;
    }
}

/** Full-screen, input-less child window of the HUD that draws the badges. */
class KeyHintWindow : public Window {
public:
    explicit KeyHintWindow(GamePlayHUD* hud);
    virtual ~KeyHintWindow();
    virtual void Paint(ICGraphics2d& graphics);

private:
    struct Badge {
        BadgeSpec spec;
        ICRenderSurface* surface;
    };

    GamePlayHUD* m_hud;
    std::vector<Badge> m_badges;
};

KeyHintWindow::KeyHintWindow(GamePlayHUD* hud)
    : Window(false), m_hud(hud)
{
    // Never take touches or focus: the HUD controls below must get them all. The window keeps the
    // full screen size of a new Window, which it needs to be painted at all (Window::HandlePaint).
    ClearFlags(FLAG_ENABLED | FLAG_FOCUSABLE);
    if (!config.keyHints) {
        ClearFlags(FLAG_VISIBLE);
        return;
    }
    int seed = SplatSeedBase;
    for (const BadgeSpec& spec : BadgeSpecs) {
        seed += SplatSeedStep;
        BoundInput input = {};
        if (!FindInput(spec.control, input)) {
            continue;
        }
        ICRenderSurface* surface = CreateBadgeSurface(MakeBadgePixels(input, seed));
        if (surface == nullptr) {
            continue;
        }
        Badge badge;
        badge.spec = spec;
        badge.surface = surface;
        m_badges.push_back(badge);
    }
    fprintf(stderr, "Key hints: %d badges\n", static_cast<int>(m_badges.size()));
}

KeyHintWindow::~KeyHintWindow()
{
    for (Badge& badge : m_badges) {
        delete badge.surface;
    }
}

void KeyHintWindow::Paint(ICGraphics2d& graphics)
{
    if (m_badges.empty()) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSwerveGame* game = app->m_helper->m_swerveGame;
    // The gates every HUD button shares: HUD shown in the settings, no intro, not faded out.
    if (game == nullptr || !app->m_settings->m_field1f || game->IsIntroTime()) {
        return;
    }
    const float alpha = m_hud->m_visibilityAlpha;
    if (!(alpha > 0.0f)) {
        return;
    }
    DGHelper* helper = app->m_helper;
    helper->PushAlphaBlend(alpha);
    for (const Badge& badge : m_badges) {
        CRectangle rect;
        if (!GetControlRect(m_hud, badge.spec.control, rect)) {
            continue;
        }
        int x = 0;
        int y = 0;
        CornerPoint(rect, badge.spec.corner, x, y);
        x += badge.spec.offsetX;
        y += badge.spec.offsetY;
        helper->DrawImage(badge.surface, x, y, 1.0f, 0, 0, 0.0f, DGHelper::DRAW_HCENTER | DGHelper::DRAW_VCENTER);
    }
    helper->PopAlphaBlend();
}

} // namespace

Window* CreateKeyHintWindow(GamePlayHUD* hud)
{
    return new KeyHintWindow(hud);
}

} // namespace ck
