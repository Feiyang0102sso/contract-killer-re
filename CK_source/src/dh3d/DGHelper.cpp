// DGHelper.cpp - helpers of the game: Swerve utilities, drawing, string formatting.
#include "DGHelper.h"
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include "CSwerve.h"
#include "CApplet.h"
#include "CApp.h"
#include "ICCore.h"
#include "CHash.h"
#include "CRandGen.h"
#include "CVector.h"
#include "CStdUtil_Cocoa.h"
#include "ICGraphics.h"
#include "ICGraphics2d.h"
#include "ICRenderSurface.h"
#include "CFont.h"
#include "CColor.h"
#include "CResource.h"
#include "ICResourceManager.h"
#include "CDrawUtil.h"
#include "DataInputStream.h"
#include "MathLib.h"
#include "App.h"
#include "CDH_ResourceManager.h"
#include "SwerveHelper.h"
#include "AnimationImpl.h"
#include "AnimatableSwerveObject.h"
#include "timelib.h"



/** Rating page of the game. */
static const wchar_t RATE_ME_LINK[] =
    L"http://gcs.glu.com/gcs/iphone/iphone.jsp?t=bounty&src=bounty&p=iphone&page=review";

/** DrawImageScaled: alpha below this is not drawn, above the next one needs no blending. */
static const float ALPHA_INVISIBLE = 0.01f;
static const float ALPHA_OPAQUE = 0.99f;

/** Swerve interface id of World (the aux render scene). */
static const int SWERVE_WORLD = 0x1f;

int DGHelper::m_alpha_blend_stack_pos;

void DGHelper::PushAlphaBlend(float alpha)
{
    if (alpha >= 0.0f && alpha <= 1.0f) {
        ICGraphics2d* graphics = ICGraphics2d::GetInstance();
        graphics->PushState(ICGraphics2d::State_4);
        graphics->PushState(ICGraphics2d::State_Blend);
        graphics->PushState(ICGraphics2d::State_2);
        graphics->Disable(ICGraphics2d::State_4);
        graphics->Enable(ICGraphics2d::State_Blend);
        graphics->SetBlendArg(ICGraphics2d::BlendArg_0);
        int amount = static_cast<int>(alpha * 65536.0f);
        graphics->SetColor(amount, amount, amount, amount);
        ++m_alpha_blend_stack_pos;
    }
}

void DGHelper::PopAlphaBlend()
{
    if (m_alpha_blend_stack_pos > 0) {
        --m_alpha_blend_stack_pos;
        ICGraphics2d* graphics = ICGraphics2d::GetInstance();
        graphics->PopState(ICGraphics2d::State_2);
        graphics->PopState(ICGraphics2d::State_Blend);
        graphics->PopState(ICGraphics2d::State_4);
    }
}

DGHelper::DGHelper()
    : m_transform(CSwerve::GetInstance()->m_swerve),
      m_rayIntersection(CSwerve::GetInstance()->m_swerve),
      m_swerveGame(NULL),
      m_gpsScreen(NULL),
      m_screenWidth(0),
      m_screenHeight(0),
      m_field118(0),
      m_field11c(0),
      m_startTimeMs(0),
      m_config(L"res/common/configs/dh3d_config.cfg"),
      m_loadingResource(false)
{
    m_startTimeMs = CStdUtil_Cocoa::GetTimeSeconds() * 1000;
    m_swerveGame = NULL;
    m_gpsScreen = NULL;
    m_gamePlayManager.init();
    initGame();
    if (m_auxRender == NULL) {
        Object3D root(
            (IQueryInterface*)CSwerve::GetInstance()->LoadM3GResource("BIN_AUX_RENDER"));
        m_auxRender = World(root);
    }
}

void DGHelper::initGame()
{
    ICRenderSurface* screen = ICGraphics::GetInstance()->GetTargetSurface();
    // b147: width is declared first (higher stack slot, sp+4 in the original).
    unsigned int width;
    unsigned int height;
    screen->GetWidthAndHeight(width, height);
    m_screenWidth = width;
    m_screenHeight = height;
}

