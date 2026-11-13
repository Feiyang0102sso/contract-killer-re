// CVector.h - CK's integer array and the port's growable array template (created by b006).
// Elements live in one malloc'd block; they are copy-constructed in place when added and
// destroyed (last to first) when removed. Member definitions follow the class below.
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

       

// ---- member definitions (formerly CVector.inl; the CK original has them in CVector.h) ----
// Member definitions of CVectorLegacy<T> (created by b006).
// No `inline` keyword here on purpose: clang only gives the raised inline-hint threshold to
// functions spelled `inline`, and the original instantiations are inlined like plain templates.

                                                                     

template <class T>
CVectorLegacy<T>::CVectorLegacy(int capacity, int growBy)
    : CClass(ClassId)
{
    m_growBy = growBy;
    m_size = 0;
    m_data = NULL;
    if (capacity > 0) {
        m_data = (T*)malloc(capacity * sizeof(T));
    }
    m_capacity = capacity;
}

template <class T>
CVectorLegacy<T>::CVectorLegacy(const CVectorLegacy<T>& other)
    : CClass(ClassId)
{
    m_growBy = other.m_growBy;
    int capacity = other.m_capacity;
    m_size = 0;
    m_data = NULL;
    if (capacity > 0) {
        m_data = (T*)malloc(capacity * sizeof(T));
    }
    m_capacity = capacity;
    AddAllNoResize(other.m_data, other.m_size);
}

template <class T>
CVectorLegacy<T>::~CVectorLegacy()
{
    if (m_data != NULL) {
        DestroyElements(m_data, m_size);
        free(m_data);
    }
}

template <class T>
CVectorLegacy<T>& CVectorLegacy<T>::operator=(const CVectorLegacy<T>& other)
{
    if (this != &other) {
        CVectorLegacy<T> copy(other);
        Swap(copy);
    }
    return *this;
}

template <class T>
unsigned char CVectorLegacy<T>::AddAll(int index, const T* elements, int count)
{
    int oldSize = m_size;
    if (oldSize + count > m_capacity) {
        // rebuild into a bigger block: head, new elements, tail
        CVectorLegacy<T> grown(GetGrownCapacity(oldSize + count), m_growBy);
        grown.AddAllNoResize(m_data, index);
        grown.AddAllNoResize(elements, count);
        grown.AddAllNoResize(m_data + index, oldSize - index);
        Swap(grown);
    } else {
        // the part landing past the old end is constructed, the rest is assigned
        int assigned = count;
        if (index + count > oldSize) {
            assigned = oldSize - index;
        }
        AddAllNoResize(elements + assigned, count - assigned);
        AddAllNoResize(m_data + index, assigned);
        for (int position = index; position < oldSize; position++) {
            Set(position, elements[position - index]);
        }
    }
    return 1;
}

template <class T>
void CVectorLegacy<T>::AddAllNoResize(const T* elements, int count)
{
    for (int i = 0; i < count; i++) {
        new (&m_data[m_size]) T(elements[i]);
        m_size++;
    }
}

template <class T>
void CVectorLegacy<T>::Set(int index, const T& value)
{
    m_data[index] = value;
}

template <class T>
void CVectorLegacy<T>::RemoveRange(int start, int end)
{
    int removed = end - start;
    if (removed < 1) {
        return;
    }
    int size = m_size;
    int to = start;
    for (int from = end; from < size; from++) {
        Set(to, m_data[from]);
        to++;
    }
    m_size -= removed;
    DestroyElements(m_data + m_size, removed);
}

template <class T>
unsigned char CVectorLegacy<T>::Remove(const T& element)
{
    int index = IndexOf(element);
    if (index < 0) {
        return 0;
    }
    RemoveRange(index, index + 1);
    return 1;
}

template <class T>
unsigned char CVectorLegacy<T>::RemoveAll(const CVectorLegacy<T>& other)
{
    unsigned char removedAny = 0;
    if (other.m_size == 0) {
        return removedAny;
    }
    for (int index = m_size - 1; index >= 0; index--) {
        if (other.IndexOf(m_data[index]) >= 0) {
            RemoveRange(index, index + 1);
            removedAny = 1;
        }
    }
    return removedAny;
}

template <class T>
void CVectorLegacy<T>::Clear()
{
    if (m_data != NULL) {
        DestroyElements(m_data, m_size);
        m_size = 0;
    }
}

template <class T>
void CVectorLegacy<T>::EnsureCapacity(int capacity)
{
    if (m_capacity < capacity) {
        CVectorLegacy<T> grown(GetGrownCapacity(capacity), m_growBy);
        grown.AddAllNoResize(m_data, m_size);
        Swap(grown);
    }
}

template <class T>
void CVectorLegacy<T>::SetSize(int size)
{
    int oldSize = m_size;
    if (oldSize < size) {
        EnsureCapacity(size);
        T empty = T();
        for (int added = size - oldSize; added > 0; added--) {
            AddAllNoResize(&empty, 1);
        }
    } else if (oldSize > size) {
        RemoveRange(size, oldSize);
    }
}

template <class T>
void CVectorLegacy<T>::TrimToSize()
{
    if (m_size < m_capacity) {
        CVectorLegacy<T> trimmed(m_size, m_growBy);
        trimmed.AddAllNoResize(m_data, m_size);
        Swap(trimmed);
    }
}

template <class T>
void CVectorLegacy<T>::DestroyElements(const T* elements, int count)
{
    for (int index = count - 1; index >= 0; index--) {
        elements[index].~T();
    }
}

       

#endif
