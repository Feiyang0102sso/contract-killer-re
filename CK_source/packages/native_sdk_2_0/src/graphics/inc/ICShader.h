// ICShader.h - shader interface.
// (first drafted by b011x as PROVISIONAL; slots completed by b037x from the vtables of CShader /
// CShader_OGLES. CK's Cocoa factory is in cocoa/src/CShader_Cocoa.cpp.)
#ifndef ICSHADER_H
#define ICSHADER_H

#include "ICGraphicsResource.h"
#include "ICGraphics.h"

                                                    

                      
class CInputStream;
class COutputStream;
class CStrWChar;
 

                    

class ICShader : public ICGraphicsResource {
public:
    /** Resource type key of shaders. */
    enum { ResourceType = 0xd65e722e };

    /** Pipeline stage of a shader. */
    enum PipelineType {
        PipelineType_Undefined = 0
    };

    /** Platform factory. */
    static ICShader* CreateInstance(ICGraphics::Abstraction abstraction);

    ICShader() : ICGraphicsResource(ResourceType) {}

    virtual ~ICShader() {}

    /** Load the shader source / binary of mime type `type` from `stream`. */
    virtual unsigned char Load(CInputStream& stream, unsigned int type) = 0;
    virtual unsigned char Save(COutputStream& stream, unsigned int type) = 0;
    virtual void Destroy() = 0;
    virtual void SetPipelineType(PipelineType type) = 0;
    virtual PipelineType GetPipelineType() const = 0;
    virtual unsigned char SetSource(CInputStream& stream, unsigned int type) = 0;
    virtual unsigned char Compile() = 0;
    virtual unsigned char Compile(CStrWChar& log) = 0;
    virtual void DiscardSource() = 0;
    virtual unsigned char SaveStateForFutureRecreation() = 0;
    virtual unsigned char Recreate() = 0;
};

       

#endif
