#ifndef CONFIG_H
#define CONFIG_H

constexpr auto WINDOW_TITLE = "Ognenny Ostrov";

constexpr int TILE_SIZE = 128;
constexpr int TILE_STEP = TILE_SIZE -8;
constexpr int MASK_SIZE    = 6;                     // une case = 6 x 6 sous-cases (collisions)
constexpr int SUBCELL_SIZE = TILE_STEP / MASK_SIZE; // 120 / 6 = 20 px


#endif // CONFIG_H
