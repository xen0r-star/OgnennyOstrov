#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

namespace {
    constexpr int CLEARANCE   = 1;   // Space between a dirt zone and a path / building / railway track
    constexpr int FIELD_GAP   = 1;   // Space between two dirt zone
    constexpr int EDGE_MARGIN = 1;   // Space between a dirt zone and the border map
    constexpr int BIG_PERCENT = 100; // Probability to make the change as large as possible

    // Possible sizes of dirt zones
    struct Size { int w, h; };
    constexpr Size SIZES[] = {
        { .w = 10, .h = 8 },
        { .w = 8, .h = 10 },
        { .w = 8, .h = 6 },
        { .w = 6, .h = 8 },
        { .w = 6, .h = 5 },
        { .w = 5, .h = 6 },
    };
}

void MapGenerator::buildDirtZones(Map& map, std::mt19937& rng, const Rect& playZone) {
    constexpr int sizeCount = std::size(SIZES);

    // ---------- 1. Occupied tiles ----------
    const int W = playZone.w;
    const int H = playZone.h;
    auto idx = [&](const int x, const int y) { return (y - playZone.y) * W + (x - playZone.x); };

    // obstacle[]: Path, House, Rail
    std::vector<char> obstacle(W * H, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (!isTileEmpty(map, x, y, GameMap::ZIndex::Layer1) ||
                !isTileEmpty(map, x, y, GameMap::ZIndex::Layer2) ||
                !isTileEmpty(map, x, y, GameMap::ZIndex::Layer3)) {
                obstacle[idx(x, y)] = 1;
            }
        }
    }

    // blocked[]: Area off-limits on the course (obstacle + surrounding path + edge + area of dirt already laid)
    std::vector<char> blocked(W * H, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            // Map border
            if (x < playZone.x + EDGE_MARGIN || x >= playZone.x + W - EDGE_MARGIN ||
                y < playZone.y + EDGE_MARGIN || y >= playZone.y + H - EDGE_MARGIN) {
                blocked[idx(x, y)] = 1;
            }
            if (!obstacle[idx(x, y)]) continue;

            // Path round the obstacle
            for (int dy = -CLEARANCE; dy <= CLEARANCE; ++dy) {
                for (int dx = -CLEARANCE; dx <= CLEARANCE; ++dx) {
                    const int nx = x + dx;
                    const int ny = y + dy;
                    if (nx < playZone.x || ny < playZone.y ||
                        nx >= playZone.x + W || ny >= playZone.y + H) continue;
                    blocked[idx(nx, ny)] = 1;
                }
            }
        }
    }

    // Is the entire dirt zone free?
    auto canPlace = [&](const int x, const int y, const int w, const int h) {
        if (x + w > playZone.x + W || y + h > playZone.y + H) return false;
        for (int yy = y; yy < y + h; ++yy) {
            for (int xx = x; xx < x + w; ++xx) {
                if (blocked[idx(xx, yy)]) return false;
            }
        }
        return true;
    };

    // ---------- 2. Dirt zone placement ----------
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (blocked[idx(x, y)]) continue;

            // Find all sizes that fit starting from this top-left corner
            Size fit[sizeCount];
            int fitCount = 0;
            int bestArea = 0;
            for (const Size& s : SIZES) {
                if (!canPlace(x, y, s.w, s.h)) continue;
                fit[fitCount++] = s;
                bestArea = std::max(bestArea, s.w * s.h);
            }
            if (fitCount == 0) continue;

            // Keep only the largest sizes (>= BIG_PERCENT % of the best area) and randomly pick one
            Size pool[sizeCount];
            int poolCount = 0;
            for (int i = 0; i < fitCount; ++i) {
                if (fit[i].w * fit[i].h * 100 >= bestArea * BIG_PERCENT) pool[poolCount++] = fit[i];
            }
            std::uniform_int_distribution pick(0, poolCount - 1);
            const auto [w, h] = pool[pick(rng)];

            // Place the dirt tiles (Layer0: replaces grass)
            for (int yy = y; yy < y + h; ++yy) {
                for (int xx = x; xx < x + w; ++xx) {
                    addTile(map, xx, yy, Tiles::GROUND_DIRT, GameMap::ZIndex::Layer0);
                }
            }

            // Mark the field and its surrounding margin as blocked
            for (int yy = y - FIELD_GAP; yy < y + h + FIELD_GAP; ++yy) {
                for (int xx = x - FIELD_GAP; xx < x + w + FIELD_GAP; ++xx) {
                    if (xx < playZone.x || yy < playZone.y ||
                        xx >= playZone.x + W || yy >= playZone.y + H) continue;
                    blocked[idx(xx, yy)] = 1;
                }
            }
        }
    }
}