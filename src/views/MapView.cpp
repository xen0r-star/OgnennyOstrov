#include "views/MapView.h"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>
#include "core/Config.h"
#include "models/map/Tiles.h"

namespace {
    constexpr int TEXTURE_TILE_SIZE = 384; // Texture tile size (px)

    bool isRail(const GameMap::Tile& tile) {
        const auto same = [&](const GameMap::Tile& ref) {
            return tile.texture.col == ref.texture.col && tile.texture.row == ref.texture.row;
        };
        return same(Tiles::RAIL_STRAIGHT) || same(Tiles::RAIL_CURVE) || same(Tiles::RAIL_CROSSROADS);
    }

    bool isBush(const GameMap::Tile& tile) {
        return tile.texture.col == Tiles::PROP_BUSH.texture.col &&
               tile.texture.row == Tiles::PROP_BUSH.texture.row;
    }
}

MapView::MapView() {
    if (!texture.loadFromFile("assets/textures/map.png")) {
        std::cerr << "Failed to load assets/textures/map.png\n";
        return;
    }

    texture.setSmooth(true);
}

MapView::~MapView() = default;

// Draws the map below the player (ground, paths, floors, tufts, bushes, rails)
void MapView::drawGround(sf::RenderWindow& window, const Map& map) const {
    drawPass(window, map, GameMap::ZIndex::Layer0, Filter::All);
    drawPass(window, map, GameMap::ZIndex::Layer1, Filter::NoBush);   // paths, floors, tufts, campfire
    drawPass(window, map, GameMap::ZIndex::Layer1, Filter::BushOnly); // bushes above the tufts
    drawPass(window, map, GameMap::ZIndex::Layer2, Filter::RailOnly); // rails stay under the player
}

// Draws the map above the player (walls, trees, doors, furniture)
void MapView::drawOverlay(sf::RenderWindow& window, const Map& map) const {
    drawPass(window, map, GameMap::ZIndex::Layer2, Filter::NoRail);   // walls, trees, doors, furniture
    drawPass(window, map, GameMap::ZIndex::Layer3, Filter::All);
}


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

void MapView::drawPass(sf::RenderWindow& window, const Map& map, const int z, const Filter filter) const {
    const sf::View view = window.getView();
    const sf::Vector2f center = view.getCenter();
    const sf::Vector2f half = view.getSize() / 2.f;
    constexpr int margin = 2;

    const int minX = std::max(0, static_cast<int>(std::floor((center.x - half.x) / TILE_STEP)) - margin);
    const int maxX = std::min(map.width - 1, static_cast<int>(std::floor((center.x + half.x) / TILE_STEP)) + margin);
    const int minY = std::max(0, static_cast<int>(std::floor((center.y - half.y) / TILE_STEP)) - margin);
    const int maxY = std::min(map.height - 1, static_cast<int>(std::floor((center.y + half.y) / TILE_STEP)) + margin);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            for (const GameMap::Tile& tile : map.cells[y][x].layers) {
                if (tile.zIndex != z) continue;
                const bool rail = isRail(tile);
                const bool bush = isBush(tile);
                if (filter == Filter::RailOnly && !rail) continue;
                if (filter == Filter::NoRail && rail) continue;
                if (filter == Filter::BushOnly && !bush) continue;
                if (filter == Filter::NoBush && bush) continue;

                drawTile(window, tile, x, y);
            }
        }
    }
}
