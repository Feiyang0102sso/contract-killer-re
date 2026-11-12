// SG_Defines.cpp - spriteGlu data model: resource stream, tints, sprites, layers,
// animations, characters and archetypes (see SG_Defines.h). (written by b042)
#include "SG_Defines.h"
#include "CApplet.h"
#include "CApp.h"
#include "CBinary.h"
#include "CColor.h"
#include "CResource.h"
#include "ICGraphics2d.h"
#include "ICRenderSurface.h"
#include "ICResourceManager.h"
#include "CDrawUtil.h"
#include "WindowApp.h"









// ---- constants ----
static const int FIXED_ONE = 0x10000;                       // 1.0 in 16.16 fixed point
static const float FIXED_TO_FLOAT = 1.0f / 65536.0f;        // 16.16 fixed point -> float
enum { DEFAULT_SCALE_PERCENT = 100 };                       // first scale of every image

// ---------------------------------------------------------------------------------------------
// BitVector

BitVector::BitVector(unsigned int bitCount)
    : m_byteCount((int)(bitCount + 7) / 8), m_bits(NULL)
{
    m_bits = (unsigned char*)malloc(m_byteCount);
    memset(m_bits, 0, m_byteCount);
    m_bitCount = bitCount;
    m_onCount = 0;
}

BitVector::~BitVector()
{
    if (m_bits != NULL) {
        free(m_bits);
        m_bits = NULL;
    }
}

void BitVector::Init(int bitCount, DataInputStream* stream)
{
    m_byteCount = (bitCount + 7) / 8;
    m_bits = (unsigned char*)malloc(m_byteCount);
    memset(m_bits, 0, m_byteCount);
    m_bitCount = bitCount;
    stream->Read(m_bits, m_byteCount);

    // count the members that were read
    for (int bit = 0; bit < bitCount; bit++) {
        if (IsMember(bit)) {
            m_onCount++;
        }
    }
}

unsigned char BitVector::IsMember(unsigned int bit)
{
    return (m_bits[bit >> 3] & (1 << (bit & 7))) != 0;
}

void BitVector::SetMember(unsigned int bit, unsigned char member)
{
    unsigned int byteIndex = bit >> 3;
    int mask = 1 << (bit & 7);
    int wasMember = m_bits[byteIndex] & mask;
    if (member) {
        if (!wasMember) {
            m_onCount++;
            m_bits[byteIndex] |= mask;
        }
    } else if (wasMember) {
        m_onCount--;
        m_bits[byteIndex] &= ~mask;
    }
}

unsigned int BitVector::GetOnCount()
{
    return m_onCount;
}

// ---------------------------------------------------------------------------------------------
// DataInputStream

DataInputStream::DataInputStream(const char* resourceName)
{
    m_resourceName = resourceName;
    CResource* resource = NULL;
    ((ICResourceManager*)CApplet::m_pApp->m_resourceManager)->CreateResource(resourceName, &resource);
    if (resource == NULL) {
        m_error = 1;
    } else {
        CBinary* binary = (CBinary*)resource->GetData();
        Open(binary->m_data, binary->m_size);
    }
}

DataInputStream::~DataInputStream()
{
    ((ICResourceManager*)CApplet::m_pApp->m_resourceManager)->ReleaseResource(m_resourceName);
}

// ---------------------------------------------------------------------------------------------
// Graphics

void Graphics::SetOffset(short x, short y)
{
    m_offsetX = x;
    m_offsetY = y;
}

void Graphics::SetClip(const CRectangle& clip)
{
    m_clip.m_x = clip.m_x;
    m_clip.m_y = clip.m_y;
    m_clip.m_width = clip.m_width;
    m_clip.m_height = clip.m_height;
}

void Graphics::DrawRegion(SG_SrcImage* image, const RenderInfo& info, int x, int y, int scale,
                          CRectangle* region, int width, int height, int rotation, int scaleX,
                          int scaleY)
{
    int offsetX = m_offsetX;
    int offsetY = m_offsetY;
    ICGraphics2d* graphics2d = ICGraphics2d::GetInstance();
    graphics2d->PushTransform();

    bool unscaled = scaleX == FIXED_ONE && scaleY == FIXED_ONE;
    bool scaled = !unscaled;
    if (rotation != 0 || scaled) {
        // rotate and scale around the center of the region (16.16 fixed point)
        int centerX = x + offsetX + (width >> 1);
        int centerY = y + offsetY + (height >> 1);
        graphics2d->Translate(centerX << 16, centerY << 16);
        if (rotation != 0) {
            graphics2d->Rotate(rotation);
        }
        if (!unscaled) {
            graphics2d->Scale(scaleX, scaleY);
        }
        graphics2d->Translate(-centerX << 16, -centerY << 16);
    }
    graphics2d->Translate((x + offsetX) << 16, (y + offsetY) << 16);
    if (scale != FIXED_ONE) {
        graphics2d->Scale(scale, scale);
    }
    graphics2d->Draw(*image->m_surface, (ICGraphics2d::Flip)info.m_flip, region);
    graphics2d->PopTransform();
}

