// CInputMapping.cpp - base of the input mappings.
#include "CInputMapping.h"

                                                                        

CInputMapping::CInputMapping() : CExecutable(InfinitelyRunning)
{
    m_classId = ClassId;
    m_targetHandle = 0;
}

CInputMapping::~CInputMapping()
{
}

       
