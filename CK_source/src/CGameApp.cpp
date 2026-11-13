// CGameApp.cpp - game application and resource selection.
#include "CGameApp.h"
#include "AdColony_Facade.h"
#include "App.h"
#include "AppNode.h"
#include "AppSettings.h"
#include "CAppExecutor.h"
#include "CAppProperties.h"
#include "CApplet.h"
#include "CBH_Player.h"
#include "CCrc32.h"
#include "CDemoMgr.h"
#include "CExecutableRegistry.h"
#include "CEventListener.h"
#include "CFileInputStream.h"
#include "CFileUtil.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CIAPCurrencyManager.h"
#include "CInputMap.h"
#include "CMessage.h"
#include "CPHInterface.h"
#include "CProfileManager.h"
#include "CRandGen.h"
#include "CRegistryAccelerateHandleQuery.h"
#include "ICAdManager.h"
#include "ICCore.h"
#include "ICFileMgr.h"
#include "ICGraphics.h"
#include "ICInAppPurchase.h"
#include "AdBannerSizes.h"
#include "ICLicenseMgr.h"
#include "ICMediaPlayer.h"
#include "ICMoviePlayer.h"
#include "ICNotification.h"
#include "ICRenderSurface.h"
#include "CResourceFactory.h"
#include "CResourceAggregate.h"
#include "CResourceBigFile.h"
#include "CResourceBinary.h"
#include "CResourceDIB.h"
#include "CResourceKeyset.h"
#include "CResourceMedia.h"
#include "CResourceManager_v2.h"
#include "CResourcePalette.h"
#include "CResourceRenderSurface.h"
#include "CResourceShader.h"
#include "CResourceStrWChar.h"
#include "TapjoyInterface.h"
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

bool CGameApp::syncServerFlag;
bool CGameApp::gserveDataConflict;
bool CGameApp::m_wasMusicEnabled;
bool CGameApp::m_wasSoundEnabled;

static const unsigned int CGameApp_InterestingEvents[] = {
    0x67542658, 0x36754280, 0x8408f369, 0x6740942f, 0x54246de3, 0x75424363,
    0x518e1a3b, 0x5152a73b, 0x695fb416, 0x1e96b3a4, 0x0096b3eb, 0xc7424be6,
    0x43a39818, 0x8251b8de, 0x8918794c, 0x52a91971, 0x7b8428cd, 0x35c43e56,
    0x35cb8f1c, 0x35c38d1c, 0x5707b968, 0xad174cef, 0x32b482c3, 0x24314258,
    0x3f972120, 0x972028c1, 0x9720a3cb, 0x7214ff0d, 0x2074d009, 0x97204784,
    0x5f61a84a, 0xd45f52de, 0x20174fb7, 0x172ebf3e, 0x172e3434, 0x71fd83f5,
    0x172e9965, 0x172ec5fa, 0x71ff1054, 0x0173eaf6, 0xd0467c9b, 0x016703b6,
    0x490287b4, 0x9036abf0, 0xbd152e06, 0x152f0ca3, 0xbd152f06, 0x152e0ca3,
    0xfe50251e, 0xfe50251d, 0xfe50251c, 0xfe50251b, 0x9f523292, 0x385a651b, 0
};

extern const wchar_t GAME_RES_FILE[];

static const wchar_t IAP_ITEM_FORMAT[] = L"%02d_%s";
static const wchar_t* const CK_PUSH_APP_KEY = L"XJAkth2_Rq-Tw5WxWW7aOQ";
static const wchar_t* const CK_PUSH_APP_SECRET = L"gQwU19wfSVqQ-4ApTjV7WQ";

CApp* CApp::CreateInstance()
{
    return new CGameApp();
}

inline CGameApp::CGameApp()
    : m_renderEnabled(1)
{
    m_eventListener = NULL;
    m_rootNode = NULL;
}

CGameApp::~CGameApp()
{
}

