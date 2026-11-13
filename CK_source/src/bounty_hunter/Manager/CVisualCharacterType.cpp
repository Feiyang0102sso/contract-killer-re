#include "CVisualCharacterType.h"
#define AD_BANNER_SIZES_NO_MARGINS
#include "AdBannerSizes.h"
#undef AD_BANNER_SIZES_NO_MARGINS

#include <stdio.h>
#include <string.h>
#include "AnimationImpl.h"
#include "App.h"
#include "CRandGen.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "ICStdUtil.h"
#include "ResMap.h"

const char* CVisualCharacterType::m_pBodyDummyName = "dummy_guyTransform";
const char* CVisualCharacterType::m_pIdleAnimationPrefix = "idle_";
const char* CVisualCharacterType::m_pAfraidAnimationPrefix = "affride";
const char* CVisualCharacterType::m_pWalkAnimationPrefix = "walk_";
const char* CVisualCharacterType::m_pWoundAnimationPrefix = "wound_";
const char* CVisualCharacterType::m_pDeathFrontAnimationPrefix = "death_shoot_front_";
const char* CVisualCharacterType::m_pDeathBackAnimationPrefix = "death_shoot_back_";
const char* CVisualCharacterType::m_pStunAnimationPrefix = "ultra_sound_pain_";
const char* CVisualCharacterType::m_pAttutideAffix[4] = {"stand", "crouch", "wallR", "wallL"};
const char* CVisualCharacterType::m_pDirectionAffix[2] = {"up", "front"};
const char* CVisualCharacterType::m_pWeaponAffix[4] = {"pistol", "rifle", "shotgun", "mashinegun"};
float CVisualCharacterType::m_walkSpeedMan;
float CVisualCharacterType::m_walkSpeedWoman;
float CVisualCharacterType::m_runSpeedMan;
float CVisualCharacterType::m_runSpeedWoman;
float CVisualCharacterType::m_runWoundSpeed;
Vector<AnimationImpl*> CVisualCharacterType::m_commonAnimation;
const char* CVisualCharacterType::m_commonMapResourceID;
const char* CVisualCharacterType::m_commonAnmResourceID;

namespace {
typedef int (*GetCount)(IQueryInterface*, int*);
typedef int (*GetChild)(IQueryInterface*, int, IQueryInterface**);
typedef int (*SetObject)(IQueryInterface*, IQueryInterface*);

AnimationImpl* AnimationAt(const Vector<AnimationImpl*>& animations, int index)
{
    if (animations.size() < 1) {
        return NULL;
    }
    if (index < 0) {
        index = 0;
    } else if (index >= animations.size()) {
        index = animations.size() - 1;
    }
    return animations[index];
}
}

CVisualCharacterType::CVisualCharacterType(TiXmlNode* node)
    : m_modelCopies(0), m_screenImage(NULL), m_animationTypes(8), m_isWoman(false), m_isMan(true)
{
    memset(m_animations, 0, sizeof(m_animations));
    Parse(node);
}

CVisualCharacterType::~CVisualCharacterType()
{
}

void CVisualCharacterType::Parse(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    ParseInfo(node->FirstChild("info"));
    ParseModel(node->FirstChild("model"));
    node->FirstChild("sounds");
}

void CVisualCharacterType::ParseInfo(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    int gender = CXmlHelper::GetAttributeValue(node, "gender").ToInt();
    m_isMan = gender == 0;
    gender = CXmlHelper::GetAttributeValue(node, "gender").ToInt();
    m_isWoman = gender == 1;
    const char* screenImage = node->ToElement()->Attribute("screenImage");
    if (screenImage == NULL) {
        m_screenImage = 0;
    } else {
        ResMap* resources = static_cast<App*>(WindowApp::m_instance)->InitResMap();
        XString name(screenImage);
        m_screenImage = resources->getResourceID(name.Replace(L"\\n", L"\n", true));
    }
}

void CVisualCharacterType::ParseModel(TiXmlNode* node)
{
    if (node != NULL) {
        m_bodyResource = CXmlHelper::GetAttributeValue(node, "body");
        m_headResource = CXmlHelper::GetAttributeValue(node, "head");
        m_bodyTexture = CXmlHelper::GetAttributeValue(node, "bodyTexture");
        m_headTexture = CXmlHelper::GetAttributeValue(node, "headTexture");
    }
}

