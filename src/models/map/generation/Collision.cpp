#include "core/Config.h"
#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"

#include <nlohmann/json.hpp>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <utility>

namespace {
    constexpr float BUSH_HIDE_RATIO = 0.8f; // Hiding circle radius

    using json = nlohmann::json;

    // (col, row) of a tile in map.png -> its hitbox shapes, with points between 0 and 1
    using HitboxLibrary = std::map<std::pair<int, int>, std::vector<Hitbox::Polygon>>;

    constexpr auto HITBOX_FILE = "assets/hitboxes/tiles.json";
    constexpr float PI = 3.14159265f;

    bool isTile(const GameMap::Tile& a, const GameMap::Tile& b) {
        return a.texture.col == b.texture.col && a.texture.row == b.texture.row;
    }

    HitboxLibrary loadHitboxes() {
        HitboxLibrary library;

        std::ifstream file(HITBOX_FILE);
        if (!file) {
            std::cerr << "Failed to load " << HITBOX_FILE << "\n";
            return library;
        }

        try {
            const json data = json::parse(file);

            for (const auto& tile : data.at("tiles")) {
                std::vector<Hitbox::Polygon> polygons;

                for (const auto& hitbox : tile.at("hitboxes")) {
                    Hitbox::Polygon polygon;
                    for (const auto& point : hitbox.at("points")) {
                        polygon.points.emplace_back(point.at(0).get<float>(), point.at(1).get<float>());
                    }
                    polygons.push_back(polygon);
                }

                library[{tile.at("col").get<int>(), tile.at("row").get<int>()}] = polygons;
            }
        } catch (const json::exception& error) {
            std::cerr << "Invalid " << HITBOX_FILE << ": " << error.what() << "\n";
        }
        return library;
    }

    // Transforms a point (0...1) from the JSON into a point in the world
    sf::Vector2f toWorld(const sf::Vector2f& point, const GameMap::Tile& tile, const int cellX, const int cellY) {
        const auto &[offsetX, offsetY, scale, rotation] = tile.modifier;
        constexpr float half = TILE_SIZE / 2.f;

        // Position relative to the center of the tile, in pixels
        const float x = (point.x * TILE_SIZE - half) * scale;
        const float y = (point.y * TILE_SIZE - half) * scale;

        const float angle = static_cast<float>(rotation) * PI / 180.f;
        const float rotatedX = x * std::cos(angle) - y * std::sin(angle);
        const float rotatedY = x * std::sin(angle) + y * std::cos(angle);

        return {
            static_cast<float>(cellX * TILE_STEP) + half + offsetX + rotatedX,
            static_cast<float>(cellY * TILE_STEP) + half + offsetY + rotatedY
        };
    }

    const HitboxLibrary& hitboxLibrary() {
        static const HitboxLibrary library = loadHitboxes();
        return library;
    }
}


void MapGenerator::buildCellCollision(Map& map, const int x, const int y) {
    MapCell& cell = map.cells[y][x];
    cell.hitboxes.clear();
    cell.hideZones.clear(); // Pense à vider hideZones comme hitboxes (vérifie le nom exact dans MapCell)

    const HitboxLibrary& library = hitboxLibrary();

    for (const GameMap::Tile& tile : cell.layers) {
        // A bush doesn't block, it only hides the player
        if (isTile(tile, Tiles::PROP_BUSH)) {
            cell.hideZones.push_back({
                .x = static_cast<float>(x * TILE_STEP) + TILE_SIZE / 2.f + tile.modifier.offsetX,
                .y = static_cast<float>(y * TILE_STEP) + TILE_SIZE / 2.f + tile.modifier.offsetY,
                .radius = TILE_SIZE / 2.f * tile.modifier.scale * BUSH_HIDE_RATIO
            });
            continue;
        }

        const auto found = library.find({tile.texture.col, tile.texture.row});
        if (found == library.end()) continue;

        for (const Hitbox::Polygon& shape : found->second) {
            Hitbox::Polygon polygon;
            for (const sf::Vector2f& point : shape.points) {
                polygon.points.push_back(toWorld(point, tile, x, y));
            }
            cell.hitboxes.push_back(polygon);
        }
    }
}

void MapGenerator::buildCollision(Map& map) {
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            buildCellCollision(map, x, y);
        }
    }
}