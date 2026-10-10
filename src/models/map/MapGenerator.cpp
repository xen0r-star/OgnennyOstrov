#include "models/map/MapGenerator.h"

#include "models/map/generation/MapTypes.h"
#include "models/map/generation/TileMapUtils.h"
#include "models/map/generation/BorderGenerator.h"
#include "models/map/generation/TerrainGenerator.h"
#include "models/map/generation/HouseLayout.h"
#include "models/map/generation/HouseStructure.h"
#include "models/map/generation/PathGenerator.h"
#include "models/map/generation/RailGenerator.h"
#include "models/map/generation/DecorationGenerator.h"

TileMap MapGenerator::generateMap(const int height, const int width, const unsigned int seed) {
    using namespace MapGen;

    constexpr int margin = 5;
    constexpr int border = margin + 1;

    Rng rng(seed);
    TileMap map = initMap(width + 2 * border, height + 2 * border);

    const Rect play {
        .x = border, .y = border,
        .w = width, .h = height
    };

    buildMapBorder(map, rng, margin);
    generateBaseTerrain(map, rng, play);

    // const std::vector<House> houses = generateHouses(map, rng, play);
    // carveHouseFloors(map, rng, houses);
    // connectAdjacentHouses(map, rng, houses);
    // buildWalls(map, rng, houses);
    // placeDoors(map, rng, houses);

    // generateHousePaths(map);
    // generatePromenadePaths(map, rng, play);
    // removePathBlocks(map);
    // autotilePathsUnified(map);

    // generateRails(map, rng, play);
    // placeDecorations(map, rng, play, houses);

    sortLayers(map);
    return map;
}
