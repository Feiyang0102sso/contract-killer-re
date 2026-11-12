// CAdManager_AdMarvel_Cocoa.cpp - lifetime and render hook of the AdMarvel adapter.
#include "CAdManager_AdMarvel_Cocoa.h"

CAdManager_AdMarvel_Cocoa::CAdManager_AdMarvel_Cocoa()
    : CSingleton(ClassId), m_delegate(NULL)
{
}

CAdManager_AdMarvel_Cocoa::~CAdManager_AdMarvel_Cocoa()
{
    Destroy();
}

void CAdManager_AdMarvel_Cocoa::HandleRender()
{
    // The iOS adapter does not draw through the game renderer.
}
