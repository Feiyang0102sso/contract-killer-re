// CIntermediateState.cpp - map / 3D game transition screen (CK 1.5.2, written by game2).
#include "CIntermediateState.h"

CIntermediateState::CIntermediateState() : Window(false), m_updateCount(0)
{
}

void CIntermediateState::Update()
{
    if (m_updateCount == 2) {
        SetInterrupt(0);
    }
    // The virtual call may change the counter: it is read again afterwards.
    m_updateCount++;
}
