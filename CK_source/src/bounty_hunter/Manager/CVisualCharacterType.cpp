#include "CVisualCharacterType.h"

#include <stdio.h>
#include <string.h>
#include "AnimationImpl.h"
#include "App.h"
#include "CRandGen.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "ResMap.h"

const char* CVisualCharacterType::m_pBodyDummyName = "dummy_guyTransform";
float CVisualCharacterType::m_walkSpeedMan;
float CVisualCharacterType::m_walkSpeedWoman;
float CVisualCharacterType::m_runSpeedMan;
float CVisualCharacterType::m_runSpeedWoman;
float CVisualCharacterType::m_runWoundSpeed;
Vector<AnimationImpl*> CVisualCharacterType::m_commonAnimation;
const char* CVisualCharacterType::m_commonMapResourceID;
const char* CVisualCharacterType::m_commonAnmResourceID;

namespace {
const char* const ShotAttitudes[] = {"stand", "crouch", "wallR", "wallL"};
const char* const ShotDirections[] = {"up", "front"};
const char* const ShotWeapons[] = {"pistol", "rifle", "shotgun", "mashinegun"};

typedef int (*GetCount)(IQueryInterface*, int*);
typedef int (*GetChild)(IQueryInterface*, int, IQueryInterface**);
typedef int (*GetObject)(IQueryInterface*, IQueryInterface**);
typedef int (*SetObject)(IQueryInterface*, IQueryInterface*);

AnimationImpl* AnimationAt(const Vector<AnimationImpl*>& animations, int index)
{
    if (animations.size() == 0) {
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
}

void CVisualCharacterType::ParseInfo(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    int gender = CXmlHelper::GetAttributeValue(node, "gender").ToInt();
    m_isWoman = gender == 1;
    m_isMan = gender == 0;
    const char* screenImage = node->ToElement()->Attribute("screenImage");
    if (screenImage != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        m_screenImage = app->InitResMap()->getResourceID(XString(screenImage).Replace(L"\\n", L"\n", true));
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
    m_afraid.removeAllElements();
    m_walk.removeAllElements();
    m_wound.removeAllElements();
    m_shot.removeAllElements();
    m_hide.removeAllElements();
    m_unhide.removeAllElements();
    m_reload.removeAllElements();
    m_getWeapon.removeAllElements();
    m_deathFront.removeAllElements();
    m_deathBack.removeAllElements();
    m_stun.removeAllElements();
    m_animationTypes.Clear();
    m_model = Object3D();
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
        App* app = static_cast<App*>(WindowApp::m_instance);
        ResMap* resources = app->InitResMap();
        m_commonMapResourceID = resources->getResourceID(CXmlHelper::GetAttributeValue(node, "map"));
        m_commonAnmResourceID = resources->getResourceID(CXmlHelper::GetAttributeValue(node, "anm"));
        if (m_commonAnmResourceID != NULL) {
            app->m_helper->LoadAnims(m_commonAnimation, m_commonAnmResourceID);
        }
    }
}

void CVisualCharacterType::DestroyCommonAnimation()
{
    SAFE_DELETE_VECTOR(m_commonAnimation);
}

void CVisualCharacterType::LoadModel() const
{
    XString::AnsiString bodyName(m_bodyResource.c_str());
    XString::AnsiString headName(m_headResource.c_str());
    Object3D body = App::LoadObject3D(bodyName);
    Group head(App::LoadObject3D(headName));
    if (body == NULL || head == NULL) {
        return;
    }
    ReplaceTexture(body, head);
    m_model = body.find(DGHelper::getSwerveID(m_pBodyDummyName));
    if (m_model == NULL) {
        return;
    }
    int headTransform = DGHelper::getSwerveID("Player01 HeadTransform");
    Group bodyAttachment(body.find(headTransform));
    Group headAttachment(head.find(headTransform));
    if (bodyAttachment == NULL || headAttachment == NULL) {
        return;
    }
    IQueryInterface* parent = NULL;
    CKMethod<GetObject>(headAttachment.m_interface, 0x80)(headAttachment.m_interface, &parent);
    Group oldParent((Object3D(parent)));
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
    if (m_model == NULL) {
        return Group();
    }
    if (m_modelCopies == 0) {
        ParseAnimation();
    }
    IQueryInterface* copy = NULL;
    CKMethod<GetObject>(m_model.m_interface, 0x48)(m_model.m_interface, &copy);
    Group result((Object3D(copy)));
    if (result != NULL) {
        ++m_modelCopies;
    }
    return result;
}

void CVisualCharacterType::ReleaseCharacterModelCopy() const
{
    if (m_modelCopies > 0 && --m_modelCopies == 0) {
        CleanupResources();
    }
}

void CVisualCharacterType::ChangeImageRecursively(Object3D object, Image2D image) const
{
    if (object == NULL) {
        return;
    }
    SwerveHelper::Mesh mesh(object);
    if (mesh != NULL) {
        int count = 0;
        CKMethod<GetCount>(mesh.m_interface, 0xc0)(mesh.m_interface, &count);
        for (int index = 0; index < count; ++index) {
            IQueryInterface* appearance = NULL;
            CKMethod<GetChild>(mesh.m_interface, 0xcc)(mesh.m_interface, index, &appearance);
            if (appearance != NULL) {
                IQueryInterface* texture = NULL;
                CKMethod<GetChild>(appearance, 0x74)(appearance, 0, &texture);
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
        int count = 0;
        CKMethod<GetCount>(group.m_interface, 0xb8)(group.m_interface, &count);
        for (int index = 0; index < count; ++index) {
            IQueryInterface* child = NULL;
            CKMethod<GetChild>(group.m_interface, 0xbc)(group.m_interface, index, &child);
            ChangeImageRecursively(Object3D(child), image);
        }
    }
}

void CVisualCharacterType::ReplaceTexture(Object3D body, Object3D head) const
{
    if (body == NULL || head == NULL) {
        return;
    }
    XString::AnsiString bodyName(m_bodyTexture.c_str());
    XString::AnsiString headName(m_headTexture.c_str());
    Image2D bodyImage(App::LoadObject3D(bodyName));
    Image2D headImage(App::LoadObject3D(headName));
    if (bodyImage != NULL && headImage != NULL) {
        ChangeImageRecursively(body, bodyImage);
        ChangeImageRecursively(head, headImage);
    }
}

int CVisualCharacterType::GetEnumerateAnimationCount(const Vector<AnimationImpl*>* animations) const
{
    int count = animations->size();
    if (count == 0) {
        return -1;
    }
    return CRandGen::GetInstance()->GetRandRange(0, count - 1);
}

AnimationImpl* CVisualCharacterType::GetAnimation(EPersonAnimType type) const
{
    const Vector<AnimationImpl*>* animations = NULL;
    switch (type) {
    case 0: animations = &m_idle; break;
    case 1: animations = &m_afraid; break;
    case 2: animations = &m_walk; break;
    case 13: animations = &m_wound; break;
    case 14: animations = &m_deathFront; break;
    case 15: animations = &m_deathBack; break;
    case 16: animations = &m_stun; break;
    default:
        if (static_cast<unsigned int>(type) < 23) {
            return m_animations[type];
        }
        return NULL;
    }
    int index = GetEnumerateAnimationCount(animations);
    if (index >= 0) {
        return (*animations)[index];
    }
    return NULL;
}

AnimationImpl* CVisualCharacterType::GetIdleAnimation(int index) const
{
    if (index < 0 || index >= m_idle.size()) {
        index = 0;
    }
    return AnimationAt(m_idle, index);
}

AnimationImpl* CVisualCharacterType::GetBossAfraidAnimation() const
{
    int index = GetEnumerateAnimationCount(&m_afraid);
    if (index == 1) {
        index = 0;
    }
    return AnimationAt(m_afraid, index);
}

AnimationImpl* CVisualCharacterType::GetShotAnimation(EPersonAnimShotAttitudeType attitude,
                                                     EPersonAnimShotDirectionType direction,
                                                     EPersonAnimWeaponType weapon) const
{
    return AnimationAt(m_shot, attitude * 8 + direction * 4 + weapon);
}

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
    const EPersonAnimType* type = m_animationTypes.Get(XString(name));
    if (type != NULL) {
        return *type;
    }
    return PERSON_ANIM_TYPE_DEFAULT;
}

void CVisualCharacterType::ParseAnimation() const
{
    ParseEnumerateAnimation(&m_idle, "idle_", static_cast<EPersonAnimType>(0));
    ParseEnumerateAnimation(&m_afraid, "affride", static_cast<EPersonAnimType>(1));
    ParseEnumerateAnimation(&m_walk, "walk_", static_cast<EPersonAnimType>(2));
    ParseShotAnimation();
    ParseHideUnhideAndReloadAnimation();
    ParseGetWeaponAnimation();
    ParseEnumerateAnimation(&m_wound, "wound_", static_cast<EPersonAnimType>(13));
    ParseEnumerateAnimation(&m_deathFront, "death_shoot_front_", static_cast<EPersonAnimType>(14));
    ParseEnumerateAnimation(&m_deathBack, "death_shoot_back_", static_cast<EPersonAnimType>(15));
    ParseEnumerateAnimation(&m_stun, "ultra_sound_pain_", static_cast<EPersonAnimType>(16));

    const struct {
        int type;
        const char* name;
    } clips[] = {
        {3, "girl_walk"}, {4, "run_simple"}, {5, "girl_run"}, {6, "run_crouch"}, {7, "run_wound"},
        {17, "tranquilise"}, {18, "start_burn"}, {19, "cyckle_burn"}, {20, "end_burn"},
        {21, "bullet_hit"}, {22, "bullet_hit_short"}
    };
    for (unsigned int index = 0; index < sizeof(clips) / sizeof(clips[0]); ++index) {
        int type = clips[index].type;
        const char* name = clips[index].name;
        m_animations[type] = AnimationImpl::getAnimationByName(name, m_commonAnimation);
        m_animationTypes.Set(XString(name), static_cast<EPersonAnimType>(type));
    }
}

void CVisualCharacterType::ParseEnumerateAnimation(Vector<AnimationImpl*>* animations,
                                                  const char* prefix, EPersonAnimType type) const
{
    char name[100];
    for (unsigned int index = 1; ; ++index) {
        snprintf(name, sizeof(name), "%s%02u", prefix, index);
        AnimationImpl* animation = AnimationImpl::getAnimationByName(name, m_commonAnimation);
        if (animation == NULL) {
            break;
        }
        animations->addElement(animation);
        m_animationTypes.Set(XString(name), type);
    }
}

void CVisualCharacterType::ParseShotAnimation() const
{
    AnimationImpl* fallback = AnimationImpl::getAnimationByName("fire_stand_mashinegun_front", m_commonAnimation);
    char name[100];
    m_shot.ensureCapacity(32);
    for (int attitude = 0; attitude < 4; ++attitude) {
        for (int direction = 0; direction < 2; ++direction) {
            for (int weapon = 0; weapon < 4; ++weapon) {
                snprintf(name, sizeof(name), "fire_%s_%s_%s", ShotAttitudes[attitude], ShotDirections[direction],
                         ShotWeapons[weapon]);
                AnimationImpl* animation = AnimationImpl::getAnimationByName(name, m_commonAnimation);
                if (animation == NULL) {
                    animation = fallback;
                }
                m_shot.addElement(animation);
                m_animationTypes.Set(XString(name), static_cast<EPersonAnimType>(8));
            }
        }
    }
}

void CVisualCharacterType::ParseHideUnhideAndReloadAnimation() const
{
    const char* const actions[] = {"hide", "unhide", "reload"};
    Vector<AnimationImpl*>* animations[] = {&m_hide, &m_unhide, &m_reload};
    char name[100];
    for (int action = 0; action < 3; ++action) {
        animations[action]->ensureCapacity(16);
        for (int attitude = 0; attitude < 4; ++attitude) {
            for (int weapon = 0; weapon < 4; ++weapon) {
                snprintf(name, sizeof(name), "%s_%s_%s", actions[action], ShotAttitudes[attitude], ShotWeapons[weapon]);
                animations[action]->addElement(AnimationImpl::getAnimationByName(name, m_commonAnimation));
                m_animationTypes.Set(XString(name), static_cast<EPersonAnimType>(9 + action));
            }
        }
    }
}

void CVisualCharacterType::ParseGetWeaponAnimation() const
{
    char name[100];
    m_getWeapon.ensureCapacity(4);
    for (int weapon = 0; weapon < 4; ++weapon) {
        snprintf(name, sizeof(name), "get_%s", ShotWeapons[weapon]);
        m_getWeapon.addElement(AnimationImpl::getAnimationByName(name, m_commonAnimation));
        m_animationTypes.Set(XString(name), static_cast<EPersonAnimType>(12));
    }
}

typedef char AssertVisualCharacterTypeSize[sizeof(CVisualCharacterType) == 0x14c ? 1 : -1];
