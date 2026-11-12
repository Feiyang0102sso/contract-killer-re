// CVertexBuffer.cpp - vertex buffer in system memory.
#include "CVertexBuffer.h"
#include "NPMalloc.h"
#include "NPMem.h"

                                                                         

void CIndexVertexPool::ResizeInternal(int size)
{
    unsigned char* newData = (unsigned char*)np_malloc(size);
    int copySize = m_cursor - m_data;
    if (copySize >= size) {
        copySize = size;
    }
    if (copySize != 0) {
        np_memcpy(newData, m_data, copySize);
    }
    if (m_data != NULL) {
        np_free(m_data);
    }
    // the old data filled the whole new pool: keep the reserve free
    if (copySize == size) {
        copySize -= m_reserve;
    }
    m_data = newData;
    m_cursor = newData + copySize;
    m_capacity = size;
    m_resized = 1;
}
