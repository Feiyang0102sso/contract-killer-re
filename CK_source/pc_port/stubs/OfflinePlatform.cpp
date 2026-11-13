// iOS platform services the PC port runs without: network, push and local notifications, in-app
// purchases, Game Center and the Glu Games Network. Each reports "unavailable".
#include <stdio.h>
#include "CStrWChar.h"
#include "CBHUtil_Cocoa.h"
#include "CGluGamesNetwork.h"
#include "CHTTPNative.h"
#include "CNetworkAvailability.h"
#include "CPushNotification_Cocoa.h"
#include "GameCenter_Facade.h"
#include "ICInAppPurchase.h"
#include "ICSocket.h"
#include "LocalPushManager.h"

namespace {

// NSURLErrorNotConnectedToInternet: what an iOS request reports without a connection.
constexpr int NotConnectedError = -1009;

class OfflineSocket final : public ICSocket {
public:
    unsigned char IsBusy() const override { return 0; }
    int GetError() const override { return Error_NotConnected; }
    int GetSocketError() const override { return 0; }
    unsigned char* GetDataPtr() const override { return nullptr; }
    unsigned int GetDataLen() const override { return 0; }
    int Connect(eProtocol, const char*, unsigned short, int, unsigned char) override { return Error_NotConnected; }
    void Disconnect() override {}
    int Send(const unsigned char*, unsigned int, int) override { return Error_NotConnected; }
    int Receive(unsigned char*, unsigned int, int) override { return Error_NotConnected; }
    void HandleUpdate(int) override {}
};

class OfflineStore final : public ICInAppPurchase {
public:
    void Init(ICInAppPurchaseDelegate*) override {}
    void SyncTransactions() override {}
    void RequestProductData(const TCVector<CStrWChar>&, int) override {}
    void BuyProduct(const CStrWChar&) override {}
    unsigned char IsTransactionActive() const override { return 0; }
    unsigned char IsAvailable() const override { return 0; }
    unsigned char AllowsConcurrentOperations() const override { return 0; }
    void HandleUpdate(int) override {}
};

} // namespace

ICSocket* ICSocket::CreateInstance() { return new OfflineSocket; }
ICInAppPurchase* ICInAppPurchase::CreateInstance() { return new OfflineStore; }

CNetworkAvailability::CNetworkAvailability() : m_reachability(nullptr) {}
CNetworkAvailability::~CNetworkAvailability() = default;
unsigned char CNetworkAvailability::IsAvailable() const { return 0; }

// Requests never start: not busy, no response, the "not connected" error.
CHTTPNative::CHTTPNative() : m_handler(nullptr) {}
CHTTPNative::~CHTTPNative() = default;
void CHTTPNative::Cancel() {}
void CHTTPNative::ClearResponse() {}
void CHTTPNative::Send(unsigned long, const char*, const char*, unsigned long, const char*, const char*, const char*) {}
bool CHTTPNative::IsBusy() const { return false; }
int CHTTPNative::GetError() { return NotConnectedError; }
const char* CHTTPNative::GetResponsePtr() { return nullptr; }
unsigned int CHTTPNative::GetResponseLen() { return 0; }

// Push notifications (CNotification_Cocoa.cpp routes ICNotification here): never registered.
CPushNotification_Cocoa::CPushNotification_Cocoa()
    : CSingleton(SingletonId), m_state(State_Failed), m_context(nullptr), m_callback(nullptr)
{
}
CPushNotification_Cocoa::~CPushNotification_Cocoa() = default;
unsigned char CPushNotification_Cocoa::Register(const wchar_t*, const wchar_t*, void*, Callback) { return 0; }
unsigned char CPushNotification_Cocoa::IsEnabled() { return 0; }
unsigned char CPushNotification_Cocoa::GetIdentity(CStrWChar&) { return 0; }
unsigned char CPushNotification_Cocoa::Cancel() { return 0; }
unsigned char CPushNotification_Cocoa::SetBadgeNumber(unsigned int) { return 0; }
unsigned char CPushNotification_Cocoa::GetBadgeNumber(unsigned int&) { return 0; }

void LocalPushManager::CancelLocalPushNotifications(bool) {}
void LocalPushManager::SetupLocalPush(const char*, const char*, int, int) {}
void LocalPushManager::SetLocalPushNotification(const char*, const char*, int, int) {}

GameCenter_Facade::GameCenter_Facade()
    : CSingleton(ClassId), m_authenticationState(RequestState_Failed), m_scorePostState(RequestState_Failed),
      m_leaderboardLoadState(RequestState_Failed), m_achievementPostState(RequestState_Failed), m_unused18(0),
      m_leaderboards(nullptr), m_playersCache(nullptr)
{
}
GameCenter_Facade::~GameCenter_Facade() = default;
bool GameCenter_Facade::isAuthenticated() { return false; }
void GameCenter_Facade::AuthenticatePlayer() {}
bool GameCenter_Facade::isGameCenterSupported() { return false; }
void GameCenter_Facade::StartPostingScore(const char*, int) {}
void GameCenter_Facade::StartLoadingLeaderboards(GameCenter_LeaderboardFilter*, int) {}
void GameCenter_Facade::StartPostingAchievement(const char*, float) {}
void GameCenter_Facade::FetchPosting() {}
int GameCenter_Facade::GetLeaderboardSize(const char*) { return 0; }
void GameCenter_Facade::GetLeaderboardData(const char*, int, GameCenter_LeaderboardData&) {}
void GameCenter_Facade::GetLocalPLayerName(CStrWChar& name) { name = L"PLAYER"; }
void GameCenter_Facade::GetLocalPlayerData(const char*, GameCenter_LeaderboardData&) {}
void GameCenter_Facade::RunLeaderboardsView(const char*) {}

void CBHUtil_Cocoa::LaunchGameCenter() {}
bool CBHUtil_Cocoa::IsGameCenterAuthenticated() { return false; }
void* CBHUtil_Cocoa::GetRootViewController() { return nullptr; }

// The Glu Games Network is a web view; without it, Show returns to the game at once.
CGluGamesNetwork::CGluGamesNetwork(const char*, void (*onExit)(void*), void* exitContext, unsigned int, bool)
    : m_onExit(onExit), m_exitContext(exitContext), m_onGServeUpdates(nullptr), m_gServeUpdatesContext(nullptr),
      m_lastGServeUpdates(0)
{
}
void CGluGamesNetwork::Show()
{
    fprintf(stderr, "Glu Games Network is not available offline\n");
    if (m_onExit != nullptr) {
        m_onExit(m_exitContext);
    }
}
void CGluGamesNetwork::Hide() {}
void CGluGamesNetwork::ReportGServeUpdates(int) {}
