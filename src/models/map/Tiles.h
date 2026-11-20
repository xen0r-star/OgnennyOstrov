#ifndef GOULAG_TILES_H
#define GOULAG_TILES_H

#include "models/map/Tile.h"

namespace Tiles {
    inline constexpr auto FLOOR_STONE_PLAIN         = GameMap::Tile(GameMap::Texture(0, 0));
    inline constexpr auto FLOOR_STONE_TILES         = GameMap::Tile(GameMap::Texture(0, 1));
    inline constexpr auto FLOOR_STONE_CORNER_DIAG   = GameMap::Tile(GameMap::Texture(0, 2));
    inline constexpr auto FLOOR_STONE_CROSS         = GameMap::Tile(GameMap::Texture(0, 3));
    inline constexpr auto FLOOR_STONE_PATTERN       = GameMap::Tile(GameMap::Texture(0, 4));
    inline constexpr auto FLOOR_STONE_CRACKED       = GameMap::Tile(GameMap::Texture(0, 5));
    inline constexpr auto FLOOR_STONE_CORNER_BEVEL  = GameMap::Tile(GameMap::Texture(1, 0));
    inline constexpr auto FLOOR_WOOD_PLANKS         = GameMap::Tile(GameMap::Texture(1, 1));

    inline constexpr auto GROUND_DIRT               = GameMap::Tile(GameMap::Texture(1, 2));
    inline constexpr auto GROUND_GRASS              = GameMap::Tile(GameMap::Texture(1, 3));

    inline constexpr auto WALL_STRAIGHT             = GameMap::Tile(GameMap::Texture(1, 4));
    inline constexpr auto WALL_CORNER_OUTER         = GameMap::Tile(GameMap::Texture(1, 5));
    inline constexpr auto WALL_STRAIGHT_DIAG        = GameMap::Tile(GameMap::Texture(2, 0));
    inline constexpr auto WALL_WIDE_CURVE_OUTER     = GameMap::Tile(GameMap::Texture(2, 1));
    inline constexpr auto WALL_CURVE_OUTER          = GameMap::Tile(GameMap::Texture(2, 2));
    inline constexpr auto WALL_CURVE_INNER          = GameMap::Tile(GameMap::Texture(2, 3));
    inline constexpr auto WALL_CORNER_INNER         = GameMap::Tile(GameMap::Texture(2, 4));
    inline constexpr auto WALL_ROUNDED_END          = GameMap::Tile(GameMap::Texture(2, 5));
    inline constexpr auto WALL_WINDOWS              = GameMap::Tile(GameMap::Texture(3, 0));
    inline constexpr auto WALL_WEAK                 = GameMap::Tile(GameMap::Texture(3, 1));
    inline constexpr auto WALL_STRAIGHT_SHORT       = GameMap::Tile(GameMap::Texture(3, 2));
    inline constexpr auto WALL_DOORWAY_OPEN         = GameMap::Tile(GameMap::Texture(3, 3));
    inline constexpr auto WALL_DOOR_FRAME           = GameMap::Tile(GameMap::Texture(3, 4));
    inline constexpr auto WALL_DOORWAY_CLOSE        = GameMap::Tile(GameMap::Texture(3, 5));
    inline constexpr auto WALL_BROKEN_DOOR          = GameMap::Tile(GameMap::Texture(4, 0));

    inline constexpr auto RAIL_STRAIGHT             = GameMap::Tile(GameMap::Texture(4, 1));
    inline constexpr auto RAIL_CURVE                = GameMap::Tile(GameMap::Texture(4, 2));
    inline constexpr auto RAIL_CROSSROADS           = GameMap::Tile(GameMap::Texture(4, 3));
    inline constexpr auto CART_WOODEN               = GameMap::Tile(GameMap::Texture(4, 4));

    inline constexpr auto PATH_STRAIGHT_1           = GameMap::Tile(GameMap::Texture(4, 5));
    inline constexpr auto PATH_CURVE_1              = GameMap::Tile(GameMap::Texture(5, 0));
    inline constexpr auto PATH_CROSSROADS           = GameMap::Tile(GameMap::Texture(5, 1));
    inline constexpr auto PATH_T_JUNCTION           = GameMap::Tile(GameMap::Texture(8, 5));
    inline constexpr auto PATH_CURVE_2              = GameMap::Tile(GameMap::Texture(5, 2));
    inline constexpr auto PATH_STRAIGHT_2           = GameMap::Tile(GameMap::Texture(5, 3));
    inline constexpr auto PATH_END                  = GameMap::Tile(GameMap::Texture(5, 4));
    inline constexpr auto PATH_STRAIGHT_1_TO_2      = GameMap::Tile(GameMap::Texture(5, 5));

    inline constexpr auto PROP_CAMPFIRE             = GameMap::Tile(GameMap::Texture(6, 0));
    inline constexpr auto PROP_GRASS_TUFT           = GameMap::Tile(GameMap::Texture(6, 1));
    inline constexpr auto PROP_BUSH                 = GameMap::Tile(GameMap::Texture(6, 2));

    inline constexpr auto OBJECT_LARGE_CRATE_WOODEN = GameMap::Tile(GameMap::Texture(6, 3));
    inline constexpr auto OBJECT_LARGE_BARREL_TOP   = GameMap::Tile(GameMap::Texture(6, 4));
    inline constexpr auto OBJECT_SMALL_CRATE_WOODEN = GameMap::Tile(GameMap::Texture(6, 5));
    inline constexpr auto OBJECT_BARRELS_CLUSTER_1  = GameMap::Tile(GameMap::Texture(7, 0));
    inline constexpr auto OBJECT_SMALL_BARREL_TOP   = GameMap::Tile(GameMap::Texture(7, 1));
    inline constexpr auto OBJECT_BARRELS_CLUSTER_2  = GameMap::Tile(GameMap::Texture(7, 2));
    inline constexpr auto OBJECT_TABLE              = GameMap::Tile(GameMap::Texture(7, 3));
    inline constexpr auto OBJECT_CHAIR              = GameMap::Tile(GameMap::Texture(7, 4));
    inline constexpr auto OBJECT_BOARD              = GameMap::Tile(GameMap::Texture(7, 5));
    inline constexpr auto OBJECT_BED                = GameMap::Tile(GameMap::Texture(8, 0));
    inline constexpr auto OBJECT_BED_2              = GameMap::Tile(GameMap::Texture(8, 1));

    inline constexpr auto PROP_TREE                 = GameMap::Tile(GameMap::Texture(8, 2));
    inline constexpr auto WALL_BARBED_WIRE          = GameMap::Tile(GameMap::Texture(8, 3));
    inline constexpr auto WATCHTOWER                = GameMap::Tile(GameMap::Texture(8, 4));
}

#endif // GOULAG_TILES_H