CGameApp* CGameApp::GetInstance()
{
    return static_cast<CGameApp*>(CApplet::m_pApp);
}

void CAppExecutor::OnExecute()
{
    if (m_modeQualifier != ModeQualifier_None) {
        return;
    }
    if (m_mode != Mode_Resuming) {
        if (m_mode != Mode_Running) {
            return;
        }
        m_registry->Run();
        CGameApp::GetInstance()->HandleUpdate();
        m_mode = Mode_Resuming;
    }
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) == GL_FRAMEBUFFER_COMPLETE_OES) {
        CGameApp::GetInstance()->HandleRender();
        m_mode = Mode_Running;
    } else {
        ICGraphics::GetInstance()->SwapBuffers();
    }
}

int CAppExecutor::InitRegistry()
{
    CExecutableRegistry* registry = m_registry;
    registry->Add(*registry->CreateSystemElement(0x957ce610, new CExecutableRegistry(), CExecutableRegistry::ClassId, 0x3fffffff));
    registry->Add(*registry->CreateSystemElement(0x92484fb2, new CInputMap(), CInputMap::ClassId, 0x3fffffff));
    registry->Add(*registry->CreateSystemElement(0xc84e9c27, new CInputMap(), CInputMap::ClassId, 0x3fffffff));
    return 0;
}

void CAppExecutor::ReleaseRegistry()
{
    CExecutableRegistry* registry = m_registry;
    CSystemElement* registered = NULL;
    CSystemElement* element = registry->m_head;
    while (element != registry->GetSentinel()) {
        CClass* object = static_cast<CClass*>(element->GetObject());
        if (object->m_classId == CEvent::ClassId || object->m_classId == CMessage::ClassId) {
            delete object;
            CSystemElement* next = element->m_next;
            registry->Remove(*element);
            if (next == NULL) {
                next = element;
            }
            delete element;
            element = next;
        } else if (element->m_next != NULL) {
            element = element->m_next;
        }
    }

    if (registry->Query(0, 0xc84e9c27, &registered)) {
        registry->Remove(*registered);
        delete static_cast<CClass*>(registered->GetObject());
        delete registered;
    }
    if (registry->Query(0, 0x92484fb2, &registered)) {
        registry->Remove(*registered);
        delete static_cast<CClass*>(registered->GetObject());
        delete registered;
    }
    if (registry->Query(0, 0x957ce610, &registered)) {
        registry->Remove(*registered);
        delete static_cast<CClass*>(registered->GetObject());
        delete registered;
    }
}

unsigned char AccelerateEventSpecificListenerQuery(unsigned int eventId, int& buckets, int& poolSize)
{
    buckets = 0;
    poolSize = 0;
    return 0;
}

unsigned char CGameApp::EventCB(CEvent& event, void* listener)
{
    CGameApp* app = static_cast<CGameApp*>(static_cast<CEventListener*>(listener)->m_object);
    const CMessage& message = static_cast<const CMessage&>(event);
    return app->HandleEvent(event.m_eventId, (unsigned int)message.m_arguments[0].m_data,
                            (unsigned int)message.m_arguments[1].m_data);
}

int CGameApp::InitRegistry()
{
    CRegistry* registry = static_cast<CRegistry*>(CApplet::m_pApp->m_registry);
    CRegistry* eventRegistry = new CRegistry();
    eventRegistry->SetAccelerator(new CRegistryAccelerateHandleQuery(150, 50));
    registry->Add(*registry->CreateSystemElement(kEventSystemHandle, eventRegistry, CRegistry::ClassId, 0x3fffffff));
    return 0;
}

void CGameApp::ReleaseRegistry()
{
    CRegistry* registry = static_cast<CRegistry*>(CApplet::m_pApp->m_registry);
    CSystemElement* entry = NULL;
    if (registry->Query(0, kEventSystemHandle, &entry)) {
        registry->Remove(*entry);
        delete static_cast<CClass*>(entry->GetObject());
        delete entry;
    }
}

