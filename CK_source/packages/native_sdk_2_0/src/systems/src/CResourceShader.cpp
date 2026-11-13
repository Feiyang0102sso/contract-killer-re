// CResourceShader.cpp - resource wrapping a ICShader.
#include "CResourceShader.h"
#include "ICShader.h"

                                                                        

CResourceShader::CResourceShader()
{
    m_classId = ClassId;
    m_shader = NULL;
    CResource::ResetMembers();
}

CResourceShader::~CResourceShader()
{
    CResourceShader::Destroy();
}

void CResourceShader::Destroy()
{
    if (m_shader != NULL) {
        delete m_shader;
        m_shader = NULL;
    }
}

void CResourceShader::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceShader::GetSize() const
{
    if (m_shader == NULL) {
        return 0;
    }
    return 8;
}

void* CResourceShader::GetData() const
{
    return m_shader;
}

unsigned int CResourceShader::CreateInternal(unsigned int handle, const char* name, CInputStream* stream,
                                          unsigned int mimeType, CIdToObjectRouter* router,
                                          unsigned char copyName)
{
    m_handle = handle;
    ICGraphics::Abstraction abstraction;
    unsigned char result;
    if (mimeType == 0x2c435e5c || mimeType == 0x2c435e5a) {
        abstraction = ICGraphics::Abstraction_Hardware;
    } else if (mimeType == 0xd65e722e) {
        abstraction = ICGraphics::Abstraction_Software;
    } else {
        goto failed;
    }

    m_shader = ICShader::CreateInstance(abstraction);
    result = CreateResult_Failed;
    if (m_shader != NULL) {
        result = CreateResult_Created;
        if (stream != NULL) {
            // ?: kept on purpose: the original selects the result instead of branching
            result = m_shader->Load(*stream, mimeType) ? CreateResult_Loaded : CreateResult_Failed;
            if (result == CreateResult_Failed) {
                goto failed;
            }
        }
    }
    SetName(name, copyName);
    m_isCreated = 1;
    return result;

failed:
    // Unknown shader type or load failure.
    Destroy();
    return CreateResult_Failed;
}

       
