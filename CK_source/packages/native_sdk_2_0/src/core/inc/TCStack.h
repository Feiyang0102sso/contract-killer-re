// PROVISIONAL (created by gfx): TCStack.h - fixed-capacity stack (CK 1.5.2; only what the
// display program uses; layout from TCStack<Component>::Initialize).
#ifndef TCSTACK_H
#define TCSTACK_H

#include <stddef.h>
#include "NPNew.h"

template <class T>
class TCStack {
public:
    // no destructor: arrays of stacks are allocated without an array cookie (PriorityStacks)
    TCStack() : m_data(NULL), m_capacity(0), m_top(0) {}

    /** Allocate room for `capacity` entries; slot 0 is the bottom, so the stack starts at 1. */
    void Initialize(unsigned int capacity)
    {
        m_data = new T[capacity];
        m_capacity = capacity;
        m_top = 1;
    }

    /** Push a copy of the top entry. */
    void PushTop()
    {
        m_data[m_top] = m_data[m_top - 1];
        m_top++;
    }

    void Destroy()
    {
        delete[] m_data;
        m_data = NULL;
        m_capacity = 0;
        m_top = 0;
    }

    T* m_data;                  // 0x00
    unsigned int m_capacity;    // 0x04
    int m_top;                  // 0x08
};

#endif
