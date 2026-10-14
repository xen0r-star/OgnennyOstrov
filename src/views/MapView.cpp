#include "views/MapView.h"
#include <SFML/Graphics.hpp>
#include "core/Config.h"
#include <iostream>



MapView::MapView() {
    if (!texture.loadFromFile("assets/textures/map.png")) {
        std::cerr << "Failed to load assets/textures/map.png\n";
        return;
    }
}

MapView::~MapView() = default;

float MapView::angleToDegrees(const Angle angle) {
    switch (angle) {
        case ANGLE_0:   return 0.f;
        case ANGLE_90:  return 90.f;
        case ANGLE_180: return 180.f;
        case ANGLE_270: return 270.f;
    }
    return 0.f;
}

void MapView::drawTile(sf::RenderWindow& window, const Tile& tile, const int i, const int j) const {
    const sf::IntRect rect(
        tile.col * TILE_SIZE, tile.row * TILE_SIZE,
        TILE_SIZE, TILE_SIZE
    );

    sf::Sprite sprite(texture, rect);
    sprite.setOrigin(TILE_SIZE / 2.f, TILE_SIZE / 2.f);
    sprite.setRotation(angleToDegrees(tile.rotation));
    sprite.setPosition(
        static_cast<float>(i * TILE_STEP) + TILE_SIZE / 2.f,
        static_cast<float>(j * TILE_STEP) + TILE_SIZE / 2.f
    );
    window.draw(sprite);
}

void MapView::drawMap(sf::RenderWindow& window, const TileMap& map) {
    static constexpr int zPasses[] = { ZIndex::Ground, ZIndex::Structure, ZIndex::Decoration };

    for (const int z : zPasses) {
        for (int i = 0; i < static_cast<int>(map.size()); ++i) {
            for (int j = 0; j < static_cast<int>(map[i].size()); ++j) {
                for (const auto& layer : map[i][j].layers) {
                    if (layer.z == z) {
                        drawTile(window, layer.tile, i, j);
                    }
                }
            }
        }
    }
}