#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <algorithm>
#include <random>
#include <vector>
#include <functional>
#include <queue>
#include <utility>

namespace {
    // addGuardPaths()
    constexpr int WIDTH_MIN = 6;   // Min distance between two parallel paths
    constexpr int WIDTH_MAX = 12;  // Max distance between two parallel paths
    constexpr int MARGIN    = 2;   // Margin between the map border

    // correctionsGuardPaths()
    constexpr int EDGE_MARGIN     = 1;   // Margin between the map border
    constexpr int FREE_COST       = 3;   // Cost of an empty square
    constexpr int TURN_COST       = 2;   // Surcharge per turn
    constexpr int NEAR_HOUSE_COST = 1;   // Surcharge for travelling along a building
    constexpr int NEAR_PATH_COST  = 6;   // Surcharge for travelling along another path
    constexpr int DIRT_COST       = 3;   // Surcharge for crossing a field
    constexpr int RAIL_COST       = 12;  // Surcharge for crossing a railway track (last resort)
}

void MapGenerator::addGuardPaths(Map& map, std::mt19937& rng, const Rect& playZone) {
    enum Direction {
        UP = 1,
        RIGHT = 2,
        DOWN = 4,
        LEFT = 8
    };

    std::uniform_int_distribution widthDist(WIDTH_MIN, WIDTH_MAX);

    // ---------- 1. Grid Line Positions ----------
    // pos[0] = x positions, pos[1] = y positions
    // Each column and row is assigned a random variable width.
    std::vector<int> pos[2];
    int size[2] = { playZone.w - 1 - 2 * MARGIN, playZone.h - 1 - 2 * MARGIN };

    for (int axis = 0; axis < 2; ++axis) {
        // a) Randomly generate widths until the play zone is filled
        std::vector<int> widths;
        int total = 0;
        while (true) {
            int width = widthDist(rng);
            if (total + width > size[axis]) break;
            total += width;
            widths.push_back(width);
        }

        // b) Ensure an odd number of segments (Node - Link - Node - ... - Node pattern)
        if (widths.size() % 2 == 0 && !widths.empty()) {
            total -= widths.back();
            widths.pop_back();
        }
        if (widths.size() < 3) return; // Zone is too small to build grid

        // c) Evenly distribute remaining leftover space across all segments (1 tile at a time)
        int leftover = size[axis] - total;
        for (int i = 0; i < leftover; ++i) {
            widths[i % widths.size()] += 1;
        }

        // d) Convert segment widths into absolute positions
        int sum = 0;
        pos[axis].push_back(0);
        for (int width : widths) {
            sum += width;
            pos[axis].push_back(sum);
        }
    }

    int gw = static_cast<int>(pos[0].size()) - 1; // Number of grid columns
    int gh = static_cast<int>(pos[1].size()) - 1; // Number of grid rows
    int nw = (gw + 1) / 2;                        // Number of horizontal node points
    int nh = (gh + 1) / 2;                        // Number of vertical node points

    int ox = playZone.x + MARGIN;
    int oy = playZone.y + MARGIN;


    // ---------- 2. Random Spanning Tree (Depth-First Search / Maze Generation Algorithm) ----------
    int pw = gw + 2;
    std::vector<int> solid(pw * (gh + 2), 0);
    std::vector<int> visited(nw * nh, 0);
    std::vector<int> stackI, stackJ;

    visited[0] = 1;
    solid[1 * pw + 1] = 1;
    stackI.push_back(0);
    stackJ.push_back(0);

    // Iterative randomized Depth-First Search (DFS) algorithm
    while (!stackI.empty()) {
        int i = stackI.back();
        int j = stackJ.back();

        int candI[4], candJ[4];
        int count = 0;
        for (int k = 0; k < 4; ++k) {
            constexpr int dj[4] = {0, 0, 1, -1};
            constexpr int di[4] = {1, -1, 0, 0};
            int ni = i + di[k];
            int nj = j + dj[k];

            if (ni >= 0 && nj >= 0 && ni < nw && nj < nh && visited[nj * nw + ni] == 0) {
                candI[count] = ni;
                candJ[count] = nj;
                ++count;
            }
        }

        if (count == 0) {
            stackI.pop_back();
            stackJ.pop_back();
            continue;
        }

        std::uniform_int_distribution<int> pick(0, count - 1);
        int c = pick(rng);
        int ni = candI[c];
        int nj = candJ[c];

        visited[nj * nw + ni] = 1;
        solid[(2 * nj + 1) * pw + (2 * ni + 1)] = 1;
        solid[(j + nj + 1) * pw + (i + ni + 1)] = 1;
        stackI.push_back(ni);
        stackJ.push_back(nj);
    }


    // ---------- 3. Spanning Tree Boundary Loop Construction (Wall Follower Algorithm) ----------
    int vw = gw + 1;
    std::vector<int> nextCorner(vw * (gh + 1), -1);

    // Trace around solid nodes to construct a continuous boundary loop
    for (int cy = 0; cy < gh; ++cy) {
        for (int cx = 0; cx < gw; ++cx) {
            if (solid[(cy + 1) * pw + (cx + 1)] == 0) continue;

            int up    = solid[cy * pw + (cx + 1)];
            int down  = solid[(cy + 2) * pw + (cx + 1)];
            int left  = solid[(cy + 1) * pw + cx];
            int right = solid[(cy + 1) * pw + (cx + 2)];

            if (up == 0)    nextCorner[cy * vw + cx]             = cy * vw + (cx + 1);
            if (right == 0) nextCorner[cy * vw + (cx + 1)]       = (cy + 1) * vw + (cx + 1);
            if (down == 0)  nextCorner[(cy + 1) * vw + (cx + 1)] = (cy + 1) * vw + cx;
            if (left == 0)  nextCorner[(cy + 1) * vw + cx]       = cy * vw + cx;
        }
    }

    int start = -1;
    for (int v = 0; v < static_cast<int>(nextCorner.size()); ++v) {
        if (nextCorner[v] >= 0) { start = v; break; }
    }
    if (start < 0) return;


    // ---------- 4. Ordered Boundary Loop Tile Traversal ----------
    std::vector<int> cellsX, cellsY;
    int v = start;
    do {
        int nv = nextCorner[v];

        // Retrieve tile-space start and end corner coordinates
        int sx = ox + pos[0][v % vw];
        int sy = oy + pos[1][v / vw];
        int ex = ox + pos[0][nv % vw];
        int ey = oy + pos[1][nv / vw];

        int dx = (ex > sx) - (ex < sx); // Direction steps: -1, 0, or 1
        int dy = (ey > sy) - (ey < sy);

        // Step along segment toward next corner (endpoint is appended by subsequent segment)
        while (sx != ex || sy != ey) {
            cellsX.push_back(sx);
            cellsY.push_back(sy);
            sx += dx;
            sy += dy;
        }
        v = nv;
    } while (v != start);


    // ---------- 5. Tile Placement & Bitmask Rotation Calculation ----------
    int n = static_cast<int>(cellsX.size());
    for (int i = 0; i < n; ++i) {
        int cx = cellsX[i];
        int cy = cellsY[i];

        int neighX[2] = { cellsX[(i + n - 1) % n], cellsX[(i + 1) % n] };
        int neighY[2] = { cellsY[(i + n - 1) % n], cellsY[(i + 1) % n] };

        int mask = 0;
        for (int k = 0; k < 2; ++k) {
            if (neighX[k] > cx)      mask |= RIGHT;
            else if (neighX[k] < cx) mask |= LEFT;
            else if (neighY[k] > cy) mask |= DOWN;
            else                     mask |= UP;
        }

        GameMap::Tile tile;
        switch (mask) {
            case UP | DOWN:    tile = Tiles::PATH_STRAIGHT_2; tile.modifier.rotation = 0;   break;
            case LEFT | RIGHT: tile = Tiles::PATH_STRAIGHT_2; tile.modifier.rotation = 90;  break;
            case RIGHT | DOWN: tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 0;   break;
            case LEFT | DOWN:  tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 90;  break;
            case LEFT | UP:    tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 180; break;
            case RIGHT | UP:   tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 270; break;
            default: continue;
        }
        addTile(map, cx, cy, tile, GameMap::ZIndex::Layer1);
    }
}

