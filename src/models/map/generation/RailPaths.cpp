#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <algorithm>
#include <random>
#include <vector>

namespace {
    constexpr int WIDTH_MIN = 10;  // Min distance between two parallel rails
    constexpr int WIDTH_MAX = 20;  // Max distance between two parallel rails
    constexpr int MARGIN    = 4;   // Margin between the map border
    constexpr int COVERAGE  = 80;  // Percentage of nodes in use
}

void MapGenerator::addRailPaths(Map &map, std::mt19937 &rng, const Rect &playZone) {
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

    // ---------- 2. Partial Spanning Tree (Randomized Depth-First Search / Maze Generation Algorithm) ----------
    // solid[] marks grid tiles that form part of the path structure (1 = occupied).
    // An outer margin of 1 empty cell padding allows reading neighbor bounds safely.
    int pw = gw + 2;
    std::vector<int> solid(pw * (gh + 2), 0);
    std::vector<int> visited(nw * nh, 0);
    std::vector<int> stackI, stackJ;

    // Target node count: stop traversal early based on COVERAGE percentage to leave empty pockets
    int target = std::max(2, nw * nh * COVERAGE / 100);
    int visitedCount = 1;

    // Pick a random starting node (rail tracks spawn randomly, unlike guard paths starting at corners)
    std::uniform_int_distribution<int> startI(0, nw - 1);
    std::uniform_int_distribution<int> startJ(0, nh - 1);
    int si = startI(rng);
    int sj = startJ(rng);

    visited[sj * nw + si] = 1;
    solid[(2 * sj + 1) * pw + (2 * si + 1)] = 1;
    stackI.push_back(si);
    stackJ.push_back(sj);

    // Iterative randomized Depth-First Search (DFS) algorithm with early termination
    while (!stackI.empty() && visitedCount < target) {
        int i = stackI.back();
        int j = stackJ.back();

        // List unvisited neighboring nodes for current position
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

        // Dead end: backtrack to previous node
        if (count == 0) {
            stackI.pop_back();
            stackJ.pop_back();
            continue;
        }

        std::uniform_int_distribution<int> pick(0, count - 1);
        int c = pick(rng);
        int ni = candI[c];
        int nj = candJ[c];

        // Node (i, j) maps to tile (2i + 1, 2j + 1) in solid[]; connection link is placed between nodes
        visited[nj * nw + ni] = 1;
        ++visitedCount;
        solid[(2 * nj + 1) * pw + (2 * ni + 1)] = 1;   // Target node
        solid[(j + nj + 1) * pw + (i + ni + 1)] = 1;   // Connection link
        stackI.push_back(ni);
        stackJ.push_back(nj);
    }

    // ---------- 3. Boundary Loop Construction (Wall Follower Algorithm) ----------
    // Map each cell corner to the next clockwise corner along the path perimeter
    int vw = gw + 1;
    std::vector<int> nextCorner(vw * (gh + 1), -1); // -1 = no corner boundary originates here

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

        // Step direction vectors (-1, 0, or 1 on each axis)
        int dx = (ex > sx) - (ex < sx);
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
    // Modulo wrapping (% n) closes the loop: the neighbor after the last tile is the first tile
    int n = static_cast<int>(cellsX.size());
    for (int i = 0; i < n; ++i) {
        int cx = cellsX[i];
        int cy = cellsY[i];

        int neighX[2] = { cellsX[(i + n - 1) % n], cellsX[(i + 1) % n] };
        int neighY[2] = { cellsY[(i + n - 1) % n], cellsY[(i + 1) % n] };

        // Bitmask calculation: identify rail outflow directions from current tile
        int mask = 0;
        for (int k = 0; k < 2; ++k) {
            if (neighX[k] > cx)      mask |= RIGHT;
            else if (neighX[k] < cx) mask |= LEFT;
            else if (neighY[k] > cy) mask |= DOWN;
            else                     mask |= UP;
        }

        // Apply corresponding rail tile texture and rotation modifier based on mask
        GameMap::Tile tile;
        switch (mask) {
            case UP | DOWN:    tile = Tiles::RAIL_STRAIGHT; tile.modifier.rotation = 0;   break;
            case LEFT | RIGHT: tile = Tiles::RAIL_STRAIGHT; tile.modifier.rotation = 90;  break;
            case RIGHT | DOWN: tile = Tiles::RAIL_CURVE;    tile.modifier.rotation = 0;   break;
            case LEFT | DOWN:  tile = Tiles::RAIL_CURVE;    tile.modifier.rotation = 90;  break;
            case LEFT | UP:    tile = Tiles::RAIL_CURVE;    tile.modifier.rotation = 180; break;
            case RIGHT | UP:   tile = Tiles::RAIL_CURVE;    tile.modifier.rotation = 270; break;
            default: continue;
        }
        addTile(map, cx, cy, tile, GameMap::ZIndex::Layer2);
    }
}