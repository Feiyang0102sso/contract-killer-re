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
