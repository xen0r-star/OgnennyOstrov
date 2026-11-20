#ifndef OGNENNYOSTROV_TILE_H
#define OGNENNYOSTROV_TILE_H

#include <array>
#include <vector>

#include "core/Config.h"

namespace GameMap {
    struct Texture {
        int col = 0;
        int row = 0;

        explicit constexpr Texture(const int col = 0, const int row = 0) {
            this->col = col;
            this->row = row;
        }
    };

    namespace ZIndex {
        inline constexpr int Layer0 = 0;
        inline constexpr int Layer1 = 10;
        inline constexpr int Layer2 = 20;
        inline constexpr int Layer3 = 30;
    }

    struct Modifier {
        float offsetX = 0.f;
        float offsetY = 0.f;
        float scale   = 1.f;
        int rotation  = 0;
    };

    struct Tile {
        Texture texture = Texture();
        int zIndex = ZIndex::Layer0;
        Modifier modifier;

        explicit constexpr Tile(
            const Texture texture = Texture(),
            const int zIndex = ZIndex::Layer0,
            const Modifier modifier = Modifier()
        ) {
            this->texture = texture;
            this->zIndex = zIndex;
            this->modifier = modifier;
        }
    };
}

using CollisionMask = std::array<bool, MASK_SIZE * MASK_SIZE>;

struct MapCell {
    std::vector<GameMap::Tile> layers;

    CollisionMask blocksMovement{};   // {} = all false: nothing blocks
    CollisionMask blocksSight{};
    bool hides = false;               // bush
};

struct Map {
    int height = 0;
    int width = 0;

    std::vector<std::vector<MapCell>> cells = std::vector<std::vector<MapCell>>();
};

#endif // OGNENNYOSTROV_TILE_H
