// CDH_Cheats.h - debug switches of the 3D game (data-only unit src/game/CDH_Cheats.cpp).
// Created by weapon: both flags are initialized to true in the original __DATA,__data (0x32eb70).
#ifndef CDH_CHEATS_H
#define CDH_CHEATS_H

class CDH_Cheats {
public:
    static bool CHEAT_SHOW_WEAPON;  // read by CDH_Weapon::paintWeaponModel
    static bool CHEAT_SHOW_TREES;
};

#endif
