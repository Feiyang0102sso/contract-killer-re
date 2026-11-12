// CVehicleManager.h - vehicle models of res/common/configs/vehicles.xml.
#ifndef CVEHICLEMANAGER_H
#define CVEHICLEMANAGER_H

#include "CVehicleType.h"
#include "Vector_ss.h"

class CVehicleManager {
public:
    CVehicleManager();
    virtual ~CVehicleManager();

    CVehicleType* GetRandomVehicle() const;

    Vector<CVehicleType*> m_vehicles;      // 0x04
};

#endif
