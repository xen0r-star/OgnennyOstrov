#include "models/map/generation/TileHelpers.h"

#include "models/map/Tiles.h"

namespace MapGen {

Angle pairAngle(const bool a, const bool b) {
    if (a && b)   return ANGLE_0;
    if (a && !b)  return ANGLE_90;
    if (!a && !b) return ANGLE_180;
    return ANGLE_270;
}

Angle singleAngle(const bool a, const bool b, const bool c) {
    if (a) return ANGLE_0;
    if (b) return ANGLE_90;
    if (c) return ANGLE_180;
    return ANGLE_270;
}

Angle randomAngle(Rng& rng) {
    switch (std::uniform_int_distribution<int>(0, 3)(rng)) {
        case 0:  return ANGLE_0;
        case 1:  return ANGLE_90;
        case 2:  return ANGLE_180;
        default: return ANGLE_270;
    }
}

bool sameTile(const Tile& a, const Tile& b) {
    return a.col == b.col && a.row == b.row;
}

bool wantsRandomRotation(const Tile& t) {
    auto same = [&](const Tile& ref) { return sameTile(t, ref); };
    return same(Tiles::PROP_CAMPFIRE)             || same(Tiles::PROP_GRASS_TUFT) ||
           same(Tiles::PROP_BUSH)                 || same(Tiles::OBJECT_BOARD) ||
           same(Tiles::OBJECT_LARGE_CRATE_WOODEN) || same(Tiles::OBJECT_LARGE_BARREL_TOP) ||
           same(Tiles::OBJECT_SMALL_CRATE_WOODEN) || same(Tiles::OBJECT_SMALL_BARREL_TOP) ||
           same(Tiles::OBJECT_BARRELS_CLUSTER_1)  || same(Tiles::OBJECT_BARRELS_CLUSTER_2) ||
           same(Tiles::PROP_TREE);
}

} // namespace MapGen
