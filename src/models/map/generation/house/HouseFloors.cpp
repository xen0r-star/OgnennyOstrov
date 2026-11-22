#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <algorithm>
#include <random>
#include <vector>
#include <iostream>

namespace {
    constexpr int GAP          = 2;   // Free space between a building and the tracks / other buildings
    constexpr int PATH_GAP     = 0;   // Free space along guard tracks
    constexpr int EDGE_MARGIN  = 2;   // Margin from the map border
    constexpr int STEP         = 2;   // Grid coordinate alignment step multiplier
    constexpr int TRIES        = 150; // Number of trials to position a building
    constexpr int ALIGN_CHANCE = 35;  // Chance of attaching the building to another building

    constexpr int CAMP_RATIO = 90;   // Filling ratio of the play area
    constexpr int FILL_PERCENT = 55; // Percentage of the free area to fill with buildings
    constexpr int MAX_FAILS = 12;    // Max number of consecutive failures in pass 0 before considering the area saturated

    constexpr int MAX_PATH_OVERLAP = 14; // Max allowed path tiles covered by a single building footprint
    constexpr int CROSS_PERCENT   = 15;  // Max percentage of placed buildings allowed to overlap path tiles

    // Dimensions by role (including outer wall footprint): MESS, STORAGE, DORM
    constexpr int MIN_W[3] = { 10, 5, 6 };
    constexpr int MAX_W[3] = { 14, 8, 10 };
    constexpr int MIN_H[3] = { 7,  5, 5 };
    constexpr int MAX_H[3] = { 10, 7, 8 };

    // Average footprint area per building role (used for target surface estimation)
    constexpr int AVG_AREA[3] = { 100, 40, 52 };

    constexpr int CRACKED_PERCENT = 15; // Percentage of cracked floor tiles
}

