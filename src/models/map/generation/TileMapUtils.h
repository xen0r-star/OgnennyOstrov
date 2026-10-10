#ifndef GOULAG_TILEMAPUTILS_H
#define GOULAG_TILEMAPUTILS_H

#include <algorithm>
#include <initializer_list>
#include <vector>

#include "models/map/Tile.h"
#include "models/map/generation/MapTypes.h"

namespace MapGen {

TileMap initMap(int width, int height);
int  mapWidth(const TileMap& map);
int  mapHeight(const TileMap& map);
bool inBounds(const TileMap& map, int x, int y);

void addTile(TileMap& map, int x, int y, Tile tile, int z);
void removeLayer(TileMap& map, int x, int y, int z);
void sortLayers(TileMap& map);

const LayerTile* findLayer(const TileMap& map, int x, int y, int z);
bool hasLayer(const TileMap& map, int x, int y, int z);
bool hasTile(const TileMap& map, int x, int y, int z, const Tile& tile);

TileType typeAt(const TileMap& map, int x, int y, int z);
inline TileType groundAt(const TileMap& map, int x, int y)    { return typeAt(map, x, y, ZIndex::Layer0); } // GRASS, DIRT, HOUSE, OPENING
inline TileType structureAt(const TileMap& map, int x, int y) { return typeAt(map, x, y, ZIndex::Layer1); } // WALL, DOOR, PATH, RAIL
inline bool hasStructure(const TileMap& map, int x, int y)    { return hasLayer(map, x, y, ZIndex::Layer1); }
inline bool hasDecoration(const TileMap& map, int x, int y)   { return hasLayer(map, x, y, ZIndex::Layer2); }

inline bool oneOf(const TileType t, const std::initializer_list<TileType> types) {
    return std::find(types.begin(), types.end(), t) != types.end();
}

bool isOpenGround(const TileMap& map, int x, int y);

std::vector<Point> findStructures(const TileMap& map, TileType type);

}

#endif // GOULAG_TILEMAPUTILS_H