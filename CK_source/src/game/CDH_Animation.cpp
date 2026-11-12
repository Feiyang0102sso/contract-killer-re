// CDH_Animation.cpp - find Swerve tracks and drive their controllers.
#include "CDH_Animation.h"

#include "SwerveHelper.h"
#include "WindowApp.h"

typedef char CDH_Animation_ARMv7_Size[(sizeof(CDH_Animation) == 0x34) ? 1 : -1];

namespace {
    template <class Function>
    Function Method(IQueryInterface* object, unsigned int slot)
    {
        return reinterpret_cast<Function>((*reinterpret_cast<void***>(object))[slot / 4]);
    }

    typedef int (*ReadInt)(IQueryInterface*, int*);
    typedef int (*ReadObject)(IQueryInterface*, IQueryInterface**);
    typedef int (*ReadIndexedObject)(IQueryInterface*, int, IQueryInterface**);
    typedef int (*SetFloat)(IQueryInterface*, float);
    typedef int (*ReadFloat)(IQueryInterface*, float*);
    typedef int (*SetTimedFloat)(IQueryInterface*, float, int);
    typedef int (*AnimateObject)(IQueryInterface*, int, int*);
}

CDH_Animation::Track getFirstControlledTrack(CDH_Animation::Object3D object, CDH_Animation::Controller controller)
{
    if (object == NULL) {
        return CDH_Animation::Track();
    }
    IQueryInterface* owner = object.m_interface;
    int count = 0;
    Method<ReadInt>(owner, 0x14)(owner, &count);
    for (int index = count - 1; index >= 0; --index) {
        IQueryInterface* handle = NULL;
        Method<ReadIndexedObject>(owner, 0x18)(owner, index, &handle);
        CDH_Animation::Track track(handle);
        if (track != NULL) {
            IQueryInterface* control = NULL;
            Method<ReadObject>(handle, 0x50)(handle, &control);
            const CDH_Animation::Object3D attached(control);
            if (attached == controller) {
                return track;
            }
        }
    }

    const Group group(object);
    if (group != NULL) {
        owner = group.m_interface;
        Method<ReadInt>(owner, 0xb8)(owner, &count);
        for (int index = count - 1; index >= 0; --index) {
            IQueryInterface* handle = NULL;
            Method<ReadIndexedObject>(owner, 0xbc)(owner, index, &handle);
            CDH_Animation::Track track = getFirstControlledTrack(CDH_Animation::Object3D(handle), controller);
            if (track != NULL) {
                return track;
            }
        }
    }
    return CDH_Animation::Track();
}

CDH_Animation::CDH_Animation(Object3D object)
    : m_object(object)
    , m_currentController(-1)
    , m_startTime(-1)
    , m_speed(1.0f)
{
}

CDH_Animation::~CDH_Animation()
{
}

bool CDH_Animation::HaveController(Object3D object, const char* name)
{
    return Controller(SwerveHelper::Find(object, name)) != NULL;
}

int CDH_Animation::addController(const XString& name)
{
    Controller controller(SwerveHelper::Find(m_object, XString::AnsiString(name.c_str())));
    if (controller == NULL) {
        return -1;
    }
    IQueryInterface* handle = controller.m_interface;
    Method<SetFloat>(handle, 0x50)(handle, 0.0f);
    Track track = getFirstControlledTrack(m_object, controller);
    if (track == NULL) {
        return -1;
    }
    IQueryInterface* sequence = NULL;
    Method<ReadObject>(track.m_interface, 0x58)(track.m_interface, &sequence);
    int duration = 0;
    if (sequence != NULL) {
        Method<ReadInt>(sequence, 0x50)(sequence, &duration);
        sequence->Release();
    }
    const int index = m_controllers.size();
    m_controllers.addElement(controller);
    m_durations.addElement(duration);
    return index;
}

int CDH_Animation::getControllerBaseDuration(int index)
{
    return m_durations[index];
}

void CDH_Animation::setControllerDuration(int index, int duration)
{
    // the original trusts the caller: no range check
    m_speed = static_cast<float>(m_durations[index]) / static_cast<float>(duration);
    IQueryInterface* controller = m_controllers[index].m_interface;
    Method<SetTimedFloat>(controller, 0x70)(controller, m_speed, WindowApp::m_instance->m_updateTime);
}

void CDH_Animation::setCurrentController(int index, bool reverse)
{
    setCurrentController(WindowApp::m_instance->m_updateTime, index, reverse);
}

void CDH_Animation::setCurrentController(int worldTime, int index, bool reverse)
{
    stopAllAnimation();
    if (index >= 0 && index < m_controllers.size()) {
        IQueryInterface* controller = m_controllers[index].m_interface;
        Method<SetFloat>(controller, 0x50)(controller, 1.0f);
        float speed = 1.0f;
        Method<ReadFloat>(controller, 0x5c)(controller, &speed);
        if (speed < 0.0f) {
            speed = -speed;
        }
        float position = 0.0f;
        if (reverse) {
            position = static_cast<float>(m_durations[index]);
            speed = -speed;
        }
        Method<SetTimedFloat>(controller, 0x6c)(controller, position, worldTime);
        Method<SetTimedFloat>(controller, 0x70)(controller, speed, worldTime);
    } else {
        worldTime = -1;
    }
    m_currentController = index;
    m_startTime = worldTime;
}

void CDH_Animation::stopAllAnimation()
{
    for (int index = 0; index < m_controllers.size(); ++index) {
        IQueryInterface* controller = m_controllers[index].m_interface;
        Method<SetFloat>(controller, 0x50)(controller, 0.0f);
    }
}

void CDH_Animation::animate()
{
    animate(WindowApp::m_instance->m_updateTime);
}

void CDH_Animation::animate(int worldTime)
{
    int validity;
    Method<AnimateObject>(m_object.m_interface, 0x24)(m_object.m_interface, worldTime, &validity);
}

bool CDH_Animation::isAnimationComplete() const
{
    if (m_currentController < 0) {
        return true;
    }
    int duration = static_cast<int>(static_cast<float>(m_durations[m_currentController]) / m_speed);
    return WindowApp::m_instance->m_updateTime - m_startTime >= duration;
}

bool CDH_Animation::isAnimationComplete(int worldTime) const
{
    if (m_currentController < 0) {
        return true;
    }
    int duration = static_cast<int>(static_cast<float>(m_durations[m_currentController]) / m_speed);
    return worldTime - m_startTime >= duration;
}
