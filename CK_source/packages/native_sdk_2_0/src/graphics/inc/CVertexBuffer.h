// CVertexBuffer.h - vertex buffer in system memory.
// (first drafted by b002 as PROVISIONAL; completed by b040x, the owner of CVertexBuffer.cpp)
// gfx 2026-11-12: replaced the CKZ port (ICVertexBuffer + CBufferSystemMemory, decl list at 0x18)
// with the CK 1.5.2 class; CK's CVertexBuffer.cpp only defines CIndexVertexPool::ResizeInternal.
#ifndef CVERTEXBUFFER_H
#define CVERTEXBUFFER_H

#include "ICVertexBuffer.h"
#include "CBufferSystemMemory.h"
#include "CVertex.h"
#include "NPNew.h"

                                                                         

/** Growable byte pool for index/vertex data (CK 1.5.2; layout from ResizeInternal). */
class CIndexVertexPool {
public:
    CIndexVertexPool() : m_data(NULL), m_cursor(NULL), m_capacity(0), m_reserve(0), m_resized(0) {}

    /** Reallocate the pool to `size` bytes, keeping the used bytes that fit. */
    void ResizeInternal(int size);

    /** Free the bytes when the pool owns them. */
    inline void Destroy()
    {
        if (m_resized) {
            delete[] m_data;
        }
        m_resized = 0;
        m_data = NULL;
        m_cursor = NULL;
        m_capacity = 0;
        m_reserve = 0;
    }

    unsigned char* m_data;      // 0x00
    unsigned char* m_cursor;    // 0x04 end of the used bytes
    int m_capacity;             // 0x08 bytes allocated
    int m_reserve;              // 0x0c bytes kept free at the end when the old data fills the pool
    unsigned char m_resized;    // 0x10
};
// gfx 2026-11-12: CK 1.5.2 CVertexBuffer (everything inline: emitted weak where used, e.g.
// CGraphics2d_Lite_Cocoa.o). Layout from Initialize / Destroy.
class CVertexBuffer {
public:
    /** One vertex attribute (16 bytes; content not recovered beyond the terminator at +8). */
    struct Attribute {
        int m_field0;       // 0x00
        int m_field4;       // 0x04
        int m_field8;       // 0x08 0 in the terminating entry
        int m_fieldC;       // 0x0c
    };

    /** Resource types (m_field4). */
    enum {
        Type_Vertex = 0x2cc50270,
        Type_IndexVertex = 0xae7e140f
    };

    explicit CVertexBuffer(unsigned int type) : m_field4(type), m_attributes(NULL), m_attributeCount(0) {}

    inline virtual ~CVertexBuffer()
    {
        Destroy();
    }

    /** Pool of `size` bytes and room for `attributeCount` attributes plus a terminator. */
    inline virtual void Initialize(int size, int attributeCount)
    {
        m_pool.Destroy();
        m_pool.ResizeInternal(size);
        delete[] m_attributes;
        m_attributes = new Attribute[attributeCount + 1];
        m_attributes[attributeCount].m_field8 = 0;
        m_attributeCount = attributeCount;
    }

    inline virtual void Destroy()
    {
        delete[] m_attributes;
        m_attributes = NULL;
        m_attributeCount = 0;
        m_pool.Destroy();
    }

    unsigned int m_field4;          // 0x04 resource type (Type_*)
    CIndexVertexPool m_pool;        // 0x08
    Attribute* m_attributes;        // 0x1c
    int m_attributeCount;           // 0x20
};

/** Vertex buffer with an index pool (inline as CVertexBuffer). */
class CIndexVertexBuffer : public CVertexBuffer {
public:
    CIndexVertexBuffer() : CVertexBuffer(Type_IndexVertex) {}

    inline virtual ~CIndexVertexBuffer()
    {
        Destroy();
    }

    using CVertexBuffer::Initialize;

    inline virtual void Destroy()
    {
        CVertexBuffer::Destroy();
        m_indices.Destroy();
    }

    /** Index pool of `indexSize` bytes plus the vertex data. */
    inline virtual void Initialize(int indexSize, int size, int attributeCount)
    {
        m_indices.Destroy();
        m_indices.ResizeInternal(indexSize);
        CVertexBuffer::Initialize(size, attributeCount);
    }

    CIndexVertexPool m_indices;     // 0x24 m_reserve holds the index size in bytes
    unsigned int m_indexType;       // 0x38
};



       

#endif