void CVisualCharacterType::CleanupResources() const
{
    memset(m_animations, 0, sizeof(m_animations));
    m_idle.removeAllElements();
    m_wound.removeAllElements();
    m_shot.removeAllElements();
    m_hide.removeAllElements();
    m_unhide.removeAllElements();
    m_reload.removeAllElements();
    m_getWeapon.removeAllElements();
    m_deathFront.removeAllElements();
    m_deathBack.removeAllElements();
    m_model.Clear();
}

void CVisualCharacterType::LoadCommonInfo(TiXmlNode* node)
{
    if (node != NULL) {
        m_walkSpeedMan = CXmlHelper::GetAttributeValue(node, "walk_speed_man").ToFloat();
        m_walkSpeedWoman = CXmlHelper::GetAttributeValue(node, "walk_speed_woman").ToFloat();
        m_runSpeedMan = CXmlHelper::GetAttributeValue(node, "run_speed_man").ToFloat();
        m_runSpeedWoman = CXmlHelper::GetAttributeValue(node, "run_speed_woman").ToFloat();
        m_runWoundSpeed = CXmlHelper::GetAttributeValue(node, "run_wound_speed").ToFloat();
    }
}

void CVisualCharacterType::LoadCommonAnimation(TiXmlNode* node)
{
    if (node != NULL) {
        XString map = CXmlHelper::GetAttributeValue(node, "map");
        XString animation = CXmlHelper::GetAttributeValue(node, "anm");
        m_commonMapResourceID = static_cast<App*>(WindowApp::m_instance)->InitResMap()->getResourceID(map);
        m_commonAnmResourceID = static_cast<App*>(WindowApp::m_instance)->InitResMap()->getResourceID(animation);
        static_cast<App*>(WindowApp::m_instance)->m_helper->LoadAnims(m_commonAnimation, m_commonAnmResourceID);
    }
}

void CVisualCharacterType::DestroyCommonAnimation()
{
    SAFE_DELETE_VECTOR(m_commonAnimation);
}

void CVisualCharacterType::LoadModel() const
{
    Object3D body = App::LoadObject3D(XString::AnsiString(m_bodyResource.c_str()));
    Group head;
    {
        XString::AnsiString name(m_headResource.c_str());
        Object3D loadedHead = App::LoadObject3D(name);
        Group headGroup(loadedHead);
        head.m_interface = headGroup.m_interface;
        QueryInterface::AddReference(head.m_interface);
    }
    if (body == NULL || head == NULL) {
        return;
    }
    ReplaceTexture(body, head);
    m_model = body.find(DGHelper::getSwerveID(m_pBodyDummyName));
    if (m_model == NULL) {
        return;
    }
    Group bodyAttachment(body.find(DGHelper::getSwerveID("Player01 HeadTransform")));
    Group headAttachment(head.find(DGHelper::getSwerveID("Player01 HeadTransform")));
    if (bodyAttachment == NULL || headAttachment == NULL) {
        return;
    }
    Group oldParent(headAttachment.getParent());
    if (oldParent != NULL) {
        CKMethod<SetObject>(oldParent.m_interface, 0xc4)(oldParent.m_interface, headAttachment.m_interface);
    }
    CKMethod<SetObject>(bodyAttachment.m_interface, 0xc0)(bodyAttachment.m_interface, headAttachment.m_interface);
}

CVisualCharacterType::Group CVisualCharacterType::GetCharacterModelCopy() const
{
    if (m_model == NULL) {
        LoadModel();
    }
    if (m_modelCopies == 0) {
        ParseAnimation();
    }
    ++m_modelCopies;
    return Group(m_model.duplicate());
}

void CVisualCharacterType::ReleaseCharacterModelCopy() const
{
    // the original does not test for a positive count first
    if (--m_modelCopies == 0) {
        CleanupResources();
    }
}

void CVisualCharacterType::ChangeImageRecursively(Object3D object, Image2D image) const
{
    SwerveHelper::Mesh mesh(object);
    if (mesh != NULL) {
        int count;
        CKMethod<GetCount>(mesh.m_interface, 0xc0)(mesh.m_interface, &count);
        for (int index = 0; index < count; ++index) {
            IQueryInterface* appearanceResult;
            CKMethod<GetChild>(mesh.m_interface, 0xcc)(mesh.m_interface, index, &appearanceResult);
            IQueryInterface* appearance = appearanceResult;
            if (appearance != NULL) {
                IQueryInterface* textureResult;
                CKMethod<GetChild>(appearance, 0x74)(appearance, 0, &textureResult);
                IQueryInterface* texture = textureResult;
                if (texture != NULL) {
                    CKMethod<SetObject>(texture, 0x88)(texture, image.m_interface);
                    texture->Release();
                }
                appearance->Release();
            }
        }
    }
    Group group(object);
    if (group != NULL) {
        int count = group.getChildCount();
        for (int index = 0; index < count; ++index) {
            Node child = group.getChild(index);
            ChangeImageRecursively(child, image);
        }
    }
}

