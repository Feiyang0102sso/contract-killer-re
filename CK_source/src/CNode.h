// CNode.h - lifecycle and rendering hooks of a CK application node.
#ifndef CNODE_H
#define CNODE_H

#include "ICGraphics2d.h"

class CNode {
public:
    CNode();
    virtual ~CNode();

    virtual int InitGraphicsOptimizations(ICGraphics2d::IDisplayProgram::Mode mode,
                                          unsigned short capacity, unsigned int layers,
                                          unsigned short capacity12, unsigned short capacity10,
                                          unsigned short capacity14);
    virtual void DestroyGraphicsOptimizations();
    virtual int Start();
    virtual void Stop();
    virtual void Activate();
    virtual void Deactivate();
    virtual int HandleEvent(unsigned int eventId, unsigned int param1, unsigned int param2);
    virtual int HandleUpdate(int elapsedMillis);
    virtual int HandleRender();
    virtual void SetInterrupt(int interrupt);
    virtual int GetInterrupt();
    virtual void ClearInterrupt();
    virtual void RenderBegin();
    virtual void RenderEnd();
    virtual void SyncExternalSystemsWithRenderBegin();

    unsigned int m_nodeId;                              // +0x04
    int m_interrupt;                                    // +0x08
    ICGraphics2d::IDisplayProgram* m_displayProgram;      // +0x0c
    ICGraphics2d::IDisplayProgram::Mode m_displayMode;    // +0x10
};

#endif
