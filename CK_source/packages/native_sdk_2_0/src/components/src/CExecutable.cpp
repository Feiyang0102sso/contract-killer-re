// CExecutable.cpp - timed / counted execution.
#include "CExecutable.h"
#include "CApplet.h"
#include "CApp.h"
#include "CAppExecutor.h"

                                                                           

const int CExecutable::InfinitelyRunning = -1;
const int CExecutable::OneShot = 1;

int CExecutable::Run()
{
    int remaining = m_runCount;
    if (remaining != 0) {
        CAppExecutor* executor = CApplet::m_pApp->GetExecutor();
        m_isExecuting = 1;
        if (executor->GetMode() == CAppExecutor::Mode_Running) {
            // Timed execution uses a wrapping 32-bit 16.16 frame counter.
            m_elapsed = static_cast<int>(static_cast<unsigned int>(m_elapsed) + executor->GetFrameTime());
            if (m_elapsed >= m_interval) {
                OnExecute();
                remaining = m_runCount;
                if (static_cast<unsigned int>(remaining) + 1 > 1) {
                    remaining = static_cast<int>(static_cast<unsigned int>(remaining) - 1);
                    m_runCount = remaining;
                }
                m_elapsed = 0;
            }
        } else {
            OnExecute();
            remaining = m_runCount;
            if (static_cast<unsigned int>(remaining) + 1 > 1) {
                remaining = static_cast<int>(static_cast<unsigned int>(remaining) - 1);
                m_runCount = remaining;
            }
        }
        m_isExecuting = 0;
    }
    if (m_deleteWhenDone && remaining == 0) {
        delete this;
        return RunResult_Deleted;
    }
    return !m_deleteWhenDone && remaining == 0;
}

       
