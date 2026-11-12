// CGameApp.h - CK application entry point and StoreKit delegate.
#ifndef CGAMEAPP_H
#define CGAMEAPP_H

#include "CApp.h"
#include "ICInAppPurchaseDelegate.h"
#include "Vector_ss.h"
#include "xString.h"

class AppNode;
class CEvent;
class CEventListener;

class CGameApp : public CApp, public ICInAppPurchaseDelegate {
public:
    CGameApp();
    virtual ~CGameApp();
    static CGameApp* GetInstance();

    virtual int OnInit();
    virtual void OnFree();
    virtual void OnTimer();
    virtual void OnSystemEvent(unsigned int eventId, unsigned int param1, unsigned int param2);
    virtual void SyncTransactionsComplete(SyncTransactionsState state,
                                          const TCVector<const ICInAppPurchaseTransaction*>& transactions);
    virtual void RequestProductDataComplete(RequestProductDataState state,
                                            const TCVector<const ICInAppPurchaseProduct*>& products);
    virtual void BuyProductComplete(BuyProductState state, const ICInAppPurchaseTransaction* transaction);

    void HandleUpdate();
    void HandleRender();
    static unsigned char EventCB(CEvent& event, void* listener);
    unsigned char HandleEvent(unsigned int eventId, unsigned int param1, unsigned int param2);
    void CreatePools() const;
    void DestroyPools() const;
    int InitRegistry();
    void ReleaseRegistry();
    static void OnTapjoyVideoStateChanged(unsigned char active, void* context);
    void ReInit();
    void OnIAPComplete(const XString& productId);
    void SetRenderingEnabled(unsigned char enabled);

    static bool syncServerFlag;
    static bool gserveDataConflict;
    static bool m_wasMusicEnabled;  // changed by app (was int: strb / ldrb + and #1)
    static bool m_wasSoundEnabled;  // changed by app

    // The ICInAppPurchaseDelegate vptr is at +0x3c. ARMv7 instance size is 0x5c.
    CEventListener* m_eventListener;     // +0x40
    AppNode* m_rootNode;                 // +0x44
    Vector<XString> m_pendingPurchases;  // +0x48
    unsigned char m_renderEnabled;      // +0x58
    unsigned char m_field59;            // +0x59
    bool m_accountChanged;              // +0x5a waiting for account change acknowledgement
};

#endif
