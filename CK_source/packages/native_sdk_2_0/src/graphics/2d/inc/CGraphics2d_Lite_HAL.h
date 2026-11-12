// CGraphics2d_Lite_HAL.h - CK's hardware 2D renderer singleton.
#ifndef CGRAPHICS2D_LITE_HAL_H
#define CGRAPHICS2D_LITE_HAL_H

#include "CGraphics2dSingleton.h"

class CGraphics2d_Lite_OGLES;

class CGraphics2d_Lite_HAL : public CGraphics2dSingleton {
public:
    CGraphics2d_Lite_HAL();
    virtual ~CGraphics2d_Lite_HAL();
    virtual unsigned char Initialize();
    virtual void Destroy();
    virtual void SetActiveAbstractionLayer(ICGraphics::Abstraction abstraction);
    virtual ICGraphics::Abstraction GetActiveAbstractionLayer();
    virtual void RenderBegin(unsigned char writeAlpha);
    virtual void RenderEnd();
    virtual void DisplayListBegin(IDisplayProgram& program, IDisplayProgram::Mode mode);
    virtual void DisplayListEnd();
    virtual void Run(IDisplayProgram& program);
    virtual void SetOrigin(Origin origin);
    virtual void SetShaderMode(ShaderMode mode);
    virtual void SetShader(ICShader* shader);
    virtual void SetColor(int alpha, int red, int green, int blue);
    virtual void SetColor(const Color_ARGB_fixed& color);
    virtual void SetColor(unsigned int argb);
    virtual void Enable(State state);
    virtual void Disable(State state);
    virtual void PushState(State state);
    virtual void PopState(State state);
    virtual void SetBlendArg(BlendArg arg);
    virtual void LoadIdentity();
    virtual void LoadTransform(const CMatrix2d* matrix, const CVector2d* translation);
    virtual void MultiplyTransform(const CMatrix2d* matrix, const CVector2d* translation);
    virtual void Translate(int x, int y);
    virtual void Rotate(int angle);
    virtual void Scale(int x, int y);
    virtual void PushTransform();
    virtual void PopTransform();
    virtual void GetTransform(const CMatrix2d** matrix, const CVector2d** translation);
    virtual void SetVertexPositionCoords(int componentCount, unsigned int type, int stride,
                                 const void* coords);
    virtual void SetClip(unsigned short x, unsigned short y, unsigned short width, unsigned short height);
    virtual void SetClip(const CRectangle& clip);
    virtual const CRectangle& GetClip() const;
    virtual void Draw(ICRenderSurface& surface, Flip flip, const CRectangle* source);
    virtual void Draw(ICRenderText2d& text);
    virtual void Draw(Primitive primitive, int first, int count);

private:
    friend class CGraphics2d_Lite_OGLES;

    void InitializeHardware();

    // m_initialized / m_softwareReady / m_hardwareReady / m_abstraction live in CGraphics2dAbstract
    ICGraphics2d* m_device;     // 0x14
};

#endif
