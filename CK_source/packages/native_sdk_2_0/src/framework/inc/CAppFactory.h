// Shared declaration of the game-specific resource manager factory (CGameApp.cpp).
#ifndef CAPPFACTORY_H
#define CAPPFACTORY_H
                                                    
                    class ICResourceManager;  
                     
/** Game-specific factories (implemented by the game, src/CGameApp.cpp). */
class CAppFactory {
public:
    static ICResourceManager* CreateResourceManager();
};
 
     
#endif