void CGameApp::CreatePools() const
{
    CEventPool::GetInstance()->Init(15, sizeof(CEvent), 1);
    CMessagePool::GetInstance()->Init(15, sizeof(CMessage), 1);
}

void CGameApp::DestroyPools() const
{
    CMessagePool* messagePool = CMessagePool::GetInstance();
    delete messagePool;
    CEventPool* eventPool = CEventPool::GetInstance();
    delete eventPool;
}

unsigned char CGameApp::HandleEvent(unsigned int eventId, unsigned int param1, unsigned int param2)
{
    switch (eventId) {
    case SystemEvent_Start: {
        CApplet::m_pApp->GetExecutor()->SetTimerMillis(5);
        CAppProperties* properties = CAppProperties::GetInstance();
        CStrWChar path;
        CFileUtil::GetApplicationDataPathForFile(path, L"properties.dat");
        if (!ICFileMgr::GetInstance()->Exists(path.GetWCharPtr())) {
            CFileUtil::GetApplicationPathForFile(path, L"properties.dat");
        }
        if (ICFileMgr::GetInstance()->Exists(path.GetWCharPtr())) {
            CFileInputStream stream;
            if (stream.Open(path.GetWCharPtr())) {
                properties->m_properties.CollectOverriddenKeysInto(&properties->m_overriddenKeys);
                properties->m_properties.Load(stream, 0);
            }
        }
        CDemoMgr::GetInstance()->Read();
        m_rootNode->Start();
        return 1;
    }
    case SystemEvent_Stop:
        m_rootNode->Stop();
        ICMediaPlayer::GetInstance()->Stop(0);
        ICMediaPlayer::GetInstance()->StopVibrate(0);
        return 1;
    case SystemEvent_Suspend:
        ICMediaPlayer::GetInstance()->HandleSuspend();
        m_rootNode->HandleEvent(eventId, param1, param2);
        return 1;
    case SystemEvent_Resume:
        ICMediaPlayer::GetInstance()->HandleResume();
        m_rootNode->HandleEvent(eventId, param1, param2);
        return 1;
    case 0x5152a73b: {
        unsigned char handled = m_rootNode->HandleEvent(eventId, param1, param2);
        CApplet::m_pCore->ExitApp(ICCore::ExitStatus_Normal);
        return handled;
    }
    case 0x695fb416:
        return 0;
    case 0xc7424be6:
    case 0x43a39818:
    case 0x0096b3eb:
    case 0x1e96b3a4:
    case 0x32b482c3:
    case 0x518e1a3b:
        break;
    default: {
        ICAdManager* ads = ICAdManager::GetInstance();
        if (ads->IsVisible() && ads->IsFullScreen()) {
            return 1;
        }
        break;
    }
    }
    return m_rootNode->HandleEvent(eventId, param1, param2);
}

void CGameApp::OnSystemEvent(unsigned int eventId, unsigned int param1, unsigned int param2)
{
    CMessage* message = new CMessage(this, eventId, 0, 2,
                                    (unsigned int)CMessage::ArgumentType_UInt32, 0u, (void*)param1,
                                    (unsigned int)CMessage::ArgumentType_UInt32, 0u, (void*)param2);
    message->Run();
    switch (eventId) {
    case SystemEvent_Start:
        ICGraphics::GetInstance()->ClearBuffers(ICGraphics::ClearFlag_Color |
                                               ICGraphics::ClearFlag_Depth |
                                               ICGraphics::ClearFlag_Stencil);
        m_executor->OnStart();
        break;
    case SystemEvent_Stop:
        AdColony_Facade::release();
        m_executor->OnStop();
        break;
    case SystemEvent_Suspend:
        m_executor->OnSuspend();
        break;
    case SystemEvent_Resume:
        m_executor->OnResume();
        TapjoyInterface::onResume();
        break;
    }
}

