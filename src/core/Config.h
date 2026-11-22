#ifndef CONFIG_H
#define CONFIG_H

constexpr auto WINDOW_TITLE = "Ognenny Ostrov";

constexpr int TILE_SIZE = 128;                 // Px
constexpr int TILE_STEP = TILE_SIZE - 8;

constexpr float CAMERA_VISIBLE_HEIGHT = 900.f; // Camera zoom (Px)
constexpr float CAMERA_SMOOTHNESS = 8.f;       // Camera smoothness (higher = smoother)

constexpr float PLAYER_DRAW_SIZE = 84.f;       // Px

constexpr unsigned short SERVER_PORT = 54000;
constexpr int TICK_RATE = 30;
constexpr float CLIENT_TIMEOUT_SECONDS = 10.f;
constexpr int MAX_PLAYERS_PER_MATCH = 4;
constexpr int MATCH_CODE_LENGTH = 5;

#endif // CONFIG_H
