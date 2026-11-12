// CVehicleType.cpp - vehicle model configuration and its reference counted model.
#include "CVehicleType.h"
#include "App.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "tinyxml.h"

CVehicleType::CVehicleType(TiXmlNode* node)
    : m_copyCount(0)
{
    m_model.Clear();
    Parse(node);
}

void CVehicleType::Parse(TiXmlNode* node)
{
    ParseInfo(node->FirstChild("info"));
    ParseModel(node->FirstChild("model"));
}

CVehicleType::~CVehicleType()
{
    m_model.Clear();
}

SwerveHelper::Group CVehicleType::GetModelCopy() const
{
    if (m_model == NULL) {
        LoadModel();
    }
    m_copyCount++;
    return SwerveHelper::Group(m_model.duplicate());
}

void CVehicleType::LoadModel() const
{
    SwerveHelper::Object3D object = App::LoadObject3D(XString::AnsiString(m_modelFile.c_str()));
    if (object != NULL) {
        m_model = object.find(DGHelper::getSwerveID(XString::AnsiString(m_dummy.c_str())));
    }
}

void CVehicleType::ReleaseVehicleModelCopy() const
{
    m_copyCount--;
    if (m_copyCount == 0) {
        m_model.Clear();
    }
}

void CVehicleType::ParseInfo(TiXmlNode* node)
{
    if (node != NULL) {
        m_dummy = CXmlHelper::GetAttributeValue(node, "dummy");
    }
}

void CVehicleType::ParseModel(TiXmlNode* node)
{
    if (node != NULL) {
        m_modelFile = CXmlHelper::GetAttributeValue(node, "m3g");
    }
}
