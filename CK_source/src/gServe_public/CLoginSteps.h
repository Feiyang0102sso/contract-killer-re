#ifndef CLOGINSTEPS_H
#define CLOGINSTEPS_H

#include "CNGSLoginFlow.h"

class DeviceOnlineCheckStep : public CNGSLoginStep {
public:
    DeviceOnlineCheckStep();
    virtual void Excute();
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
};

class VersionCheckStep : public CNGSLoginStep_CheckVersion {
public:
    VersionCheckStep();
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
};

class FacebookLoginStep : public CNGSLoginStep_SocialNetwork {
public:
    explicit FacebookLoginStep(bool force);
    virtual void Prepare();
    virtual void Excute();
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);

private:
    bool m_force;
};

class GameCenterLoginStep : public CNGSLoginStep_SocialNetwork {
public:
    GameCenterLoginStep();
    virtual void Prepare();
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
    virtual unsigned int GetClassId() { return 0x90e4b4fa; }
};

class LoadFriendsListStep : public CNGSLoginStep_LoadFriendsList {
public:
    explicit LoadFriendsListStep(int network);
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
};

class LoadPrivateGameDataStep : public CNGSLoginStep {
public:
    LoadPrivateGameDataStep();
    virtual void Excute();
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
};

class LoadGameDataStep : public CNGSLoginStep {
public:
    LoadGameDataStep();
    virtual void Excute();
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
};

class InitIncentiveOffersStep : public CNGSLoginStep {
public:
    InitIncentiveOffersStep();
    virtual void Excute();
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
};

class GameReloadStep : public CNGSLoginStep {
public:
    GameReloadStep();
    virtual void Excute();
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
};

class CheckOverrideDataStep : public CNGSLoginStep {
public:
    CheckOverrideDataStep();
    virtual void Excute();
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
};

#endif