void CVisualCharacterType::ReplaceTexture(Object3D body, Object3D head) const
{
    if (body == NULL || head == NULL) {
        return;
    }
    Image2D bodyImage(App::LoadObject3D(XString::AnsiString(m_bodyTexture.c_str())));
    Image2D headImage(App::LoadObject3D(XString::AnsiString(m_headTexture.c_str())));
    if (bodyImage != NULL && headImage != NULL) {
        ChangeImageRecursively(body, bodyImage);
        ChangeImageRecursively(head, headImage);
    }
}

int CVisualCharacterType::GetEnumerateAnimationCount(const Vector<AnimationImpl*>* animations) const
{
    int count = animations->size();
    if (count < 1) {
        return -1;
    }
    int lastIndex = count - 1;
    int index = CRandGen::GetInstance()->GetRandRange(0, lastIndex);
    if (index < 0) {
        return 0;
    }
    if (index >= count) {
        return lastIndex;
    }
    return index;
}

AnimationImpl* CVisualCharacterType::GetAnimation(EPersonAnimType type) const
{
    switch (type) {
    case 0: {
        int index = GetEnumerateAnimationCount(&m_idle);
        if (index >= 0) {
            return m_idle[index];
        }
        return NULL;
    }
    case 1: {
        int index = GetEnumerateAnimationCount(&m_afraid);
        if (index >= 0) {
            return m_afraid[index];
        }
        return NULL;
    }
    case 2: {
        int index = GetEnumerateAnimationCount(&m_walk);
        if (index >= 0) {
            return m_walk[index];
        }
        return NULL;
    }
    case 13: {
        int index = GetEnumerateAnimationCount(&m_wound);
        if (index >= 0) {
            return m_wound[index];
        }
        return NULL;
    }
    case 14: {
        int index = GetEnumerateAnimationCount(&m_deathFront);
        if (index >= 0) {
            return m_deathFront[index];
        }
        return NULL;
    }
    case 15: {
        int index = GetEnumerateAnimationCount(&m_deathBack);
        if (index >= 0) {
            return m_deathBack[index];
        }
        return NULL;
    }
    case 16: {
        int index = GetEnumerateAnimationCount(&m_stun);
        if (index >= 0) {
            return m_stun[index];
        }
        return NULL;
    }
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
        return m_animations[type];
    default:
        return NULL;
    }
}

AnimationImpl* CVisualCharacterType::GetIdleAnimation(int index) const
{
    if (index < 0 || index >= m_idle.size()) {
        index = 0;
    }
    return m_idle[index];
}

AnimationImpl* CVisualCharacterType::GetBossAfraidAnimation() const
{
    // the original returns NULL for a negative index and does not clamp
    int index = GetEnumerateAnimationCount(&m_afraid);
    if (index < 0) {
        return NULL;
    }
    if (index == 1) {
        index = 0;
    }
    return m_afraid[index];
}

AnimationImpl* CVisualCharacterType::GetShotAnimation(EPersonAnimShotAttitudeType attitude,
                                                     EPersonAnimShotDirectionType direction,
                                                     EPersonAnimWeaponType weapon) const
{
    return AnimationAt(m_shot, attitude * 8 + direction * 4 + weapon);
}

__attribute__((noinline))
AnimationImpl* CVisualCharacterType::GetHideAndReloadAnimationImpl(const Vector<AnimationImpl*>* animations,
                                                                  EPersonAnimShotAttitudeType attitude,
                                                                  EPersonAnimWeaponType weapon) const
{
    return AnimationAt(*animations, attitude * 4 + weapon);
}

AnimationImpl* CVisualCharacterType::GetHideAnimation(EPersonAnimShotAttitudeType attitude, EPersonAnimWeaponType weapon) const
{
    return GetHideAndReloadAnimationImpl(&m_hide, attitude, weapon);
}

AnimationImpl* CVisualCharacterType::GetUnhideAnimation(EPersonAnimShotAttitudeType attitude, EPersonAnimWeaponType weapon) const
{
    return GetHideAndReloadAnimationImpl(&m_unhide, attitude, weapon);
}

