// PROVISIONAL (created by b057x): header-only game template, no owning unit (first user src/FPSGameplay/AI/CScriptedUnitLogicExecutor.cpp)
// HashTable.h - chained hash table with a power of two bucket count; only what src/general uses.
// b077: the member functions below follow the weak instances in the bounty_hunter managers
// (HashTable<XString, T*> in CAchievementManager.o, CBH_PersonTypeManager.o, CEnemyWeaponTypeManager.o).
// Every chain is kept sorted by XString::Cmp; the table doubles when it holds more than twice
// as many entries as buckets.
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stddef.h>
#include <string.h>
#include "MathLib.h"
#include "xString.h"  // added by b077

template <class Key, class Value>
class HashTable {
public:
    /** One stored pair. */  // changed by b077: defined here (was only declared)
    struct Entry {  // added by b077
        Key m_key;          // 0x00  // added by b077
        Value m_value;      // 0x04  // added by b077
        Entry* m_next;      // 0x08 next entry of the bucket chain  // added by b077

        Entry(const Key& key, const Value& value)  // added by b077
            : m_key(key), m_value(value)  // added by b077
        {  // added by b077
        }  // added by b077
    };  // added by b077

    HashTable(int bucketCount = 8)
    {
        unsigned int count = 8;
        if (bucketCount > 0) {
            count = MathLib::CeilPowerOfTwo(bucketCount);
        }
        m_bucketCount = count;
        m_buckets = new Entry*[m_bucketCount];
        memset(m_buckets, 0, m_bucketCount * sizeof(Entry*));
        m_count = 0;
    }

    ~HashTable()
    {
        Clear();
        delete[] m_buckets;
    }

    /** Remove every entry. */
    void Clear();
    /** Original SurfaceManager 119940: walks the whole bucket chain, unlinks and destroys the Entry only on a hit. */
    bool Remove(const Key& key); // added by b074i

    /** Store `value` under `key` (replacing an existing value); the stored value. */  // added by b077
    Value* Set(const Key& key, const Value& value);  // added by b077
    /** Insert without looking for an existing key. */  // added by b077
    void Add(const Key& key, const Value& value);  // added by b077
    /** The value stored under `key`, NULL when absent. */  // added by b077
    Value* Get(const Key& key) const;  // added by b077
    /** The `index`-th value in bucket order (its key in `key` when not NULL), NULL when out of range. */  // added by b077
    Value* GetAt(int index, Key** key) const;  // added by b077
    /** delete every value, then remove every entry. */  // added by b077
    void DeleteContents();  // added by b077
    /** Double the bucket count and re-insert every entry. */  // added by b077
    void Grow();  // added by b077
    /** Rehash into max(8, CeilPowerOfTwo(size)) buckets (nothing when the count does not change). */  // added by b055x
    void SetTableSize(int size);  // added by b055x

    /** Bucket of `key`. */  // added by b077
    Entry** Bucket(const Key& key) const  // added by b077
    {  // added by b077
        unsigned int hash = XString::HashData(key.m_data, key.ByteSize());  // added by b077
        return &m_buckets[hash & (m_bucketCount - 1)];  // added by b077
    }  // added by b077

    Entry** m_buckets;              // 0x00
    unsigned int m_bucketCount;     // 0x04
    int m_count;                    // 0x08
};

template <class Key, class Value>  // added by b077
void HashTable<Key, Value>::Clear()  // added by b077
{  // added by b077
    if (m_count <= 0) {  // added by b077
        return;  // added by b077
    }  // added by b077
    for (int index = 0; index < (int)m_bucketCount; index++) {  // added by b077
        Entry* entry = m_buckets[index];  // added by b077
        while (entry != NULL) {  // added by b077
            Entry* next = entry->m_next;  // added by b077
            delete entry;  // added by b077
            entry = next;  // added by b077
        }  // added by b077
        m_buckets[index] = NULL;  // added by b077
    }  // added by b077
    m_count = 0;  // added by b077
}  // added by b077

template <class Key, class Value>
bool HashTable<Key, Value>::Remove(const Key& key)
{
    Entry** bucket = Bucket(key);
    Entry* previous = NULL;
    Entry* entry = *bucket;
    while (entry != NULL) {
        // The original Remove has no ordered early exit like Get/Set; any nonzero Cmp always continues.
        if (entry->m_key.Cmp(key, 0x1fffffff) == 0) {
            if (previous == NULL) {
                *bucket = entry->m_next;
            } else {
                previous->m_next = entry->m_next;
            }
            --m_count;
            delete entry;
            return true;
        }
        previous = entry;
        entry = entry->m_next;
    }
    return false;
}

