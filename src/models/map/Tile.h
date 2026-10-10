#ifndef GOULAG_TILE_H
#define GOULAG_TILE_H

#include <vector>

enum Angle {
    ANGLE_0,
    ANGLE_90,
    ANGLE_180,
    ANGLE_270
};

enum TileType {
    NONE,
    GRASS,
    DIRT,
    HOUSE,
    OPENING,
    WALL,
    DOOR,
    PATH,
    RAIL,
    DECORATION
};

struct Tile {
    int col = 0;
    int row = 0;
    Angle rotation = ANGLE_0;
    TileType type = NONE;

    explicit constexpr Tile(
        const int col = 0, const int row = 0,
        const TileType type = NONE, const Angle rotation = ANGLE_0
    ) {
        this->col = col;
        this->row = row;
        this->type = type;
        this->rotation = rotation;
    }
};

namespace ZIndex {
    inline constexpr int Layer0 = 0;
    inline constexpr int Layer1 = 10;
    inline constexpr int Layer2 = 20;
    inline constexpr int Layer3 = 30;
}

struct TileVariation {
    float offsetX = 0.f;
    float offsetY = 0.f;
    float scale   = 1.f;
};

struct LayerTile {
    Tile tile;
    int z = ZIndex::Layer0;
    TileVariation var;
};


struct MapCell {
    std::vector<LayerTile> layers;
};

using TileMap = std::vector<std::vector<MapCell>>;

#endif // GOULAG_TILE_H
