#include "models/map/generation/PathGenerator.h"

#include <algorithm>
#include <climits>
#include <cmath>

#include "models/map/Tiles.h"
#include "models/map/generation/Pathfinding.h"
#include "models/map/generation/TileHelpers.h"
#include "models/map/generation/TileMapUtils.h"

// Un chemin est directement posé sous forme de tuile (Layer1) :
//  - PATH_STRAIGHT_1 = chemin de bâtiment (provisoire, remplacé par l'autotile)
//  - PATH_STRAIGHT_2 = "promenade" (le style B est déduit de cette tuile)

namespace MapGen {

namespace {

constexpr int DX[4] = {0, 1, 0, -1};
constexpr int DY[4] = {-1, 0, 1, 0};

bool blocksPath(const TileMap& map, const int x, const int y) {
    return oneOf(structureAt(map, x, y), {WALL, DOOR}) ||
           oneOf(groundAt(map, x, y),    {HOUSE, OPENING, DIRT});
}

bool isPromenadeTile(const TileMap& map, const int x, const int y) {
    for (const Tile& t : {Tiles::PATH_STRAIGHT_2, Tiles::PATH_CURVE_2, Tiles::PATH_ARROW})
        if (hasTile(map, x, y, ZIndex::Layer1, t)) return true;
    return false;
}

Point randomBoundaryPoint(Rng& rng, const Rect& play, const int side) {
    switch (side) {
        case 0:  return {std::uniform_int_distribution(play.x, play.right())(rng), play.y};
        case 1:  return {play.right(), std::uniform_int_distribution(play.y, play.bottom())(rng)};
        case 2:  return {std::uniform_int_distribution(play.x, play.right())(rng), play.bottom()};
        default: return {play.x, std::uniform_int_distribution(play.y, play.bottom())(rng)};
    }
}

} // namespace

void generateHousePaths(TileMap& map) {
    const std::vector<Point> doors = findStructures(map, DOOR);
    if (doors.size() < 2) return;

    // Case libre devant une porte
    auto exteriorOf = [&](const Point& d) -> Point {
        for (int k = 0; k < 4; ++k) {
            const int nx = d.first + DX[k], ny = d.second + DY[k];
            if (!inBounds(map, nx, ny) || blocksPath(map, nx, ny)) continue;
            return {nx, ny};
        }
        return {d.first, d.second + 1};
    };

    auto blocked = [&map](const int x, const int y) { return blocksPath(map, x, y); };
    auto onGrass = [&](int x, int y) { return isOpenGround(map, x, y) && groundAt(map, x, y) == GRASS; };

    // Relie chaque porte à la plus proche déjà connectée
    std::vector connected(doors.size(), false);
    connected[0] = true;
    for (size_t step = 1; step < doors.size(); ++step) {
        int bestI = -1, bestJ = -1, bestDist = INT_MAX;
        for (size_t i = 0; i < doors.size(); ++i) {
            if (!connected[i]) continue;
            for (size_t j = 0; j < doors.size(); ++j) {
                if (connected[j]) continue;
                int d = std::abs(doors[i].first - doors[j].first) +
                        std::abs(doors[i].second - doors[j].second);
                if (d < bestDist) { bestDist = d; bestI = (int)i; bestJ = (int)j; }
            }
        }
        if (bestJ == -1) break;

        const auto route = findRoute(map, exteriorOf(doors[bestI]), exteriorOf(doors[bestJ]), blocked);
        for (const auto& [x, y] : route)
            if (onGrass(x, y))
                addTile(map, x, y, Tiles::PATH_STRAIGHT_1, ZIndex::Layer1);
        connected[bestJ] = true;
    }
}

void generatePromenadePaths(TileMap& map, Rng& rng, const Rect& play) {
    if (play.w < 11 || play.h < 11) return;

    auto blocked = [&map](const int x, const int y) { return blocksPath(map, x, y); };
    auto onGrass = [&](int x, int y) { return isOpenGround(map, x, y) && groundAt(map, x, y) == GRASS; };

    int roadCount = std::uniform_int_distribution(1, 2)(rng);
    for (int i = 0; i < roadCount; ++i) {
        int sideA = std::uniform_int_distribution(0, 3)(rng);
        int sideB = (sideA + 1 + std::uniform_int_distribution(0, 2)(rng)) % 4;

        Point a = randomBoundaryPoint(rng, play, sideA);
        Point b = randomBoundaryPoint(rng, play, sideB);
        if (blocked(a.first, a.second) || blocked(b.first, b.second)) continue;

        const auto route = findRoute(map, a, b, blocked);
        for (const auto& [x, y] : route)
            if (onGrass(x, y))
                addTile(map, x, y, Tiles::PATH_STRAIGHT_2, ZIndex::Layer1);
    }
}

void removePathBlocks(TileMap& map) {
    auto isPath   = [&](int x, int y) { return structureAt(map, x, y) == PATH; };
    auto pathLike = [&](int x, int y) { return oneOf(structureAt(map, x, y), {PATH, DOOR}); };

    const int W = mapWidth(map), H = mapHeight(map);

    auto countComponents = [&]() {
        std::vector<char> seen(W * H, 0);
        std::vector<Point> stack;
        int comps = 0;
        for (int x = 0; x < W; ++x)
            for (int y = 0; y < H; ++y) {
                if (!pathLike(x, y) || seen[x * H + y]) continue;
                ++comps;
                seen[x * H + y] = 1;
                stack.assign(1, Point{x, y});
                while (!stack.empty()) {
                    const Point p = stack.back();
                    stack.pop_back();
                    for (int k = 0; k < 4; ++k) {
                        const int nx = p.first + DX[k], ny = p.second + DY[k];
                        if (!pathLike(nx, ny) || seen[nx * H + ny]) continue;
                        seen[nx * H + ny] = 1;
                        stack.emplace_back(nx, ny);
                    }
                }
            }
        return comps;
    };

    auto neighbours = [&](const Point& p) {
        int n = 0;
        for (int k = 0; k < 4; ++k) n += pathLike(p.first + DX[k], p.second + DY[k]);
        return n;
    };

    const int components = countComponents();
    bool changed = true;
    for (int pass = 0; changed && pass < 10; ++pass) {
        changed = false;
        for (int x = 0; x + 1 < W; ++x) {
            for (int y = 0; y + 1 < H; ++y) {
                const Point cells[4] = {{x, y}, {x + 1, y}, {x, y + 1}, {x + 1, y + 1}};
                bool full = true;
                for (const Point& c : cells)
                    if (!isPath(c.first, c.second)) { full = false; break; }
                if (!full) continue;

                int order[4] = {0, 1, 2, 3};
                std::sort(order, order + 4, [&](int a, int b) {
                    return neighbours(cells[a]) < neighbours(cells[b]);
                });
                for (int idx : order) {
                    const Point p = cells[idx];
                    const Tile saved = findLayer(map, p.first, p.second, ZIndex::Layer1)->tile;
                    removeLayer(map, p.first, p.second, ZIndex::Layer1);
                    if (countComponents() <= components) { changed = true; break; }
                    addTile(map, p.first, p.second, saved, ZIndex::Layer1);
                }
            }
        }
    }
}

void autotilePathsUnified(TileMap& map) {
    auto isPath   = [&](int x, int y) { return structureAt(map, x, y) == PATH; };
    auto pathLike = [&](int x, int y) { return oneOf(structureAt(map, x, y), {PATH, DOOR}); };

    for (int x = 0; x < mapWidth(map); ++x) {
        for (int y = 0; y < mapHeight(map); ++y) {
            if (!isPath(x, y)) continue;

            const bool styleB = isPromenadeTile(map, x, y);
            const Tile straightTile = styleB ? Tiles::PATH_STRAIGHT_2 : Tiles::PATH_STRAIGHT_1;
            const Tile curveTile    = styleB ? Tiles::PATH_CURVE_2    : Tiles::PATH_CURVE_1;

            bool n = pathLike(x, y - 1), s = pathLike(x, y + 1);
            bool e = pathLike(x + 1, y), w = pathLike(x - 1, y);
            int count = n + s + e + w;

            Tile t;
            if (count == 4) {
                t = styleB ? Tiles::PATH_ARROW : Tiles::PATH_CROSSROADS;
            } else if (count == 3) {
                if (styleB) {
                    t = Tiles::PATH_ARROW;
                } else {
                    t = Tiles::PATH_T_JUNCTION;
                    t.rotation = singleAngle(!s, !w, !n);
                }
            } else if (count == 2) {
                bool vertical = n && s;
                bool horizontal = e && w;
                if (vertical || horizontal) {
                    t = straightTile;
                    t.rotation = vertical ? ANGLE_0 : ANGLE_90;
                } else {
                    t = curveTile;
                    t.rotation = pairAngle(s, e);
                }
            } else if (count == 1) {
                t = straightTile;
                t.rotation = (n || s) ? ANGLE_0 : ANGLE_90;
            } else {
                t = straightTile;
            }
            addTile(map, x, y, t, ZIndex::Layer1);
        }
    }
}

} // namespace MapGen
