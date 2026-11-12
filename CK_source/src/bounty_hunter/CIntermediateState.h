// CIntermediateState.h - transition screen between the map and the 3D game: interrupts its parent
// on the third update. Owner: src/bounty_hunter/CIntermediateState.cpp (game2).
// CK layout from CIntermediateState::CIntermediateState() 0xfb700: Window (0xa4) + update counter.
#ifndef CINTERMEDIATESTATE_H
#define CINTERMEDIATESTATE_H

#include "WindowBase.h"

class CIntermediateState : public Window {
public:
    CIntermediateState();
    virtual ~CIntermediateState() {} // Original weak D1/D0 only call the Window destructor.
    virtual void Update();

    int m_updateCount; // 0xa4 updates so far; the third one calls SetInterrupt(0)
};

#endif