void CGameApp::ReInit()
{
    ICInAppPurchase* purchase = ICInAppPurchase::GetInstance();
    delete purchase;
    ICInAppPurchase::GetInstance()->Init(this);
}

void CGameApp::OnTapjoyVideoStateChanged(unsigned char active, void* context)
{
    if (active) {
        m_wasMusicEnabled = static_cast<App*>(WindowApp::m_instance)->m_settings->IsMusicEnabled();
        m_wasSoundEnabled = static_cast<App*>(WindowApp::m_instance)->m_settings->IsSoundEnabled();
        static_cast<App*>(WindowApp::m_instance)->m_settings->SetMusicEnabled(false);
        static_cast<App*>(WindowApp::m_instance)->m_settings->SetSoundEnabled(false);
    } else {
        static_cast<App*>(WindowApp::m_instance)->m_settings->SetMusicEnabled(m_wasMusicEnabled);
        static_cast<App*>(WindowApp::m_instance)->m_settings->SetSoundEnabled(m_wasSoundEnabled);
    }
}

int CGameApp::OnInit()
{
    XString::Init();
    CStrWChar platform = CApplet::m_pCore->GetPlatformIdentifier();
    if (wcscmp(platform.GetWCharPtr(), L"iPhone1,2") == 0 ||
        wcscmp(platform.GetWCharPtr(), L"iPod1,2") == 0 ||
        wcscmp(platform.GetWCharPtr(), L"iPod2,1") == 0) {
        App::m_platformType = 0;
    }

    CHash* singletons = new CHash;
    singletons->Init(20, 20, 1);
    m_singletons = singletons;
    CreatePools();
    m_registry = new CRegistry();
    if (m_registry != NULL) {
        InitRegistry();
    }

    m_resourceManager = new CResourceManager_v2();
    if (m_resourceManager == NULL) {
        OnFree();
        return 1;
    }
    int initialized = m_resourceManager->Initialize(CResourceManager::DefaultTableSize,
                                                    CResourceManager::DefaultNumOfElementsInInitialAlloc, 1);
    CResource* resource;
    if (CApplet::m_pCore->CanDetectLanguage()) {
        CStrChar language;
        CApplet::m_pCore->DetectLanguage(language);
        CResourceManager* manager = m_resourceManager;
        XString resourceName(language.GetCharPtr());
        manager->CreateResource(XString::AnsiString(resourceName.c_str()), &resource);
    }
    int englishLoaded = m_resourceManager->CreateResource("en.big", &resource);
    bool ready = initialized != 0 && englishLoaded != 0;
    if (App::IsWVGA() == 1 && !m_resourceManager->CreateResource("wvga.big", &resource)) {
        ready = false;
    }
    if (App::IsXGA() == 1 && !m_resourceManager->CreateResource("xga.big", &resource)) {
        ready = false;
    }
    if ((App::IsXGA() || App::IsVGA() == 1) && !m_resourceManager->CreateResource("vga.big", &resource)) {
        ready = false;
    }
    int gameLoaded = m_resourceManager->CreateResource(GAME_RES_FILE, &resource);
    if (!ready || gameLoaded == 0) {
        return 1;
    }

    m_executor = new CAppExecutor();
    if (m_executor != NULL) {
        m_executor->Init();
    }
    CEventListener* listener = new CEventListener();
    m_eventListener = listener;
    unsigned int listenerHandle = CStringToKey("GameApp", 0) | 0x80000000;
    listener->m_object = this;
    listener->m_handle = listenerHandle;
    listener->m_callback = EventCB;
    m_eventListener->Register(CGameApp_InterestingEvents, 0x3fffffff);

    ICLicenseMgr::GetInstance();
    ICFileMgr::GetInstance();
    ICMediaPlayer::GetInstance()->Initialize(1, 16, 44100, 7056, 10, 0);
    ICMoviePlayer::GetInstance();
    CRandGen::GetInstance();
    CCrc32::GetInstance();
    CAppProperties::GetInstance();
    CDemoMgr::GetInstance();
    CFontMgr::GetInstance();

    ICGraphics::Param graphicsParams[] = {{0, 0}};
    ICGraphics* graphics = ICGraphics::GetInstance();
    graphics->Initialize(graphicsParams);
    const unsigned int clearFlags = ICGraphics::ClearFlag_Color | ICGraphics::ClearFlag_Depth |
                                    ICGraphics::ClearFlag_Stencil;
    graphics->ClearBuffers(clearFlags);
    graphics->SwapBuffers();
    graphics->ClearBuffers(clearFlags);
    ICGraphics2d* graphics2d = ICGraphics2d::GetInstance();
    graphics2d->Initialize();
    graphics->SetDefaultActiveAbstractionLayer(graphics2d->GetActiveAbstractionLayer());
    // Third-party advertising providers use the integration available in the CK rebuild.
    ICAdManager::GetInstance()->Init(9657, 9658);
    CPHInterface::initFromBundle();
    ICNotification* notification = ICNotification::GetInstance();
    if (!notification->IsEnabled(ICNotification::Type_Push)) {
        ICNotification::GetInstance()->Register(ICNotification::Type_Push, CK_PUSH_APP_KEY, CK_PUSH_APP_SECRET,
                                                 NULL, NULL);
    }
    ICInAppPurchase::GetInstance()->Init(this);
    TapjoyInterface::initializeFromBundle();
    TapjoyInterface::registerVideoCallback(OnTapjoyVideoStateChanged, NULL);
    m_rootNode = new AppNode();
    AdColony_Facade::initFromBundle();
    return 0;
}

