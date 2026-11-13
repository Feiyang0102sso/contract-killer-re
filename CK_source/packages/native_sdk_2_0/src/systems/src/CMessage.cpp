// CMessage.cpp - events carrying typed arguments.
#include "CMessage.h"

#include <stdarg.h>

typedef char MessageSizeCheck[sizeof(CMessage) == 0xc0 ? 1 : -1];
                                                                        

CMessage::CMessage(CClass* sender, unsigned int messageId, unsigned int param, int argumentCount, ...)
{
    m_classId = ClassId;
    m_argumentCount = argumentCount;
    m_eventId = messageId;
    m_sender = sender;
    m_param = param;

    va_list arguments;
    va_start(arguments, argumentCount);
    for (int index = 0; index < argumentCount; index++) {
        m_arguments[index].m_type = va_arg(arguments, unsigned int);
        m_arguments[index].m_count = va_arg(arguments, unsigned int);
        m_arguments[index].m_data = va_arg(arguments, void*);
    }
    va_end(arguments);
}

       
