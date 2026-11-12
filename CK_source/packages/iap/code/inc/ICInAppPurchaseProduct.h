// ICInAppPurchaseProduct.h - one product returned by an in-app purchase store query (created by b033x).
#ifndef ICINAPPPURCHASEPRODUCT_H
#define ICINAPPPURCHASEPRODUCT_H

#include "CStrWChar.h"

/** Read-only description of a store product (platform implementation: CInAppPurchaseProduct_Cocoa). */
class ICInAppPurchaseProduct {
public:
    virtual ~ICInAppPurchaseProduct() {}

    virtual CStrWChar GetDescription() const = 0;
    virtual CStrWChar GetTitle() const = 0;
    virtual CStrWChar GetProductIdentifier() const = 0;
    virtual float GetPrice() const = 0;
    virtual CStrWChar GetCurrencySymbol() const = 0;
    virtual CStrWChar GetLocaleIdentifier() const = 0;
};

#endif
