#ifndef CAUXRENDERTEXT_H
#define CAUXRENDERTEXT_H

#include "CSingleton.h"

class CAuxRenderText : public CSingleton {
public:
    enum { SingletonId = 0x88510fb7 };

    static CAuxRenderText* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        CAuxRenderText* instance = static_cast<CAuxRenderText*>(found);
        if (instance == NULL) {
            instance = new CAuxRenderText();
        }
        return instance;
    }

    CAuxRenderText();
    virtual ~CAuxRenderText() {}
    void AddTextInfo(const wchar_t* format, ...);

private:
    int m_textCount;
};

#endif
