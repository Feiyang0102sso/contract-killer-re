// SwvPort.h - Glu's porting layer of the Swerve 3D engine as linked into CK (swv* entry points
// with C linkage, called by the prebuilt engine core). (created by ggn)
// CK predates the malij297_* layer of CKZ (SwerveApi.h): different type names (swvImage,
// swvVertexData, swvVBOFormat from the mangled names) and layouts recovered from the CK code.
// Field names other than the type names are descriptive guesses.
#ifndef SWVPORT_H
#define SWVPORT_H

#include <stddef.h>

class ICFileMgr;
class ICFile;
struct GLRuntime;

/** Image the engine renders into (mutable Image2D); see swvGLBindImage / swvGLReleaseImage. */
struct swvImage {
    int m_width;                    // 0x00
    int m_height;                   // 0x04
    unsigned int m_format;          // 0x08 0x64: RGBA, otherwise RGB
    unsigned int m_unknown0c;       // 0x0c
    unsigned char* m_pixels;        // 0x10 receives the rendered pixels on release
    GLRuntime* m_glRuntime;         // 0x14 render state while bound
};

/** One source vertex array: component type and data. */
struct swvVertexArray {
    int m_type;                     // 0x00 positions/normals: 4 = 16 bit, else 8 bit; texcoords: 5..8
    void* m_data;                   // 0x04 NULL when absent
};

/** Vertex arrays handed to the VBO populator (layout from swvPopulateVBO_CM_V3F_N3F_T23F_C4UB). */
struct swvVertexData {
    int m_vertexCount;              // 0x00
    swvVertexArray m_positions;     // 0x04
    swvVertexArray m_normals;       // 0x0c
    swvVertexArray m_colors;        // 0x14 type 2: RGB bytes, else RGBA bytes
    int m_alpha;                    // 0x1c 16.16 fixed point
    swvVertexArray m_texCoords[2];  // 0x20
};

/** Where one attribute lives in the interleaved VBO. */
struct swvVBOArrayFormat {
    int m_offset;                   // 0x00 -1 when absent
    int m_stride;                   // 0x04
    int m_type;                     // 0x08 GL component type
};

/** Layout of the interleaved VBO. */
struct swvVBOFormat {
    swvVBOArrayFormat m_positions;      // 0x00
    swvVBOArrayFormat m_normals;        // 0x0c
    swvVBOArrayFormat m_colors;         // 0x18
    swvVBOArrayFormat m_texCoords[2];   // 0x24
};

/** VBO populator handed to the engine (swvGetGLPopulateVBO); returns the byte size of the VBO. */
typedef int (*swvPopulateVBOFunction)(const swvVertexData* data, void* buffer, swvVBOFormat* format);

/** State of one zlib decompression (0x40c bytes). */
struct swvDecompressor {
    unsigned int m_format;              // 0x00 'ZLIB'
    void* m_userData;                   // 0x04 first argument of decompress_AcceptBytes
    struct z_stream_s* m_stream;        // 0x08
    unsigned char m_block[0x400];       // 0x0c inflate output
};

struct swvImage2D;

/** Function table of an engine Image2D (slots used by the PNG loader). */
struct swvImage2DFunctions {
    void* m_slots00[0x4c / 4];
    /** Set the pixels; returns 0 on success. */
    int (*SetImage)(swvImage2D* self, unsigned int format, unsigned int width, unsigned int height,
                    unsigned int size, const unsigned char* pixels);                          // 0x4c
    /** Set palette indices and the palette; returns 0 on success. */
    int (*SetPalettedImage)(swvImage2D* self, unsigned int format, unsigned int width, unsigned int height,
                            unsigned int size, const unsigned char* pixels, unsigned int paletteSize,
                            const unsigned char* palette);                                   // 0x50
};

/** An engine Image2D. */
struct swvImage2D {
    const swvImage2DFunctions* m_functions;
};

/** Function table of the engine object handed to the loaders. */
struct swvEngineFunctions {
    void* m_slots00[0x10 / 4];
    /** Create an object of `type` (10: Image2D); returns 0 on success. */
    int (*CreateObject)(int type, void* object);                                              // 0x10 (no self)
};

/** The engine object handed to the loaders. */
struct swvEngine {
    const swvEngineFunctions* m_functions;
};

extern "C" {

// ---- ssstdlib.cpp ----
ICFileMgr* swvFileManagerGetInstance();
void swvFileManagerRelease(ICFileMgr* fileManager);
int swvFileManagerOpenFileForReading(ICFileMgr* fileManager, const char* name, ICFile** file);
void swvFileRelease(ICFile* file);
unsigned int swvFileSize(ICFile* file);
int swvFileSeek(ICFile* file, int origin, int offset);
unsigned int swvFileReadBytes(ICFile* file, unsigned char* buffer, unsigned int size);
size_t swvStrLen(const char* text);
char* swvStrDup(const char* text);
char* swvStrLwr(char* text);
int swvStrNCmp(const char* left, const char* right, size_t count);
char* swvStrChr(const char* text, int character);
void* swvSystemAllocate(size_t size);
void swvSystemFree(void* pointer);
void swvMemSet(void* destination, int value, size_t size);
void swvMemCpy(void* destination, const void* source, size_t size);
void swvExit();
void swvDbgPrintf(const char* format, ...);

// ---- swvdecompressorapi.cpp ----
int swvDecompressOnDataStart(unsigned int format, void* userData, swvDecompressor** handle);
int swvDecompressOnData(const unsigned char* data, unsigned int size, swvDecompressor** handle);
int swvDecompressOnDataEnd(swvDecompressor** handle);
void swvDecompressError(swvDecompressor** handle);
unsigned long swvAdler32(unsigned long adler, const unsigned char* data, unsigned int size);
/** Engine core: receives one inflated block; returns 0 to stop decompressing. */
int decompress_AcceptBytes(void* userData, int size, const unsigned char* data);

// ---- swvglruntime.cpp ----
void swvGLRuntimePrepare();
void swvGLRuntimeCleanup();
void* swvGetGLStretchBlit();
int swvGLBindImage(swvImage* image);
int swvGLRebindImage(swvImage* image);
int swvGLReleaseImage(swvImage* image);
int swvGLShareContext();
int swvGLUnpalettizeImage2D();
int swvGLCanDeleteImage2DData();
void swvGetGLMaxViewportDims(int* dimensions);
int swvGetGLMaxTextureSize();
int swvGetGLSubmeshPacking();
int swvGLSupportVBO();
swvPopulateVBOFunction swvGetGLPopulateVBO();
void glGenBuffersSUPERSCAPE(int count, unsigned int* buffers);
void glDeleteBuffersSUPERSCAPE(int count, const unsigned int* buffers);
void glBindBufferSUPERSCAPE(unsigned int target, unsigned int buffer);
void glBufferDataSUPERSCAPE(unsigned int target, int size, const void* data, unsigned int usage);
void glBufferSubDataSUPERSCAPE(unsigned int target, int offset, int size, const void* data);

// ---- swvpngload.cpp ----
int swvPNGLoadBuffer(swvEngine* engine, unsigned int size, const unsigned char* data, swvImage2D** image);

}

#endif
