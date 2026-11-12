// CInAppPurchaseProduct_Cocoa.h - ICInAppPurchaseProduct backed by a retained SKProduct (created by b033x).
#ifndef CINAPPPURCHASEPRODUCT_COCOA_H
#define CINAPPPURCHASEPRODUCT_COCOA_H

#include "ICInAppPurchaseProduct.h"

#ifdef __OBJC__
@class SKProduct;
#endif

class CInAppPurchaseProduct_Cocoa : public ICInAppPurchaseProduct {
public:
    /** Retains `product`. */
    CInAppPurchaseProduct_Cocoa(SKProduct* product);
    virtual ~CInAppPurchaseProduct_Cocoa();

    virtual CStrWChar GetDescription() const;
    virtual CStrWChar GetTitle() const;
    virtual CStrWChar GetProductIdentifier() const;
    virtual float GetPrice() const;
    virtual CStrWChar GetCurrencySymbol() const;
    virtual CStrWChar GetLocaleIdentifier() const;

private:
    SKProduct* m_product;   // 0x04 retained
};

#endif
