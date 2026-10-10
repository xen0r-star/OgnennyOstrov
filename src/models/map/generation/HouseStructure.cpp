#include "models/map/generation/HouseStructure.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>

#include "models/map/Tiles.h"
#include "models/map/generation/TileHelpers.h"
#include "models/map/generation/TileMapUtils.h"

namespace MapGen {

void buildWalls(TileMap& map, Rng& rng, const std::vector<House>& houses) {
    struct Style { int windowPeriod; float weakChance; Tile corner; };
    const Style styles[3] = {
        {3, 0.04f, Tiles::WALL_CORNER_OUTER},
        {4, 0.10f, Tiles::WALL_CORNER_OUTER},
        {2, 0.02f, Tiles::WALL_CORNER_OUTER},
    };
    std::uniform_real_distribution chance(0.f, 1.f);

    for (const House& h : houses) {
        const Style& st = styles[h.type % 3];
        for (int x = h.x; x <= h.right(); ++x) {
            for (int y = h.y; y <= h.bottom(); ++y) {
                bool top = y == h.y, bottom = y == h.bottom();
                bool left = x == h.x, right = x == h.right();
                bool border = top || bottom || left || right;
                if (!border) continue;
                if (groundAt(map, x, y) == OPENING) continue;

                bool isCorner = (left || right) && (top || bottom);

                Tile t;
                if (isCorner) {
                    t = st.corner;
                    t.rotation = pairAngle(top, left);
                } else {
                    const bool horizontal = top || bottom;
                    const int len = horizontal ? h.w : h.h;
                    const int i   = horizontal ? x - h.x : y - h.y;
                    const bool inner = i >= 2 && i <= len - 3;

                    if (inner && st.windowPeriod > 0 && (i - len / 2) % st.windowPeriod == 0)
                        t = Tiles::WALL_WINDOWS;
                    else if (chance(rng) < st.weakChance)
                        t = Tiles::WALL_WEAK;
                    else
                        t = Tiles::WALL_STRAIGHT;
                    t.rotation = singleAngle(top, right, bottom);
                }
                addTile(map, x, y, t, ZIndex::Layer1);
            }
        }
    }
}

void placeDoors(TileMap& map, Rng& rng, const std::vector<House>& houses) {
    std::uniform_real_distribution chance(0.f, 1.f);

    constexpr int DX[4] = {0, 1, 0, -1};
    constexpr int DY[4] = {-1, 0, 1, 0};

    std::array<int, 3> sidePerType, offsetPerType;
    for (int t = 0; t < 3; ++t) {
        sidePerType[t]   = std::uniform_int_distribution(0, 3)(rng);
        offsetPerType[t] = std::uniform_int_distribution(-1, 1)(rng);
    }

    auto pickDoor = [&](const House& h, int side, int offset) -> Point {
        std::vector<Point> cells;
        switch (side) {
            case 0:  for (int x = h.x + 1; x < h.right();  ++x) cells.emplace_back(x, h.y);       break;
            case 1:  for (int y = h.y + 1; y < h.bottom(); ++y) cells.emplace_back(h.right(), y); break;
            case 2:  for (int x = h.x + 1; x < h.right();  ++x) cells.emplace_back(x, h.bottom()); break;
            default: for (int y = h.y + 1; y < h.bottom(); ++y) cells.emplace_back(h.x, y);       break;
        }
        // On ne garde que les murs dont l'extérieur est libre
        cells.erase(std::remove_if(cells.begin(), cells.end(), [&](const Point& p) {
            if (structureAt(map, p.first, p.second) != WALL) return true;
            const int ox = p.first + DX[side], oy = p.second + DY[side];
            if (!inBounds(map, ox, oy)) return true;
            return hasStructure(map, ox, oy) || oneOf(groundAt(map, ox, oy), {HOUSE, OPENING});
        }), cells.end());
        if (cells.empty()) return {-1, -1};

        const bool horizontal = (side == 0 || side == 2);
        const int target = horizontal ? h.x + (h.w - 1) / 2 + offset
                                      : h.y + (h.h - 1) / 2 + offset;
        Point best = cells[0];
        int bestD = INT_MAX;
        for (const Point& p : cells) {
            const int d = std::abs((horizontal ? p.first : p.second) - target);
            if (d < bestD) { bestD = d; best = p; }
        }
        return best;
    };

    auto placeDoor = [&](int x, int y, int side) {
        const float r = chance(rng);
        Tile t = r < 0.45f ? Tiles::WALL_DOORWAY_OPEN
               : r < 0.75f ? Tiles::WALL_DOOR_FRAME
               : r < 0.92f ? Tiles::WALL_DOORWAY_CLOSE
                           : Tiles::WALL_BROKEN_DOOR;
        t.rotation = singleAngle(side == 0, side == 1, side == 2);
        addTile(map, x, y, t, ZIndex::Layer1);
    };

    for (const House& h : houses) {
        const int type = h.type % 3;
        int side = sidePerType[type];
        Point p = pickDoor(h, side, offsetPerType[type]);

        for (int k = 1; k < 4 && p.first < 0; ++k) {
            side = (sidePerType[type] + k) % 4;
            p = pickDoor(h, side, 0);
        }
        if (p.first < 0) continue;
        placeDoor(p.first, p.second, side);

        if (chance(rng) < 0.2f) {
            const int back = (side + 2) % 4;
            const Point q = pickDoor(h, back, 0);
            if (q.first >= 0) placeDoor(q.first, q.second, back);
        }
    }
}

} // namespace MapGen
