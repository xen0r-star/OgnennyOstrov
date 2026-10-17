#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <algorithm>
#include <functional>
#include <queue>
#include <random>
#include <utility>
#include <vector>

namespace {
    constexpr int EDGE_MARGIN     = 1;  // Margin between paths and the map border
    constexpr int FREE_COST       = 3;  // Cost of an empty tile
    constexpr int OWN_COST        = 1;  // Cost of a tile already used by a house path (reuse)
    constexpr int NEAR_HOUSE_COST = 2;  // Extra cost for walking along a building wall
    constexpr int CROSS_COST      = 5;  // Extra cost for crossing a guard path / a rail
    constexpr int TURN_COST       = 2;  // Extra cost per turn
    constexpr int LOOP_PERCENT    = 10; // Percent chance of adding an extra link (loop) between two nearby buildings
    constexpr int LOOP_MAX_DIST   = 14; // Maximum distance (in tiles) for an extra link

    // Tile rotations
    constexpr int ROT_T_MISSING_DOWN  = 0;
    constexpr int ROT_T_MISSING_LEFT  = 90;
    constexpr int ROT_T_MISSING_UP    = 180;
    constexpr int ROT_T_MISSING_RIGHT = 270;
    constexpr int ROT_END_UP         = 0;
    constexpr int ROT_END_RIGHT      = 90;
    constexpr int ROT_END_DOWN       = 180;
    constexpr int ROT_END_LEFT       = 270;

    // Floor textures that count as "building interior"
    constexpr GameMap::Texture FLOOR_TEXTURES[] = {
        Tiles::FLOOR_STONE_PLAIN.texture,
        Tiles::FLOOR_STONE_TILES.texture,
        Tiles::FLOOR_STONE_CROSS.texture,
        Tiles::FLOOR_STONE_PATTERN.texture,
        Tiles::FLOOR_STONE_CRACKED.texture,
        Tiles::FLOOR_WOOD_PLANKS.texture,
    };
    // Dirt field texture (Layer0)
    constexpr GameMap::Texture DIRT_TEXTURES[] = {
        Tiles::GROUND_DIRT.texture,
    };
    // Guard path textures (Layer1)
    constexpr GameMap::Texture PATH_TEXTURES[] = {
        Tiles::PATH_STRAIGHT_1.texture,
        Tiles::PATH_CURVE_1.texture,
        Tiles::PATH_CROSSROADS.texture,
        Tiles::PATH_T_JUNCTION.texture,
        Tiles::PATH_CURVE_2.texture,
        Tiles::PATH_STRAIGHT_2.texture,
        Tiles::PATH_STRAIGHT_1_TO_2.texture,
        Tiles::PATH_END.texture,
    };
    // Rail textures (Layer2)
    constexpr GameMap::Texture RAIL_TEXTURES[] = {
        Tiles::RAIL_STRAIGHT.texture,
        Tiles::RAIL_CURVE.texture,
        Tiles::RAIL_CROSSROADS.texture,
        Tiles::CART_WOODEN.texture,
    };
}

