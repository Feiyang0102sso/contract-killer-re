#include "CAuxRenderText.h"

CAuxRenderText::CAuxRenderText() : CSingleton(SingletonId)
{
    m_textCount = 0;
}

// The retail ARMv7 build leaves the debug text call in place.
void CAuxRenderText::AddTextInfo(const wchar_t* format, ...)
{
}