void CGameApp::OnFree()
{
    ICInAppPurchase* purchase = ICInAppPurchase::GetInstance();
    delete purchase;
    TapjoyInterface::destroy();
    if (m_rootNode != NULL) {
        delete m_rootNode;
        m_rootNode = NULL;
    }

    CFontMgr* fonts = CFontMgr::GetInstance();
    delete fonts;
    ICAdManager* ads = ICAdManager::GetInstance();
    if (ads != NULL) {
        ads->Destroy();
        delete ads;
    }
    CPHInterface::destroy();
    ICGraphics2d* graphics2d = ICGraphics2d::GetInstance();
    if (graphics2d != NULL) {
        graphics2d->Destroy();
        delete graphics2d;
    }
    ICGraphics* graphics = ICGraphics::GetInstance();
    if (graphics != NULL) {
        graphics->Destroy();
        delete graphics;
    }
    delete ICMediaPlayer::GetInstance();
    delete ICMoviePlayer::GetInstance();

    if (m_resourceManager != NULL) {
        m_resourceManager->ReleaseResource("BountyHunter.big");
        m_resourceManager->Destroy();
        if (m_resourceManager != NULL) {
            delete m_resourceManager;
            m_resourceManager = NULL;
        }
    }
    if (m_eventListener != NULL) {
        m_eventListener->UnregisterAll();
        if (m_eventListener != NULL) {
            delete m_eventListener;
            m_eventListener = NULL;
        }
    }
    if (m_executor != NULL) {
        delete m_executor;
        m_executor = NULL;
    }
    if (m_registry != NULL) {
        ReleaseRegistry();
        if (m_registry != NULL) {
            delete m_registry;
            m_registry = NULL;
        }
    }
    DestroyPools();
    if (m_singletons != NULL) {
        int bucketCount = m_singletons->m_bucketCount;
        for (int bucket = 0; bucket < bucketCount; ++bucket) {
            CHash::Entry* entry = m_singletons->m_buckets[bucket];
            while (entry != NULL) {
                CSingleton* singleton = static_cast<CSingleton*>(entry->m_value);
                entry = entry->m_next;
                delete singleton;
            }
        }
        if (m_singletons != NULL) {
            delete m_singletons;
            m_singletons = NULL;
        }
    }
    XString::Shutdown();
}

