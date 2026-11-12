// CShopItem.h - CK item configuration, images and shop manager iteration.
#ifndef CSHOPITEM_H
#define CSHOPITEM_H

#include "xString.h"

class COverrideManager;
class ICRenderSurface;
class TiXmlElement;

/** Base of an item sold or awarded in the game. */
class CItem {
public:
    /** Item category (getType()); only the type name is recovered so far. */  // added by gps
    enum EItemTypes {  // added by gps
        ITEM_TYPE_FIRST = 0  // added by gps
    };  // added by gps
    explicit CItem(int id);
    virtual ~CItem();

    virtual void init(const TiXmlElement& element);
    virtual void setPriceIAP(int value) { m_priceIAP = value; m_originalPriceIAP = value; }
    virtual ICRenderSurface* getBigImage();
    virtual ICRenderSurface* getShopImage() const { return m_shopImage; }
    virtual bool isHidden() const { return m_hidden; }
    virtual bool isAvailable() const { return false; }
    virtual void setAvailable(bool available) = 0;
    virtual int getType() = 0;
    virtual void loadResources();
    virtual void releaseResources();
    virtual void ReleaseBigShopImage();
    virtual XString GetStatsString() const;
    virtual int getCurrentAmount() { return 0; }
    virtual const char* getBigImageResId() const { return m_bigImageId; }
    virtual const char* getShopImageResId() const { return m_shopImageId; }
    virtual void setId(int id) { m_id = id; }
    virtual void setName(const XString& name) { m_originalName = (m_name = name); }
    virtual void setDescription(const XString& description) { m_originalDescription = (m_description = description); }
    virtual void setExtraText(const XString& text) { m_originalExtraText = (m_extraText = text); }
    virtual void setPriceCredits(int price) { m_priceCredits = price; m_originalPriceCredits = price; }
    virtual void setPricePoints(int price) { m_pricePoints = price; m_originalPricePoints = price; }
    virtual void setUnlockLvl(int level) { m_unlockLevel = level; m_originalUnlockLevel = level; }
    virtual void setBigImageResId(const char* name) { m_bigImageId = name; }
    virtual void setShopImageResId(const char* name) { m_shopImageId = name; }
    virtual void setBigImage(ICRenderSurface* surface) { m_bigImage = surface; }
    virtual void setShopImage(ICRenderSurface* surface) { m_shopImage = surface; }
    virtual void Parse(const TiXmlElement& element);
    virtual void ParseTexts(const TiXmlElement& element);
    virtual void ParseStats(const TiXmlElement& element);
    virtual void ParseGraphics(const TiXmlElement& element);
    virtual bool doOverride(COverrideManager& manager, const XString& id);
    virtual XString getUIDString() const = 0;

    bool checkOverride();
    bool TryPurchase();
    int getPriceCredits() const;
    XString& GetName() { return m_name; }
    XString& GetDescription() { return m_description; }

    int m_overrideMark;                        // 0x04
    int m_id;                                  // 0x08
    XString m_name;                             // 0x0c
    XString m_originalName;                     // 0x10
    XString m_description;                      // 0x14
    XString m_originalDescription;              // 0x18
    XString m_originalExtraText;                // 0x1c
    XString m_extraText;                        // 0x20
    int m_priceCredits;                         // 0x24
    int m_originalPriceCredits;                 // 0x28
    int m_pricePoints;                          // 0x2c
    int m_originalPricePoints;                  // 0x30
    int m_priceIAP;                             // 0x34
    int m_originalPriceIAP;                     // 0x38
    int m_unlockLevel;                           // 0x3c
    int m_originalUnlockLevel;                   // 0x40
    const char* m_shopImageId;                   // 0x44 borrowed from ResMap
    const char* m_bigImageId;                    // 0x48 borrowed from ResMap
    ICRenderSurface* m_shopImage;                 // 0x4c
    ICRenderSurface* m_bigImage;                  // 0x50
    bool m_new;                                 // 0x54
    bool m_sale;                                // 0x55
    bool m_field56;                             // 0x56
    bool m_limited;                             // 0x57
    bool m_featured;                            // 0x58
    bool m_hidden;                              // 0x59
    bool m_canEquipInSafehouse;                  // 0x5a
};

/** Base of the managers of shop items (no virtual destructor). */
class CItemManager {
public:
    virtual int getItemsCount() = 0;
    virtual int getAmount(int) { return 0; }
    virtual CItem* getItemByName(XString name);
    virtual CItem* getItemById(int id);
    virtual CItem* firstItem();
    virtual CItem* nextItem();
    virtual void loadResources() = 0;
    virtual void freeResources() = 0;
    virtual CItem* getItemByInternalId(int index) = 0;

    CItem* getItemByStringUId(const XString& id);
    int m_iterator;                             // 0x04 firstItem / nextItem position
};

#endif
