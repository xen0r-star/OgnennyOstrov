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
    static TileMap generateMap(int height, int width, unsigned int seed);

private:
    static void initMap(TileMap& map, int height, int width);
    static void addTile(TileMap& map, int x, int y, Map::Tile tile, int zIndex);
    static bool isTileEmpty(const TileMap& map, int x, int y, int zIndex);

    static void buildMapBorder(TileMap& map, std::mt19937& rng, int margin);
    static void addGuardPaths(TileMap& map, std::mt19937& rng, const Rect& playZone);
    static void addRailPaths(TileMap& map, std::mt19937& rng, const Rect& playZone);
    static void buildHouseFloors(TileMap& map, std::mt19937& rng, const Rect& playZone);
    static void buildDirtZones(TileMap& map, std::mt19937& rng, const Rect& playZone);
    static void correctionsGuardPaths(TileMap& map, std::mt19937& rng, const Rect& playZone);
    static void buildHouseWalls(TileMap& map, std::mt19937& rng, const Rect& playZone);
    static void addHousePaths(TileMap& map, std::mt19937& rng, const Rect& playZone);
    static void addHouseDoors(TileMap& map, std::mt19937& rng, const Rect& playZone);
    static void addDecorations(TileMap& map, std::mt19937& rng, const Rect& playZone);
};

#endif // GOULAG_MAPGENERATOR_H
