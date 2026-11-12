// CHttpDataChunk.cpp - chunk of a chunked HTTP response.
#include "CHttpDataChunk.h"

                                                                        

CHttpDataChunk::CHttpDataChunk()
{
    m_data = NULL;
    m_size = 0;
    m_received = 0;
}

CHttpDataChunk::~CHttpDataChunk()
{
    if (m_data != NULL) {
        free(m_data);
        m_data = NULL;
    }
}

       
