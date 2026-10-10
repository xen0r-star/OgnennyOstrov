#ifndef GOULAG_TILES_H
#define GOULAG_TILES_H

#include "models/map/Tile.h"

namespace Tiles {
    inline constexpr Tile FLOOR_STONE_PLAIN         = Tile(0, 0, OPENING);
    inline constexpr Tile FLOOR_STONE_TILES         = Tile(0, 1, HOUSE);
    inline constexpr Tile FLOOR_STONE_CORNER_DIAG   = Tile(0, 2, HOUSE);
    inline constexpr Tile FLOOR_STONE_CROSS         = Tile(0, 3, HOUSE);
    inline constexpr Tile FLOOR_STONE_PATTERN       = Tile(0, 4, HOUSE);
    inline constexpr Tile FLOOR_STONE_CRACKED       = Tile(0, 5, HOUSE);
    inline constexpr Tile FLOOR_STONE_CORNER_BEVEL  = Tile(1, 0, HOUSE);
    inline constexpr Tile FLOOR_WOOD_PLANKS         = Tile(1, 1, HOUSE);

    inline constexpr Tile GROUND_DIRT               = Tile(1, 2, DIRT);
    inline constexpr Tile GROUND_GRASS              = Tile(1, 3, GRASS);

    inline constexpr Tile WALL_STRAIGHT             = Tile(1, 4, WALL);
    inline constexpr Tile WALL_CORNER_OUTER         = Tile(1, 5, WALL);
    inline constexpr Tile WALL_STRAIGHT_DIAG        = Tile(2, 0, WALL);
    inline constexpr Tile WALL_WIDE_CURVE_OUTER     = Tile(2, 1, WALL);
    inline constexpr Tile WALL_CURVE_OUTER          = Tile(2, 2, WALL);
    inline constexpr Tile WALL_CURVE_INNER          = Tile(2, 3, WALL);
    inline constexpr Tile WALL_CORNER_INNER         = Tile(2, 4, WALL);
    inline constexpr Tile WALL_ROUNDED_END          = Tile(2, 5, WALL);
    inline constexpr Tile WALL_WINDOWS              = Tile(3, 0, WALL);
    inline constexpr Tile WALL_WEAK                 = Tile(3, 1, WALL);
    inline constexpr Tile WALL_STRAIGHT_SHORT       = Tile(3, 2, WALL);
    inline constexpr Tile WALL_DOORWAY_OPEN         = Tile(3, 3, DOOR);
    inline constexpr Tile WALL_DOOR_FRAME           = Tile(3, 4, DOOR);
    inline constexpr Tile WALL_DOORWAY_CLOSE        = Tile(3, 5, DOOR);
    inline constexpr Tile WALL_BROKEN_DOOR          = Tile(4, 0, DOOR);

    inline constexpr Tile RAIL_STRAIGHT             = Tile(4, 1, RAIL);
    inline constexpr Tile RAIL_CURVE                = Tile(4, 2, RAIL);
    inline constexpr Tile RAIL_CROSSROADS           = Tile(4, 3, RAIL);
    inline constexpr Tile CART_WOODEN               = Tile(4, 4, DECORATION);

    inline constexpr Tile PATH_STRAIGHT_1           = Tile(4, 5, PATH);
    inline constexpr Tile PATH_CURVE_1              = Tile(5, 0, PATH);
    inline constexpr Tile PATH_CROSSROADS           = Tile(5, 1, PATH);
    inline constexpr Tile PATH_T_JUNCTION           = Tile(8, 5, PATH);
    inline constexpr Tile PATH_CURVE_2              = Tile(5, 2, PATH);
    inline constexpr Tile PATH_STRAIGHT_2           = Tile(5, 3, PATH);
    inline constexpr Tile PATH_ARROW                = Tile(5, 4, PATH);
    inline constexpr Tile PATH_END                  = Tile(5, 5, PATH);

    inline constexpr Tile PROP_CAMPFIRE             = Tile(6, 0, DECORATION);
    inline constexpr Tile PROP_GRASS_TUFT           = Tile(6, 1, DECORATION);
    inline constexpr Tile PROP_BUSH                 = Tile(6, 2, DECORATION);

    inline constexpr Tile OBJECT_LARGE_CRATE_WOODEN = Tile(6, 3, DECORATION);
    inline constexpr Tile OBJECT_LARGE_BARREL_TOP   = Tile(6, 4, DECORATION);
    inline constexpr Tile OBJECT_SMALL_CRATE_WOODEN = Tile(6, 5, DECORATION);
    inline constexpr Tile OBJECT_BARRELS_CLUSTER_1  = Tile(7, 0, DECORATION);
    inline constexpr Tile OBJECT_SMALL_BARREL_TOP   = Tile(7, 1, DECORATION);
    inline constexpr Tile OBJECT_BARRELS_CLUSTER_2  = Tile(7, 2, DECORATION);
    inline constexpr Tile OBJECT_TABLE              = Tile(7, 3, DECORATION);
    inline constexpr Tile OBJECT_CHAIR              = Tile(7, 4, DECORATION);
    inline constexpr Tile OBJECT_BOARD              = Tile(7, 5, DECORATION);
    inline constexpr Tile OBJECT_BED                = Tile(8, 0, DECORATION);
    inline constexpr Tile OBJECT_BED_2              = Tile(8, 1, DECORATION);

    inline constexpr Tile PROP_TREE                 = Tile(8, 2, DECORATION);
    inline constexpr Tile WALL_BARBED_WIRE          = Tile(8, 3, WALL);
    inline constexpr Tile WATCHTOWER                = Tile(8, 4, WALL);
}

#endif // GOULAG_TILES_H
