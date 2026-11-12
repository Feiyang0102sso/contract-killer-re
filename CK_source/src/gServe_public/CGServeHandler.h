// CGServeHandler.h - login state and account dialogs used by the game.
#ifndef CGSERVEHANDLER_H
#define CGSERVEHANDLER_H

#include "TCVector.h"

class Event;
class Window;
class CBloodWindow;

class CGServeHandler {
public:
    // All Show* / NeedTo* are non-static in the original: callers pass app->m_gserveHandler as this.
    CGServeHandler();
    virtual ~CGServeHandler();

    void Init();
    void HandleUpdate();
    bool OnCommand(Event& event);
    bool CloseCurrentSocialNetworkConnectDialog();
    void ShowSocialNetworkAccountsConflictHelpDialogs();
    void SelectProfileServerData(const TCVector<int>& collections);
    void ShowSocialNetworkProfileDataConflictHelp();
    void ShowSocialNetworkProfileDataConflict();
    void ShowSocialNetworkProfileDataConflictUseLocal();
    void ShowSocialNetworkProfileDataConflictUseServer();
    void ShowFacebookLoginDialog();
    void ShowSocialNetworkConnectDialogs(int network);
    void ShowSocialNetworkAccountsConflictDialogs();
    void ShowSocialNetworkPrimaryUserChanged();
    void ShowSocialNetworkSessionInvalidDialog();
    void ShowSocialNetworkConnectionErrorDialog();
    void ShowOfflineModeDialog();
    void ShowLoginRequired();
    bool NeedToShowFacebookLogin();
    bool NeedToShowLoginRequired();

    unsigned char m_wasLoginRunning;  // +0x04 CNGSLoginFlow::IsRunning() of the last update
    CBloodWindow* m_accountConflictDialog;  // +0x08 CConflictLogoutPrompt
    bool m_offlineAccepted;           // +0x0c
    int m_pendingSwitchNetwork;      // +0x10
};

#endif
