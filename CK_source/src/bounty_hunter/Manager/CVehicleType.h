// CVehicleType.h - one vehicle model of vehicles.xml (shared model, copies counted).
#ifndef CVEHICLETYPE_H
#define CVEHICLETYPE_H

#include "SwerveHelper.h"
#include "xString.h"

class TiXmlNode;

class CVehicleType {
public:
    explicit CVehicleType(TiXmlNode* node);
    ~CVehicleType();

    void Parse(TiXmlNode* node);
    SwerveHelper::Group GetModelCopy() const;
    void LoadModel() const;
    void ReleaseVehicleModelCopy() const;
    void ParseInfo(TiXmlNode* node);
    void ParseModel(TiXmlNode* node);

    mutable SwerveHelper::Object3D m_model;  // 0x00
    XString m_dummy;                          // 0x04 root node of the model
    XString m_modelFile;                      // 0x08 m3g resource
    mutable int m_copyCount;                  // 0x0c
};

#endif
