// CVehicleManager.cpp - read the vehicle catalog and pick a random vehicle.
#include "CVehicleManager.h"
#include "App.h"
#include "CRandGen.h"
#include "ResMap.h"
#include "tinyxml.h"

CVehicleManager::CVehicleManager()
{
    char* config = static_cast<App*>(WindowApp::m_instance)->InitResMap()->loadText(XString(L"res/common/configs/vehicles.xml"));
    TiXmlDocument document;
    document.Parse(config);
    if (config != NULL) {
        delete[] config;
    }
    for (TiXmlNode* node = document.FirstChild("vehicle"); node != NULL; node = node->NextSibling("vehicle")) {
        CVehicleType* vehicle = new CVehicleType(node);
        m_vehicles.addElement(vehicle);
    }
}

CVehicleManager::~CVehicleManager()
{
    SAFE_DELETE_VECTOR(m_vehicles);
}

CVehicleType* CVehicleManager::GetRandomVehicle() const
{
    int count = m_vehicles.size();
    if (count < 1) {
        return NULL;
    }
    int last = count - 1;
    int index = CRandGen::GetInstance()->GetRandRange(0, last);
    // clamp the random index into the vector
    // ?: keeps the original select shape of the clamp
    if (index < 0 || index >= count) {
        index = index < 0 ? 0 : (index > last ? last : index);
    }
    return m_vehicles[index];
}
