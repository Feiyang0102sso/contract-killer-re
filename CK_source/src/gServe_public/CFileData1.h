// CFileData1.h - the CK XML save envelope carried as a profile content file.
#ifndef CFILEDATA1_H
#define CFILEDATA1_H

#include "CSaveRestoreInterface.h"

class CFileData1 : public CSaveRestoreInterface {
public:
    CFileData1();
    ~CFileData1();

    void setData(const unsigned char* data, int size);
    virtual void resetData();
    virtual unsigned char compareData(const CSaveRestoreInterface* other);
    virtual unsigned char copyData(const CSaveRestoreInterface* other);
    virtual void getData(TCVector<CNGSAttribute*>& attributes);
    virtual void setData(const CHash* attributes);
    virtual CStrWChar asString();
    virtual void debugPrint();

    unsigned char* m_data;  // +0x20
    int m_size;             // +0x24
};

#endif
