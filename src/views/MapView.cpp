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

float MapView::angleToDegrees(const AngleTile angle) {
    switch (angle) {
        case ANGLE_0:   return 0.f;
        case ANGLE_90:  return 90.f;
        case ANGLE_180: return 180.f;
        case ANGLE_270: return 270.f;
    }
    return 0.f;
}

void MapView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (const auto* resized = event.getIf<sf::Event::Resized>()) {
        const sf::Vector2f newSize(
            static_cast<float>(resized->size.x),
            static_cast<float>(resized->size.y)
        );
        view.setSize(newSize);
    }

    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mousePressed->button == sf::Mouse::Button::Left) {
            isDragging = true;
            oldMousePos = { mousePressed->position.x, mousePressed->position.y };
        }
    }
    else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mouseReleased->button == sf::Mouse::Button::Left) {
            isDragging = false;
        }
    }
    else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
        if (isDragging) {
            const sf::Vector2i newMousePos = { mouseMoved->position.x, mouseMoved->position.y };
            const sf::Vector2f delta = window.mapPixelToCoords(oldMousePos) - window.mapPixelToCoords(newMousePos);

            view.move(delta);
            oldMousePos = newMousePos;
        }
    }
}

void MapView::drawTile(sf::RenderWindow& window, const Tile& tile, const int i, const int j) const {
    const auto rect = sf::IntRect(
        { tile.col * TILE_SIZE, tile.row * TILE_SIZE },
        { TILE_SIZE, TILE_SIZE }
    );

    sf::Sprite sprite(texture, rect);
    sprite.setOrigin({TILE_SIZE / 2.f, TILE_SIZE / 2.f});
    sprite.setRotation(sf::degrees(angleToDegrees(tile.rotation)));
    sprite.setPosition({
        static_cast<float>(i * TILE_STEP) + TILE_SIZE / 2.f,
        static_cast<float>(j * TILE_STEP) + TILE_SIZE / 2.f
    });
    window.draw(sprite);
}

void MapView::drawMap(sf::RenderWindow& window, const TileMap& map) {
    if (view.getSize().x == 0.f && view.getSize().y == 0.f) {
        view = window.getDefaultView();

        if (!map.empty() && !map[0].empty()) {
            const float mapWidth  = static_cast<float>(map.size() * TILE_STEP);
            const float mapHeight = static_cast<float>(map[0].size() * TILE_STEP);
            const sf::Vector2f winSize = view.getSize();

            if (winSize.x > 0.f && winSize.y > 0.f) {
                const float scaleX = mapWidth / winSize.x;
                const float scaleY = mapHeight / winSize.y;
                const float scale = std::max(scaleX, scaleY) * 1.05f;
                if (scale > 1.f) {
                    view.setSize(winSize * scale);
                }
            }

            view.setCenter({ mapWidth / 2.f, mapHeight / 2.f });
        }
    }

    window.setView(view);


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