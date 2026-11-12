// CBH_GPSScan.h - the SpriteGlu presenter owned by the GPS map.
#ifndef CBH_GPSSCAN_H
#define CBH_GPSSCAN_H

class SG_Instance;
class SG_Presenter;

class CBH_GPSScan {
public:
    CBH_GPSScan();
    void CreateScanAnimation();
    void Paint(int x, int y);
    ~CBH_GPSScan();
    void DeleteScanAnimation();

    SG_Presenter* m_presenter;   // 0x00
    unsigned char m_reserved04[0x18];
    int m_x;                    // 0x1c
    int m_y;                    // 0x20
    int m_field24;
    int m_field28;
    int m_field2c;
    int m_field30;
    int m_field34;
    bool m_visible;              // 0x38
    SG_Instance* m_instance;    // 0x3c
};

typedef char GPSScanSizeCheck[sizeof(CBH_GPSScan) == 0x40 ? 1 : -1];

#endif
