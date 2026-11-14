// Test tools of the test program, see TestTools.h.
#include "TestTools.h"
#include "ArmProfiler.h"

namespace ck::test {

void UpdatePickTest()
{
}

void UpdateAutoPlay()
{
}

bool HandleTestKey(CApplet&, SDL_Keycode key, bool down)
{
    // F8 starts and stops the ARM profiler (it only exists with CK_ARM_PROFILE and Engine=swerve).
    if (key == SDLK_F8) {
        if (down) {
            ToggleArmProfile();
        }
        return true;
    }
    return false;
}

} // namespace ck::test
