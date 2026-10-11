#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <random>
#include <vector>

namespace {
    constexpr int WINDOW_PERCENT    = 30; // % de chances qu'un segment de mur ait une fenêtre
    constexpr int WEAK_PERCENT      = 4;  // % de chances qu'un segment soit un mur fragile
    constexpr int CORNER_RESERVE    = 2;  // pas de fenêtre à moins de N tuiles d'un coin (place pour les portes)
    constexpr int WINDOW_SPACING    = 2;  // nombre minimal de murs pleins entre deux fenêtres

    // Rotations des murs droits selon le côté du bâtiment
    constexpr int ROT_TOP    = 0;
    constexpr int ROT_RIGHT  = 90;
    constexpr int ROT_BOTTOM = 180;
    constexpr int ROT_LEFT   = 270;

    // Rotations de WALL_CORNER_OUTER (sens horaire, comme les courbes des chemins)
    constexpr int ROT_CORNER_TOP_LEFT     = 0;
    constexpr int ROT_CORNER_TOP_RIGHT    = 90;
    constexpr int ROT_CORNER_BOTTOM_RIGHT = 180;
    constexpr int ROT_CORNER_BOTTOM_LEFT  = 270;

    // Textures de sol qui comptent comme "intérieur de bâtiment"
    constexpr Map::Texture FLOOR_TEXTURES[] = {
        Tiles::FLOOR_STONE_PLAIN.texture,
        Tiles::FLOOR_STONE_TILES.texture,
        Tiles::FLOOR_STONE_CROSS.texture,
        Tiles::FLOOR_STONE_PATTERN.texture,
        Tiles::FLOOR_STONE_CRACKED.texture,
        Tiles::FLOOR_WOOD_PLANKS.texture,
    };
}

void MapGenerator::buildHouseWalls(TileMap& map, std::mt19937& rng, const Rect& playZone) {
    // ---------- 2. Détection des sols ----------
    const int W = playZone.w;
    const int H = playZone.h;
    auto idx = [&](const int x, const int y) { return (y - playZone.y) * W + (x - playZone.x); };

    auto isFloor = [&](const int x, const int y) {
        if (x < playZone.x || y < playZone.y || x >= playZone.x + W || y >= playZone.y + H) return false;
        for (const Map::Tile& t : map.cells[y][x].layers) {
            if (t.zIndex != Map::ZIndex::Layer1) continue;
            for (const Map::Texture& f : FLOOR_TEXTURES) {
                if (t.texture.col == f.col && t.texture.row == f.row) return true;
            }
        }
        return false;
    };

    auto putWall = [&](const int x, const int y, Map::Tile tile, const int rotation) {
        tile.modifier.rotation = rotation;
        addTile(map, x, y, tile, Map::ZIndex::Layer2);
    };

    std::uniform_int_distribution<int> percent(0, 99);

    // Choisit le type d'un mur droit (plein, fenêtre, fragile).
    // index : position le long du côté (0 = coin, len - 1 = coin), lastWindow : dernière fenêtre posée.
    auto pickStraight = [&](const int index, const int len, int& lastWindow) {
        const bool windowAllowed =
            index >= CORNER_RESERVE && index <= len - 1 - CORNER_RESERVE &&
            index - lastWindow > WINDOW_SPACING;

        if (windowAllowed && percent(rng) < WINDOW_PERCENT) {
            lastWindow = index;
            return Tiles::WALL_WINDOWS;
        }
        if (percent(rng) < WEAK_PERCENT) return Tiles::WALL_WEAK;
        return Tiles::WALL_STRAIGHT;
    };

    // ---------- 3. Un bâtiment = un rectangle de sol ----------
    std::vector<char> visited(W * H, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (visited[idx(x, y)] || !isFloor(x, y)) continue;

            // Coin haut-gauche trouvé : on mesure le rectangle
            int w = 0;
            while (isFloor(x + w, y)) ++w;
            int h = 0;
            while (isFloor(x, y + h)) ++h;

            for (int yy = y; yy < y + h; ++yy) {
                for (int xx = x; xx < x + w; ++xx) {
                    visited[idx(xx, yy)] = 1;
                }
            }

            if (w < 3 || h < 3) continue; // trop petit pour avoir un intérieur

            // Coins
            putWall(x,             y,         Tiles::WALL_CORNER_OUTER, ROT_CORNER_TOP_LEFT);
            putWall(x + w - 1,   y,         Tiles::WALL_CORNER_OUTER, ROT_CORNER_TOP_RIGHT);
            putWall(x + w - 1, y + h - 1, Tiles::WALL_CORNER_OUTER, ROT_CORNER_BOTTOM_RIGHT);
            putWall(x,           y + h - 1, Tiles::WALL_CORNER_OUTER, ROT_CORNER_BOTTOM_LEFT);

            // Haut et bas (longueur w)
            int lastTop = -100, lastBottom = -100;
            for (int i = 1; i < w - 1; ++i) {
                putWall(x + i, y,         pickStraight(i, w, lastTop),    ROT_TOP);
                putWall(x + i, y + h - 1, pickStraight(i, w, lastBottom), ROT_BOTTOM);
            }

            // Gauche et droite (longueur h)
            int lastLeft = -100, lastRight = -100;
            for (int j = 1; j < h - 1; ++j) {
                putWall(x,         y + j, pickStraight(j, h, lastLeft),  ROT_LEFT);
                putWall(x + w - 1, y + j, pickStraight(j, h, lastRight), ROT_RIGHT);
            }
        }
    }
}