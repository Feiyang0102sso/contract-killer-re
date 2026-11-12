// swvglruntime.cpp - OpenGL ES runtime services of the Swerve porting layer: mutable images rendered
// through Glu render surfaces, renderer capabilities, the VBO populator and the buffer object
// entry points under engine-private names. The engine core calls them with C linkage.
#include <OpenGLES/ES1/gl.h>
#include "NPMalloc.h"
#include "ICGraphics.h"
#include "ICRenderSurface.h"
#include "CSwerve.h"
#include "CSwerveGraphics3D.h"
#include "SwvPort.h"

// ---- constants ----
/** swvImage format whose pixels keep their alpha (otherwise RGB). */
static const unsigned int IMAGE_FORMAT_RGBA = 0x64;
/** Largest viewport / texture the engine is told about. */
static const int MAX_VIEWPORT_SIZE = 0x400;
static const int MAX_TEXTURE_SIZE = 0x400;
/** Submesh packing reported to the engine. */
static const int SUBMESH_PACKING_STITCH = 2;
/** Texture coordinate array types of swvVertexData. */
enum {
    TEXCOORD_FLOAT2 = 5,
    TEXCOORD_FLOAT3 = 6,
    TEXCOORD_FIXED2 = 7,
    TEXCOORD_FIXED3 = 8
};
/** Position / normal array type holding 16 bit components (else 8 bit). */
static const int VERTEX_TYPE_SHORT = 4;
/** Color array type holding RGB bytes (else RGBA). */
static const int COLOR_TYPE_RGB = 2;
/** Texture coordinate units of the populator. */
static const int TEXCOORD_UNITS = 2;

static int swvPopulateVBO_CM_V3F_N3F_T23F_C4UB(const swvVertexData* data, void* buffer, swvVBOFormat* format);

extern "C" {

void swvGLRuntimePrepare()
{
}

void swvGLRuntimeCleanup()
{
}

/** No stretch blit support. */
void* swvGetGLStretchBlit()
{
    return NULL;
}

/** Bind a render surface to `image` and make it the target. Returns 1 on success. */
int swvGLBindImage(swvImage* image)
{
    GLRuntime* runtime = new GLRuntime();
    if (runtime != NULL) {
        ICRenderSurface* surface = CSwerve::GetInstance()->m_graphics3D->GetMutableSurface(image);
        if (surface != NULL) {
            runtime->m_surface = surface;
            runtime->m_previousTarget = ICGraphics::GetInstance()->GetTargetSurface();
            image->m_glRuntime = runtime;
            runtime->m_surface->Bind();
            return 1;
        }
        delete runtime;
    }
    return 0;
}

/** Make the surface of a bound image the target again. Returns 1 on success. */
int swvGLRebindImage(swvImage* image)
{
    GLRuntime* runtime = image->m_glRuntime;
    if (runtime == NULL) {
        return 0;
    }
    ICRenderSurface* surface = runtime->m_surface;
    if (surface == NULL) {
        return 0;
    }
    surface->Bind();
    return 1;
}

/**
 * Read the rendered pixels back into the image (GL rows run bottom-up, so they are flipped; RGB
 * images drop the alpha) and unbind it. Returns 1 when the pixels were read.
 */
int swvGLReleaseImage(swvImage* image)
{
    int result = 0;
    if (swvGLRebindImage(image) == 0) {
        return result;
    }

    int width = image->m_width;
    int height = image->m_height;
    unsigned char* destination = image->m_pixels;
    GLRuntime* runtime = image->m_glRuntime;
    int rowBytes = width * 4;
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    unsigned char* pixels = new unsigned char[rowBytes * height];
    if (pixels != NULL) {
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        if (image->m_format == IMAGE_FORMAT_RGBA) {
            unsigned char* source = pixels + (height - 1) * width * 4;
            for (int y = 0; y < height; y++) {
                swvMemCpy(destination, source, rowBytes);
                destination += rowBytes;
                source -= width * 4;
            }
        } else {
            unsigned char* sourceRow = pixels + (height - 1) * width * 4;
            for (int y = 0; y < height; y++) {
                unsigned char* source = sourceRow;
                unsigned char* target = destination;
                for (int x = 0; x < width; x++) {
                    *target++ = *source++;
                    *target++ = *source++;
                    *target++ = *source++;
                    source++;
                }
                destination += width * 3;
                sourceRow -= width * 4;
            }
        }
        delete[] pixels;
        result = 1;
    }
    delete runtime;
    image->m_glRuntime = NULL;
    return result;
}

int swvGLShareContext()
{
    return 1;
}

int swvGLUnpalettizeImage2D()
{
    return 0;
}

int swvGLCanDeleteImage2DData()
{
    return 1;
}

void swvGetGLMaxViewportDims(int* dimensions)
{
    dimensions[0] = MAX_VIEWPORT_SIZE;
    dimensions[1] = MAX_VIEWPORT_SIZE;
}

int swvGetGLMaxTextureSize()
{
    return MAX_TEXTURE_SIZE;
}

int swvGetGLSubmeshPacking()
{
    swvDbgPrintf("swvGetGLSubmeshPacking: %s", "STITCH");
    return SUBMESH_PACKING_STITCH;
}

int swvGLSupportVBO()
{
    swvDbgPrintf("swvGLSupportVBO: %s", "TRUE");
    return 1;
}

swvPopulateVBOFunction swvGetGLPopulateVBO()
{
    return swvPopulateVBO_CM_V3F_N3F_T23F_C4UB;
}

}

