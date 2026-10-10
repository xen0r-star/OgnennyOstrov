#include "models/map/generation/HouseLayout.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>

#include "models/map/Tiles.h"
#include "models/map/generation/TileMapUtils.h"

namespace MapGen {

std::vector<House> generateHouses(const TileMap& map, Rng& rng, const Rect& play) {
    const int x0 = play.x, y0 = play.y, x1 = play.right(), y1 = play.bottom();
    const int areaW = play.w;
    const int areaH = play.h;
    if (areaW < 5 || areaH < 5) return {};

    constexpr int spacing   = 2;
    constexpr int maxRows   = 4;
    constexpr int maxHouses = 10;

    // Centre de gravité de la terre battue : les bâtiments se regroupent autour
    long sx = 0, sy = 0, n = 0;
    for (int x = x0; x <= x1; ++x)
        for (int y = y0; y <= y1; ++y)
            if (groundAt(map, x, y) == DIRT) { sx += x; sy += y; ++n; }
    const int cx = n ? static_cast<int>(sx / n) : (x0 + x1) / 2;
    const int cy = n ? static_cast<int>(sy / n) : (y0 + y1) / 2;

    struct Size { int w, h; };
    std::vector<House> best;

    for (int attempt = 0; attempt < 6 && best.size() < 3; ++attempt) {
        const int dirtGap = (attempt < 4) ? 1 : 0;
        const int shrink  = std::min(attempt, 3);

        std::uniform_int_distribution wDist(std::max(5, 6 - shrink), std::max(5, 9 - 2 * shrink));
        std::uniform_int_distribution hDist(5, std::max(5, 8 - 2 * shrink));
        Size sizes[3];
        for (auto& s : sizes) {
            s.w = std::min(wDist(rng), areaW);
            s.h = std::min(hDist(rng), areaH);
        }

        auto touchesDirt = [&](int x, int y, int w, int h) {
            for (int i = std::max(x0, x - dirtGap); i <= std::min(x1, x + w - 1 + dirtGap); ++i)
                for (int j = std::max(y0, y - dirtGap); j <= std::min(y1, y + h - 1 + dirtGap); ++j)
                    if (groundAt(map, i, j) == DIRT) return true;
            return false;
        };

        std::array<int, 3> order{0, 1, 2};
        std::shuffle(order.begin(), order.end(), rng);
        std::vector<int> rowTypes;
        int blockH = 0;
        for (int r = 0; r < maxRows; ++r) {
            const int t = order[r % 3];
            const int add = sizes[t].h + (rowTypes.empty() ? 0 : spacing);
            if (blockH + add > areaH) break;
            blockH += add;
            rowTypes.push_back(t);
        }
        if (rowTypes.empty()) continue;

        std::vector<House> attemptBest;
        int bestDist = INT_MAX;
        for (int by = y0; by <= y1 - blockH + 1; ++by) {
            std::vector<House> cand;
            int y = by;
            for (int t : rowTypes) {
                const Size s = sizes[t];
                const int cols   = std::max(1, (areaW + spacing) / (s.w + spacing));
                const int totalW = cols * s.w + (cols - 1) * spacing;
                const int offX   = x0 + (areaW - totalW) / 2;
                for (int c = 0; c < cols; ++c) {
                    House h;
                    h.x = offX + c * (s.w + spacing);
                    h.y = y;
                    h.w = s.w;
                    h.h = s.h;
                    h.type = t;
                    if (h.right() > x1 || h.bottom() > y1) continue;
                    if (touchesDirt(h.x, h.y, h.w, h.h)) continue;
                    cand.push_back(h);
                }
                y += s.h + spacing;
            }
            const int dist = std::abs(by + blockH / 2 - cy);
            const int cnt  = std::min<int>(cand.size(), maxHouses);
            const int bcnt = std::min<int>(attemptBest.size(), maxHouses);
            if (cnt > bcnt || (cnt == bcnt && dist < bestDist)) {
                attemptBest = std::move(cand);
                bestDist = dist;
            }
        }
        if (attemptBest.size() > best.size()) best = std::move(attemptBest);
    }

    if (static_cast<int>(best.size()) > maxHouses) {
        auto dist2 = [&](const House& h) {
            const int dx = h.x + h.w / 2 - cx, dy = h.y + h.h / 2 - cy;
            return dx * dx + dy * dy;
        };
        std::sort(best.begin(), best.end(),
                  [&](const House& a, const House& b) { return dist2(a) < dist2(b); });
        best.resize(maxHouses);
    }
    return best;
}

void carveHouseFloors(TileMap& map, Rng& rng, const std::vector<House>& houses) {
    std::uniform_real_distribution chance(0.f, 1.f);
    for (const House& h : houses) {
        for (int x = h.x; x <= h.right(); ++x) {
            for (int y = h.y; y <= h.bottom(); ++y) {
                Tile floor = (chance(rng) < 0.15f) ? Tiles::FLOOR_STONE_CRACKED
                                                   : Tiles::FLOOR_STONE_TILES;
                addTile(map, x, y, floor, ZIndex::Layer0);
            }
        }
    }
}

void connectAdjacentHouses(TileMap& map, Rng& rng, const std::vector<House>& houses) {
    std::uniform_real_distribution chance(0.f, 1.f);

    // Un "opening" = sol de pierre lisse (FLOOR_STONE_PLAIN), pas de mur posé dessus.
    auto openAt = [&](int x, int y) {
        addTile(map, x, y, Tiles::FLOOR_STONE_PLAIN, ZIndex::Layer0);
    };

    for (size_t i = 0; i < houses.size(); ++i) {
        for (size_t j = i + 1; j < houses.size(); ++j) {
            const House& a = houses[i];
            const House& b = houses[j];

            if (b.x == a.right() + 2) {
                int start = std::max(a.y + 1, b.y + 1);
                int end   = std::min(a.bottom() - 1, b.bottom() - 1);
                if (end >= start && chance(rng) < 0.5f) {
                    std::uniform_int_distribution pick(start, end);
                    int y = pick(rng);
                    for (int x : {a.right(), a.right() + 1, b.x}) openAt(x, y);
                }
            }
            if (b.y == a.bottom() + 2) {
                int start = std::max(a.x + 1, b.x + 1);
                int end   = std::min(a.right() - 1, b.right() - 1);
                if (end >= start && chance(rng) < 0.5f) {
                    std::uniform_int_distribution pick(start, end);
                    int x = pick(rng);
                    for (int y : {a.bottom(), a.bottom() + 1, b.y}) openAt(x, y);
                }
            }
        }
    }
}

} // namespace MapGen
