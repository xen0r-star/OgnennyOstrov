#include "models/map/generation/TerrainGenerator.h"

#include <algorithm>

#include "models/map/Tiles.h"
#include "models/map/generation/TileMapUtils.h"

namespace MapGen {

void generateBaseTerrain(TileMap& map, Rng& rng, const Rect& play) {
    const int x0 = play.x, y0 = play.y, x1 = play.right(), y1 = play.bottom();
    const int areaW = play.w;
    const int areaH = play.h;

    auto fillDirt = [&](const Rect& r) {
        for (int x = std::max(x0, r.x); x <= std::min(x1, r.right()); ++x) {
            for (int y = std::max(y0, r.y); y <= std::min(y1, r.bottom()); ++y) {
                addTile(map, x, y, Tiles::GROUND_DIRT, ZIndex::Layer0);
            }
        }
    };

    const int maxSize = std::max(3, std::min(areaW, areaH) / 4);
    std::uniform_int_distribution sizeDist(3, maxSize);
    const int w   = std::min(sizeDist(rng), areaW);
    const int h   = std::min(sizeDist(rng), areaH);
    const int gap = std::uniform_int_distribution(1, 2)(rng);

    const bool horizontal = std::uniform_int_distribution(0, 1)(rng) == 0;

    const int mainSize  = horizontal ? areaW : areaH;
    const int crossSize = horizontal ? areaH : areaW;
    const int mainBase  = horizontal ? x0 : y0;
    const int crossBase = horizontal ? y0 : x0;
    const int mainLen   = horizontal ? w : h;
    const int crossLen  = horizontal ? h : w;

    int count = std::uniform_int_distribution(2, 3)(rng);
    while (count > 1 && count * mainLen + (count - 1) * gap > mainSize) --count;

    const int total = count * mainLen + (count - 1) * gap;
    int pos            = mainBase  + std::uniform_int_distribution(0, mainSize  - total)(rng);
    const int crossPos = crossBase + std::uniform_int_distribution(0, crossSize - crossLen)(rng);

    for (int i = 0; i < count; ++i) {
        if (horizontal) {
            fillDirt(Rect{
                .x = pos, .y = crossPos,
                .w = w, .h = h
            });
        } else {
            fillDirt(Rect{
                .x = crossPos, .y = pos,
                .w = w, .h = h
            });
        }

        pos += mainLen + gap;
    }
}

} // namespace MapGen