void Graphics::FillRect(int x, int y, unsigned int width, unsigned int height, int color, int rotation,
                        int scaleX, int scaleY)
{
    ICGraphics2d* graphics2d = ICGraphics2d::GetInstance();
    graphics2d->PushTransform();

    bool unscaled = scaleX == FIXED_ONE && scaleY == FIXED_ONE;
    bool scaled = !unscaled;
    if (rotation != 0 || scaled) {
        // rotate and scale around the center of the rectangle (16.16 fixed point)
        int centerX = x + width / 2 + m_offsetX;
        int centerY = y + height / 2 + m_offsetY;
        graphics2d->Translate(centerX << 16, centerY << 16);
        if (rotation != 0) {
            graphics2d->Rotate(rotation);
        }
        if (!unscaled) {
            graphics2d->Scale(scaleX, scaleY);
        }
        graphics2d->Translate(-centerX << 16, -centerY << 16);
    }
    int left = (short)(m_offsetX + x);
    int top = (short)(m_offsetY + y);
    CDrawUtil::Fill(left, top, (short)width + left, (short)height + top, color | 0xff000000,
                    ICGraphics2d::Primitive_Rectangles);
    graphics2d->PopTransform();
}

// ---------------------------------------------------------------------------------------------
// SG_Imagesprite / SG_Rectsprite / SG_TextureMap

SG_Imagesprite::SG_Imagesprite()
    : m_tint(0), m_tag(0), m_transform(0)
{
}

SG_Imagesprite::~SG_Imagesprite()
{
}

unsigned char SG_Imagesprite::Load(DataInputStream& stream)
{
    m_tint = stream.ReadUInt16();
    m_transform = stream.ReadUInt8();
    m_tag = stream.ReadUInt8();
    return stream.m_error == 0;
}

SG_Rectsprite::SG_Rectsprite()
{
    m_color = -1;
    m_tag = 0;
    m_height = 0;
    m_width = 0;
}

SG_Rectsprite::~SG_Rectsprite()
{
}

unsigned char SG_Rectsprite::Load(DataInputStream& stream)
{
    m_color = stream.ReadInt32();
    m_width = stream.ReadUInt16();
    m_height = stream.ReadUInt16();
    m_tag = stream.ReadInt8();
    return stream.m_error == 0;
}

unsigned char SG_Image::LoadTransform(unsigned char transform, SG_SrcImage* image)
{
    // the first surface of a group is shared by all its transforms
    if (SquareTransform_IsTransposed(transform)) {
        if (m_transposedImages.Size() == 0) {
            m_transposedImages.Add(image);
        }
    } else {
        if (m_images.Size() == 0) {
            m_images.Add(image);
        }
    }
    unsigned char loaded = m_loadedTransforms;
    m_loadedTransforms = loaded | (1 << transform);
    return (loaded & (1 << transform)) == 0;
}

unsigned char SG_Image::DumpTransform(unsigned char transform)
{
    m_loadedTransforms &= ~(1 << transform);
    if (IsTransformGroupLoaded(transform)) {
        return 1;
    }
    if (SquareTransform_IsTransposed(transform)) {
        ClearImageTransposedVectors();
    } else {
        ClearImageVectors();
    }
    return 0;
}

void SG_Image::GetSrcImageAndRenderInfo(unsigned char transform, SG_SrcImage*& image, Graphics::RenderInfo& info,
                                        unsigned short scale, unsigned char& isScaled)
{
    isScaled = GetScaleIndex(scale) != 0;
    int mirrorVertical = transform & SquareTransform_MirrorVertical;
    if (SquareTransform_IsTransposed(transform)) {
        // transposed surface: the mirror bits swap axes
        if (mirrorVertical && (transform & SquareTransform_MirrorHorizontal)) {
            info.m_flip = ICGraphics2d::Flip_Both;
        } else if (transform & SquareTransform_MirrorHorizontal) {
            info.m_flip = ICGraphics2d::Flip_Vertical;
        } else if (mirrorVertical) {
            info.m_flip = ICGraphics2d::Flip_Horizontal;
        } else {
            info.m_flip = ICGraphics2d::Flip_None;
        }
        image = m_transposedImages[GetScaleIndex(scale)];
    } else {
        if (mirrorVertical) {
            if (transform & SquareTransform_MirrorHorizontal) {
                info.m_flip = ICGraphics2d::Flip_Both;
            } else {
                info.m_flip = ICGraphics2d::Flip_Vertical;
            }
        } else {
            if (transform & SquareTransform_MirrorHorizontal) {
                info.m_flip = ICGraphics2d::Flip_Horizontal;
            } else {
                info.m_flip = ICGraphics2d::Flip_None;
            }
        }
        image = m_images[GetScaleIndex(scale)];
    }
}