template <class Key, class Value>  // added by b077
Value* HashTable<Key, Value>::Set(const Key& key, const Value& value)  // added by b077
{  // added by b077
    if ((m_count >> 1) > (int)m_bucketCount) {  // added by b077
        Grow();  // added by b077
    }  // added by b077
    Entry** link = Bucket(key);  // added by b077
    Entry* entry;  // added by b077
    while ((entry = *link) != NULL) {  // added by b077
        int order = entry->m_key.Cmp(key, 0x1fffffff);  // added by b077
        if (order == 0) {  // added by b077
            entry->m_value = value;  // added by b077
            return &entry->m_value;  // added by b077
        }  // added by b077
        if (order > 0) {  // added by b077
            break;  // added by b077
        }  // added by b077
        link = &entry->m_next;  // added by b077
    }  // added by b077
    m_count++;  // added by b077
    Entry* added = new Entry(key, value);  // added by b077
    *link = added;  // added by b077
    added->m_next = entry;  // added by b077
    return &added->m_value;  // added by b077
}  // added by b077

template <class Key, class Value>  // added by b077
void HashTable<Key, Value>::Add(const Key& key, const Value& value)  // added by b077
{  // added by b077
    if ((m_count >> 1) > (int)m_bucketCount) {  // added by b077
        Grow();  // added by b077
    }  // added by b077
    Entry** link = Bucket(key);  // added by b077
    Entry* entry;  // added by b077
    while ((entry = *link) != NULL && entry->m_key.Cmp(key, 0x1fffffff) < 0) {  // added by b077
        link = &entry->m_next;  // added by b077
    }  // added by b077
    m_count++;  // added by b077
    Entry* added = new Entry(key, value);  // added by b077
    *link = added;  // added by b077
    added->m_next = entry;  // added by b077
}  // added by b077

template <class Key, class Value>  // added by b077
Value* HashTable<Key, Value>::Get(const Key& key) const  // added by b077
{  // added by b077
    Entry** link = Bucket(key);  // added by b077
    Entry* entry;  // added by b077
    while ((entry = *link) != NULL) {  // added by b077
        int order = entry->m_key.Cmp(key, 0x1fffffff);  // added by b077
        if (order == 0) {  // added by b077
            return &entry->m_value;  // added by b077
        }  // added by b077
        if (order > 0) {  // added by b077
            return NULL;  // added by b077
        }  // added by b077
        link = &entry->m_next;  // added by b077
    }  // added by b077
    return NULL;  // added by b077
}  // added by b077

template <class Key, class Value>  // added by b077
Value* HashTable<Key, Value>::GetAt(int index, Key** key) const  // added by b077
{  // added by b077
    if (index >= 0 && index < m_count) {  // added by b077
        int position = 0;  // added by b077
        for (int bucket = 0; bucket < (int)m_bucketCount; bucket++) {  // added by b077
            for (Entry* entry = m_buckets[bucket]; entry != NULL; entry = entry->m_next) {  // added by b077
                if (position == index) {  // added by b077
                    if (key != NULL) {  // added by b077
                        *key = &entry->m_key;  // added by b077
                    }  // added by b077
                    return &entry->m_value;  // added by b077
                }  // added by b077
                position++;  // added by b077
            }  // added by b077
        }  // added by b077
    }  // added by b077
    if (key != NULL) {  // added by b077
        *key = NULL;  // added by b077
    }  // added by b077
    return NULL;  // added by b077
}  // added by b077

template <class Key, class Value>  // added by b077
void HashTable<Key, Value>::DeleteContents()  // added by b077
{  // added by b077
    for (int index = 0; index < (int)m_bucketCount; index++) {  // added by b077
        Entry* entry = m_buckets[index];  // added by b077
        m_buckets[index] = NULL;  // added by b077
        while (entry != NULL) {  // added by b077
            Entry* next = entry->m_next;  // added by b077
            delete entry->m_value;  // added by b077
            delete entry;  // added by b077
            entry = next;  // added by b077
        }  // added by b077
    }  // added by b077
    m_count = 0;  // added by b077
}  // added by b077

