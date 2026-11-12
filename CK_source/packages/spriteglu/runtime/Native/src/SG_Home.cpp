#include "SG_Home.h"

SG_Home::SG_Home() : CSingleton(SingletonId)
{
}

SG_Home::~SG_Home()
{
    m_instance.Destroy();
}

SG_Instance* SG_Home::GetDefaultSGInstance()
{
    return &m_instance;
}
