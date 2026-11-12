// PROVISIONAL (created by gfx): CVertexAttribute.h - vertex attribute declarations (CK 1.5.2).
// Only what PriorityStacks::AquireVtxBuffer reads.
#ifndef CVERTEXATTRIBUTE_H
#define CVERTEXATTRIBUTE_H

class CVertexAttribute {
public:
    /** One attribute (16 bytes); a list ends with an entry whose m_type is 0. */
    struct Decl {
        int m_field0;           // 0x00
        int m_field4;           // 0x04
        int m_type;             // 0x08 0 ends the list
        unsigned char m_fieldC; // 0x0c
        signed char m_stride;   // 0x0d bytes per vertex (first entry)
        unsigned char m_fieldE; // 0x0e
        unsigned char m_fieldF; // 0x0f
    };
};

#endif
