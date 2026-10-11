#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <iostream>
#include <random>
#include <vector>

namespace {
    constexpr int ROT_TOP    = 0;
    constexpr int ROT_RIGHT  = 90;
    constexpr int ROT_BOTTOM = 180;
    constexpr int ROT_LEFT   = 270;

    // Types de porte et leur poids de tirage (en %), dans le même ordre
    constexpr Map::Tile DOOR_TILES[] = {
        Tiles::WALL_DOORWAY_OPEN,
        Tiles::WALL_DOOR_FRAME,
        Tiles::WALL_DOORWAY_CLOSE,
        Tiles::WALL_BROKEN_DOOR,
    };
    std::discrete_distribution DOOR_PICK({ 10.0, 0.0, 80.0, 10.0 });

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

void MapGenerator::addHouseDoors(TileMap& map, std::mt19937& rng, const Rect& playZone) {
    // Bits combinables : dans quelles directions part une tuile de chemin
    enum Direction {
        UP = 1,
        RIGHT = 2,
        DOWN = 4,
        LEFT = 8
    };

    // ---------- 2. Lecture de la map ----------
    const int W = playZone.w;
    const int H = playZone.h;
    auto idx = [&](const int x, const int y) { return (y - playZone.y) * W + (x - playZone.x); };
    auto inZone = [&](const int x, const int y) {
        return x >= playZone.x && y >= playZone.y && x < playZone.x + W && y < playZone.y + H;
    };

    auto isFloor = [&](const int x, const int y) {
        if (!inZone(x, y)) return false;
        for (const Map::Tile& t : map.cells[y][x].layers) {
            if (t.zIndex != Map::ZIndex::Layer1) continue;
            for (const Map::Texture& f : FLOOR_TEXTURES) {
                if (t.texture.col == f.col && t.texture.row == f.row) return true;
            }
        }
        return false;
    };

    // Directions dans lesquelles une tuile de chemin est ouverte (même conventions que addHousePaths)
    auto connections = [&](const Map::Tile& t) -> int {
        const int rot = ((t.modifier.rotation % 360) + 360) % 360;
        auto is = [&](const Map::Tile& ref) {
            return t.texture.col == ref.texture.col && t.texture.row == ref.texture.row;
        };

        if (is(Tiles::PATH_STRAIGHT_1) || is(Tiles::PATH_STRAIGHT_2)) {
            return (rot % 180 == 0) ? (UP | DOWN) : (LEFT | RIGHT);
        }
        if (is(Tiles::PATH_CURVE_1) || is(Tiles::PATH_CURVE_2)) {
            switch (rot) {
                case 0:   return RIGHT | DOWN;
                case 90:  return LEFT | DOWN;
                case 180: return LEFT | UP;
                default:  return RIGHT | UP;
            }
        }
        if (is(Tiles::PATH_T_JUNCTION)) {
            // La rotation désigne la branche manquante : 0 = bas, 90 = gauche, 180 = haut, 270 = droite
            int missing = RIGHT;
            if (rot == 0)        missing = DOWN;
            else if (rot == 90)  missing = LEFT;
            else if (rot == 180) missing = UP;
            return (UP | RIGHT | DOWN | LEFT) & ~missing;
        }
        if (is(Tiles::PATH_CROSSROADS)) return UP | RIGHT | DOWN | LEFT;
        if (is(Tiles::PATH_END)) {
            switch (rot) {
                case 0:   return UP;
                case 90:  return RIGHT;
                case 180: return DOWN;
                default:  return LEFT;
            }
        }
        return 0;
    };

    // Direction dans laquelle la case (x, y) est ouverte, d'après sa tuile de chemin sur Layer1
    auto pathOpenings = [&](const int x, const int y) {
        if (!inZone(x, y)) return 0;
        int mask = 0;
        for (const Map::Tile& t : map.cells[y][x].layers) {
            if (t.zIndex == Map::ZIndex::Layer1) mask |= connections(t);
        }
        return mask;
    };

    // ---------- 3. Un bâtiment = un rectangle de sol ----------
    struct Candidate { int wx, wy, rotation; bool connected; bool outsideFree; };

    std::vector<char> visited(W * H, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (visited[idx(x, y)] || !isFloor(x, y)) continue;

            int w = 0;
            while (isFloor(x + w, y)) ++w;
            int h = 0;
            while (isFloor(x, y + h)) ++h;

            for (int yy = y; yy < y + h; ++yy) {
                for (int xx = x; xx < x + w; ++xx) {
                    visited[idx(xx, yy)] = 1;
                }
            }
            if (w < 3 || h < 3) continue;

            // ---------- 4. Murs droits candidats ----------
            std::vector<Candidate> candidates;
            // (wx, wy) : case de mur ; (ox, oy) : case devant ; toward : direction de (ox, oy) vers le mur
            auto addCandidate = [&](const int wx, const int wy, const int ox, const int oy,
                                    const int toward, const int rotation) {
                const bool connected  = (pathOpenings(ox, oy) & toward) != 0;
                const bool outsideFree = inZone(ox, oy) && !isFloor(ox, oy);
                candidates.push_back({ wx, wy, rotation, connected, outsideFree });
            };
            for (int i = 1; i < w - 1; ++i) {
                addCandidate(x + i, y,         x + i, y - 1, DOWN, ROT_TOP);
                addCandidate(x + i, y + h - 1, x + i, y + h, UP,   ROT_BOTTOM);
            }
            for (int j = 1; j < h - 1; ++j) {
                addCandidate(x,         y + j, x - 1, y + j, RIGHT, ROT_LEFT);
                addCandidate(x + w - 1, y + j, x + w, y + j, LEFT,  ROT_RIGHT);
            }

            // ---------- 5. Poser les portes ----------
            auto placeDoor = [&](const Candidate& c) {
                Map::Tile tile = DOOR_TILES[DOOR_PICK(rng)];
                tile.modifier.rotation = c.rotation;
                addTile(map, c.wx, c.wy, tile, Map::ZIndex::Layer2);
            };

            int placed = 0;
            for (const Candidate& c : candidates) {
                if (!c.connected) continue;
                placeDoor(c);
                ++placed;
            }

            // Aucun chemin n'arrive ici : une porte au hasard sur un mur dégagé
            if (placed == 0) {
                std::vector<int> free;
                for (int k = 0; k < static_cast<int>(candidates.size()); ++k) {
                    if (candidates[k].outsideFree) free.push_back(k);
                }
                if (!free.empty()) {
                    std::uniform_int_distribution<int> pick(0, static_cast<int>(free.size()) - 1);
                    placeDoor(candidates[free[pick(rng)]]);
                }
            }
        }
    }
}