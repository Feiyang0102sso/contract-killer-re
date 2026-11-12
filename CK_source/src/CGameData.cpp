// CGameData.cpp - the game-data singleton.
#include "CGameData.h"

CGameData::CGameData() : CSingleton(SingletonId)
{
    // CDH_RootNode assigns m_resBank after construction.
    m_field0c = false;
    m_field0d = false;
    m_field10 = 0;
}

CGameData::~CGameData()
{
    // CSingleton unregisters this object.
}