void DGHelper::initSwerveGame(CSwerveGame* game)
{
    m_swerveGame = game;
    initGame();
}

void DGHelper::SetSwerveGamePtr(CSwerveGame* game)
{
    m_swerveGame = game;
}

void DGHelper::initGPSScreen(CGPSMapScreen* screen)
{
    m_gpsScreen = screen;
    initGame();
}

DGHelper::~DGHelper()
{
}

int DGHelper::getSwerveID(const char* name)
{
    int length = strlen(name);
    unsigned int hash = 0;
    while (length > 0) {
        --length;
        hash = hash * 0x19660dU + static_cast<unsigned char>(name[length]) + 0x3c6ef35fU;
    }
    unsigned int sign = static_cast<int>(hash) >> 31;
    return ((hash ^ sign) - sign) & 0x3fffffff;
}

vec3 DGHelper::GetPointOnNear(float x, float y, Camera camera)
{
    Transform projection = SwerveHelper::NewTransform();
    camera.getProjection(projection);
    projection.invert();
    vec3 point((x - 0.5f) * 2, (y - 0.5f) * 2, -1.0f);
    TransformPoint(projection, point);
    return point;
}

void DGHelper::SetCameraSize(Camera& camera, int width, int height,
                             camera_size_update_type_t updateType)
{
    float params[4];
    camera.getProjection(params);
    float aspect;
    // b147: default is written first (original block order: default, 0/1, 2; range test first).
    switch (updateType) {
    default:
        aspect = params[1];
        break;
    case CAMERA_SIZE_SET_ASPECT:
    case CAMERA_SIZE_SET_ASPECT_ONLY:
        aspect = (float)width / (float)height;
        params[1] = aspect;
        break;
    case CAMERA_SIZE_KEEP_FOV: {
        aspect = (float)width / (float)height;
        float oldAspect = params[1];
        params[1] = aspect;
        params[0] = params[0] * (oldAspect / aspect);
        break;
    }
    }
    camera.setPerspective(params[0], aspect, params[2], params[3]);
}

void DGHelper::TransformPoint(Transform& transform, vec3& point)
{
    float vector[4];
    vector[0] = point.m_x;
    vector[1] = point.m_y;
    vector[2] = point.m_z;
    vector[3] = 1.0f;
    transform.transform(4, vector);
    point.m_x = vector[0];
    point.m_y = vector[1];
    point.m_z = vector[2];
}

void DGHelper::TransformPoint(Transform& transform, swvVector3D& point)
{
    // swvVector3D has the layout of vec3 (x, y, z floats)
    TransformPoint(transform, (vec3&)point);
}

void DGHelper::DrawCircle(int centerX, int centerY, int radius, int color, bool filled)
{
    ICGraphics2d* graphics = ICGraphics2d::GetInstance();
    int x = 0;
    int y = radius;
    DrawCirclePixels(centerX, centerY, x, y, color, graphics, filled);
    int decision = 1 - radius;
    int deltaEast = 3;
    int deltaSouthEast = 5 - 2 * radius;
    while (y > x) {
        if (decision < 0) {
            decision += deltaEast;
        } else {
            decision += deltaSouthEast;
            deltaSouthEast += 2;
            y--;
        }
        deltaEast += 2;
        deltaSouthEast += 2;
        x++;
        DrawCirclePixels(centerX, centerY, x, y, color, graphics, filled);
    }
}

void DGHelper::DrawCirclePixels(int centerX, int centerY, int x, int y, int color,
                                ICGraphics2d* graphics, bool filled)
{
    for (int pass = 0; pass < 2; pass++) {
        if (filled) {
            CDrawUtil::Fill(centerX - x, centerY - y, centerX + x + 1, centerY - y + 1, color,
                            (ICGraphics2d::Primitive)3);
            CDrawUtil::Fill(centerX - x, centerY + y, centerX + x + 1, centerY + y + 1, color,
                            (ICGraphics2d::Primitive)3);
        } else {
            CDrawUtil::Fill(centerX - x, centerY - y, centerX - x + 1, centerY - y + 1, color,
                            (ICGraphics2d::Primitive)3);
            CDrawUtil::Fill(centerX - x, centerY + y, centerX - x + 1, centerY + y + 1, color,
                            (ICGraphics2d::Primitive)3);
            CDrawUtil::Fill(centerX + x, centerY + y, centerX + x + 1, centerY + y + 1, color,
                            (ICGraphics2d::Primitive)3);
            CDrawUtil::Fill(centerX + x, centerY - y, centerX + x + 1, centerY - y + 1, color,
                            (ICGraphics2d::Primitive)3);
        }
        // the second pass draws the octants mirrored at the diagonal
        int swap = x;
        x = y;
        y = swap;
    }
}