SG_Tint::SG_Tint()
{
    m_paletteIndex = -1;
}

SG_Tint::~SG_Tint()
{
}

unsigned char SG_Tint::Load(DataInputStream& stream)
{
    m_surfaceIndex = stream.ReadUInt16();
    m_paletteIndex = stream.ReadInt8();
    return stream.m_error == 0;
}

SG_SrcImage::SG_SrcImage() : m_surface(NULL), m_resourceName(NULL)
{
}

void SG_SrcImage::SetSurface(ICRenderSurface* surface, const char* name)
{
    m_surface = surface;
    m_resourceName = name;
}

SG_SrcImage::~SG_SrcImage()
{
    if (m_surface != NULL) {
        ((ICResourceManager*)CApplet::m_pApp->m_resourceManager)->ReleaseResource(m_resourceName);
    }
    m_resourceName = NULL;
}

SG_Image::SG_Image()
{
    m_loadedTransforms = 0;
    m_scales.Add(DEFAULT_SCALE_PERCENT);
}

SG_Image::~SG_Image()
{
}

void SG_Image::GetSize(unsigned char transform, int& width, int& height)
{
    TCVector<SG_SrcImage*>* images = &m_images;
    if (transform >= 4) {
        images = &m_transposedImages;
    }
    unsigned int surfaceWidth = 0;
    unsigned int surfaceHeight = 0;
    if (images->Size() > 0 && (*images)[0] != NULL) {
        (*images)[0]->m_surface->GetWidthAndHeight(surfaceWidth, surfaceHeight);
    }
    width = surfaceWidth;
    height = surfaceHeight;
}

unsigned char SG_Image::LoadTransformRequiresRawImage(unsigned char transform)
{
    return !IsTransformGroupLoaded(transform);
}

unsigned char SG_Image::IsTransformLoaded(unsigned char transform)
{
    return (m_loadedTransforms & (1 << transform)) != 0;
}

unsigned char SG_Image::DumpAllTransforms()
{
    m_loadedTransforms = 0;
    unsigned char groups = (m_images.Size() > 0) + (m_transposedImages.Size() > 0);
    ClearImageVectors();
    ClearImageTransposedVectors();
    return groups;
}

void SG_Image::ClearImageTransposedVectors()
{
    for (int index = 0; index < m_transposedImages.Size(); index++) {
        if (m_transposedImages[index] != NULL) {
            delete m_transposedImages[index];
            m_transposedImages[index] = NULL;
        }
    }
    m_transposedImages.Clear();
}

void SG_Image::ClearImageVectors()
{
    for (int index = 0; index < m_images.Size(); index++) {
        if (m_images[index] != NULL) {
            delete m_images[index];
            m_images[index] = NULL;
        }
    }
    m_images.Clear();
}

// ---------------------------------------------------------------------------------------------
// SG_Spritemap / SG_Layer / SG_Animation

SG_Spritemap::SG_Spritemap()
{
    m_count = 0;
    m_entries = NULL;
}

SG_Spritemap::~SG_Spritemap()
{
    if (m_entries != NULL) {
        delete[] m_entries;
        m_entries = NULL;
    }
}

unsigned char SG_Spritemap::Load(DataInputStream& stream)
{
    m_count = stream.ReadUInt16();
    if (m_count == 0) {
        return 1;
    }
    m_entries = new SG_SpritemapEntry[m_count];
    if (m_entries == NULL) {
        return 0;
    }
    for (int index = 0; index < m_count; index++) {
        m_entries[index].m_sprite = stream.ReadUInt16();
        m_entries[index].m_replacement = stream.ReadUInt16();
        m_entries[index].m_mode = stream.ReadUInt8();
        m_entries[index].m_x = stream.ReadInt16();
        m_entries[index].m_y = stream.ReadInt16();
    }
    return stream.m_error == 0;
}

