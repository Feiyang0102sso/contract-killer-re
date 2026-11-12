// CVector.h - CK's integer array and the port's growable array template (created by b006).
// Elements live in one malloc'd block; they are copy-constructed in place when added and
// destroyed (last to first) when removed. Member definitions are in CVector.inl.
#ifndef CVECTOR_H
#define CVECTOR_H

#include <stddef.h>
#include <stdlib.h>
#include "CClass.h"

// Placement new comes from NPNew.h (prefix header).

/** CK's growable array of 32-bit values. */
class CVector : public CClass {
public:
    enum { ClassId = 0x36034282 };

    CVector();
    virtual ~CVector();

    void Clear();
    int Size();
    int Add(int value);
    int Add(int index, int value);
    int Remove(int index, int& value);
    int Get(int index, int& value) const;

    int* m_data;    // 0x08

private:
    int AdjustVectorCapacity(int capacity, unsigned char exact);

    int m_size;     // 0x0c
    int m_capacity; // 0x10
};

// The remaining CKZ-derived callers still use the old template API.
                                                                     

template <class T>
class CVectorLegacy : public CClass {
public:
    /** Runtime class id: CStringToKey("CVectorLegacy"). */
    static const unsigned int ClassId = 0x36034282;

    /** `capacity` elements preallocated; `growBy` extra slots per growth (<= 0: double). */
    CVectorLegacy(int capacity = 0, int growBy = 0);
    CVectorLegacy(const CVectorLegacy<T>& other);
    virtual ~CVectorLegacy();

    CVectorLegacy<T>& operator=(const CVectorLegacy<T>& other);

    int Size() const
    {
        return m_size;
    }

    int GetCapacity() const
    {
        return m_capacity;
    }

    T& operator[](int index)
    {
        return m_data[index];
    }

    const T& operator[](int index) const
    {
        return m_data[index];
    }

    T& Get(int index)
    {
        return m_data[index];
    }

    const T& Get(int index) const
    {
        return m_data[index];
    }

    /** Raw element block (NULL while nothing was ever allocated). */
    T* GetData()
    {
        return m_data;
    }

    /** Append one element. */
    unsigned char Add(const T& element)
    {
        return AddAll(m_size, &element, 1);
    }

    /** Insert one element before `index`. */
    unsigned char Insert(int index, const T& element)
    {
        return AddAll(index, &element, 1);
    }

    /** Index of the first element equal to `element`, -1 if absent. */
    inline int IndexOf(const T& element) const
    {
        for (int index = 0; index < m_size; index++) {
            if (element == m_data[index]) {
                return index;
            }
        }
        return -1;
    }

    inline unsigned char Contains(const T& element) const
    {
        for (int index = 0; index < m_size; index++) {
            if (m_data[index] == element) {
                return 1;
            }
        }
        return 0;
    }

    /** Remove the element at `index`. */
    void RemoveAt(int index)
    {
        RemoveRange(index, index + 1);
    }

    /** Insert `count` elements before `index` (index == Size() appends). Always returns 1. */
    unsigned char AddAll(int index, const T* elements, int count);
    /** Append `count` copies without checking the capacity. */
    void AddAllNoResize(const T* elements, int count);
    /** Assign the element at `index`. */
    void Set(int index, const T& value);
    /** Remove the elements [start, end). */
    void RemoveRange(int start, int end);
    /** Remove the first element equal to `element`; returns 1 when one was found. */
    unsigned char Remove(const T& element);
    /** Remove every element that also appears in `other`; returns 1 when anything was removed. */
    unsigned char RemoveAll(const CVectorLegacy<T>& other);
    /** Destroy all elements (the storage is kept). */
    void Clear();
    /** Make room for at least `capacity` elements. */
    void EnsureCapacity(int capacity);
    /** Grow with default-constructed elements or shrink to `size` elements. */
    void SetSize(int size);
    /** Shrink the storage to the current size. */
    void TrimToSize();

    /** Destroy `count` elements starting at `elements`, last first. */
    static void DestroyElements(const T* elements, int count);

    T* m_data;          // 0x08 element storage
    int m_size;         // 0x0c elements in use
    int m_capacity;     // 0x10 elements allocated
    int m_growBy;       // 0x14 growth step (<= 0: grow by the current capacity)

private:
    /** Capacity to grow to so that at least `required` elements fit. */
    int GetGrownCapacity(int required) const
    {
        int capacity = m_capacity;
        if (capacity < required) {
            int step = m_capacity;
            if (m_growBy > 0) {
                step = m_growBy;
            }
            capacity = m_capacity + step;
            if (capacity < required) {
                capacity = required;
            }
        }
        return capacity;
    }

    /** Exchange storage and settings with `other` (used to commit a rebuilt copy). */
    void Swap(CVectorLegacy<T>& other)
    {
        T* data = m_data;
        m_data = other.m_data;
        other.m_data = data;
        int size = m_size;
        m_size = other.m_size;
        other.m_size = size;
        int capacity = m_capacity;
        m_capacity = other.m_capacity;
        other.m_capacity = capacity;
        int growBy = m_growBy;
        m_growBy = other.m_growBy;
        other.m_growBy = growBy;
    }
};

       

#include "CVector.inl"

#endif