template <class Key, class Value>  // added by b077
void HashTable<Key, Value>::Grow()  // added by b077
{  // added by b077
    Entry** oldBuckets = m_buckets;  // added by b077
    int oldCount = m_bucketCount;  // added by b077
    m_bucketCount = oldCount * 2;  // added by b077
    m_count = 0;  // added by b077
    m_buckets = new Entry*[m_bucketCount];  // added by b077
    memset(m_buckets, 0, m_bucketCount * sizeof(Entry*));  // added by b077
    for (int index = 0; index < oldCount; index++) {  // added by b077
        Entry* entry = oldBuckets[index];  // added by b077
        while (entry != NULL) {  // added by b077
            Entry* next = entry->m_next;  // added by b077
            Add(entry->m_key, entry->m_value);  // added by b077
            delete entry;  // added by b077
            entry = next;  // added by b077
        }  // added by b077
    }  // added by b077
    delete[] oldBuckets;  // added by b077
}  // added by b077
template <class Key, class Value>  // added by b055x
void HashTable<Key, Value>::SetTableSize(int size)  // added by b055x
{  // added by b055x
    unsigned int count = 8;  // added by b055x
    if (size > 0) {  // added by b055x
        count = MathLib::CeilPowerOfTwo(size);  // added by b055x
    }  // added by b055x
    if (count == m_bucketCount) {  // added by b055x
        return;  // added by b055x
    }  // added by b055x
    Entry** oldBuckets = m_buckets;  // added by b055x
    int oldCount = m_bucketCount;  // added by b055x
    m_bucketCount = count;  // added by b055x
    m_count = 0;  // added by b055x
    m_buckets = new Entry*[m_bucketCount];  // added by b055x
    memset(m_buckets, 0, m_bucketCount * sizeof(Entry*));  // added by b055x
    for (int index = 0; index < oldCount; index++) {  // added by b055x
        Entry* entry = oldBuckets[index];  // added by b055x
        while (entry != NULL) {  // added by b055x
            Entry* next = entry->m_next;  // added by b055x
            Add(entry->m_key, entry->m_value);  // added by b055x
            delete entry;  // added by b055x
            entry = next;  // added by b055x
        }  // added by b055x
    }  // added by b055x
    delete[] oldBuckets;  // added by b055x
}  // added by b055x

// b070g: integer-key instance at original 0x10a910..0x10af26; hashing/comparison rules differ from XString.
// ---- integer-key instance (formerly HashTableInt.inl; the original has no such file) ----
#include <stdint.h>
#include "Vector_ss.h"

/** Real linked-list hash instance for integer wrapper keys; weak implementation from the original CUnitsController.o. */
template <class T>
struct HashTableKey {
    HashTableKey(T value) : m_value(value) {}
    T m_value;
};

template <class T, class Value>
class HashTable<HashTableKey<T>, Value> {
public:
    struct Entry {
        Entry(const HashTableKey<T>& key, const Value& value) : m_key(key), m_value(value) {}
        HashTableKey<T> m_key;
        Value m_value;
        Entry* m_next;
    };

    HashTable(int bucketCount = 8)
    {
        unsigned int count = 8;
        if (bucketCount > 0) {
            count = MathLib::CeilPowerOfTwo(bucketCount);
        }
        m_bucketCount = count;
        m_buckets = new Entry*[m_bucketCount];
        memset(m_buckets, 0, m_bucketCount * sizeof(Entry*));
        m_count = 0;
    }
    ~HashTable()
    {
        Clear();
        delete[] m_buckets;
    }
    void Add(const HashTableKey<T>& key, const Value& value);
    /** Store `value` under `key` (replacing an existing value); weak original at 0xf1050. */  // added by player
    Value* Set(const HashTableKey<T>& key, const Value& value);  // added by player
    Value* Get(const HashTableKey<T>& key) const;
    Value* GetAt(int index, HashTableKey<T>** key) const;
    void ToKeys(Vector<HashTableKey<T> >& keys);
    bool Remove(const HashTableKey<T>& key);
    void Clear();
    void DeleteContents();
    void Grow();

    Entry** m_buckets;
    unsigned int m_bucketCount;
    int m_count;
};

template <class T, class Value>
void HashTable<HashTableKey<T>, Value>::ToKeys(Vector<HashTableKey<T> >& keys)
{
    for (int bucket = 0; bucket < static_cast<int>(m_bucketCount); ++bucket) {
        for (Entry* entry = m_buckets[bucket]; entry != NULL; entry = entry->m_next) {
            keys.addElement(entry->m_key);
        }
    }
}

template <class T, class Value>
void HashTable<HashTableKey<T>, Value>::Add(const HashTableKey<T>& key, const Value& value)
{
    if ((m_count >> 1) > (int)m_bucketCount) {
        Grow();
    }
    Entry** link = &m_buckets[(uintptr_t)key.m_value & (m_bucketCount - 1)];
    Entry* entry;
    while ((entry = *link) != NULL && entry->m_key.m_value < key.m_value) {
        link = &entry->m_next;
    }
    ++m_count;
    Entry* added = new Entry(key, value);
    *link = added;
    added->m_next = entry;
}

