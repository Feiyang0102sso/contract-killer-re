// CProperties.h - "key = value" settings file (created by b006).
// Lines are "key=value" or "key:value"; lines starting with '#' or '!' are comments.
// Settings are looked up by CStringToKey(key) only (the key text is not kept).
#ifndef CPROPERTIES_H
#define CPROPERTIES_H

#include "CClass.h"
#include "CStrChar.h"
#include "CVector.h"

                                                                           

class CInputStream;

class CProperties : public CClass {
public:
    /** Runtime class id: CStringToKey("CProperties"). */
    static const unsigned int ClassId = 0x8625d957;

    /** One loaded setting. */
    struct Setting {
        unsigned int m_keyHash;     // 0x00 CStringToKey(key, 0)
        CStrChar m_value;           // 0x04
    };

    CProperties();
    virtual ~CProperties();
    void Destroy();

    /**
     * Read all lines of `stream`. Without `merge` the current settings are dropped first; with
     * `merge` existing keys are overwritten and their names recorded (see CollectOverriddenKeysInto).
     */
    void Load(CInputStream& stream, unsigned char merge);
    /** Read one line (without the '\n') into `line`. */
    void ReadLine(CInputStream& stream, CStrChar& line);

    unsigned char Exists(const char* key);
    /** Change the value of an existing key; returns 1 when the key exists. */
    unsigned char Replace(const char* key, const char* value);
    /** Add a new key, returning 1 when the key was inserted. */
    unsigned char Add(const char* key, const char* value);

    /** Value text of `key`, NULL when absent. */
    const char* GetStr(const char* key);
    /** atoi of the value, 0 when absent. */
    int GetInt(const char* key);
    /** Value equals "true" (any case). */
    unsigned char GetBool(const char* key);

    /** Following merging loads append the names of the keys they overwrite to `keys`. */
    void CollectOverriddenKeysInto(CVector* keys);

    CVector m_settings;                  // 0x08 owned Setting pointers
    CVector* m_overriddenKeys;           // 0x1c not owned

private:
    /** Index of the setting with `keyHash`, -1 when absent. */
    int KeyToIndex(unsigned int keyHash);
};

       

#endif
