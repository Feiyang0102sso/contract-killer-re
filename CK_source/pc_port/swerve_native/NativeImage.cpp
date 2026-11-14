// Native Swerve Image2D. Images inside M3G files are made by m3gcore's loader; the game itself makes
// one kind: CreateObject(Image2D) followed by image2d_initialize with RGBA pixels (SwerveHelper's
// sprites). image2d_initializePalettized is what swvPNGLoadBuffer calls; only the emulated engine
// calls that loader, but the slot costs nothing.
#include "NativeRuntime.h"
#include <stdio.h>

namespace ck::native {
namespace {

// Image formats (Swerve and M3G share the values 0x60-0x64).
constexpr unsigned int FormatAlpha = 0x60;
constexpr unsigned int FormatLuminance = 0x61;
constexpr unsigned int FormatLuminanceAlpha = 0x62;
constexpr unsigned int FormatRGB = 0x63;
constexpr unsigned int FormatRGBA = 0x64;
constexpr int MaxPaletteEntries = 256;

int BytesPerPixel(unsigned int format)
{
    switch (format) {
    case FormatAlpha:
    case FormatLuminance:
        return 1;
    case FormatLuminanceAlpha:
        return 2;
    case FormatRGB:
        return 3;
    case FormatRGBA:
        return 4;
    default:
        return 0;
    }
}

/** Make the m3gcore image immutable and attach it to `self`. */
int FinishImage(Object* self, M3GImage created)
{
    if (created == nullptr) {
        const int error = TakeError();
        if (error == ErrorNone) {
            return ErrorNoMemory;
        }
        return error;
    }
    m3gCommitImage(created);
    AttachHandle(self, reinterpret_cast<M3GObject>(created));
    return TakeError();
}

/** image2d_initialize(format, width, height, size, pixels): an immutable image (JSR-184 Image2D(format, w, h, image)). */
int Initialize(Object* self, unsigned int format, int width, int height, unsigned int size, const unsigned char* pixels)
{
    if (self->handle != nullptr) {
        return ErrorInvalidState;
    }
    const int bytes = BytesPerPixel(format);
    if (bytes == 0 || width <= 0 || height <= 0) {
        fprintf(stderr, "Swerve native: Image2D format 0x%x %dx%d is not supported\n", format, width, height);
        return ErrorInvalidParameter;
    }
    if (size < static_cast<unsigned int>(width * height * bytes) || pixels == nullptr) {
        return ErrorInvalidParameter;
    }
    TakeError();
    M3GImage created = m3gCreateImage(Interface(), static_cast<M3GImageFormat>(format), width, height, 0);
    if (created != nullptr) {
        m3gSetImage(created, pixels);
    }
    return FinishImage(self, created);
}

/**
 * image2d_initializePalettized(format, width, height, indexSize, indices, paletteSize, palette):
 * one index byte per pixel and a palette of `format` entries (paletteSize bytes).
 */
int InitializePalettized(Object* self, unsigned int format, int width, int height, unsigned int indexSize,
                         const unsigned char* indices, unsigned int paletteSize, const unsigned char* palette)
{
    if (self->handle != nullptr) {
        return ErrorInvalidState;
    }
    const int entryBytes = BytesPerPixel(format);
    if (entryBytes == 0 || width <= 0 || height <= 0 || indexSize < static_cast<unsigned int>(width * height)) {
        return ErrorInvalidParameter;
    }
    int entries = static_cast<int>(paletteSize) / entryBytes;
    if (entries > MaxPaletteEntries) {
        entries = MaxPaletteEntries;
    }
    TakeError();
    M3GImage created = m3gCreateImage(Interface(), static_cast<M3GImageFormat>(format), width, height, M3G_PALETTED);
    if (created != nullptr) {
        m3gSetImage(created, indices);
        m3gSetImagePalette(created, entries, palette);
    }
    return FinishImage(self, created);
}

} // namespace

Object* CreateImage2D()
{
    Object* image = new Object();
    InitObject(image, ClassImage2D);
    return image;
}

void FillImage2DSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, slot::image2d::initialize, &Initialize);
    SetSlot(objectClass, slot::image2d::initializePalettized, &InitializePalettized);
}

} // namespace ck::native
