#include "views/MapView.h"
#include <SFML/Graphics.hpp>
#include "core/Config.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace {
    constexpr int TEXTURE_TILE_SIZE = 384; // Texture tile size (px)
}

MapView::MapView() {
    if (!texture.loadFromFile("assets/textures/map.png")) {
        std::cerr << "Failed to load assets/textures/map.png\n";
        return;
    }

    texture.setSmooth(true);
}

MapView::~MapView() = default;

void MapView::drawTile(sf::RenderWindow& window, const GameMap::Tile& tile, const int x, const int y) const {
    const sf::IntRect rect(
        tile.texture.col * TEXTURE_TILE_SIZE, tile.texture.row * TEXTURE_TILE_SIZE,
        TEXTURE_TILE_SIZE, TEXTURE_TILE_SIZE
    );

    const auto &[offsetX, offsetY, scale, rotation] = tile.modifier;

    sf::Sprite sprite(texture, rect);
    sprite.setOrigin(TILE_SIZE / 2.f, TILE_SIZE / 2.f);
    sprite.setRotation(static_cast<float>(rotation));

    sprite.setOrigin(TEXTURE_TILE_SIZE / 2.f, TEXTURE_TILE_SIZE / 2.f);
    constexpr float k = static_cast<float>(TILE_SIZE) / TEXTURE_TILE_SIZE;  // 0.5
    sprite.setScale(scale * k, scale * k);

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

    const sf::View view = window.getView();
    const sf::Vector2f center = view.getCenter();
    const sf::Vector2f half = view.getSize() / 2.f;
    constexpr int margin = 2;

    const int minX = std::max(0, static_cast<int>(std::floor((center.x - half.x) / TILE_STEP)) - margin);
    const int maxX = std::min(map.width - 1, static_cast<int>(std::floor((center.x + half.x) / TILE_STEP)) + margin);
    const int minY = std::max(0, static_cast<int>(std::floor((center.y - half.y) / TILE_STEP)) - margin);
    const int maxY = std::min(map.height - 1, static_cast<int>(std::floor((center.y + half.y) / TILE_STEP)) + margin);

    for (const int z : zPasses) {
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                for (const GameMap::Tile& tile : map.cells[y][x].layers) {
                    if (tile.zIndex == z) {
                        drawTile(window, tile, x, y);
                    }
                }
            }
        }
    }
}