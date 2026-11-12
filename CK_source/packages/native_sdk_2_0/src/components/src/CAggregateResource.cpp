// CAggregateResource.cpp - decode the aggregate resource table and its payload.
#include "CAggregateResource.h"
#include "CApp.h"

enum {
    AggregateSequentialIds = 0x8000,
    AggregateWideOffsets = 0x4000,
    AggregateHasTypes = 0x2000
};

CAggregateResource::CAggregateResource()
    : CClass(CApp::ClassId)
{
    m_classId = ClassId;
    m_entryCount = 0;
    m_entryTypes = NULL;
    m_entryIds = NULL;
    m_entryOffsets = NULL;
    m_data = NULL;
    m_dataOffset = 0;
    m_dataSize = 0;
}

CAggregateResource::~CAggregateResource()
{
    Destroy();
}

void CAggregateResource::Destroy()
{
    m_entryCount = 0;
    if (m_entryIds != NULL) {
        delete[] m_entryIds;
        m_entryIds = NULL;
    }
    if (m_entryOffsets != NULL) {
        delete[] m_entryOffsets;
        m_entryOffsets = NULL;
    }
    if (m_entryTypes != NULL) {
        delete[] m_entryTypes;
        m_entryTypes = NULL;
    }
    if (m_data != NULL) {
        delete[] m_data;
    }
    m_stream.Close();
    m_data = NULL;
    m_dataOffset = 0;
    m_dataSize = 0;
}

unsigned char CAggregateResource::Load(CInputStream& stream, unsigned int mimeType)
{
    if (!LoadTOC(stream, mimeType) || m_dataSize == 0) {
        return 0;
    }
    m_data = new unsigned char[m_dataSize];
    if (m_data == NULL) {
        return 0;
    }
    stream.Read(m_data, m_dataSize);
    return stream.m_error == 0;
}

unsigned char CAggregateResource::LoadTOC(CInputStream& stream, unsigned int mimeType)
{
    unsigned int originalSize = stream.Available();
    if (originalSize == 0) {
        return 0;
    }

    unsigned int flags = stream.ReadUInt16();
    unsigned int entryCount = stream.ReadUInt16();
    if (entryCount != 0) {
        m_entryIds = new unsigned int[entryCount];
        m_entryOffsets = new unsigned int[entryCount + 1];
        if (m_entryIds == NULL || m_entryOffsets == NULL) {
            return 0;
        }

        unsigned int firstId = 0;
        if (flags & AggregateSequentialIds) {
            firstId = stream.ReadUInt16();
        }
        for (unsigned int index = 0; index < entryCount; index++) {
            if (flags & AggregateSequentialIds) {
                m_entryIds[index] = firstId + index;
            } else {
                m_entryIds[index] = stream.ReadUInt16();
            }
            unsigned int offset;
            if (flags & AggregateWideOffsets) {
                offset = stream.ReadUInt32();
            } else {
                offset = stream.ReadUInt16();
            }
            m_entryOffsets[index] = offset;
        }
        m_entryOffsets[entryCount] = stream.ReadUInt32();
        if (flags & AggregateHasTypes) {
            m_entryTypes = new unsigned int[entryCount];
            if (m_entryTypes == NULL) {
                return 0;
            }
            for (unsigned int index = 0; index < entryCount; index++) {
                m_entryTypes[index] = stream.ReadUInt32();
            }
        }
        m_entryCount = entryCount;
    }
    m_dataOffset = originalSize - stream.Available();
    m_dataSize = stream.Available();
    return stream.m_error == 0;
}
