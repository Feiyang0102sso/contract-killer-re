// CResourceFactory.cpp - mime type -> CResource subclass.
#include "CResourceFactory.h"


                                                                        

unsigned char CResourceFactory::CreateResource(unsigned int mimeType, CResource** resource)
{
    *resource = CreateResource(mimeType);
    return *resource != NULL;
}