void CGameApp::HandleUpdate()
{
    if (syncServerFlag) {
        CProfileManager* profiles = static_cast<App*>(WindowApp::m_instance)->m_profileManager;
        if (profiles->isReady() && profiles->m_writeRequestsOutstanding == 0) {
            syncServerFlag = false;
        }
    }

    int elapsed = m_executor->m_frameMillis;
    ICAdManager* ads = ICAdManager::GetInstance();
    ads->HandleUpdate(elapsed);
    if (ads->IsFullScreen()) {
        return;
    }
    m_rootNode->HandleUpdate(elapsed);
    ICMediaPlayer::GetInstance()->HandleUpdate(elapsed);
    if (static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager() != NULL && m_pendingPurchases.size() > 0) {
        for (int index = 0; index < m_pendingPurchases.size(); index++) {
            OnIAPComplete(m_pendingPurchases[index]);
        }
        m_pendingPurchases.removeAllElements();
    }
    ICInAppPurchase* purchase = ICInAppPurchase::GetInstance();
    if (purchase->IsTransactionActive()) {
        purchase->HandleUpdate(elapsed);
    }
}

void CGameApp::HandleRender()
{
    if (!m_renderEnabled) {
        return;
    }
    ICGraphics* graphics = ICGraphics::GetInstance();
    ICGraphics2d::GetInstance();
    ICRenderSurface* surface = graphics->GetTargetSurface();
    ICAdManager* ads = ICAdManager::GetInstance();
    if (ads->IsFullScreen()) {
        ads->HandleRender();
        return;
    }
    unsigned int width;
    unsigned int height;
    surface->GetWidthAndHeight(width, height);
    m_rootNode->HandleRender();
    if (ads->IsVisible()) {
        ads->HandleRender();
    }
    graphics->SwapBuffers();
    ICRenderSurface::Description description;
    surface->GetDescription(description);
    if (description.m_colorKind == 3 || description.m_colorKind == 4) {
        m_rootNode->HandleEvent(0x24314258, 0, 0);
    }
}

void CGameApp::OnIAPComplete(const XString& productId)
{
    // Keep both lookup temporaries alive until the original format call.
    XString name = XString::Format(IAP_ITEM_FORMAT,
        static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->getIAPCurrencyByAppleId(productId)->m_id,
        static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->getIAPCurrencyByAppleId(productId)->m_name.c_str());
    CIAPCurrencyManager* manager = static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager();
    CGameAnalytics::logItemPurchased(name, manager->getIAPCurrencyByAppleId(productId)->m_priceIAP,
                                     CGameAnalytics::CURRENCY_IAP);
    CGameAnalytics::logIAPPurchased(name, CBH_Player::GetInstance()->FieldAt(0x28));
    manager = static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager();
    manager->getIAPCurrencyByAppleId(productId)->Buy();
    manager = static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager();
    WindowApp::HandleTunnelCommand((int)0xbe344666,
        reinterpret_cast<int>(manager->getIAPCurrencyByAppleId(productId)), 0, 0);
}

void CGameApp::RequestProductDataComplete(RequestProductDataState state,
    const TCVector<const ICInAppPurchaseProduct*>& products)
{
    int command;
    if (state == REQUEST_PRODUCT_DATA_TIMEOUT || state == REQUEST_PRODUCT_DATA_FAILED) {
        command = (int)0xc24364a4;
    } else if (state == REQUEST_PRODUCT_DATA_SUCCESS) {
        static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->SetAllUnavailable();
        for (int index = 0; index < products.Size(); ++index) {
            const ICInAppPurchaseProduct* product = products[index];
            CIAPCurrencyManager* manager = static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager();
            CIAPCurrency* item = manager->getIAPCurrencyByAppleId(XString(product->GetProductIdentifier().GetWCharPtr()));
            item->setAvailable(true);
            item->setPriceIAP(static_cast<int>(product->GetPrice() * 100.0f));
        }
        command = (int)0xff403571;
    } else {
        return;
    }
    WindowApp::HandleTunnelCommand(command, 0, 0, 0);
}

