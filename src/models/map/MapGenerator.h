#ifndef GOULAG_MAPGENERATOR_H
#define GOULAG_MAPGENERATOR_H

#include "models/map/Tile.h"
#include <random>

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

class MapGenerator {
public:
    static Map generateMap(int height, int width, unsigned int seed);

private:
    static void initMap(Map& map, int height, int width);
    static void addTile(Map& map, int x, int y, GameMap::Tile tile, int zIndex);
    static bool isTileEmpty(const Map& map, int x, int y, int zIndex);

    static void buildMapBorder(Map& map, std::mt19937& rng, int margin);
    static void addGuardPaths(Map& map, std::mt19937& rng, const Rect& playZone);
    static void addRailPaths(Map& map, std::mt19937& rng, const Rect& playZone);
    static void buildHouseFloors(Map& map, std::mt19937& rng, const Rect& playZone);
    static void buildDirtZones(Map& map, std::mt19937& rng, const Rect& playZone);
    static void correctionsGuardPaths(Map& map, std::mt19937& rng, const Rect& playZone);
    static void buildHouseWalls(Map& map, std::mt19937& rng, const Rect& playZone);
    static void addHousePaths(Map& map, std::mt19937& rng, const Rect& playZone);
    static void addHouseDoors(Map& map, std::mt19937& rng, const Rect& playZone);
    static void addHouseDecorations(Map& map, std::mt19937& rng, const Rect& playZone);
    static void addDecorations(Map& map, std::mt19937& rng, const Rect& playZone);
};

#endif // GOULAG_MAPGENERATOR_H
