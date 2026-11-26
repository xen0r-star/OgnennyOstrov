#ifndef OGNENNYOSTROV_WEAPONS_H
#define OGNENNYOSTROV_WEAPONS_H

namespace Weapons {
    enum Id {
        DAGGER, SHORT_SWORD, LONG_SWORD, SHORT_SPEAR, SPEAR, CROSSBOW, BOW, ARROW,
        SMALL_DOUBLE_AXE, DOUBLE_AXE, BATTLE_AXE, AXE, HAMMER, MACE, SHIELD, SHIELD_2,
        COUNT
    };

    struct Sprite {
        int col;
        int row;
        float gripY;
    };

    inline constexpr Sprite SPRITES[COUNT] = {
        {0, 0, 0.82f}, {1, 0, 0.81f}, {2, 0, 0.80f}, {3, 0, 0.64f},
        {4, 0, 0.64f}, {5, 0, 0.50f}, {6, 0, 0.50f}, {7, 0, 0.62f},
        {0, 1, 0.71f}, {1, 1, 0.71f}, {2, 1, 0.72f}, {3, 1, 0.72f},
        {4, 1, 0.71f}, {5, 1, 0.70f}, {6, 1, 0.50f}, {7, 1, 0.50f},
    };
}

#endif // OGNENNYOSTROV_WEAPONS_H