#include "models/map/generation/BorderGenerator.h"

#include <functional>

#include "models/map/Tiles.h"
#include "models/map/generation/TileHelpers.h"
#include "models/map/generation/TileMapUtils.h"

namespace MapGen {

void buildMapBorder(TileMap& map, Rng& rng, const int margin) {
    const int width  = mapWidth(map);
    const int height = mapHeight(map);

    const int x0 = margin,             y0 = margin;
    const int x1 = width - 1 - margin, y1 = height - 1 - margin;

    constexpr int WATCHTOWER_STRIDE = 6;
    const std::function isCorner = [&](const int x, const int y) {
        return (x == x0 || x == x1) && (y == y0 || y == y1);
    };


    // TOP / BOTTOM walls
    for (int x = x0; x <= x1; ++x) {
        for (const int y : {y0, y1}) {
            const bool tower = isCorner(x, y) || (x - x0) % WATCHTOWER_STRIDE == 0;

            Tile t = tower ? Tiles::WATCHTOWER : Tiles::WALL_BARBED_WIRE;
            if (!tower) t.rotation = ANGLE_0;

            addTile(map, x, y, t, ZIndex::Layer1);
        }
    }
    // LEFT / RIGHT walls
    for (int y = y0 + 1; y < y1; ++y) {
        for (const int x : {x0, x1}) {
            const bool tower = (y - y0) % WATCHTOWER_STRIDE == 0;

            Tile t = tower ? Tiles::WATCHTOWER : Tiles::WALL_BARBED_WIRE;
            if (!tower) t.rotation = ANGLE_90;

            addTile(map, x, y, t, ZIndex::Layer1);
        }
    }


    // Vegetation in the margin
    std::uniform_real_distribution chance(0.f, 1.f);

    constexpr Tile vegetationProps[] = {
        Tiles::PROP_TREE,
        Tiles::PROP_BUSH,
        Tiles::PROP_GRASS_TUFT
    };

    std::discrete_distribution propPick({4.0, 1.0, 3.0});

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            const bool inMargin = x < margin || x >= width - margin ||
                                  y < margin || y >= height - margin;
            if (!inMargin) continue;

            if (chance(rng) < 0.4f) {
                Tile t = vegetationProps[propPick(rng)];
                t.rotation = randomAngle(rng);
                addTile(map, x, y, t, ZIndex::Layer2);
            }
        }
    }
}

} // namespace MapGen
