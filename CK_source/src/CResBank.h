// PROVISIONAL (created by b055x): owner src/CResBank.cpp not written yet
// CResBank.h - bank of loaded resource key sets owned by a screen (36 bytes, layout from CResBank::CResBank()).
// Restored by b063; the preceding PROVISIONAL is historical.
#ifndef CRESBANK_H
#define CRESBANK_H

#include "CClass.h"
#include "Vector_ss.h"

class CKeysetResource;

class CResBank {
public:
    enum State { Idle, Loading, Failed, Loaded };

    /** Twelve-byte ownership record used by removeAllKeysets; names inferred from releases. */
    struct TKeyset {
        const char* m_resourceName;
        CKeysetResource* m_object;
        void* m_ownedData;
    };
    CResBank();
    virtual ~CResBank();

    /** Release every key set of the bank. */
    void removeAllKeysets();
    int GetState();
    unsigned int GetProgressPercent();
    void HandleUpdate(int elapsed);
    bool loadNextResource();
    void startNextKeyset();
    void addKeyset(const char* name);
    void load(const char* name);

    // Former m_data04[0x24 - 0x04]: layout not known yet (0x14 count, 0x1c = 4, 0x20 buffer).
    unsigned int m_keyIndex;               // 0x04 next resource of the active keyset
    unsigned int m_loadedCount;            // 0x08 resources already loaded
    unsigned int m_totalCount;             // 0x0c resources across all queued keysets
    int m_state;                           // 0x10
    Vector<TKeyset> m_keysets;              // 0x14 three-pointer records, 12-byte stride.
};

#endif
