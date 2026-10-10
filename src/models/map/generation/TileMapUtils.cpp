#include "models/map/generation/TileMapUtils.h"

#include <algorithm>
#include <initializer_list>

#include "models/map/Tiles.h"
#include "models/map/generation/TileHelpers.h"

namespace MapGen {

// ---------- Construction / accès ----------

TileMap initMap(const int width, const int height) {
    TileMap map(width, std::vector<MapCell>(height));
    for (int x = 0; x < width; ++x)
        for (int y = 0; y < height; ++y)
            addTile(map, x, y, Tiles::GROUND_GRASS, ZIndex::Layer0);
    return map;
}

int mapWidth(const TileMap& map)  { return static_cast<int>(map.size()); }
int mapHeight(const TileMap& map) { return map.empty() ? 0 : static_cast<int>(map[0].size()); }

bool inBounds(const TileMap& map, const int x, const int y) {
    return x >= 0 && x < mapWidth(map) && y >= 0 && y < mapHeight(map);
}

void addTile(TileMap& map, const int x, const int y, const Tile tile, const int z) {
    auto& layers = map[x][y].layers;
    if (z != ZIndex::Layer2) {
        layers.erase(std::remove_if(layers.begin(), layers.end(),
                     [z](const LayerTile& l) { return l.z == z; }),
                     layers.end());
    }
    layers.push_back({.tile = tile, .z = z});
}

void removeLayer(TileMap& map, const int x, const int y, const int z) {
    auto& layers = map[x][y].layers;
    layers.erase(std::remove_if(layers.begin(), layers.end(),
                 [z](const LayerTile& l) { return l.z == z; }),
                 layers.end());
}

void sortLayers(TileMap& map) {
    for (auto& column : map)
        for (auto& cell : column)
            std::stable_sort(cell.layers.begin(), cell.layers.end(),
                             [](const LayerTile& a, const LayerTile& b) { return a.z < b.z; });
}

const LayerTile* findLayer(const TileMap& map, const int x, const int y, const int z) {
    if (!inBounds(map, x, y)) return nullptr;
    for (const auto& layer : map[x][y].layers)
        if (layer.z == z) return &layer;
    return nullptr;
}

bool hasLayer(const TileMap& map, const int x, const int y, const int z) {
    return findLayer(map, x, y, z) != nullptr;
}

bool hasTile(const TileMap& map, const int x, const int y, const int z, const Tile& tile) {
    const LayerTile* layer = findLayer(map, x, y, z);
    return layer && sameTile(layer->tile, tile);
}

// ---------- Questions sémantiques ----------
bool isGrass(const TileMap& map, const int x, const int y) {
    return !hasLayer(map, x, y, ZIndex::Layer1) &&
           hasTile(map, x, y, ZIndex::Layer0, Tiles::GROUND_GRASS);
}

bool isDirt(const TileMap& map, const int x, const int y) {
    return !hasLayer(map, x, y, ZIndex::Layer1) &&
           hasTile(map, x, y, ZIndex::Layer0, Tiles::GROUND_DIRT);
}

bool isOpening(const TileMap& map, const int x, const int y) {
    return hasTile(map, x, y, ZIndex::Layer0, Tiles::FLOOR_STONE_PLAIN);
}
} // namespace MapGen
