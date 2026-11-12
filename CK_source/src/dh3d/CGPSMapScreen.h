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
    void SkipTutorialAutoStart();

private:
    int m_actualGpsTime;                // 0xa4
    CachedData* m_cachedData;           // used by the map reconstruction
    int m_reservedAc;
    float m_offerPollSeconds;           // 0xb0
    XString m_fieldb4;
    unsigned char m_reservedB8[0x20];
    PointerEventController m_pointer;  // 0xd8
    unsigned char m_reserved100[0x14];
    int m_mapState;                     // 0x114
    CBH_GPSMap* m_map;                  // 0x118
    bool m_mapAttached;
    bool m_skipTutorialAutoStart;
    unsigned char m_reserved11e[0x0e];
    Window* m_auxiliaryWindow;         // 0x12c
    bool m_reserved130;
    bool m_resourcesCreated;           // 0x131
};

#endif
