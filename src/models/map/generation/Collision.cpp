#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"

#include <initializer_list>

namespace {

    bool isTile(const GameMap::Tile& a, const GameMap::Tile& b) {
        return a.texture.col == b.texture.col && a.texture.row == b.texture.row;
    }

    CollisionMask makeMask(const std::initializer_list<const char*> rows) {
        CollisionMask m{};
        int y = 0;
        for (const char* row : rows) {
            for (int x = 0; x < MASK_SIZE; ++x) {
                m[y * MASK_SIZE + x] = (row[x] == '#');
            }
            ++y;
        }
        return m;
    }

    CollisionMask fullMask() {
        CollisionMask m{};
        m.fill(true);
        return m;
    }

    bool isObstacleObject(const GameMap::Tile& t) {
        using namespace Tiles;
        return isTile(t, OBJECT_LARGE_CRATE_WOODEN) || isTile(t, OBJECT_SMALL_CRATE_WOODEN)
            || isTile(t, OBJECT_LARGE_BARREL_TOP)   || isTile(t, OBJECT_SMALL_BARREL_TOP)
            || isTile(t, OBJECT_BARRELS_CLUSTER_1)  || isTile(t, OBJECT_BARRELS_CLUSTER_2)
            || isTile(t, OBJECT_TABLE)              || isTile(t, OBJECT_CHAIR)
            || isTile(t, OBJECT_BED)                || isTile(t, OBJECT_BED_2)
            || isTile(t, OBJECT_BOARD)              || isTile(t, CART_WOODEN)
            || isTile(t, PROP_CAMPFIRE);
    }

    CollisionMask moveMaskOf(const GameMap::Tile& t) {
        using namespace Tiles;
        if (isTile(t, WALL_BARBED_WIRE) || isTile(t, WATCHTOWER) || isTile(t, PROP_TREE))
            return fullMask();

        if (isTile(t, WALL_STRAIGHT) || isTile(t, WALL_WINDOWS) || isTile(t, WALL_WEAK)
            || isTile(t, WALL_STRAIGHT_SHORT))
            return makeMask({ "######", "######", "......", "......", "......", "......" });

        if (isTile(t, WALL_CORNER_OUTER))
            return makeMask({ "######", "######", "##....", "##....", "##....", "##...." });

        if (isTile(t, WALL_CORNER_INNER))
            return makeMask({ "##....", "##....", "......", "......", "......", "......" });

        if (isTile(t, WALL_DOORWAY_OPEN))
            return makeMask({ "#....#", "#....#", ".....#", ".....#", ".....#", ".....#" });

        if (isTile(t, WALL_DOOR_FRAME) || isTile(t, WALL_BROKEN_DOOR))
            return makeMask({ "#....#", "#....#", "......", "......", "......", "......" });

        if (isTile(t, WALL_DOORWAY_CLOSE))
            return makeMask({ "######", "#....#", "......", "......", "......", "......" });

        if (isObstacleObject(t)) return fullMask();
        return {};
    }

    CollisionMask sightMaskOf(const GameMap::Tile& t) {
        if (isObstacleObject(t)) return {};
        return moveMaskOf(t);
    }

    // Clockwise quarter turn: (x, y) -> (MASK_SIZE - 1 - y, x)
    CollisionMask rotateMask(const CollisionMask& m, const int quarterTurns) {
        CollisionMask out = m;
        for (int k = 0; k < quarterTurns; ++k) {
            CollisionMask next{};
            for (int y = 0; y < MASK_SIZE; ++y) {
                for (int x = 0; x < MASK_SIZE; ++x) {
                    if (out[y * MASK_SIZE + x]) {
                        next[x * MASK_SIZE + (MASK_SIZE - 1 - y)] = true;
                    }
                }
            }
            out = next;
        }
        return out;
    }

    // Rotation is stored in degrees (0, 90, 180, 270)
    int quarterTurnsOf(const GameMap::Tile& t) {
        return (((t.modifier.rotation % 360) + 360) % 360) / 90;
    }
}

void MapGenerator::buildCollision(Map& map) {
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            MapCell& cell = map.cells[y][x];

            // If ONE layer blocks a sub-cell, the whole cell blocks it (logical OR)
            for (const GameMap::Tile& tile : cell.layers) {
                const int turns = quarterTurnsOf(tile);
                const CollisionMask move  = rotateMask(moveMaskOf(tile),  turns);
                const CollisionMask sight = rotateMask(sightMaskOf(tile), turns);

                for (std::size_t i = 0; i < cell.blocksMovement.size(); ++i) {
                    cell.blocksMovement[i] = cell.blocksMovement[i] || move[i];
                    cell.blocksSight[i]    = cell.blocksSight[i]    || sight[i];
                }

                if (isTile(tile, Tiles::PROP_BUSH)) cell.hides = true;
            }
        }
    }
}