// CInAppPurchaseTransaction_Cocoa.h - ICInAppPurchaseTransaction backed by a retained SKPaymentTransaction (created by b033x).
#ifndef CINAPPPURCHASETRANSACTION_COCOA_H
#define CINAPPPURCHASETRANSACTION_COCOA_H

#include "ICInAppPurchaseTransaction.h"

#ifdef __OBJC__
@class SKPaymentTransaction;
#endif

class CInAppPurchaseTransaction_Cocoa : public ICInAppPurchaseTransaction {
public:
    /** Retains `transaction`. */
    CInAppPurchaseTransaction_Cocoa(const SKPaymentTransaction* transaction);
    virtual ~CInAppPurchaseTransaction_Cocoa();

    virtual int GetTransactionState() const;
    virtual CStrWChar GetProductIdentifier() const;
    virtual CStrWChar GetTransactionIdentifier() const;
    virtual CStrWChar GetErrorDescription() const;
    virtual CStrWChar GetReceiptDescription() const;
    virtual CStrWChar GetDate() const;

private:
    const SKPaymentTransaction* m_transaction;  // 0x04 retained
};

#endif
