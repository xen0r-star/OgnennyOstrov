#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"

void MapGenerator::initMap(TileMap& map, const int height, const int width) {
    map.height = height;
    map.width = width;
    map.cells.assign(height, std::vector<MapCell>(width));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            addTile(map, x, y, Tiles::GROUND_GRASS, Map::ZIndex::Layer0);
        }
    }
}

void MapGenerator::addTile(TileMap& map, const int x, const int y, Map::Tile tile, const int zIndex) {
    if (x < 0 || y < 0 || x >= map.width || y >= map.height) return;

    auto& layers = map.cells[y][x].layers;

    std::vector<Map::Tile> kept;
    for (const Map::Tile & t : layers) {
        if (t.zIndex != zIndex) {
            kept.push_back(t);
        }
    }
    layers = kept;

    tile.zIndex = zIndex;
    layers.push_back(tile);
}

bool MapGenerator::isTileEmpty(const TileMap& map, const int x, const int y, const int zIndex) {
    if (x < 0 || y < 0 || x >= map.width || y >= map.height) return false;

    const auto& layers = map.cells[y][x].layers;
    for (const Map::Tile& t : layers) {
        if (t.zIndex == zIndex) {
            return false;
        }
    }
    return true;
}