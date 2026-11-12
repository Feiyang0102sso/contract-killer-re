#ifndef CNGSLOGINFLOW_H
#define CNGSLOGINFLOW_H

#include "CSingleton.h"
#include "CStrChar.h"
#include "TCVector.h"

enum eLoginFlowEvent {
    LoginFlowNone = -1,
    LoginFlowSuccess,
    LoginFlowCancel,
    LoginFlowFail,
    LoginFlowPaused,
    LoginFlowResume,
    LoginFlowForceStopped,
    LoginFlowVersionSuccess,
    LoginFlowVersionInvalid,
    LoginFlowSessionInvalid,
    LoginFlowServerConnectionError,
    LoginFlowNetworkConnectionError,
    LoginFlowAccountConflict,
    LoginFlowPrimaryUserChanged,
    LoginFlowTimeout
};

/** Step owned and deleted by the login flow. */
class CNGSLoginStep {
public:
    /** (added by gserve2) Inline in the original: "Unknown", no network, no timeout, the login flow id. */
    CNGSLoginStep()
    {
        m_name = "Unknown";
        m_snType = -1;
        m_timeout = -1;
        m_classId = 0x916da8fd;  // CNGSLoginFlow::ClassId
    }

    virtual void Prepare() { Excute(); }
    virtual void Excute() {}
    virtual void OnEvent(eLoginFlowEvent event, const char* caller) = 0;
    virtual bool isEventValid(eLoginFlowEvent event, const char* caller);
    virtual unsigned int GetClassId() { return m_classId; }
    virtual int GetSnType() { return m_snType; }
    virtual void SetTimeout(int timeout) { m_timeout = timeout; }
    virtual int GetTimeout() { return m_timeout; }
    virtual const char* GetName() { return m_name.GetCharPtr(); }
    virtual void SetName(const char* name) { m_name = name; }

    unsigned int m_classId;  // 0x04
    CStrChar m_name;         // 0x08
    int m_snType;            // 0x18
    int m_timeout;           // 0x1c
};

class CNGSLoginStep_CheckVersion : public CNGSLoginStep {
public:
    CNGSLoginStep_CheckVersion()
    {
        m_name = "CheckVersion";
        m_classId = 0x81a8233f;
    }

    virtual void Excute();
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
};

class CNGSLoginStep_SocialNetwork : public CNGSLoginStep {
public:
    explicit CNGSLoginStep_SocialNetwork(int network = -1)
    {
        m_name = "SocialNetwork";
        m_snType = network;
        m_classId = 0x90e4b4fa;
    }

    virtual void Excute();
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
};

class CNGSLoginStep_LoadFriendsList : public CNGSLoginStep {
public:
    explicit CNGSLoginStep_LoadFriendsList(int network = -1)
    {
        m_name = "LoadFriendsList";
        m_snType = network;
        m_classId = 0xe0f9e913;
    }

    virtual void Excute();
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
};

/** Queues gServe login steps and advances the active step. */
class CNGSLoginFlow : public CSingleton {
public:
    enum { ClassId = 0x916da8fd };

    CNGSLoginFlow();
    virtual ~CNGSLoginFlow();

    static inline CNGSLoginFlow* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(ClassId, &found);
        CNGSLoginFlow* instance = (CNGSLoginFlow*)found;
        if (instance == NULL) {
            instance = new CNGSLoginFlow();
        }
        return instance;
    }

    void Reset();
    unsigned char IsRunning();
    void Queue(CNGSLoginStep* step);
    void Start();
    void PrepareStep(int step);
    void OnEvent(eLoginFlowEvent event, const char* caller);
    void ExcuteStep(int step);
    void NextStep();
    int GetRemainingTimeout(int elapsedMs);
    CNGSLoginStep* GetCurrentStep();
    int GetStepCount();
    CNGSLoginStep* GetStep(int step);

    int m_currentStep;                  // 0x08 -1 while idle
    TCVector<CNGSLoginStep*> m_steps;    // 0x0c
    int m_activeNetwork;                // 0x24
    int m_remainingTimeout;             // 0x28 milliseconds
    unsigned char m_timeoutReported;    // 0x2c
    unsigned char m_timeoutsEnabled;    // 0x2d
};

#endif
