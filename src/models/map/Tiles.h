#ifndef GOULAG_TILES_H
#define GOULAG_TILES_H

#include "models/map/Tile.h"

namespace Tiles {
    inline constexpr auto FLOOR_STONE_PLAIN         = Map::Tile(Map::Texture(0, 0));
    inline constexpr auto FLOOR_STONE_TILES         = Map::Tile(Map::Texture(0, 1));
    inline constexpr auto FLOOR_STONE_CORNER_DIAG   = Map::Tile(Map::Texture(0, 2));
    inline constexpr auto FLOOR_STONE_CROSS         = Map::Tile(Map::Texture(0, 3));
    inline constexpr auto FLOOR_STONE_PATTERN       = Map::Tile(Map::Texture(0, 4));
    inline constexpr auto FLOOR_STONE_CRACKED       = Map::Tile(Map::Texture(0, 5));
    inline constexpr auto FLOOR_STONE_CORNER_BEVEL  = Map::Tile(Map::Texture(1, 0));
    inline constexpr auto FLOOR_WOOD_PLANKS         = Map::Tile(Map::Texture(1, 1));

    inline constexpr auto GROUND_DIRT               = Map::Tile(Map::Texture(1, 2));
    inline constexpr auto GROUND_GRASS              = Map::Tile(Map::Texture(1, 3));

    inline constexpr auto WALL_STRAIGHT             = Map::Tile(Map::Texture(1, 4));
    inline constexpr auto WALL_CORNER_OUTER         = Map::Tile(Map::Texture(1, 5));
    inline constexpr auto WALL_STRAIGHT_DIAG        = Map::Tile(Map::Texture(2, 0));
    inline constexpr auto WALL_WIDE_CURVE_OUTER     = Map::Tile(Map::Texture(2, 1));
    inline constexpr auto WALL_CURVE_OUTER          = Map::Tile(Map::Texture(2, 2));
    inline constexpr auto WALL_CURVE_INNER          = Map::Tile(Map::Texture(2, 3));
    inline constexpr auto WALL_CORNER_INNER         = Map::Tile(Map::Texture(2, 4));
    inline constexpr auto WALL_ROUNDED_END          = Map::Tile(Map::Texture(2, 5));
    inline constexpr auto WALL_WINDOWS              = Map::Tile(Map::Texture(3, 0));
    inline constexpr auto WALL_WEAK                 = Map::Tile(Map::Texture(3, 1));
    inline constexpr auto WALL_STRAIGHT_SHORT       = Map::Tile(Map::Texture(3, 2));
    inline constexpr auto WALL_DOORWAY_OPEN         = Map::Tile(Map::Texture(3, 3));
    inline constexpr auto WALL_DOOR_FRAME           = Map::Tile(Map::Texture(3, 4));
    inline constexpr auto WALL_DOORWAY_CLOSE        = Map::Tile(Map::Texture(3, 5));
    inline constexpr auto WALL_BROKEN_DOOR          = Map::Tile(Map::Texture(4, 0));

    inline constexpr auto RAIL_STRAIGHT             = Map::Tile(Map::Texture(4, 1));
    inline constexpr auto RAIL_CURVE                = Map::Tile(Map::Texture(4, 2));
    inline constexpr auto RAIL_CROSSROADS           = Map::Tile(Map::Texture(4, 3));
    inline constexpr auto CART_WOODEN               = Map::Tile(Map::Texture(4, 4));

    inline constexpr auto PATH_STRAIGHT_1           = Map::Tile(Map::Texture(4, 5));
    inline constexpr auto PATH_CURVE_1              = Map::Tile(Map::Texture(5, 0));
    inline constexpr auto PATH_CROSSROADS           = Map::Tile(Map::Texture(5, 1));
    inline constexpr auto PATH_T_JUNCTION           = Map::Tile(Map::Texture(8, 5));
    inline constexpr auto PATH_CURVE_2              = Map::Tile(Map::Texture(5, 2));
    inline constexpr auto PATH_STRAIGHT_2           = Map::Tile(Map::Texture(5, 3));
    inline constexpr auto PATH_ARROW                = Map::Tile(Map::Texture(5, 4));
    inline constexpr auto PATH_END                  = Map::Tile(Map::Texture(5, 5));

    inline constexpr auto PROP_CAMPFIRE             = Map::Tile(Map::Texture(6, 0));
    inline constexpr auto PROP_GRASS_TUFT           = Map::Tile(Map::Texture(6, 1));
    inline constexpr auto PROP_BUSH                 = Map::Tile(Map::Texture(6, 2));

    inline constexpr auto OBJECT_LARGE_CRATE_WOODEN = Map::Tile(Map::Texture(6, 3));
    inline constexpr auto OBJECT_LARGE_BARREL_TOP   = Map::Tile(Map::Texture(6, 4));
    inline constexpr auto OBJECT_SMALL_CRATE_WOODEN = Map::Tile(Map::Texture(6, 5));
    inline constexpr auto OBJECT_BARRELS_CLUSTER_1  = Map::Tile(Map::Texture(7, 0));
    inline constexpr auto OBJECT_SMALL_BARREL_TOP   = Map::Tile(Map::Texture(7, 1));
    inline constexpr auto OBJECT_BARRELS_CLUSTER_2  = Map::Tile(Map::Texture(7, 2));
    inline constexpr auto OBJECT_TABLE              = Map::Tile(Map::Texture(7, 3));
    inline constexpr auto OBJECT_CHAIR              = Map::Tile(Map::Texture(7, 4));
    inline constexpr auto OBJECT_BOARD              = Map::Tile(Map::Texture(7, 5));
    inline constexpr auto OBJECT_BED                = Map::Tile(Map::Texture(8, 0));
    inline constexpr auto OBJECT_BED_2              = Map::Tile(Map::Texture(8, 1));

    inline constexpr auto PROP_TREE                 = Map::Tile(Map::Texture(8, 2));
    inline constexpr auto WALL_BARBED_WIRE          = Map::Tile(Map::Texture(8, 3));
    inline constexpr auto WATCHTOWER                = Map::Tile(Map::Texture(8, 4));
}

#endif // GOULAG_TILES_H
