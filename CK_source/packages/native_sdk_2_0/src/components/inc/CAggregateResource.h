#ifndef CAGGREGATERESOURCE_H
#define CAGGREGATERESOURCE_H

#include "CArrayInputStream.h"
#include "CClass.h"

/** Table of contents and payload for a Glu aggregate resource. */
class CAggregateResource : public CClass {
public:
    enum { ClassId = 0x521b2402 };

    CAggregateResource();
    virtual ~CAggregateResource();
    void Destroy();
    unsigned char Load(CInputStream& stream, unsigned int mimeType);
    unsigned char LoadTOC(CInputStream& stream, unsigned int mimeType);

private:
    unsigned int m_entryCount;        // 0x08
    unsigned int* m_entryTypes;       // 0x0c optional
    unsigned int* m_entryIds;         // 0x10
    unsigned int* m_entryOffsets;     // 0x14 includes the end offset
    unsigned char* m_data;            // 0x18 owned payload
    unsigned int m_dataOffset;        // 0x1c bytes consumed by the table
    unsigned int m_dataSize;          // 0x20
    CArrayInputStream m_stream;       // 0x24
};

#endif