/**
 * Interleave float positions, float normals, up to two float texture coordinates (2 or 3
 * components) and RGBA byte colors into `buffer` and describe the layout in `format`.
 * Returns the byte size of the VBO; with a NULL buffer only the layout and size are computed.
 */
static int swvPopulateVBO_CM_V3F_N3F_T23F_C4UB(const swvVertexData* data, void* buffer, swvVBOFormat* format)
{
    format->m_positions.m_offset = -1;
    format->m_normals.m_offset = -1;
    for (int unit = 0; unit < TEXCOORD_UNITS; unit++) {
        format->m_texCoords[unit].m_offset = -1;
    }
    format->m_colors.m_offset = -1;

    // Layout of one vertex
    int stride = 0;
    if (data->m_positions.m_data != NULL) {
        format->m_positions.m_offset = 0;
        format->m_positions.m_type = GL_FLOAT;
        stride = 12;
    }
    if (data->m_normals.m_data != NULL) {
        format->m_normals.m_offset = stride;
        format->m_normals.m_type = GL_FLOAT;
        stride += 12;
    }
    for (int unit = 0; unit < TEXCOORD_UNITS; unit++) {
        if (data->m_texCoords[unit].m_data != NULL) {
            format->m_texCoords[unit].m_offset = stride;
            format->m_texCoords[unit].m_type = GL_FLOAT;
            switch (data->m_texCoords[unit].m_type) {
            case TEXCOORD_FLOAT2:
            case TEXCOORD_FIXED2:
                stride += 8;
                break;
            case TEXCOORD_FLOAT3:
            case TEXCOORD_FIXED3:
                stride += 12;
                break;
            }
        }
    }
    if (data->m_colors.m_data != NULL) {
        format->m_colors.m_offset = stride;
        format->m_colors.m_type = GL_UNSIGNED_BYTE;
        stride += 4;
    }
    format->m_positions.m_stride = stride;
    format->m_normals.m_stride = stride;
    for (int unit = 0; unit < TEXCOORD_UNITS; unit++) {
        format->m_texCoords[unit].m_stride = stride;
    }
    format->m_colors.m_stride = stride;

    if (buffer != NULL) {
        unsigned char* cursor = (unsigned char*)buffer;
        // alpha byte of RGB colors (m_alpha is 16.16 fixed point)
        int alpha = (data->m_alpha * 255 + 0x8000) >> 16;
        for (int vertex = 0; vertex < data->m_vertexCount; vertex++) {
            if (data->m_positions.m_data != NULL) {
                float position[3];
                if (data->m_positions.m_type == VERTEX_TYPE_SHORT) {
                    const short* source = (const short*)data->m_positions.m_data;
                    position[0] = source[vertex * 3];
                    position[1] = source[vertex * 3 + 1];
                    position[2] = source[vertex * 3 + 2];
                } else {
                    const signed char* source = (const signed char*)data->m_positions.m_data;
                    position[0] = source[vertex * 3];
                    position[1] = source[vertex * 3 + 1];
                    position[2] = source[vertex * 3 + 2];
                }
                swvMemCpy(cursor, position, 12);
                cursor += 12;
            }
            if (data->m_normals.m_data != NULL) {
                float normal[3];
                if (data->m_normals.m_type == VERTEX_TYPE_SHORT) {
                    const short* source = (const short*)data->m_normals.m_data;
                    normal[0] = source[vertex * 3];
                    normal[1] = source[vertex * 3 + 1];
                    normal[2] = source[vertex * 3 + 2];
                } else {
                    const signed char* source = (const signed char*)data->m_normals.m_data;
                    normal[0] = source[vertex * 3];
                    normal[1] = source[vertex * 3 + 1];
                    normal[2] = source[vertex * 3 + 2];
                }
                swvMemCpy(cursor, normal, 12);
                cursor += 12;
            }
            for (int unit = 0; unit < TEXCOORD_UNITS; unit++) {
                if (data->m_texCoords[unit].m_data != NULL) {
                    switch (data->m_texCoords[unit].m_type) {
                    case TEXCOORD_FLOAT3: {
                        const float* source = (const float*)data->m_texCoords[unit].m_data;
                        swvMemCpy(cursor, source + vertex * 3, 12);
                        cursor += 12;
                        break;
                    }
                    case TEXCOORD_FLOAT2: {
                        const float* source = (const float*)data->m_texCoords[unit].m_data;
                        swvMemCpy(cursor, source + vertex * 2, 8);
                        cursor += 8;
                        break;
                    }
                    case TEXCOORD_FIXED3: {
                        // 16.16 fixed point to float
                        const int* source = (const int*)data->m_texCoords[unit].m_data;
                        float texCoord[3];
                        texCoord[0] = (float)source[vertex * 3] * (1.0f / 65536.0f);
                        texCoord[1] = (float)source[vertex * 3 + 1] * (1.0f / 65536.0f);
                        texCoord[2] = (float)source[vertex * 3 + 2] * (1.0f / 65536.0f);
                        swvMemCpy(cursor, texCoord, 12);
                        cursor += 12;
                        break;
                    }
                    case TEXCOORD_FIXED2: {
                        const int* source = (const int*)data->m_texCoords[unit].m_data;
                        float texCoord[2];
                        texCoord[0] = (float)source[vertex * 2] * (1.0f / 65536.0f);
                        texCoord[1] = (float)source[vertex * 2 + 1] * (1.0f / 65536.0f);
                        swvMemCpy(cursor, texCoord, 8);
                        cursor += 8;
                        break;
                    }
                    }
                }
            }
            if (data->m_colors.m_data != NULL) {
                int components = 4;
                if (data->m_colors.m_type == COLOR_TYPE_RGB) {
                    components = 3;
                }
                const unsigned char* source = (const unsigned char*)data->m_colors.m_data;
                cursor[0] = source[vertex * components];
                cursor[1] = source[vertex * components + 1];
                cursor[2] = source[vertex * components + 2];
                if (data->m_colors.m_type == COLOR_TYPE_RGB) {
                    cursor[3] = alpha;
                } else {
                    cursor[3] = (source[vertex * components + 3] * data->m_alpha + 0x8000) >> 16;
                }
                cursor += 4;
            }
        }
    }
    return data->m_vertexCount * stride;
}

extern "C" {

void glGenBuffersSUPERSCAPE(int count, unsigned int* buffers)
{
    glGenBuffers(count, buffers);
}

void glDeleteBuffersSUPERSCAPE(int count, const unsigned int* buffers)
{
    glDeleteBuffers(count, buffers);
}

void glBindBufferSUPERSCAPE(unsigned int target, unsigned int buffer)
{
    glBindBuffer(target, buffer);
}

void glBufferDataSUPERSCAPE(unsigned int target, int size, const void* data, unsigned int usage)
{
    glBufferData(target, size, data, usage);
}

void glBufferSubDataSUPERSCAPE(unsigned int target, int offset, int size, const void* data)
{
    glBufferSubData(target, offset, size, data);
}

}