void DGHelper::DrawImageScaled(ICRenderSurface* surface, float scaleX, float scaleY, int x, int y,
                               float alpha, int pivotX, int pivotY, float rotation, unsigned int flags)
{
    ICGraphics2d* graphics = ICGraphics2d::GetInstance();
    if (alpha < ALPHA_INVISIBLE) {
        return;
    }
    if (flags != 0) {
        unsigned int width;
        unsigned int height;
        surface->GetWidthAndHeight(width, height);
        if (flags & DRAW_RIGHT) {
            pivotX += (int)(width * scaleX);
        } else if (flags & DRAW_HCENTER) {
            pivotX += (int)(width * scaleX) >> 1;
        }
        if (flags & DRAW_BOTTOM) {
            pivotY += (int)(height * scaleY);
        } else if (flags & DRAW_VCENTER) {
            pivotY += (int)(height * scaleY) >> 1;
        }
    }
    if (alpha < ALPHA_OPAQUE) {
        PushAlphaBlend(alpha);
    } else {
        // opaque: the original keeps state 4 enabled for the draw
        graphics->PushState(ICGraphics2d::State_4);
        graphics->Enable(ICGraphics2d::State_4);
    }
    graphics->PushTransform();
    graphics->LoadIdentity();
    graphics->Translate(x * 65536, y * 65536);
    if (rotation != 0) {
        graphics->Rotate(static_cast<int>(rotation * 65536.0f));
    }
    if (pivotX != 0 || pivotY != 0) {
        graphics->Translate(-pivotX * 65536, -pivotY * 65536);
    }
    if (scaleX != 1.0f || scaleY != 1.0f) {
        graphics->Scale(static_cast<int>(scaleX * 65536.0f), static_cast<int>(scaleY * 65536.0f));
    }
    // The alpha comes from PushAlphaBlend. ICGraphics2d takes 16.16 transforms.
    graphics->Draw(*surface, (ICGraphics2d::Flip)0, NULL);
    graphics->PopTransform();
    if (alpha < ALPHA_OPAQUE) {
        PopAlphaBlend();
    } else {
        graphics->Disable(ICGraphics2d::State_4);
        graphics->PopState(ICGraphics2d::State_4);
    }
}

bool DGHelper::LoadImageSurface(ICRenderSurface** surface, const char* name, bool markLoading)
{
    CResource* resource = NULL;
    ICResourceManager* resources = CApplet::m_pApp->m_resourceManager;
    if (markLoading) {
        m_loadingResource = true;
        resources->CreateResource(name, &resource);
        m_loadingResource = false;
    } else {
        resources->CreateResource(name, &resource);
    }
    if (resource == NULL) {
        return false;
    }
    *surface = static_cast<ICRenderSurface*>(resource->GetData());
    if (*surface == NULL) {
        resources->ReleaseResource(name);
        return false;
    }
    return true;
}

void DGHelper::DrawImage(ICRenderSurface* surface, int x, int y, float alpha, int pivotX, int pivotY,
                         float rotation, unsigned int flags)
{
    DrawImageScaled(surface, 1.0f, 1.0f, x, y, alpha, pivotX, pivotY, rotation, flags);
}

bool DGHelper::LoadAnims(Vector<AnimationImpl*>& animations, const char* name)
{
    CResource* resource = NULL;
    ICResourceManager* resources = (ICResourceManager*)CApplet::m_pApp->m_resourceManager;
    resources->CreateResource(name, &resource);
    if (resource == NULL) {
        return false;
    }
    {
        JDataInputStream stream((CBinary*)resource->GetData());
        AnimationImpl::load(&stream, animations);
        ((ICResourceManager*)CApplet::m_pApp->m_resourceManager)->ReleaseResource(name);
    }
    return resource != NULL;
}