void CGameApp::BuyProductComplete(BuyProductState state, const ICInAppPurchaseTransaction* transaction)
{
    switch (state) {
    case BUY_PRODUCT_FAILED: {
        XString productId(transaction->GetProductIdentifier().GetWCharPtr());
        XString name = XString::Format(IAP_ITEM_FORMAT,
            static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->getIAPCurrencyByAppleId(productId)->m_id,
            static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->getIAPCurrencyByAppleId(productId)->m_name.c_str());
        CGameAnalytics::logIAPPurchaseFailed(name);
        WindowApp::HandleTunnelCommand(0x659570dc, 0, 0, 0);
        return;
    }
    case BUY_PRODUCT_CANCELLED: {
        XString productId(transaction->GetProductIdentifier().GetWCharPtr());
        XString name = XString::Format(IAP_ITEM_FORMAT,
            static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->getIAPCurrencyByAppleId(productId)->m_id,
            static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager()->getIAPCurrencyByAppleId(productId)->m_name.c_str());
        CGameAnalytics::logIAPPurchaseCanceled(name);
        WindowApp::HandleTunnelCommand(0x5f9d3f54, 0, 0, 0);
        return;
    }
    case BUY_PRODUCT_SUCCESS: {
        XString productId(transaction->GetProductIdentifier().GetWCharPtr());
        CIAPCurrencyManager* manager = static_cast<App*>(WindowApp::m_instance)->InitIAPCurrencyManager();
        if (manager == NULL) {
            m_pendingPurchases.addElement(productId);
        } else {
            OnIAPComplete(productId);
        }
        return;
    }
    default:
        return;
    }
}

void CGameApp::OnTimer()
{
    if (m_executor != NULL) {
        m_executor->Run();
    }
}

void CGameApp::SetRenderingEnabled(unsigned char enabled)
{
    m_renderEnabled = enabled;
}

void CGameApp::SyncTransactionsComplete(SyncTransactionsState state,
                                        const TCVector<const ICInAppPurchaseTransaction*>& transactions)
{
}

// CK's resource factory uses the game application's MIME type table.
CResource* CResourceFactory::CreateResource(unsigned int mimeType)
{
    switch ((int)mimeType) {
    case 0x107dd145:
    case (int)0xb717945f:
        return new CResourceDIB();
    case 0x2c435e5a:
    case 0x2c435e5c:
    case (int)0xd65e722e:
        return new CResourceShader();
    case 0x593f8140:
    case 0x59efbc42:
    case 0x59f4bc42:
    case 0x59f4bc59:
    case (int)0x9f4bc472:
    case (int)0xe179efbe:
    case (int)0xe179f4be:
    case (int)0xfc31810d:
        return new CResourceRenderSurface();
    case 0x374f8659:
    case (int)0xf4e02223:
        return new CResourceBinary();
    case 0x43735eb4:
    case (int)0xf686aadc:
        return new CResourceStrWChar();
    case 0x027b32f1:
    case 0x2fd9aa54:
    case 0x2fd9b34a:
    case 0x32fd8642:
    case 0x32fd99e1:
    case 0x32fd9f47:
    case (int)0xfd878582:
    case (int)0xfd8a7754:
    case (int)0xfd94b3c1:
        return new CResourceMedia();
    case 0x40546f85:
    case 0x40547da2:
    case (int)0xa6efb9a2:
        return new CResourcePalette();
    case 0x521b2402:
        return new CResourceAggregate();
    case 0x69e5d35c:
        return new CResourceKeyset();
    case 0x5ed4e9cb:
    case 0x69e4c505:
    case 0x74f30fa9:
        return new CResourceBigFile();
    default:
        return NULL;
    }
}