AnimationImpl* CVisualCharacterType::GetReloadAnimation(EPersonAnimShotAttitudeType attitude, EPersonAnimWeaponType weapon) const
{
    return GetHideAndReloadAnimationImpl(&m_reload, attitude, weapon);
}

AnimationImpl* CVisualCharacterType::GetTakeOutWeaponAnimation(EPersonAnimWeaponType weapon) const
{
    return AnimationAt(m_getWeapon, weapon);
}

EPersonAnimType CVisualCharacterType::GetAnimationTypeByName(const char* name) const
{
    // the original does not check for an unknown name
    return *m_animationTypes.Get(XString(name));
}

void CVisualCharacterType::ParseAnimation() const
{
    for (int index = 0; index < 23; ++index) {
        switch (index) {
        case 0:
            ParseEnumerateAnimation(&m_idle, m_pIdleAnimationPrefix, static_cast<EPersonAnimType>(0));
            m_animations[index] = NULL;
            break;
        case 1:
            ParseEnumerateAnimation(&m_afraid, m_pAfraidAnimationPrefix, static_cast<EPersonAnimType>(1));
            m_animations[index] = NULL;
            break;
        case 2:
            ParseEnumerateAnimation(&m_walk, m_pWalkAnimationPrefix, static_cast<EPersonAnimType>(2));
            m_animations[index] = NULL;
            break;
        case 3:
            m_animations[index] = AnimationImpl::getAnimationByName("girl_walk", m_commonAnimation);
            m_animationTypes.Set(XString("girl_walk"), static_cast<EPersonAnimType>(3));
            break;
        case 4:
            m_animations[index] = AnimationImpl::getAnimationByName("run_simple", m_commonAnimation);
            m_animationTypes.Set(XString("run_simple"), static_cast<EPersonAnimType>(4));
            break;
        case 5:
            m_animations[index] = AnimationImpl::getAnimationByName("girl_run", m_commonAnimation);
            m_animationTypes.Set(XString("girl_run"), static_cast<EPersonAnimType>(5));
            break;
        case 6:
            m_animations[index] = AnimationImpl::getAnimationByName("run_crouch", m_commonAnimation);
            m_animationTypes.Set(XString("run_crouch"), static_cast<EPersonAnimType>(6));
            break;
        case 7:
            m_animations[index] = AnimationImpl::getAnimationByName("run_wound", m_commonAnimation);
            m_animationTypes.Set(XString("run_wound"), static_cast<EPersonAnimType>(7));
            break;
        case 8:
            ParseShotAnimation();
            m_animations[index] = NULL;
            break;
        default:
            // Each of the three hide/unhide/reload types triggers the full parse.
            ParseHideUnhideAndReloadAnimation();
            m_animations[index] = NULL;
            break;
        case 12:
            ParseGetWeaponAnimation();
            m_animations[index] = NULL;
            break;
        case 13:
            ParseEnumerateAnimation(&m_wound, m_pWoundAnimationPrefix, static_cast<EPersonAnimType>(13));
            m_animations[index] = NULL;
            break;
        case 14:
            ParseEnumerateAnimation(&m_deathFront, m_pDeathFrontAnimationPrefix, static_cast<EPersonAnimType>(14));
            m_animations[index] = NULL;
            break;
        case 15:
            ParseEnumerateAnimation(&m_deathBack, m_pDeathBackAnimationPrefix, static_cast<EPersonAnimType>(15));
            m_animations[index] = NULL;
            break;
        case 16:
            ParseEnumerateAnimation(&m_stun, m_pStunAnimationPrefix, static_cast<EPersonAnimType>(16));
            m_animations[index] = NULL;
            break;
        case 17:
            m_animations[index] = AnimationImpl::getAnimationByName("tranquilise", m_commonAnimation);
            m_animationTypes.Set(XString("tranquilise"), static_cast<EPersonAnimType>(17));
            break;
        case 18:
            m_animations[index] = AnimationImpl::getAnimationByName("start_burn", m_commonAnimation);
            m_animationTypes.Set(XString("start_burn"), static_cast<EPersonAnimType>(18));
            break;
        case 19:
            m_animations[index] = AnimationImpl::getAnimationByName("cyckle_burn", m_commonAnimation);
            m_animationTypes.Set(XString("cyckle_burn"), static_cast<EPersonAnimType>(19));
            break;
        case 20:
            m_animations[index] = AnimationImpl::getAnimationByName("end_burn", m_commonAnimation);
            m_animationTypes.Set(XString("end_burn"), static_cast<EPersonAnimType>(20));
            break;
        case 21:
            m_animations[index] = AnimationImpl::getAnimationByName("bullet_hit", m_commonAnimation);
            m_animationTypes.Set(XString("bullet_hit"), static_cast<EPersonAnimType>(21));
            break;
        case 22:
            m_animations[index] = AnimationImpl::getAnimationByName("bullet_hit_short", m_commonAnimation);
            m_animationTypes.Set(XString("bullet_hit_short"), static_cast<EPersonAnimType>(22));
            break;
        }
    }
}

