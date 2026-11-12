// CVector.cpp - CK's growable array of 32-bit values.
#include "CVector.h"

enum {
    VectorSuccess = 0,
    VectorOutOfMemory = 2,
    VectorIndexOutOfRange = 5,
    VectorMinimumCapacity = 10
};

CVector::CVector()
    : CClass(ClassId), m_data(NULL), m_size(0), m_capacity(0)
{
}

CVector::~CVector()
{
    if (m_data != NULL) {
        delete[] m_data;
        m_data = NULL;
    }
    m_size = 0;
    m_capacity = 0;
}

void CVector::Clear()
{
    m_size = 0;
}

int CVector::Size()
{
    return m_size;
}

int CVector::Add(int value)
{
    return Add(m_size, value);
}

int CVector::Add(int index, int value)
{
    if (index < 0 || index > m_size) {
        return VectorIndexOutOfRange;
    }
    int result = AdjustVectorCapacity(m_size + 1, 0);
    if (result == VectorSuccess) {
        int position = m_size++;
        for (; position > index; position--) {
            m_data[position] = m_data[position - 1];
        }
        m_data[index] = value;
    }
    return result;
}

int CVector::AdjustVectorCapacity(int capacity, unsigned char exact)
{
    if (!exact) {
        if (m_capacity < capacity) {
            capacity = (int)((unsigned int)capacity * 2);
            if (capacity < VectorMinimumCapacity) {
                capacity = VectorMinimumCapacity;
            }
        }
        if (capacity <= m_capacity) {
            return VectorSuccess;
        }
    } else {
        if (capacity == 0) {
            if (m_data != NULL) {
                delete[] m_data;
                m_data = NULL;
            }
            m_capacity = 0;
            return VectorSuccess;
        }
        if (capacity == m_capacity) {
            return VectorSuccess;
        }
    }

    int* oldData = m_data;
    m_data = new int[capacity];
    if (m_data == NULL) {
        return VectorOutOfMemory;
    }
    if (oldData != NULL) {
        int copied = capacity;
        if (m_size < copied) {
            copied = m_size;
        }
        if (copied > 0) {
            memcpy(m_data, oldData, copied * sizeof(int));
        }
        delete[] oldData;
    }
    int remaining = capacity - m_size;
    if (remaining > 0) {
        memset(m_data + m_size, 0, remaining * sizeof(int));
    }
    m_capacity = capacity;
    return VectorSuccess;
}

int CVector::Remove(int index, int& value)
{
    if (index < 0 || index >= m_size) {
        return VectorIndexOutOfRange;
    }
    value = m_data[index];
    for (int position = index; position < m_size - 1; position++) {
        m_data[position] = m_data[position + 1];
    }
    int result = AdjustVectorCapacity(m_size - 1, 0);
    if (result == VectorSuccess) {
        m_size--;
    }
    return result;
}

int CVector::Get(int index, int& value) const
{
    if (index < 0 || index >= m_size) {
        return VectorIndexOutOfRange;
    }
    value = m_data[index];
    return VectorSuccess;
}
