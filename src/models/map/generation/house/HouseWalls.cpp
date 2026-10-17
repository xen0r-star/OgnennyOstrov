#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <random>
#include <vector>

namespace {
    constexpr int WINDOW_PERCENT = 30; // Percentage chance for a wall segment to contain a window
    constexpr int WEAK_PERCENT   = 4;  // Percentage chance for a segment to be a weak/destructible wall
    constexpr int CORNER_RESERVE = 2;  // Reserve N tiles from corners without windows (leaves space for doors)
    constexpr int WINDOW_SPACING = 2;  // Min number of solid walls between two windows

    // Rotations for straight wall tiles according to building side
    constexpr int ROT_TOP    = 0;
    constexpr int ROT_RIGHT  = 90;
    constexpr int ROT_BOTTOM = 180;
    constexpr int ROT_LEFT   = 270;

    // Rotations for WALL_CORNER_OUTER tiles (clockwise order, matching path curves)
    constexpr int ROT_CORNER_TOP_LEFT     = 0;
    constexpr int ROT_CORNER_TOP_RIGHT    = 90;
    constexpr int ROT_CORNER_BOTTOM_RIGHT = 180;
    constexpr int ROT_CORNER_BOTTOM_LEFT  = 270;

    // Floor textures that register as "building interior"
    constexpr GameMap::Texture FLOOR_TEXTURES[] = {
        Tiles::FLOOR_STONE_PLAIN.texture,
        Tiles::FLOOR_STONE_TILES.texture,
        Tiles::FLOOR_STONE_CROSS.texture,
        Tiles::FLOOR_STONE_PATTERN.texture,
        Tiles::FLOOR_STONE_CRACKED.texture,
        Tiles::FLOOR_WOOD_PLANKS.texture,
    };
}

void MapGenerator::buildHouseWalls(Map& map, std::mt19937& rng, const Rect& playZone) {
    // ---------- 1. Reading and Floor Detection ----------
    const int W = playZone.w;
    const int H = playZone.h;
    auto idx = [&](const int x, const int y) { return (y - playZone.y) * W + (x - playZone.x); };

    auto isFloor = [&](const int x, const int y) {
        if (x < playZone.x || y < playZone.y || x >= playZone.x + W || y >= playZone.y + H) return false;
        for (const GameMap::Tile& t : map.cells[y][x].layers) {
            if (t.zIndex != GameMap::ZIndex::Layer1) continue;
            for (const GameMap::Texture& f : FLOOR_TEXTURES) {
                if (t.texture.col == f.col && t.texture.row == f.row) return true;
            }
        }
        return false;
    };

    auto putWall = [&](const int x, const int y, GameMap::Tile tile, const int rotation) {
        tile.modifier.rotation = rotation;
        addTile(map, x, y, tile, GameMap::ZIndex::Layer2);
    };

    std::uniform_int_distribution<int> percent(0, 99);

    // Select straight wall segment type (solid, window, weak/destructible)
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

    // ---------- 2. Building Footprint Extraction ----------
    std::vector<char> visited(W * H, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (visited[idx(x, y)] || !isFloor(x, y)) continue;

            // Top-left corner found: measure dimensions of the floor rectangle
            int w = 0;
            while (isFloor(x + w, y)) ++w;
            int h = 0;
            while (isFloor(x, y + h)) ++h;

            for (int yy = y; yy < y + h; ++yy) {
                for (int xx = x; xx < x + w; ++xx) {
                    visited[idx(xx, yy)] = 1;
                }
            }

            if (w < 3 || h < 3) continue; // Building area too small to contain an interior space

            // ---------- 3. Wall Tile Placement ----------
            // Outer Corners
            putWall(x,         y,         Tiles::WALL_CORNER_OUTER, ROT_CORNER_TOP_LEFT);
            putWall(x + w - 1, y,         Tiles::WALL_CORNER_OUTER, ROT_CORNER_TOP_RIGHT);
            putWall(x + w - 1, y + h - 1, Tiles::WALL_CORNER_OUTER, ROT_CORNER_BOTTOM_RIGHT);
            putWall(x,         y + h - 1, Tiles::WALL_CORNER_OUTER, ROT_CORNER_BOTTOM_LEFT);

            // Top and Bottom walls (length w)
            int lastTop = -100, lastBottom = -100;
            for (int i = 1; i < w - 1; ++i) {
                putWall(x + i, y,         pickStraight(i, w, lastTop),    ROT_TOP);
                putWall(x + i, y + h - 1, pickStraight(i, w, lastBottom), ROT_BOTTOM);
            }

            // Left and Right walls (length h)
            int lastLeft = -100, lastRight = -100;
            for (int j = 1; j < h - 1; ++j) {
                putWall(x,         y + j, pickStraight(j, h, lastLeft),  ROT_LEFT);
                putWall(x + w - 1, y + j, pickStraight(j, h, lastRight), ROT_RIGHT);
            }
        }
    }
}