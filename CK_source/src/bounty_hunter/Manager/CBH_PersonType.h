// CBH_PersonType.h - one kind of person (enemy, civilian, boss) read from persons.xml.
// game1: CK 1.5.2 layout (0x28) from CBH_PersonType::CBH_PersonType and ParseInfo; the CKZ
// fields (damage, ranges, arcade rewards, scale) do not exist in CK.
#ifndef CBH_PERSONTYPE_H
#define CBH_PERSONTYPE_H

#include "xString.h"

class TiXmlNode;
class CEnemyWeaponType;
class CVisualCharacterType;
class CNationalityType;

class CBH_PersonType {
public:
    CBH_PersonType(TiXmlNode* node);

    /** Read the name and the <info> child of a <person> node. */
    void Parse(TiXmlNode* node);
    /** level_range_min <= level <= level_range_max. */
    bool IsSuitableForPlayerLevel(int level) const;
    void ParseInfo(TiXmlNode* node);

    XString m_name;                             // 0x00
    CVisualCharacterType* m_visualCharacter;    // 0x04
    CNationalityType* m_nationality;            // 0x08
    CEnemyWeaponType* m_weapon;                 // 0x0c NULL when the weapon attribute is empty
    int m_levelRangeMin;                        // 0x10
    int m_levelRangeMax;                        // 0x14
    int m_baseXp;                               // 0x18
    int m_baseHp;                               // 0x1c
    float m_baseAccuracy;                       // 0x20
    bool m_spawnCivilians;                      // 0x24
};

#endif