bool DGHelper::CreateAnimatableObject(Group group,
                                      AnimatableSwerveObject** object, const char* name)
{
    CResource* resource = NULL;
    if (group.m_interface == NULL) {
        return false;
    }
    ((ICResourceManager*)CApplet::m_pApp->m_resourceManager)->CreateResource(name, &resource);
    if (resource == NULL) {
        return false;
    }
    {
        JDataInputStream stream((CBinary*)resource->GetData());
        *object = new AnimatableSwerveObject(group, &stream);
        ((ICResourceManager*)CApplet::m_pApp->m_resourceManager)->ReleaseResource(name);
    }
    return resource != NULL;
}

void DGHelper::GetPosition(Transform transform, vec3& position)
{
    vec3 result = GetPosition(transform);
    position.m_x = result.m_x;
    position.m_y = result.m_y;
    position.m_z = result.m_z;
}

vec3 DGHelper::GetPosition(Transform transform)
{
    Transform local = transform;
    float matrix[16];
    local.get(matrix);
    vec3 position;
    position.m_x = matrix[3];
    position.m_y = matrix[7];
    position.m_z = matrix[11];
    return position;
}

int DGHelper::GetTextWidth(const char* text, CFont* font)
{
    int length = strlen(text);
    int width = 0;
    if (length > 0) {
        for (int index = 0; index < length; index++) {
            width += font->GetWidth(text[index]);
        }
        width = font->GetMinCharSpacing() * (length - 1) + width;
    }
    return width;
}

void DGHelper::getUserParameter(Object3D& object, int id, char** value)
{
    if (object == NULL) {
        return;
    }
    int length = object.getUserParameter(id, 0, NULL);
    if (length < 1) {
        return;
    }
    *value = new char[length + 1];
    (*value)[length] = 0;
    object.getUserParameter(id, length, *value);
}

void DGHelper::getUserParameter(Object3D& object, int id, float* value)
{
    char* text = NULL;
    getUserParameter(object, id, &text);
    if (text != NULL) {
        *value = atof(text);
        delete[] text;
        text = NULL;
    }
}

void DGHelper::getUserParameter(Object3D& object, const char* name,
                                char** value)
{
    getUserParameter(object, getSwerveID(name), value);
}

void DGHelper::getUserParameter(Object3D& object, const char* name,
                                float* value)
{
    getUserParameter(object, getSwerveID(name), value);
}

void DGHelper::getUserParameter(Object3D& object, const char* name,
                                int* value)
{
    int id = getSwerveID(name); float number = -1.0f;  // id first, as the original orders it
    getUserParameter(object, id, &number);
    *value = (int)number;
}

float DGHelper::sqrt(float value)
{
    float half = value * 0.5f;
    int bits = *(int*)&value;
    bits = 0x5f3759df - (bits >> 1);
    float inverse = *(float*)&bits;
    inverse = inverse * (1.5f - (inverse * inverse) * half);
    return value * inverse;
}

game_type_t DGHelper::GetGameType()
{
    return m_gamePlayManager.m_gameType;
}

CDH_PlayerData* DGHelper::GetPlayerData()
{
    return &m_gamePlayManager.m_playerData;
}

void DGHelper::LaunchRateMe()
{
    CApplet::m_pCore->LaunchURL(RATE_ME_LINK, 0);
}

XString DGHelper::GetRandomBackgroundResId()
{
    // the original always uses the one key set (no XGA variant exists in the resources)
    CDH_KeySet* keys = (CDH_KeySet*)((App*)WindowApp::m_instance)->m_resourceManager->getResourceData("KEYSET_LOADING_SCREENS", false);
    CRandGen* random = CRandGen::GetInstance();
    int index = random->GetRandRange(0, keys->m_count - 1);
    const char* key = keys->GetKey(index);
    XString result(key);
    ((App*)WindowApp::m_instance)->m_resourceManager->releaseResource("KEYSET_LOADING_SCREENS");
    return result;
}

