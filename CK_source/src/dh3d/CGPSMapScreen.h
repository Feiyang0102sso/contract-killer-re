#ifndef CGPSMAPSCREEN_H
#define CGPSMAPSCREEN_H

#include "PointerEventController.h"
#include "Serializable.h"
#include "Vector_ss.h"
#include "WindowBase.h"

struct Point2D {
    int x;
    int y;
};

class CBH_GPSMap;

class CGPSMapScreen : public Window {
public:
    static const unsigned int CDH_GPS_SCREEN_HUNT_DAY_DURATION_MS;

    /** Map state stored in the original hunt-day save format. */
    class CachedData : public Serializable {
    public:
        CachedData();
        virtual ~CachedData();
        virtual void Serialize(DataOutputStream& stream);
        virtual void DeSerialize(JDataInputStream& stream);

        Vector<Point2D> m_points;        // 0x04
        unsigned int m_huntDayElapsed;   // 0x14, milliseconds
        unsigned char m_field18;
        unsigned char m_field19;
        signed char m_field1a;
        bool m_field1b;
        bool m_field1c;
        int m_field20;
        int m_field24;
    };

    explicit CGPSMapScreen(CachedData* data);
    virtual ~CGPSMapScreen();
    void onReceiveTapjoyPoints(int points);
    virtual void Init();
    virtual void OnPointerEvent(Event& event);
    virtual void Update();
    virtual void OnCreateState(Window** window, int state);
    virtual void OnStateInterrupt(int state, Window* window);
    virtual void OnCommand(Event& event);
    virtual void CreateResources();
    virtual void ReleaseResources();
    virtual void Build();
    int GetState();

private:
    int m_actualGpsTime;                // 0xa4
    int m_reservedA8;
    int m_reservedAc;
    float m_offerPollSeconds;           // 0xb0
    XString m_fieldb4;
    unsigned char m_reservedB8[0x20];
    PointerEventController m_pointer;  // 0xd8
    int m_reserved100;
    int m_field104;
    int m_field108;
    bool m_field10c;
    unsigned char m_reserved10d[7];
    int m_mapState;                     // 0x114
    CBH_GPSMap* m_map;                  // 0x118
    unsigned char m_reserved11c[8];
    int m_field124;
    int m_reserved128;
    Window* m_auxiliaryWindow;         // 0x12c
    bool m_reserved130;
    bool m_resourcesCreated;           // 0x131
};

#endif
