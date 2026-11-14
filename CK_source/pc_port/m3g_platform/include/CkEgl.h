// Render target surfaces of the fake EGL that m3gcore runs on (see CkEgl.cpp).
#pragma once

#include <EGL/egl.h>

namespace ck::egl {

/**
 * Create a surface for m3gBindEGLSurfaceTarget. It only carries a size: m3gcore draws into
 * whatever framebuffer is bound in the current GL context when it renders.
 */
EGLSurface CreateSurface(EGLint width, EGLint height);

/** Change the size reported for a surface, e.g. after the window was resized. */
void ResizeSurface(EGLSurface surface, EGLint width, EGLint height);

void DestroySurface(EGLSurface surface);

} // namespace ck::egl
