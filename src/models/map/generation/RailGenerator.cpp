#include "models/map/generation/RailGenerator.h"

#include <algorithm>

#include "models/map/Tiles.h"
#include "models/map/generation/Pathfinding.h"
#include "models/map/generation/TileHelpers.h"
#include "models/map/generation/TileMapUtils.h"

namespace MapGen {

namespace {

void autotileRails(TileMap& map) {
    auto isRail = [&](int x, int y) { return structureAt(map, x, y) == RAIL; };   // dans autotileRails

    for (int x = 0; x < mapWidth(map); ++x) {
        for (int y = 0; y < mapHeight(map); ++y) {
            if (!isRail(x, y)) continue;

            bool n = isRail(x, y - 1), s = isRail(x, y + 1);
            bool e = isRail(x + 1, y), w = isRail(x - 1, y);
            int count = n + s + e + w;

            Tile t;
            if (count >= 3) {
                t = Tiles::RAIL_CROSSROADS;
            } else if (count == 2) {
                bool vertical = n && s;
                bool horizontal = e && w;
                if (vertical || horizontal) {
                    t = Tiles::RAIL_STRAIGHT;
                    t.rotation = vertical ? ANGLE_0 : ANGLE_90;
                } else {
                    t = Tiles::RAIL_CURVE;
                    t.rotation = pairAngle(s, e);
                }
            } else if (count == 1) {
                t = Tiles::RAIL_STRAIGHT;
                t.rotation = (n || s) ? ANGLE_0 : ANGLE_90;
            } else {
                t = Tiles::RAIL_STRAIGHT;
            }
            addTile(map, x, y, t, ZIndex::Layer1);
        }
    }
}

} // namespace

void generateRails(TileMap& map, Rng& rng, const Rect& play) {
    std::uniform_real_distribution chance(0.f, 1.f);
    if (chance(rng) > 0.3f) return;
    if (play.w < 2 || play.h < 2) return;

    auto blocked = [&](int x, int y) {
        return oneOf(structureAt(map, x, y), {WALL, DOOR, PATH}) ||
               oneOf(groundAt(map, x, y),    {HOUSE, OPENING});
    };

    std::uniform_int_distribution edgeY(play.y, play.bottom());
    std::uniform_int_distribution lengthDist(5, 10);

    bool fromLeft = chance(rng) < 0.5f;
    Point a = fromLeft ? Point{play.x, edgeY(rng)} : Point{play.right(), edgeY(rng)};
    int length = lengthDist(rng);
    int bx = fromLeft ? std::min(play.right(), a.first + length)
                      : std::max(play.x, a.first - length);
    Point b = {bx, edgeY(rng)};

    const auto route = findRoute(map, a, b, blocked);
    if (route.empty()) return;

    for (const auto& [x, y] : route)
        if (isOpenGround(map, x, y))   // herbe ou terre
            addTile(map, x, y, Tiles::RAIL_STRAIGHT, ZIndex::Layer1);
    autotileRails(map);

    const auto [cx, cy] = route.back();
    if (structureAt(map, cx, cy) == RAIL)
        addTile(map, cx, cy, Tiles::CART_WOODEN, ZIndex::Layer2);
}

} // namespace MapGen