unsigned char SG_Layer::Load(DataInputStream& stream)
{
    m_count = stream.ReadUInt8();
    if (m_count == 0) {
        return 1;
    }
    m_entries = new SG_LayerEntry[m_count];
    if (m_entries == NULL) {
        return 0;
    }
    for (int index = 0; index < m_count; index++) {
        m_entries[index].m_sprite = stream.ReadUInt16();
        m_entries[index].m_x = stream.ReadInt16();
        m_entries[index].m_y = stream.ReadInt16();
        m_entries[index].m_rotation = stream.ReadInt32();
        m_entries[index].m_scaleX = stream.ReadInt32();
        m_entries[index].m_scaleY = stream.ReadInt32();
    }
    return stream.m_error == 0;
}

SG_Animation::~SG_Animation()
{
    if (m_frameLayers != NULL) {
        delete[] m_frameLayers;
        m_frameLayers = NULL;
    }
    if (m_frameDurations != NULL) {
        delete[] m_frameDurations;
        m_frameDurations = NULL;
    }
}

unsigned char SG_Animation::Load(DataInputStream& stream)
{
    m_id = stream.ReadUInt8();
    m_frameCount = stream.ReadUInt8();
    if (m_frameCount == 0) {
        return 1;
    }
    m_frameLayers = new unsigned short[m_frameCount];
    m_frameDurations = new unsigned short[m_frameCount];
    if (m_frameLayers == NULL || m_frameDurations == NULL) {
        return 0;
    }
    for (unsigned int frame = 0; frame < m_frameCount; frame++) {
        m_frameLayers[frame] = stream.ReadUInt16();
        m_frameDurations[frame] = stream.ReadUInt16() * 10;     // stored in 1/100 s
    }
    return stream.m_error == 0;
}

// ---------------------------------------------------------------------------------------------
// SG_Character / SG_Archetype

SG_Character::~SG_Character()
{
}

unsigned short SG_Character::GetScaleFactorForCharacter(unsigned short scale)
{
    int count = m_scales.Size();
    if (count < 1) {
        return scale;
    }
    for (int index = 0; index < count; index++) {
        unsigned short smaller = m_scales[index];
        if (smaller < scale) {
            // between the previous (larger) scale and this one: take the closer
            unsigned short larger = DEFAULT_SCALE_PERCENT;
            if (index != 0) {
                larger = m_scales[index - 1];
            }
            unsigned short distanceDown = scale - smaller;
            unsigned short distanceUp = larger - scale;
            if (distanceUp > distanceDown) {
                return smaller;
            }
            return larger;
        }
    }
    return m_scales[count - 1];
}

SG_Archetype::SG_Archetype()
{
    m_layerCount = 0;
    m_layers = NULL;
    m_animationCount = 0;
    m_animations = NULL;
    m_characterCount = 0;
    m_characters = NULL;
}

SG_Archetype::~SG_Archetype()
{
    if (m_layers != NULL) {
        delete[] m_layers;
        m_layers = NULL;
    }
    if (m_animations != NULL) {
        delete[] m_animations;
        m_animations = NULL;
    }
    if (m_characters != NULL) {
        delete[] m_characters;
        m_characters = NULL;
    }
}

unsigned char SG_Archetype::LoadLayers(DataInputStream& stream)
{
    m_layerCount = stream.ReadUInt16();
    if (m_layerCount == 0) {
        return 1;
    }
    int layer = 0;
    m_layers = new SG_Layer[m_layerCount];
    for (; layer < m_layerCount; layer++) {
        m_layers[layer].Load(stream);
    }
    return stream.m_error == 0;
}

unsigned char SG_Archetype::LoadAnimations(DataInputStream& stream)
{
    m_animationCount = stream.ReadUInt16();
    if (m_animationCount == 0) {
        return 1;
    }
    unsigned int animation = 0;
    m_animations = new SG_Animation[m_animationCount];
    for (; animation < m_animationCount; animation++) {
        m_animations[animation].Load(stream);
    }
    return stream.m_error == 0;
}

unsigned char SG_Archetype::LoadCharacters(DataInputStream& stream, unsigned short spriteCount)
{
    m_characterCount = stream.ReadUInt8();
    if (m_characterCount == 0) {
        return 1;
    }
    unsigned int index = 0;
    m_characters = new SG_Character[m_characterCount];
    for (; index < m_characterCount; index++) {
        SG_Character& character = m_characters[index];
        character.m_loaded = 0;
        character.m_sprites.Init(spriteCount, &stream);
        character.m_spritemap = stream.ReadInt8();
    }
    return stream.m_error == 0;
}
