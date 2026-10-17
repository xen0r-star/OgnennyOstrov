#include "models/map/MapGenerator.h"

Map MapGenerator::generateMap(const int height, const int width, const unsigned int seed) {
    Map map;
    std::mt19937 rng(seed);

    constexpr int margin = 5;
    constexpr int border = margin + 1;

    initMap(map, height + 2 * border, width + 2 * border);
    buildMapBorder(map, rng, margin);

    const Rect playZone {
        .x = border, .y = border,
        .w = width,  .h = height
    };

    addGuardPaths(map, rng, playZone);
    addRailPaths(map, rng, playZone);
    buildHouseFloors(map, rng, playZone);
    buildDirtZones(map, rng, playZone);

    correctionsGuardPaths(map, rng, playZone);

    buildHouseWalls(map, rng, playZone);
    addHousePaths(map, rng, playZone);
    addHouseDoors(map, rng, playZone);
    addHouseDecorations(map, rng, playZone);

    addDecorations(map, rng, playZone);

    return map;
}