void MapGenerator::buildHouseFloors(Map& map, std::mt19937& rng, const Rect& playZone) {
    enum Role { MESS = 0, STORAGE = 1, DORM = 2 };
    enum Cell { PATH = 1, RAIL = 2, HOUSE = 4 };

    // ---------- 1. Camp Zone Definition ----------
    // Randomly select rectangle bounds inside play zone margins.
    int areaW = playZone.w - 2 * EDGE_MARGIN;
    int areaH = playZone.h - 2 * EDGE_MARGIN;
    if (areaW < 16 || areaH < 16) return; // Zone too small to build

    int campW = std::min(areaW, std::max(16, areaW * CAMP_RATIO / 100));
    int campH = std::min(areaH, std::max(16, areaH * CAMP_RATIO / 100));

    std::uniform_int_distribution campXDist(0, areaW - campW);
    std::uniform_int_distribution campYDist(0, areaH - campH);
    int campX = playZone.x + EDGE_MARGIN + campXDist(rng);
    int campY = playZone.y + EDGE_MARGIN + campYDist(rng);

    // ---------- 2. Tile Content Classification ----------
    // cell[] matches play zone dimensions.
    std::vector<int> cell(playZone.w * playZone.h, 0);
    int freeCount = 0; // Unoccupied tile count within active camp bounds

    for (int y = playZone.y; y < playZone.y + playZone.h; ++y) {
        for (int x = playZone.x; x < playZone.x + playZone.w; ++x) {
            int index = (y - playZone.y) * playZone.w + (x - playZone.x);

            // Layer1 = guard paths, Layer2 = rails.
            if (!isTileEmpty(map, x, y, GameMap::ZIndex::Layer1)) cell[index] |= PATH;
            if (!isTileEmpty(map, x, y, GameMap::ZIndex::Layer2)) cell[index] |= RAIL;

            bool inCamp = x >= campX && x < campX + campW && y >= campY && y < campY + campH;
            if (inCamp && cell[index] == 0) ++freeCount;
        }
    }

    int pathCells = 0, railCells = 0;
    for (int c : cell) {
        if (c & PATH) ++pathCells;
        if (c & RAIL) ++railCells;
    }

    // ---------- 3. Building Role Queue Generation ----------
    // Sample building roles until target area surface is satisfied.
    // Pass 0 halts early when the zone becomes saturated.
    std::discrete_distribution rolePick({ 10.0, 25.0, 65.0 }); // MESS, STORAGE, DORM

    int targetArea = freeCount * FILL_PERCENT / 100;
    std::vector<int> roles;
    roles.push_back(MESS); // Ensure at least one mess hall is queued
    int area = AVG_AREA[MESS];
    while (area < targetArea) {
        int r = rolePick(rng);
        roles.push_back(r);
        area += AVG_AREA[r];
    }

    // Sort roles by footprint size descending (MESS = 0 < STORAGE = 1 < DORM = 2) for optimal packing
    std::stable_sort(roles.begin(), roles.end());

    // ---------- 4. Building Footprint Placement (Two-Pass Packing Strategy) ----------
    // Reference of already placed buildings used for alignment
    std::vector<int> houseX, houseY, houseW, houseH, houseRole;
    std::vector<int> failedRoles; // Unplaced buildings from pass 0 retried in pass 1
    int countByRole[3] = { 0, 0, 0 };

    std::uniform_int_distribution percent(0, 99);
    std::uniform_int_distribution coin(0, 1);

    int crossCount  = 0; // Count of placed buildings overlapping path tiles
    int crossBudget = 0; // Maximum permitted overlap budget

    for (int pass = 0; pass < 2; ++pass) {
        const std::vector<int> list = (pass == 0) ? roles : failedRoles;
        const bool allowPath = (pass == 1);

        if (pass == 1) {
            crossBudget = 1 + static_cast<int>(houseX.size()) * CROSS_PERCENT / 100;
        }

        int fails = 0; // Consecutive fail counter (pass 0 saturation check)

        for (int role : list) {
            if (pass == 0 && fails >= MAX_FAILS) break; // Camp area saturated

            std::uniform_int_distribution wDist(MIN_W[role], MAX_W[role]);
            std::uniform_int_distribution hDist(MIN_H[role], MAX_H[role]);
            bool placed = false;

            for (int attempt = 0; attempt < TRIES && !placed; ++attempt) {
                int w = wDist(rng);
                int h = hDist(rng);

                // a) Candidate position evaluation.
                // Pass 1 strictly attempts alignment adjacent to an existing building footprint:
                // Alignment justifies allowing path tile overlap.
                bool aligned = (pass == 1) || (!houseX.empty() && percent(rng) < ALIGN_CHANCE);
                if (aligned && houseX.empty()) break; // No reference building available to align against

                int x, y;
                if (aligned) {
                    // Place building snapped to the right or bottom side of an existing structure
                    std::uniform_int_distribution pickHouse(0, static_cast<int>(houseX.size()) - 1);
                    int k = pickHouse(rng);
                    if (coin(rng) == 0) {
                        x = houseX[k] + houseW[k] + GAP;
                        y = houseY[k];
                    } else {
                        x = houseX[k];
                        y = houseY[k] + houseH[k] + GAP;
                    }
                } else {
                    // Random placement within camp grid stepped by STEP multiple
                    int rangeX = (campW - w) / STEP;
                    int rangeY = (campH - h) / STEP;
                    if (rangeX < 0 || rangeY < 0) continue; // Footprint exceeds camp bounds
                    std::uniform_int_distribution xDist(0, rangeX);
                    std::uniform_int_distribution yDist(0, rangeY);
                    x = campX + STEP * xDist(rng);
                    y = campY + STEP * yDist(rng);
                }

                // b) Boundary constraint check (must remain strictly inside camp bounds)
                if (x < campX || y < campY) continue;
                if (x + w > campX + campW || y + h > campY + campH) continue;

                // c) Collision & clearance validation.
                int flags = 0;      // RAIL | HOUSE flags detected within expanded GAP bounding box
                int pathNear = 0;   // PATH flags within expanded PATH_GAP bounding box
                int pathTiles = 0;  // PATH tiles directly beneath candidate building footprint

                for (int yy = y - GAP; yy < y + h + GAP; ++yy) {
                    for (int xx = x - GAP; xx < x + w + GAP; ++xx) {
                        int c = cell[(yy - playZone.y) * playZone.w + (xx - playZone.x)];
                        flags |= (c & (RAIL | HOUSE));

                        bool inside = xx >= x && xx < x + w && yy >= y && yy < y + h;
                        bool near   = xx >= x - PATH_GAP && xx < x + w + PATH_GAP &&
                                      yy >= y - PATH_GAP && yy < y + h + PATH_GAP;

                        if (inside && (c & PATH)) ++pathTiles;
                        if (near && (c & PATH))   ++pathNear;
                    }
                }

                if (flags) continue;                                      // Must not overlap rails or adjacent building clearances
                if (!allowPath && pathNear > 0) continue;                 // Strictly avoid path proximity
                if (pathTiles > MAX_PATH_OVERLAP) continue;               // Enforce maximum path tile coverage limit
                if (pathTiles > 0 && crossCount >= crossBudget) continue; // Path crossing quota exceeded

                // d) Commit footprint occupancy
                for (int yy = y; yy < y + h; ++yy) {
                    for (int xx = x; xx < x + w; ++xx) {
                        cell[(yy - playZone.y) * playZone.w + (xx - playZone.x)] |= HOUSE;
                    }
                }

                houseX.push_back(x);
                houseY.push_back(y);
                houseW.push_back(w);
                houseH.push_back(h);
                houseRole.push_back(role);
                ++countByRole[role];
                if (pathTiles > 0) ++crossCount;
                placed = true;
            }

            if (placed) fails = 0; else ++fails;
            if (!placed && pass == 0 && failedRoles.size() < 6) failedRoles.push_back(role);
        }
    }

    // ---------- 5. Floor Tile Rendering ----------
    for (size_t k = 0; k < houseX.size(); ++k) {
        const int x = houseX[k];
        const int y = houseY[k];
        const int w = houseW[k];
        const int h = houseH[k];
        const bool isMess = (houseRole[k] == MESS);

        for (int yy = y; yy < y + h; ++yy) {
            for (int xx = x; xx < x + w; ++xx) {
                GameMap::Tile tile = isMess ? Tiles::FLOOR_STONE_CROSS : Tiles::FLOOR_STONE_TILES;
                if (!isMess && percent(rng) < CRACKED_PERCENT) tile = Tiles::FLOOR_STONE_CRACKED;
                addTile(map, xx, yy, tile, GameMap::ZIndex::Layer1);
            }
        }

        // Odd-dimensioned square buildings receive a decorative center pattern tile
        if (w == h && w % 2 == 1) {
            addTile(map, x + w / 2, y + h / 2, Tiles::FLOOR_STONE_PATTERN, GameMap::ZIndex::Layer1);
        }
    }
}