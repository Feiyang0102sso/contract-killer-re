// CGraphics_OGLES_Cocoa_mm.mm - Objective-C side of the EAGL graphics device: EAGL version query.
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <QuartzCore/CAEAGLLayer.h>

/** EAGL framework version (major, minor). */
void CGraphics_OGLES_Cocoa_GetEAGLVersion(unsigned int* major, unsigned int* minor)
{
    EAGLGetVersion(major, minor);
}

void* CGraphics_OGLES_Cocoa_CreateContext(void* layer, NSString* format)
{
    CAEAGLLayer* drawable = (CAEAGLLayer*)layer;
    [drawable setOpaque:YES];
    [drawable setDrawableProperties:[NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
        format, kEAGLDrawablePropertyColorFormat, nil]];
    return [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
}

void* CGraphics_OGLES_Cocoa_CreateContext_R5G6B5(void* layer)
{
    return CGraphics_OGLES_Cocoa_CreateContext(layer, kEAGLColorFormatRGB565);
}

void* CGraphics_OGLES_Cocoa_CreateContext_A8B8G8R8(void* layer)
{
    return CGraphics_OGLES_Cocoa_CreateContext(layer, kEAGLColorFormatRGBA8);
}

void CGraphics_OGLES_Cocoa_ReleaseContext(void* context)
{
    [(EAGLContext*)context release];
}

void CGraphics_OGLES_Cocoa_SetCurrentContext(void* context)
{
    [EAGLContext setCurrentContext:(EAGLContext*)context];
}

void CGraphics_OGLES_Cocoa_PresentRenderBuffer(void* context)
{
    [(EAGLContext*)context presentRenderbuffer:GL_RENDERBUFFER_OES];
}

unsigned char CGraphics_OGLES_Cocoa_RenderBufferStorage(void* layer, void* context)
{
    return [(EAGLContext*)context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(CAEAGLLayer*)layer] != NO;
}