XString DGHelper::ClockStringFromSeconds(unsigned int seconds)
{
    unsigned int hours = seconds / 3600;
    unsigned int rest = seconds - hours * 3600;
    unsigned int minutes = rest / 60;
    unsigned int secondsOfMinute = rest - minutes * 60;
    XString result;
    if (hours > 0) {
        result.Append(XString::Format(L"%d", hours) + L":");
    }
    if (minutes > 0 || hours > 0) {
        result.Append(XString::Format(L"%02d", minutes) + L":");
    } else if (hours == 0) {
        result.Append(L"0:", wcslen(L"0:") * sizeof(wchar_t));
    }
    result.Append(XString::Format(L"%02d", secondsOfMinute));
    return result;
}

int DGHelper::GetDaysFromTimestamp(unsigned int timestamp)
{
    timelib_time time;
    timelib_time2local(&time, 172800);
    return (time.sse + timestamp) / 86400;
}

XString DGHelper::FormatMoneyString(int money)
{
    wchar_t character;  // declared first: highest stack slot in the original
    XString digits(money);
    XString result;
    for (int index = 0; index < digits.Length(); index++) {
        if (index % 3 == 0 && index >= 1) {
            result = XString(",") + result;
        }
        character = digits.Char(digits.Length() - 1 - index);
        result.InsertData(0, &character, sizeof(wchar_t));
    }
    return result;
}

void DGHelper::SplitStringAndTrim(Vector<XString>& parts, XString& text, const XString& separator)
{
    text.Split(parts, separator.c_str());
    for (int index = 0; index < parts.size(); index++) {
        parts[index] = parts[index].Trim(true, true);
    }
}

bool DGHelper::CropString(XString& name, int maxLength)
{
    if (maxLength < 4) {
        return false;
    }
    if (name.Length() <= maxLength) {
        return false;
    }
    name = name.SubString(0, maxLength - 3);
    name.Append(XString("..."));
    return true;
}

bool DGHelper::ChangeImageRecursively(Object3D object,
                                      Image2D image, const char* name)
{
    bool changed = false;
    Mesh mesh(object);
    if (mesh.m_interface != NULL) {
        int submeshCount = mesh.getSubmeshCount();
        for (int submesh = 0; submesh < submeshCount; submesh++) {
            Appearance appearance = mesh.getAppearance(submesh);
            if (appearance.m_interface != NULL) {
                for (int unit = 0; unit < 2; unit++) {
                    Texture2D texture = appearance.getTexture(unit);
                    if (texture.m_interface != NULL) {
                        if (name == NULL || texture.getUserID() == getSwerveID(name)) {
                            changed = true;
                            texture.setImage(image);
                        }
                    }
                }
            }
        }
    }
    Group group(object);
    if (group.m_interface != NULL) {
        int childCount = group.getChildCount();
        for (int index = 0; index < childCount; index++) {
            Node child = group.getChild(index);
            changed |= ChangeImageRecursively(child, image, name);
        }
    }
    return changed;
}

void DGHelper::SetUserIDRecursively(Object3D object, int userId)
{
    Mesh mesh(object);
    if (mesh.m_interface != NULL) {
        mesh.setUserID(userId);
    }
    Group group(object);
    if (group.m_interface != NULL) {
        int childCount = group.getChildCount();
        for (int index = 0; index < childCount; index++) {
            Node child = group.getChild(index);
            SetUserIDRecursively(child, userId);
        }
    }
}

XString DGHelper::GetFriendName(int index, XString name)
{
    XString friendName;
    if (index != -1) {
        friendName = L"Test User Name";
    }
    return XString(L"");
}

bool DGHelper::CropString(XString& text, CFont* font, int maxWidth)
{
    while (font->MeasureTextWidth(text.c_str(), -1, -1, NULL) >= maxWidth) {
        // drop the last characters (and a previous ellipsis) and mark the cut
        text = text.SubString(0, text.Length() - 6);
        text = text.Append(XString("..."));
    }
    return true;
}