template <class T, class Value>  // added by player
Value* HashTable<HashTableKey<T>, Value>::Set(const HashTableKey<T>& key, const Value& value)  // added by player
{  // added by player
    if ((m_count >> 1) > (int)m_bucketCount) {  // added by player
        Grow();  // added by player
    }  // added by player
    Entry** link = &m_buckets[(uintptr_t)key.m_value & (m_bucketCount - 1)];  // added by player
    Entry* entry;  // added by player
    while ((entry = *link) != NULL) {  // added by player
        if (entry->m_key.m_value < key.m_value) {  // added by player
            link = &entry->m_next;  // added by player
            continue;  // added by player
        }  // added by player
        if (entry->m_key.m_value > key.m_value) {  // added by player
            break;  // added by player
        }  // added by player
        entry->m_value = value;  // added by player
        return &entry->m_value;  // added by player
    }  // added by player
    ++m_count;  // added by player
    Entry* added = new Entry(key, value);  // added by player
    *link = added;  // added by player
    added->m_next = entry;  // added by player
    return &added->m_value;  // added by player
}  // added by player

template <class T, class Value>
Value* HashTable<HashTableKey<T>, Value>::Get(const HashTableKey<T>& key) const
{
    Entry* entry = m_buckets[(uintptr_t)key.m_value & (m_bucketCount - 1)];
    while (entry != NULL) {
        // changed by player: the inlined original copies (CChallengeStats) test `<` first, then `>`.
        if (entry->m_key.m_value < key.m_value) {
            entry = entry->m_next;
            continue;
        }
        if (entry->m_key.m_value > key.m_value) {
            return NULL;
        }
        return &entry->m_value;
    }
    return NULL;
}

template <class T, class Value>
bool HashTable<HashTableKey<T>, Value>::Remove(const HashTableKey<T>& key)
{
    unsigned int bucket = (uintptr_t)key.m_value & (m_bucketCount - 1);
    Entry* entry = m_buckets[bucket];
    Entry* previous = NULL;
    while (entry != NULL) {
        if (entry->m_key.m_value == key.m_value) {
            if (previous == NULL) {
                m_buckets[bucket] = entry->m_next;
            } else {
                previous->m_next = entry->m_next;
            }
            --m_count;
            delete entry;
            return true;
        }
        previous = entry;
        entry = entry->m_next;
    }
    return false;
}

template <class T, class Value>
void HashTable<HashTableKey<T>, Value>::Clear()
{
    if (m_count <= 0) {
        return;
    }
    for (int index = 0; index < (int)m_bucketCount; ++index) {
        Entry* entry = m_buckets[index];
        while (entry != NULL) {
            Entry* next = entry->m_next;
            delete entry;
            entry = next;
        }
        m_buckets[index] = NULL;
    }
    m_count = 0;
}

template <class T, class Value>
void HashTable<HashTableKey<T>, Value>::Grow()
{
    Entry** oldBuckets = m_buckets;
    int oldCount = m_bucketCount;
    m_bucketCount = oldCount * 2;
    m_count = 0;
    m_buckets = new Entry*[m_bucketCount];
    memset(m_buckets, 0, m_bucketCount * sizeof(Entry*));
    for (int index = 0; index < oldCount; ++index) {
        Entry* entry = oldBuckets[index];
        while (entry != NULL) {
            Entry* next = entry->m_next;
            Add(entry->m_key, entry->m_value);
            delete entry;
            entry = next;
        }
    }
    delete[] oldBuckets;
}

template <class T, class Value>
Value* HashTable<HashTableKey<T>, Value>::GetAt(int index, HashTableKey<T>** key) const
{
    if (index >= 0 && index < m_count) {
        int position = 0;
        for (unsigned int bucket = 0; bucket < m_bucketCount; ++bucket) {
            for (Entry* entry = m_buckets[bucket]; entry != NULL; entry = entry->m_next) {
                if (position == index) {
                    if (key != NULL) {
                        *key = &entry->m_key;
                    }
                    return &entry->m_value;
                }
                ++position;
            }
        }
    }
    if (key != NULL) {
        *key = NULL;
    }
    return NULL;
}

template <class T, class Value>
void HashTable<HashTableKey<T>, Value>::DeleteContents()
{
    for (unsigned int bucket = 0; bucket < m_bucketCount; ++bucket) {
        Entry* entry = m_buckets[bucket];
        m_buckets[bucket] = NULL;
        while (entry != NULL) {
            Entry* next = entry->m_next;
            delete entry->m_value;
            delete entry;
            entry = next;
        }
    }
    m_count = 0;
}
#endif
