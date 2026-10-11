#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"

#include <functional>

namespace {
    constexpr int WATCHTOWER_STRIDE = 6; // distance entre deux tours de garde
}

void MapGenerator::buildMapBorder(TileMap& map, std::mt19937& rng, const int margin) {
    const int x0 = margin;
    const int y0 = margin;
    const int x1 = map.width - 1 - margin;
    const int y1 = map.height - 1 - margin;

    const std::function isCorner = [&](const int x, const int y) {
        return (x == x0 || x == x1) && (y == y0 || y == y1);
    };


    // TOP / BOTTOM walls
    for (int x = x0; x <= x1; ++x) {
        for (const int y : {y0, y1}) {
            const bool tower = isCorner(x, y) || (x - x0) % WATCHTOWER_STRIDE == 0;

            Map::Tile t = tower ? Tiles::WATCHTOWER : Tiles::WALL_BARBED_WIRE;
            if (!tower) t.modifier.rotation = 0;

            addTile(map, x, y, t, Map::ZIndex::Layer2);
        }
    }
    // LEFT / RIGHT walls
    for (int y = y0 + 1; y < y1; ++y) {
        for (const int x : {x0, x1}) {
            const bool tower = (y - y0) % WATCHTOWER_STRIDE == 0;

            Map::Tile t = tower ? Tiles::WATCHTOWER : Tiles::WALL_BARBED_WIRE;
            if (!tower) t.modifier.rotation = 90;

            addTile(map, x, y, t, Map::ZIndex::Layer2);
        }
    }


    // Vegetation in the margin
    std::uniform_real_distribution chance(0.f, 1.f);
    std::uniform_int_distribution angleDist(0, 3);
    std::uniform_real_distribution offsetDist(-20.f, 20.f);
    std::uniform_real_distribution scaleDist(0.8f, 1.2f);

    constexpr Map::Tile vegetationProps[] = {
        Tiles::PROP_TREE,
        Tiles::PROP_BUSH,
        Tiles::PROP_GRASS_TUFT
    };

    std::discrete_distribution propPick({4.0, 1.0, 3.0});

    for (int x = 0; x < map.width; ++x) {
        for (int y = 0; y < map.height; ++y) {
            const bool inMargin =
                x < margin ||
                x >= map.width - margin ||
                y < margin ||
                y >= map.height - margin;

            if (!inMargin) continue;

            if (chance(rng) < 0.4f) {
                Map::Tile tile = vegetationProps[propPick(rng)];

                tile.modifier.rotation = angleDist(rng) * 90;
                tile.modifier.offsetX  = offsetDist(rng);
                tile.modifier.offsetY  = offsetDist(rng);
                tile.modifier.scale    = scaleDist(rng);

                addTile(map, x, y, tile, Map::ZIndex::Layer1);
            }
        }
    }
}