void MapGenerator::addHousePaths(Map& map, std::mt19937& rng, const Rect& playZone) {
    enum Direction {
        UP = 1,
        RIGHT = 2,
        DOWN = 4,
        LEFT = 8
    };
    constexpr int DX[4] = { 0, 1, 0, -1 };
    constexpr int DY[4] = { -1, 0, 1, 0 };

    enum Line { GUARD = 1, RAIL = 2 };

    // ---------- 1. Reading the map ----------
    const int W = playZone.w;
    const int H = playZone.h;
    auto idx = [&](const int x, const int y) { return (y - playZone.y) * W + (x - playZone.x); };
    auto inZone = [&](const int x, const int y) {
        return x >= playZone.x + EDGE_MARGIN && x < playZone.x + W - EDGE_MARGIN &&
               y >= playZone.y + EDGE_MARGIN && y < playZone.y + H - EDGE_MARGIN;
    };

    // Does the tile have a texture from the list on this layer?
    auto hasTexture = [&](const int x, const int y, const int zIndex, const GameMap::Texture* list, const int count) {
        for (const GameMap::Tile& t : map.cells[y][x].layers) {
            if (t.zIndex != zIndex) continue;
            for (int i = 0; i < count; ++i) {
                if (t.texture.col == list[i].col && t.texture.row == list[i].row) return true;
            }
        }
        return false;
    };
    constexpr int floorCount = std::size(FLOOR_TEXTURES);
    constexpr int pathCount  = std::size(PATH_TEXTURES);
    constexpr int railCount  = std::size(RAIL_TEXTURES);

    // Existing lines (guard paths, rails)
    std::vector<unsigned char> lineKind(W * H, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (hasTexture(x, y, GameMap::ZIndex::Layer1, PATH_TEXTURES, pathCount)) lineKind[idx(x, y)] |= GUARD;
            if (hasTexture(x, y, GameMap::ZIndex::Layer2, RAIL_TEXTURES, railCount)) lineKind[idx(x, y)] |= RAIL;
        }
    }

    // Dirt zone: impassable for house paths
    constexpr int dirtCount = std::size(DIRT_TEXTURES);
    std::vector<unsigned char> dirt(W * H, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (hasTexture(x, y, GameMap::ZIndex::Layer0, DIRT_TEXTURES, dirtCount)) dirt[idx(x, y)] = 1;
        }
    }

    // ---------- 2. Buildings ----------
    struct Anchor { int x, y, outDir; };
    struct House {
        int x, y, w, h;
        std::vector<Anchor> anchors;
    };
    std::vector<House> houses;
    std::vector<int> houseId(W * H, -1);

    auto isFloor = [&](const int x, const int y) {
        if (x < playZone.x || y < playZone.y || x >= playZone.x + W || y >= playZone.y + H) return false;
        return hasTexture(x, y, GameMap::ZIndex::Layer1, FLOOR_TEXTURES, floorCount);
    };

    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (houseId[idx(x, y)] >= 0 || !isFloor(x, y)) continue;

            // Top-left corner found: measure the rectangle
            int w = 0;
            while (isFloor(x + w, y)) ++w;
            int h = 0;
            while (isFloor(x, y + h)) ++h;
            if (w < 3 || h < 3) continue;

            const int id = static_cast<int>(houses.size());
            for (int yy = y; yy < y + h; ++yy) {
                for (int xx = x; xx < x + w; ++xx) {
                    houseId[idx(xx, yy)] = id;
                }
            }
            houses.push_back({ x, y, w, h, {} });
        }
    }

    const int n = static_cast<int>(houses.size());
    if (n < 2) return;

    // Tiles next to a building (to avoid walking along walls)
    std::vector<unsigned char> nearHouse(W * H, 0);
    for (const House& hs : houses) {
        for (int yy = hs.y - 1; yy <= hs.y + hs.h; ++yy) {
            for (int xx = hs.x - 1; xx <= hs.x + hs.w; ++xx) {
                if (xx < playZone.x || yy < playZone.y || xx >= playZone.x + W || yy >= playZone.y + H) continue;
                nearHouse[idx(xx, yy)] = 1;
            }
        }
    }

    // Anchor points: tiles in front of the walls (no corners). outDir = direction pointing away from the building.
    auto addAnchor = [&](House& hs, const int ax, const int ay, const int outDir) {
        if (!inZone(ax, ay)) return;
        const int i = idx(ax, ay);
        if (houseId[i] >= 0 || lineKind[i] != 0 || dirt[i]) return;
        hs.anchors.push_back({ ax, ay, outDir });
    };
    for (House& hs : houses) {
        for (int i = 1; i < hs.w - 1; ++i) {
            addAnchor(hs, hs.x + i, hs.y - 1,     0); // in front of the top wall
            addAnchor(hs, hs.x + i, hs.y + hs.h,  2); // in front of the bottom wall
        }
        for (int j = 1; j < hs.h - 1; ++j) {
            addAnchor(hs, hs.x + hs.w, hs.y + j,  1); // in front of the right wall
            addAnchor(hs, hs.x - 1,    hs.y + j,  3); // in front of the left wall
        }
    }

    // ---------- 3. Which houses to connect? ----------
    // Manhattan distance between two building rectangles
    auto gapDist = [&](const House& a, const House& b) {
        const int dx = std::max(0, std::max(a.x - (b.x + b.w), b.x - (a.x + a.w)));
        const int dy = std::max(0, std::max(a.y - (b.y + b.h), b.y - (a.y + a.h)));
        return dx + dy;
    };

    std::vector<std::pair<int, int>> links;
    std::vector<char> linked(n * n, 0);
    std::vector<char> inTree(n, 0);
    inTree[0] = 1;

    // Minimum spanning tree (Prim): everything is connected, closest first
    for (int step = 1; step < n; ++step) {
        // Shortest link between a house in the tree and one outside it
        int best = 1 << 30, bestA = -1, bestB = -1;
        for (int a = 0; a < n; ++a) {
            if (!inTree[a]) continue;
            for (int b = 0; b < n; ++b) {
                if (inTree[b]) continue;
                const int d = gapDist(houses[a], houses[b]);
                if (d < best) { best = d; bestA = a; bestB = b; }
            }
        }
        inTree[bestB] = 1;
        links.emplace_back(bestA, bestB);
        linked[bestA * n + bestB] = linked[bestB * n + bestA] = 1;
    }

    // A few extra links to create loops
    std::uniform_int_distribution<int> percent(0, 99);
    for (int a = 0; a < n; ++a) {
        for (int b = a + 1; b < n; ++b) {
            if (linked[a * n + b]) continue;
            if (gapDist(houses[a], houses[b]) > LOOP_MAX_DIST) continue;
            if (percent(rng) >= LOOP_PERCENT) continue;
            links.emplace_back(a, b);
            linked[a * n + b] = linked[b * n + a] = 1;
        }
    }

    // ---------- 4. Routing the links ----------
    // edge[]: direction bits in which the path continues from each tile. 0 = no path.
    std::vector<unsigned char> edge(W * H, 0);
    std::vector<unsigned char> targetDoor(W * H, 0);

    constexpr int INF = 1 << 29;
    std::vector dist(W * H * 4, INF);
    std::vector prev(W * H * 4, -1);

    // Can a path cross a guard path / rail here? Only straight across, never along it.
    auto canCross = [&](const int x, const int y, const int d) {
        const int kind = lineKind[idx(x, y)];
        if (kind == (GUARD | RAIL)) return false; // intersection of two lines: avoid
        const bool horizontal = (d % 2 == 1);
        const int ax = horizontal ? 1 : 0;
        const int ay = horizontal ? 0 : 1;
        for (int s = -1; s <= 1; s += 2) {
            const int nx = x + s * ax;
            const int ny = y + s * ay;
            if (nx < playZone.x || ny < playZone.y || nx >= playZone.x + W || ny >= playZone.y + H) continue;
            if (lineKind[idx(nx, ny)] & kind) return false;
        }
        return true;
    };

    // Does the tile already hold a path (house path or guard path)?
    auto isPathCell = [&](const int x, const int y) {
        if (x < playZone.x || y < playZone.y || x >= playZone.x + W || y >= playZone.y + H) return false;
        const int i = idx(x, y);
        return edge[i] != 0 || (lineKind[i] & GUARD) != 0;
    };

    // Would this tile complete a 2x2 block of path? (s = current state, its last tiles count too)
    auto makesBlock = [&](const int nx, const int ny, const int s) {
        int chain[3] = { -1, -1, -1 };
        int t = s;
        for (int k = 0; k < 3 && t != -1; ++k) {
            chain[k] = t >> 2;
            t = prev[t];
        }
        auto occupied = [&](const int x, const int y) {
            if (x < playZone.x || y < playZone.y || x >= playZone.x + W || y >= playZone.y + H) return false;
            if (isPathCell(x, y)) return true;
            const int i = idx(x, y);
            return chain[0] == i || chain[1] == i || chain[2] == i;
        };
        // The 4 possible 2x2 squares containing the tile
        for (int ox = -1; ox <= 0; ++ox) {
            for (int oy = -1; oy <= 0; ++oy) {
                bool full = true;
                for (int i = 0; i < 2 && full; ++i) {
                    for (int j = 0; j < 2; ++j) {
                        const int x = nx + ox + i;
                        const int y = ny + oy + j;
                        if (x == nx && y == ny) continue;
                        if (!occupied(x, y)) { full = false; break; }
                    }
                }
                if (full) return true;
            }
        }
        return false;
    };

    // Dijkstra for each link: find the cheapest route between the two houses
    for (const auto& [a, b] : links) {
        const House& from = houses[a];
        const House& to   = houses[b];
        if (from.anchors.empty() || to.anchors.empty()) continue;

        std::fill(dist.begin(), dist.end(), INF);
        std::fill(prev.begin(), prev.end(), -1);
        std::fill(targetDoor.begin(), targetDoor.end(), 0);

        for (const Anchor& an : to.anchors) {
            targetDoor[idx(an.x, an.y)] = static_cast<unsigned char>(1 << ((an.outDir + 2) & 3));
        }

        using Entry = std::pair<int, int>;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<>> queue;
        for (const Anchor& an : from.anchors) {
            const int ai = idx(an.x, an.y);
            if (edge[ai] == 0 && makesBlock(an.x, an.y, -1)) continue;
            const int s = ai * 4 + an.outDir;
            dist[s] = 0;
            queue.emplace( 0, s );
        }

        int goal = -1;
        while (!queue.empty()) {
            const auto [cost, s] = queue.top();
            queue.pop();
            if (cost > dist[s]) continue;

            const int cell = s >> 2;
            const int d    = s & 3;
            const int cx   = playZone.x + cell % W;
            const int cy   = playZone.y + cell / W;

            if (targetDoor[cell]) { goal = s; break; } // reached the target house

            const bool onLine = lineKind[cell] != 0; // no turning while on a guard path / rail
            for (int turn = 0; turn < 3; ++turn) {
                // straight, right, left
                const int nd = (turn == 0) ? d : (turn == 1 ? (d + 1) & 3 : (d + 3) & 3);
                if (onLine && nd != d) continue;

                const int nx = cx + DX[nd];
                const int ny = cy + DY[nd];
                if (!inZone(nx, ny)) continue;

                const int ni = idx(nx, ny);
                if (houseId[ni] >= 0) continue;
                if (dirt[ni]) continue;

                int step = (edge[ni] != 0) ? OWN_COST : FREE_COST;
                if (lineKind[ni] != 0) {
                    if (onLine || !canCross(nx, ny, nd)) continue;
                    step += CROSS_COST;
                }

                const bool isNewPath = edge[ni] == 0 && !(lineKind[ni] & GUARD);
                if (isNewPath && makesBlock(nx, ny, s)) continue;
                if (nearHouse[ni] && edge[ni] == 0) step += NEAR_HOUSE_COST;
                if (nd != d) step += TURN_COST;

                const int ns = ni * 4 + nd;
                const int nc = cost + step;
                if (nc < dist[ns]) {
                    dist[ns] = nc;
                    prev[ns] = s;
                    queue.emplace( nc, ns );
                }
            }
        }

        if (goal < 0) continue;

        // Rebuild the route from goal back to start
        std::vector<int> states;
        for (int s = goal; s != -1; s = prev[s]) states.push_back(s);
        std::reverse(states.begin(), states.end());

        for (size_t i = 0; i + 1 < states.size(); ++i) {
            const int ca = states[i] >> 2;
            const int cb = states[i + 1] >> 2;
            const int d  = states[i + 1] & 3;
            edge[ca] |= static_cast<unsigned char>(1 << d);
            edge[cb] |= static_cast<unsigned char>(1 << ((d + 2) & 3));
        }

        const int startOut = states.front() & 3;
        edge[states.front() >> 2] |= static_cast<unsigned char>(1 << ((startOut + 2) & 3));
        edge[goal >> 2] |= targetDoor[goal >> 2];
    }

    // ---------- 5. Merging thin / thick paths ----------
    std::vector<unsigned char> thick(W * H, 0);
    for (bool changed = true; changed;) {
        changed = false;
        for (int y = playZone.y; y < playZone.y + H; ++y) {
            for (int x = playZone.x; x < playZone.x + W; ++x) {
                const int i = idx(x, y);
                if (edge[i] == 0 || lineKind[i] != 0 || thick[i]) continue;

                int touching = 0;
                for (int d = 0; d < 4; ++d) {
                    if (edge[i] & (1 << d)) continue;
                    const int nx = x + DX[d];
                    const int ny = y + DY[d];
                    if (nx < playZone.x || ny < playZone.y || nx >= playZone.x + W || ny >= playZone.y + H) continue;
                    const int ni = idx(nx, ny);
                    if ((lineKind[ni] & GUARD) || thick[ni]) touching |= (1 << d);
                }
                if (touching == 0) continue; // nothing thick nearby

                thick[i] = 1;
                edge[i] |= static_cast<unsigned char>(touching);
                for (int d = 0; d < 4; ++d) {
                    if (!(touching & (1 << d))) continue;
                    edge[idx(x + DX[d], y + DY[d])] |= static_cast<unsigned char>(1 << ((d + 2) & 3));
                }
                changed = true; // thickness may spread further, so loop again
            }
        }
    }

    // ---------- 6. Placing the tiles ----------
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            const int i = idx(x, y);
            int mask = edge[i];
            if (mask == 0) continue;

            // A crossed guard path becomes a junction: keep its own directions
            if (lineKind[i] & GUARD) {
                for (int d = 0; d < 4; ++d) {
                    const int nx = x + DX[d];
                    const int ny = y + DY[d];
                    if (nx < playZone.x || ny < playZone.y || nx >= playZone.x + W || ny >= playZone.y + H) continue;
                    if (lineKind[idx(nx, ny)] & GUARD) mask |= (1 << d);
                }
            }

            // Thick (style 2), thin (style 1)
            const bool isThick = thick[i] || (lineKind[i] & GUARD);
            const GameMap::Tile straight = isThick ? Tiles::PATH_STRAIGHT_2 : Tiles::PATH_STRAIGHT_1;
            const GameMap::Tile curve    = isThick ? Tiles::PATH_CURVE_2    : Tiles::PATH_CURVE_1;

            GameMap::Tile tile;
            switch (mask) {
                case UP | DOWN:    tile = straight; tile.modifier.rotation = 0;   break;
                case LEFT | RIGHT: tile = straight; tile.modifier.rotation = 90;  break;
                case RIGHT | DOWN: tile = curve;    tile.modifier.rotation = 0;   break;
                case LEFT | DOWN:  tile = curve;    tile.modifier.rotation = 90;  break;
                case LEFT | UP:    tile = curve;    tile.modifier.rotation = 180; break;
                case RIGHT | UP:   tile = curve;    tile.modifier.rotation = 270; break;

                case RIGHT | DOWN | LEFT: tile = Tiles::PATH_T_JUNCTION; tile.modifier.rotation = ROT_T_MISSING_UP;    break;
                case UP | DOWN | LEFT:    tile = Tiles::PATH_T_JUNCTION; tile.modifier.rotation = ROT_T_MISSING_RIGHT; break;
                case UP | RIGHT | LEFT:   tile = Tiles::PATH_T_JUNCTION; tile.modifier.rotation = ROT_T_MISSING_DOWN;  break;
                case UP | RIGHT | DOWN:   tile = Tiles::PATH_T_JUNCTION; tile.modifier.rotation = ROT_T_MISSING_LEFT;  break;

                case UP | RIGHT | DOWN | LEFT: tile = Tiles::PATH_CROSSROADS; tile.modifier.rotation = 0; break;

                case UP:    tile = Tiles::PATH_END; tile.modifier.rotation = ROT_END_UP;    break;
                case RIGHT: tile = Tiles::PATH_END; tile.modifier.rotation = ROT_END_RIGHT; break;
                case DOWN:  tile = Tiles::PATH_END; tile.modifier.rotation = ROT_END_DOWN;  break;
                case LEFT:  tile = Tiles::PATH_END; tile.modifier.rotation = ROT_END_LEFT;  break;
                default: continue;
            }
            addTile(map, x, y, tile, GameMap::ZIndex::Layer1);
        }
    }
}