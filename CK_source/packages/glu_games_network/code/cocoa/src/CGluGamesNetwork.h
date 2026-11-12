// CGluGamesNetwork.h - C++ entry point of the Glu Games Network (gWorld) overlay.
// The game creates one instance, Show()s it, and gets its exit callback once the player leaves.
#ifndef CGLUGAMESNETWORK_H
#define CGLUGAMESNETWORK_H

class CGluGamesNetwork
{
public:
    // gameId: Glu game code; onExit(exitContext) runs when the overlay is left
    CGluGamesNetwork(const char* gameId, void (*onExit)(void*), void* exitContext, unsigned int gServeId, bool standaloneComponent);

    void Show();
    void Hide();

    // Forwards a changed gServe update count to the update callback
    void ReportGServeUpdates(int count);

    void    (*m_onExit)(void*);                 // +0x00
    void*   m_exitContext;                      // +0x04
    void    (*m_onGServeUpdates)(void*, int);   // +0x08 (never set in CK)
    void*   m_gServeUpdatesContext;             // +0x0c
    int     m_lastGServeUpdates;                // +0x10
};

#endif // CGLUGAMESNETWORK_H
