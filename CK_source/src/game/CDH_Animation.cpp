// CDH_Animation.cpp - find Swerve tracks and drive their controllers.
#include "CDH_Animation.h"

#include "DGHelper.h"
#include "SwerveHelper.h"
#include "WindowApp.h"

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CDH_Animation_ARMv7_Size[(sizeof(CDH_Animation) == 0x34) ? 1 : -1];
#endif

namespace {
#ifdef CK_PC
    // PORT: the slots hold ARM code of the emulated engine (CKMethod of pc_port's tws.h).
    template <class Function>
    ck::arm::Function<std::remove_pointer_t<Function>> Method(IQueryInterface* object, unsigned int slot)
    {
        return CKMethod<Function>(object, slot);
    }
#else
    template <class Function>
    Function Method(IQueryInterface* object, unsigned int slot)
    {
        return reinterpret_cast<Function>((*reinterpret_cast<void***>(object))[slot / 4]);
    }
#endif

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
    int count;
    Method<ReadInt>(object.m_interface, 0x14)(object.m_interface, &count);
    while (count > 0) {
        int index = count - 1;
        IQueryInterface* owner = object.m_interface;
        IQueryInterface* handle;
        Method<ReadIndexedObject>(owner, 0x18)(owner, index, &handle);
        CDH_Animation::Track track(handle);
        bool matches;
        {
            IQueryInterface* control;
            Method<ReadObject>(handle, 0x50)(handle, &control);
            const CDH_Animation::Object3D attached(control);
            matches = attached == controller;
        }
        if (matches) {
            return track;
        }
        count = index;
    }

    const Group group(object);
    if (group != NULL) {
        IQueryInterface* owner = group.m_interface;
        Method<ReadInt>(owner, 0xb8)(owner, &count);
        for (int index = count - 1; index >= 0; --index) {
            IQueryInterface* childInterface;
            Method<ReadIndexedObject>(owner, 0xbc)(owner, index, &childInterface);
            CDH_Animation::Object3D child(childInterface);
            CDH_Animation::Track track = getFirstControlledTrack(child, controller);
            child.Clear();
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
    Object3D found = object.find(DGHelper::getSwerveID(name));
    IQueryInterface* controller = NULL;
    if (found.m_interface == NULL) {
        return false;
    }
    found.m_interface->QueryInterface(Controller::InterfaceId, reinterpret_cast<void**>(&controller));
    found.Clear();
    bool available = controller != NULL;
    if (available) {
        controller->Release();
    }
    return available;
}

int CDH_Animation::addController(const XString& name)
{
    IQueryInterface* controllerInterface = NULL;
    {
        XString::AnsiString asciiName(name.c_str());
        Object3D found = m_object.find(DGHelper::getSwerveID(asciiName));
        if (found.m_interface != NULL) {
            found.m_interface->QueryInterface(Controller::InterfaceId, reinterpret_cast<void**>(&controllerInterface));
        }
    }
    Controller controller(controllerInterface);
    Method<SetFloat>(controllerInterface, 0x50)(controllerInterface, 0.0f);
    int duration;
    {
        Track track = getFirstControlledTrack(m_object, controller);
        IQueryInterface* sequence;
        Method<ReadObject>(track.m_interface, 0x58)(track.m_interface, &sequence);
        Method<ReadInt>(sequence, 0x50)(sequence, &duration);
        if (sequence != NULL) {
            sequence->Release();
        }
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
        Controller handle = m_controllers[index];
        IQueryInterface* controller = handle.m_interface;
        Method<SetFloat>(controller, 0x50)(controller, 1.0f);
        float reportedSpeed;
        Method<ReadFloat>(controller, 0x5c)(controller, &reportedSpeed);
        float speed = reportedSpeed;
        if (speed < 0.0f) {
            speed = -speed;
        }
        if (reverse == true) {
            float position = static_cast<float>(m_durations[index]);
            Method<SetTimedFloat>(controller, 0x6c)(controller, position, worldTime);
            Method<SetTimedFloat>(controller, 0x70)(controller, -speed, worldTime);
        } else {
            Method<SetTimedFloat>(controller, 0x6c)(controller, 0.0f, worldTime);
            Method<SetTimedFloat>(controller, 0x70)(controller, speed, worldTime);
        }
    }
    if (index <= -1) {
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