void CVisualCharacterType::ParseEnumerateAnimation(Vector<AnimationImpl*>* animations,
                                                  const char* prefix, EPersonAnimType type) const
{
    char format[100];
    ICStdUtil::SPrintF(format, "%s%s", prefix, "%02u");
    int count = -1;
    char probe[100];
    do {
        ++count;
        ICStdUtil::SPrintF(probe, format, count + 1);
    } while (AnimationImpl::getAnimationByName(probe, m_commonAnimation) != NULL);
    if (count > 0) {
        animations->ensureCapacity(count);
        for (int index = 0; index < count; ++index) {
            char name[100];
            ICStdUtil::SPrintF(name, format, index + 1);
            AnimationImpl* animation = AnimationImpl::getAnimationByName(name, m_commonAnimation);
            animations->addElement(animation);
            m_animationTypes.Set(XString(name), type);
        }
    }
}

void CVisualCharacterType::ParseShotAnimation() const
{
    char name[200];
    m_shot.ensureCapacity(32);
    for (int attitude = 0; attitude < 4; ++attitude) {
        for (int direction = 0; direction < 2; ++direction) {
            for (int weapon = 0; weapon < 4; ++weapon) {
                ICStdUtil::SPrintF(name, "fire_%s_%s_%s", m_pAttutideAffix[attitude], m_pDirectionAffix[direction],
                         m_pWeaponAffix[weapon]);
                AnimationImpl* animation = AnimationImpl::getAnimationByName(name, m_commonAnimation);
                if (animation == NULL) {
                    animation = AnimationImpl::getAnimationByName("fire_stand_mashinegun_front", m_commonAnimation);
                }
                m_shot.addElement(animation);
                m_animationTypes.Set(XString(name), static_cast<EPersonAnimType>(8));
            }
        }
    }
}

void CVisualCharacterType::ParseHideUnhideAndReloadAnimation() const
{
    m_hide.ensureCapacity(16);
    m_unhide.ensureCapacity(16);
    m_reload.ensureCapacity(16);
    char name[200];
    for (int attitude = 0; attitude < 4; ++attitude) {
        for (int weapon = 0; weapon < 4; ++weapon) {
            const char* attitudeName = m_pAttutideAffix[attitude];
            const char* weaponName = m_pWeaponAffix[weapon];

            ICStdUtil::SPrintF(name, "hide_%s_%s", attitudeName, weaponName);
            m_hide.addElement(AnimationImpl::getAnimationByName(name, m_commonAnimation));
            m_animationTypes.Set(XString(name), static_cast<EPersonAnimType>(9));

            ICStdUtil::SPrintF(name, "unhide_%s_%s", attitudeName, weaponName);
            m_unhide.addElement(AnimationImpl::getAnimationByName(name, m_commonAnimation));
            m_animationTypes.Set(XString(name), static_cast<EPersonAnimType>(10));

            ICStdUtil::SPrintF(name, "reload_%s_%s", attitudeName, weaponName);
            m_reload.addElement(AnimationImpl::getAnimationByName(name, m_commonAnimation));
            m_animationTypes.Set(XString(name), static_cast<EPersonAnimType>(11));
        }
    }
}

void CVisualCharacterType::ParseGetWeaponAnimation() const
{
    char name[200];
    m_getWeapon.ensureCapacity(4);
    for (int weapon = 0; weapon < 4; ++weapon) {
        ICStdUtil::SPrintF(name, "get_%s", m_pWeaponAffix[weapon]);
        m_getWeapon.addElement(AnimationImpl::getAnimationByName(name, m_commonAnimation));
        m_animationTypes.Set(XString(name), static_cast<EPersonAnimType>(12));
    }
}

typedef char AssertVisualCharacterTypeSize[sizeof(CVisualCharacterType) == 0x14c ? 1 : -1];
