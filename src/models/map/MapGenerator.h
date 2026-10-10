#ifndef GOULAG_MAPGENERATOR_H
#define GOULAG_MAPGENERATOR_H

#include "models/map/Tile.h"

class MapGenerator {
public:
    static TileMap generateMap(int height, int width, unsigned int seed);
};

#endif // GOULAG_MAPGENERATOR_H
