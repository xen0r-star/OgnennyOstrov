#include "views/MapView.h"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <iostream>

constexpr int TILE_SIZE = 128;
constexpr int TILE_STEP = TILE_SIZE - 8;

MapView::MapView() {
    if (!texture.loadFromFile("assets/textures/map.png")) {
        std::cerr << "Failed to load assets/textures/map.png\n";
        return;
    }
}

MapView::~MapView() = default;

void MapView::drawTile(sf::RenderWindow& window, const Map::Tile& tile, const int x, const int y) const {
    const sf::IntRect rect(
        tile.texture.col * TILE_SIZE, tile.texture.row * TILE_SIZE,
        TILE_SIZE, TILE_SIZE
    );

    const Map::Modifier& m = tile.modifier;

    sf::Sprite sprite(texture, rect);
    sprite.setOrigin(TILE_SIZE / 2.f, TILE_SIZE / 2.f);
    sprite.setRotation(static_cast<float>(m.rotation));
    sprite.setScale(m.scale, m.scale);
    sprite.setPosition(
        static_cast<float>(x * TILE_STEP) + TILE_SIZE / 2.f + m.offsetX,
        static_cast<float>(y * TILE_STEP) + TILE_SIZE / 2.f + m.offsetY
    );
    window.draw(sprite);
}

void MapView::drawMap(sf::RenderWindow& window, const TileMap& map) {
    // For testing
    if (!viewInitialized) {
        viewInitialized = true;
        view = window.getDefaultView();

        if (map.width > 0 && map.height > 0) {
            const float mapWidth  = static_cast<float>(map.width * TILE_STEP);
            const float mapHeight = static_cast<float>(map.height * TILE_STEP);
            const sf::Vector2f winSize = view.getSize();

            if (winSize.x > 0.f && winSize.y > 0.f) {
                const float scaleX = mapWidth / winSize.x;
                const float scaleY = mapHeight / winSize.y;
                const float scale = std::max(scaleX, scaleY) * 1.05f;
                if (scale > 1.f) {
                    view.setSize(winSize * scale);
                }
            }

            view.setCenter(mapWidth / 2.f, mapHeight / 2.f);
        }
    }
    window.setView(view);


    static constexpr int zPasses[] = {
        Map::ZIndex::Layer0,
        Map::ZIndex::Layer1,
        Map::ZIndex::Layer2,
        Map::ZIndex::Layer3
    };

    for (const int z : zPasses) {
        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) {
                for (const Map::Tile& tile : map.cells[y][x].layers) {
                    if (tile.zIndex == z) {
                        drawTile(window, tile, x, y);
                    }
                }
            }
        }
    }
}