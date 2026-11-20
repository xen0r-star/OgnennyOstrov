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

void MapView::drawTile(sf::RenderWindow& window, const GameMap::Tile& tile, const int x, const int y) const {
    const sf::IntRect rect(
        tile.texture.col * TILE_SIZE, tile.texture.row * TILE_SIZE,
        TILE_SIZE, TILE_SIZE
    );

    const auto &[offsetX, offsetY, scale, rotation] = tile.modifier;

    sf::Sprite sprite(texture, rect);
    sprite.setOrigin(TILE_SIZE / 2.f, TILE_SIZE / 2.f);
    sprite.setRotation(static_cast<float>(rotation));
    sprite.setScale(scale, scale);
    sprite.setPosition(
        static_cast<float>(x * TILE_STEP) + TILE_SIZE / 2.f + offsetX,
        static_cast<float>(y * TILE_STEP) + TILE_SIZE / 2.f + offsetY
    );
    window.draw(sprite);
}

void MapView::drawMap(sf::RenderWindow& window, const Map& map) const {
    static constexpr int zPasses[] = {
        GameMap::ZIndex::Layer0,
        GameMap::ZIndex::Layer1,
        GameMap::ZIndex::Layer2,
        GameMap::ZIndex::Layer3
    };

    for (const int z : zPasses) {
        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) {
                for (const GameMap::Tile& tile : map.cells[y][x].layers) {
                    if (tile.zIndex == z) {
                        drawTile(window, tile, x, y);
                    }
                }
            }
        }
    }
}