void MapGenerator::correctionsGuardPaths(Map& map, std::mt19937& rng, const Rect& playZone) {
    (void)rng;

    enum Direction {
        UP = 1,
        RIGHT = 2,
        DOWN = 4,
        LEFT = 8
    };
    constexpr int DX[4] = { 0, 1, 0, -1 };
    constexpr int DY[4] = { -1, 0, 1, 0 };
    constexpr int INF = 1 << 29;

    // ---------- 1. Helper Lambdas & Index Mapping ----------
    const int W = playZone.w;
    const int H = playZone.h;
    const int N = W * H;
    auto idx = [&](const int x, const int y) { return (y - playZone.y) * W + (x - playZone.x); };
    auto inBounds = [&](const int x, const int y) {
        return x >= playZone.x && y >= playZone.y && x < playZone.x + W && y < playZone.y + H;
    };
    auto inZone = [&](const int x, const int y) {
        return x >= playZone.x + EDGE_MARGIN && x < playZone.x + W - EDGE_MARGIN &&
               y >= playZone.y + EDGE_MARGIN && y < playZone.y + H - EDGE_MARGIN;
    };
    auto sameTex = [](const GameMap::Texture& a, const GameMap::Texture& b) {
        return a.col == b.col && a.row == b.row;
    };

    // Calculate open connectivity direction mask for path tiles
    auto guardMask = [&](const GameMap::Tile& t) -> int {
        const int rot = ((t.modifier.rotation % 360) + 360) % 360;
        if (sameTex(t.texture, Tiles::PATH_STRAIGHT_2.texture)) {
            return (rot % 180 == 0) ? (UP | DOWN) : (LEFT | RIGHT);
        }
        if (sameTex(t.texture, Tiles::PATH_CURVE_2.texture)) {
            switch (rot) {
                case 0:   return RIGHT | DOWN;
                case 90:  return LEFT | DOWN;
                case 180: return LEFT | UP;
                default:  return RIGHT | UP;
            }
        }
        return 0;
    };

    // ---------- 2. Read and Classify Map Layers ----------
    std::vector<int>  cur(N, 0);        // Open direction mask per path tile
    std::vector<char> pathCell(N, 0);   // Flag indicating cell belongs to guard path
    std::vector<char> blocked(N, 0);    // Impassable obstacles (e.g. house floors)
    std::vector<char> railCell(N, 0);   // Railway track tiles (crossable only as a last resort)
    std::vector<char> dirt(N, 0);       // Dirt/farm field tiles
    std::vector<char> changed(N, 0);    // Mark tiles flagged for re-rendering

    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            const int i = idx(x, y);
            for (const GameMap::Tile& t : map.cells[y][x].layers) {
                if (t.zIndex == GameMap::ZIndex::Layer1) cur[i] |= guardMask(t);
                if (t.zIndex == GameMap::ZIndex::Layer0 && sameTex(t.texture, Tiles::GROUND_DIRT.texture)) dirt[i] = 1;
            }
            if (cur[i] != 0) pathCell[i] = 1;
            else if (!isTileEmpty(map, x, y, GameMap::ZIndex::Layer1)) blocked[i] = 1;
            if (!isTileEmpty(map, x, y, GameMap::ZIndex::Layer3)) blocked[i] = 1;
            if (!isTileEmpty(map, x, y, GameMap::ZIndex::Layer2)) railCell[i] = 1;
        }
    }

    // Identify tiles adjacent to obstacles (8-neighbor proximity mask)
    std::vector<char> nearHouse(N, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (!blocked[idx(x, y)]) continue;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (inBounds(x + dx, y + dy)) nearHouse[idx(x + dx, y + dy)] = 1;
                }
            }
        }
    }

    // ---------- 3. Connected Components Identification (Disjoint-Set / Union-Find Algorithm) ----------
    std::vector<int> parent(N);
    for (int i = 0; i < N; ++i) parent[i] = i;
    auto find = [&](int a) {
        while (parent[a] != a) {
            parent[a] = parent[parent[a]]; // Path compression optimization
            a = parent[a];
        }
        return a;
    };
    auto unite = [&](const int a, const int b) { parent[find(a)] = find(b); };

    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            const int i = idx(x, y);
            if (!pathCell[i]) continue;
            for (int d = 1; d <= 2; ++d) { // RIGHT and DOWN checks suffice
                if (!(cur[i] & (1 << d))) continue;
                const int nx = x + DX[d];
                const int ny = y + DY[d];
                if (!inBounds(nx, ny)) continue;
                const int ni = idx(nx, ny);
                if (cur[ni] & (1 << ((d + 2) & 3))) unite(i, ni);
            }
        }
    }

    // ---------- 4. Dead-End & Broken Path Segment Detection ----------
    // An open connection direction whose target neighbor does not respond indicates a cut path (e.g. building placed over it).
    // Disconnected direction flags are removed, incrementing `need` to track missing connections.
    std::vector<int> stubMask(N, 0);
    std::vector<int> need(N, 0);
    std::vector<int> needCells;

    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            const int i = idx(x, y);
            if (!pathCell[i]) continue;
            for (int d = 0; d < 4; ++d) {
                const int bit = 1 << d;
                if (!(cur[i] & bit)) continue;
                const int nx = x + DX[d];
                const int ny = y + DY[d];
                const bool linked = inBounds(nx, ny) && (cur[idx(nx, ny)] & (1 << ((d + 2) & 3)));
                if (!linked) {
                    stubMask[i] |= bit;
                    ++need[i];
                }
            }
            if (need[i] > 0) needCells.push_back(i);
        }
    }
    if (needCells.empty()) return; // Path loop is complete and undamaged
    for (const int c : needCells) {
        cur[c] &= ~stubMask[c];
        changed[c] = 1;
    }

    // Validate perpendicular rail crossing (ensure adjacent track alignment is clear)
    auto canCrossRail = [&](const int x, const int y, const int d) {
        const bool horizontal = (d % 2 == 1);
        const int ax = horizontal ? 1 : 0;
        const int ay = horizontal ? 0 : 1;
        for (int s = -1; s <= 1; s += 2) {
            const int nx = x + s * ax;
            const int ny = y + s * ay;
            if (inBounds(nx, ny) && railCell[idx(nx, ny)]) return false;
        }
        return true;
    };

    // ---------- 5. Path Repair & Detour Routing (Weighted Dijkstra's Algorithm) ----------
    std::vector<int> dist(N * 4);
    std::vector<int> prev(N * 4);

    // Would this tile complete a 2x2 block of path? (s = current state, its last tiles count too)
    auto makesBlock = [&](const int nx, const int ny, const int s) {
        int chain[3] = { -1, -1, -1 };
        int t = s;
        for (int k = 0; k < 3 && t != -1; ++k) {
            chain[k] = t >> 2;
            t = prev[t];
        }
        auto occupied = [&](const int x, const int y) {
            if (!inBounds(x, y)) return false;
            const int i = idx(x, y);
            return pathCell[i] || chain[0] == i || chain[1] == i || chain[2] == i;
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

    for (const int startCell : needCells) {
        while (need[startCell] > 0) {
            // Attempt connection to distinct unconnected path components first, otherwise close loop locally
            bool otherChain = false;
            for (const int c : needCells) {
                if (need[c] > 0 && find(c) != find(startCell)) { otherChain = true; break; }
            }
            auto isGoal = [&](const int c) {
                return c != startCell && need[c] > 0 && (!otherChain || find(c) != find(startCell));
            };
            // Check if cell (x, y) is adjacent to an established path cell
            auto touchesPath = [&](const int x, const int y) {
                for (int d = 0; d < 4; ++d) {
                    const int nx = x + DX[d];
                    const int ny = y + DY[d];
                    if (!inBounds(nx, ny)) continue;
                    const int ni = idx(nx, ny);
                    if (pathCell[ni] && ni != startCell && need[ni] == 0) return true;
                }
                return false;
            };

            int goal = -1;
            // Attempt 1: Route without crossing tracks. Attempt 2: Allow perpendicular track crossing.
            for (int attempt = 0; attempt < 2 && goal < 0; ++attempt) {
                const bool allowRail = (attempt == 1);

                std::fill(dist.begin(), dist.end(), INF);
                std::fill(prev.begin(), prev.end(), -1);

                using Entry = std::pair<int, int>; // Priority tuple: (cost, state); state = cell_index * 4 + direction
                std::priority_queue<Entry, std::vector<Entry>, std::greater<>> queue;
                for (int d = 0; d < 4; ++d) {
                    const int s = startCell * 4 + d;
                    dist[s] = 0;
                    queue.emplace(0, s);
                }

                // Weighted Shortest Path Traversal via Dijkstra's Algorithm
                while (!queue.empty()) {
                    const auto [cost, s] = queue.top();
                    queue.pop();
                    if (cost > dist[s]) continue;

                    const int cell = s >> 2;
                    const int d    = s & 3;
                    if (cell != startCell && isGoal(cell)) { goal = s; break; }

                    const int cx = playZone.x + cell % W;
                    const int cy = playZone.y + cell / W;
                    const bool onRail = railCell[cell] && !pathCell[cell];

                    for (int turn = 0; turn < 3; ++turn) {
                        const int nd = (turn == 0) ? d : (turn == 1 ? (d + 1) & 3 : (d + 3) & 3);
                        if (onRail && nd != d) continue; // Track traversal must maintain linear direction

                        const int nx = cx + DX[nd];
                        const int ny = cy + DY[nd];
                        if (!inZone(nx, ny)) continue;

                        const int ni = idx(nx, ny);
                        if (blocked[ni]) continue;
                        if (!pathCell[ni] && makesBlock(nx, ny, s)) continue;

                        int step = FREE_COST;
                        if (pathCell[ni]) {
                            if (!isGoal(ni)) continue; // Avoid overlapping existing completed path cells
                        } else {
                            if (railCell[ni]) {
                                if (!allowRail || onRail || !canCrossRail(nx, ny, nd)) continue;
                                step += RAIL_COST;
                            }
                            if (dirt[ni])               step += DIRT_COST;
                            if (nearHouse[ni])          step += NEAR_HOUSE_COST;
                            if (touchesPath(nx, ny))    step += NEAR_PATH_COST;
                        }
                        if (nd != d) step += TURN_COST;

                        const int ns = ni * 4 + nd;
                        const int nc = cost + step;
                        if (nc < dist[ns]) {
                            dist[ns] = nc;
                            prev[ns] = s;
                            queue.emplace(nc, ns);
                        }
                    }
                }
            }

            if (goal < 0) break; // No viable detour path found; retain original tile state

            // Backtrack path trajectory and apply open directions to visited cells
            std::vector<int> states;
            for (int s = goal; s != -1; s = prev[s]) states.push_back(s);
            std::reverse(states.begin(), states.end());

            for (size_t i = 0; i + 1 < states.size(); ++i) {
                const int ca = states[i] >> 2;
                const int cb = states[i + 1] >> 2;
                const int d  = states[i + 1] & 3; // Direction vector step: a -> b
                cur[ca] |= 1 << d;
                cur[cb] |= 1 << ((d + 2) & 3);
                changed[ca] = changed[cb] = 1;
                pathCell[ca] = pathCell[cb] = 1;
            }

            const int goalCell = goal >> 2;
            --need[startCell];
            --need[goalCell];
            unite(startCell, goalCell);
        }
    }

    // ---------- 6. Tile Re-rendering & Finalization ----------
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            const int i = idx(x, y);
            if (!changed[i] || need[i] != 0) continue; // Leave unchanged or unresolved tiles intact

            GameMap::Tile tile;
            switch (cur[i]) {
                case UP | DOWN:    tile = Tiles::PATH_STRAIGHT_2; tile.modifier.rotation = 0;   break;
                case LEFT | RIGHT: tile = Tiles::PATH_STRAIGHT_2; tile.modifier.rotation = 90;  break;
                case RIGHT | DOWN: tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 0;   break;
                case LEFT | DOWN:  tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 90;  break;
                case LEFT | UP:    tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 180; break;
                case RIGHT | UP:   tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 270; break;
                default: continue;
            }
            addTile(map, x, y, tile, GameMap::ZIndex::Layer1);
        }
    }
}