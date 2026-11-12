// TCVector.h - simple growable array template of the Glu platform (CK 1.5.2; header-only).
// Created by gserve (2026-11-10) from the CK weak instantiations: vtable only holds the two
// destructors; the storage comes from new[] (np_malloc, with an array cookie for types that
// need destruction) and elements are copied with operator=.
// Layout: +0x00 vtable, +0x04 class id, +0x08 data, +0x0c size, +0x10 capacity, +0x14 growBy.
#ifndef TCVECTOR_H
#define TCVECTOR_H

#include <stddef.h>
#include "CClass.h"

template <class T>
class TCVector : public CClass {
public:
    /** Runtime class id (stored by every inlined constructor). */
    static const unsigned int ClassId = 0x0603428f;

    TCVector()
        : CClass(ClassId)
    {
        m_data = NULL;
        m_size = 0;
        m_capacity = 0;
        m_growBy = 0;
    }

    TCVector(const TCVector<T>& other)
        : CClass(ClassId), m_data(NULL)
    {
        Copy(other);
    }

    TCVector<T>& operator=(const TCVector<T>& other)
    {
        // no self-assignment check in the original (TCVector<TCVector<T> >::CopyInto calls Copy directly) (gserve2)
        Copy(other);
        return *this;
    }

    virtual ~TCVector()
    {
        if (m_data != NULL) {
            delete[] m_data;
        }
    }

    int Size() const
    {
        return m_size;
    }

    T& operator[](int index)
    {
        return m_data[index];
    }

    const T& operator[](int index) const
    {
        return m_data[index];
    }

    /** Forget all elements (the storage and the elements' memory are kept). */
    void Clear()
    {
        m_size = 0;
    }

    /** Make room for at least `capacity` elements (grows by m_growBy, or doubles when <= 0). */
    void EnsureCapacity(int capacity)
    {
        if (m_capacity < capacity) {
            int step = m_capacity;
            if (m_growBy > 0) {
                step = m_growBy;
            }
            int grown = step + m_capacity;
            if (grown < capacity) {
                grown = capacity;
            }
            m_capacity = grown;
            T* data = new T[grown];
            CopyInto(data, m_size, 0);
            if (m_data != NULL) {
                delete[] m_data;
            }
            m_data = data;
        }
    }

    /** Copy up to `count` elements starting at `start` into `dest`. */
    void CopyInto(T* dest, int count, int start)
    {
        while (start < m_size && count != 0) {
            *dest = m_data[start];
            count--;
            dest++;
            start++;
        }
    }

    /** Append one element; always returns 1. */
    unsigned char Add(const T& element)
    {
        EnsureCapacity(m_size + 1);
        m_data[m_size] = element;
        m_size++;
        return 1;
    }

    /** Insert one element before `index`. */
    void Add(int index, const T& element)
    {
        EnsureCapacity(m_size + 1);
        for (int position = m_size - 1; position >= index; position--) {
            m_data[position + 1] = m_data[position];
        }
        m_data[index] = element;
        m_size++;
    }

    /** Append all elements of `other`; always returns 1. */
    unsigned char AddAll(const TCVector<T>& other)
    {
        EnsureCapacity(m_size + other.m_size);
        int start = m_size;
        for (int index = m_size; index < m_size + other.m_size; index++) {
            m_data[index] = other.m_data[index - start];
        }
        m_size = m_size + other.m_size;
        return 1;
    }

    /** Remove the element at `index` (later elements move down). */
    void RemoveAt(int index)
    {
        // the original shifts the tail down with the CopyInto loop written out (gserve2)
        T* dest = m_data + index;
        int count = m_size - index - 1;
        int start = index + 1;
        while (start < m_size && count != 0) {
            *dest = m_data[start];
            count--;
            dest++;
            start++;
        }
        m_size--;
    }

    /** Remove the first element equal to `element`; returns 1 when one was found. */
    unsigned char Remove(const T& element)
    {
        for (int index = 0; index < m_size; index++) {
            if (element == m_data[index]) {
                RemoveAt(index);
                return 1;
            }
        }
        return 0;
    }

    /** Remove every element that also appears in `other`; returns 1 when anything was removed. */
    unsigned char RemoveAll(const TCVector<T>& other)
    {
        unsigned char removed = 0;
        for (int index = 0; index < other.m_size; index++) {
            for (int position = m_size - 1; position >= 0; position--) {
                if (other.m_data[index] == m_data[position]) {
                    RemoveAt(position);
                    removed = 1;
                }
            }
        }
        return removed;
    }

    /** Replace the contents with a copy of `other` (same capacity and growth step). */
    void Copy(const TCVector<T>& other)
    {
        if (m_data != NULL) {
            delete[] m_data;
        }
        if (other.m_capacity != 0) {
            m_data = new T[other.m_capacity];
            for (int index = 0; index < other.m_size; index++) {
                m_data[index] = other.m_data[index];
            }
        } else {
            m_data = NULL;
        }
        m_capacity = other.m_capacity;
        m_growBy = other.m_growBy;
        m_size = other.m_size;
    }

    T* m_data;          // +0x08 element storage (new[])
    int m_size;         // +0x0c elements in use
    int m_capacity;     // +0x10 elements allocated
    int m_growBy;       // +0x14 growth step (<= 0: grow by the current capacity)
};

#endif
