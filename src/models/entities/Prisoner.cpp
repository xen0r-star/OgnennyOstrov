#include "models/entities/Prisoner.h"
#include "core/Config.h"
#include "models/map/Tile.h"
#include "models/map/Hitbox.h"
#include <cmath>
#include <algorithm>

Prisoner::Prisoner(const sf::Vector2f position) : position(position) {}

const sf::Vector2f& Prisoner::getPosition() const {
    return position;
}

void Prisoner::setDirection(const sf::Vector2f newDirection) {
    direction = newDirection;
}

void Prisoner::update(const sf::Time dt, const Map& map) {
    const sf::Vector2f move = direction * speed * dt.asSeconds();

    // Axe X d'abord
    sf::Vector2f next = position;
    next.x += move.x;
    if (canStandAt(next, map)) {
        position.x = next.x;
    }

    // Puis axe Y, séparément → on glisse le long des murs au lieu de rester collé
    next = position;
    next.y += move.y;
    if (canStandAt(next, map)) {
        position.y = next.y;
    }
}


bool Prisoner::canStandAt(const sf::Vector2f pos, const Map& map) {
    // Outside of the map: forbidden
    if (pos.x - HITBOX_RADIUS < 0.f || pos.y - HITBOX_RADIUS < 0.f ||
        pos.x + HITBOX_RADIUS > static_cast<float>(map.width * TILE_STEP) ||
        pos.y + HITBOX_RADIUS > static_cast<float>(map.height * TILE_STEP)) {
        return false;
    }

    // We only check the cell of the prisoner and its 8 neighbours
    const int cellX = static_cast<int>(std::floor(pos.x / TILE_STEP));
    const int cellY = static_cast<int>(std::floor(pos.y / TILE_STEP));

    for (int y = std::max(cellY - 1, 0); y <= std::min(cellY + 1, map.height - 1); ++y) {
        for (int x = std::max(cellX - 1, 0); x <= std::min(cellX + 1, map.width - 1); ++x) {
            for (const Hitbox::Polygon& polygon : map.cells[y][x].hitboxes) {
                if (Hitbox::circleTouchesPolygon(pos, HITBOX_RADIUS, polygon)) {
                    return false;   // one shape touched is enough
                }
            }
        }
    }
    return true;
}

sf::Vector2f Prisoner::cellCenter(const int cellX, const int cellY) {
    return {(static_cast<float>(cellX) + 0.5f) * TILE_STEP,
            (static_cast<float>(cellY) + 0.5f) * TILE_STEP};
}

bool Prisoner::canStandInCell(const int cellX, const int cellY, const Map& map) const {
    if (cellX < 0 || cellY < 0 || cellX >= map.width || cellY >= map.height) return false;
    return canStandAt(cellCenter(cellX, cellY), map);
}

// Tests every 10 px so a thin obstacle (closed door) can't slip between two tests
bool Prisoner::canWalkToNeighborCell(const int cellX, const int cellY, const int dx, const int dy, const Map& map) const {
    constexpr int steps = TILE_STEP / 10;   // one test every 10 px
    const sf::Vector2f from = cellCenter(cellX, cellY);
    const sf::Vector2f to = cellCenter(cellX + dx, cellY + dy);

    for (int i = 0; i <= steps; ++i) {
        const float t = static_cast<float>(i) / steps;
        const sf::Vector2f pos(from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t);
        if (!canStandAt(pos, map)) return false;
    }
    return true;
}

int Prisoner::floodFillCells(const int startX, const int startY, const Map& map, std::vector<char>& visited) const {
    std::vector stack{startY * map.width + startX};
    visited[stack.back()] = 1;
    int count = 0;

    while (!stack.empty()) {
        const int current = stack.back();
        stack.pop_back();
        ++count;

        const int x = current % map.width;
        const int y = current / map.width;

        for (int k = 0; k < 4; ++k) {
            constexpr int dy[] = {0, 0, 1, -1};
            constexpr int dx[] = {1, -1, 0, 0};

            const int nx = x + dx[k];
            const int ny = y + dy[k];
            if (nx < 0 || ny < 0 || nx >= map.width || ny >= map.height) continue;

            const int next = ny * map.width + nx;
            if (visited[next]) continue;
            if (!canWalkToNeighborCell(x, y, dx[k], dy[k], map)) continue;
            visited[next] = 1;
            stack.push_back(next);
        }
    }
    return count;
}

bool Prisoner::moveToNearestFreeSpot(const sf::Vector2f target, const Map& map) {
    const int startX = static_cast<int>(std::floor(target.x / TILE_STEP));
    const int startY = static_cast<int>(std::floor(target.y / TILE_STEP));
    const int maxRadius = std::max(map.width, map.height);

    int standable = 0;
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            if (canStandInCell(x, y, map)) ++standable;
        }
    }

    std::vector<char> visited(static_cast<std::size_t>(map.width) * map.height, 0);
    bool hasFallback = false;
    sf::Vector2f fallback;

    for (int radius = 0; radius <= maxRadius; ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != radius) continue;

                const int cellX = startX + dx;
                const int cellY = startY + dy;
                if (!canStandInCell(cellX, cellY, map)) continue;

                if (!hasFallback) {
                    hasFallback = true;
                    fallback = cellCenter(cellX, cellY);
                }

                if (visited[cellY * map.width + cellX]) continue;

                // At least half of the walkable cells = main area, not a closed room
                if (floodFillCells(cellX, cellY, map, visited) * 2 >= standable) {
                    position = cellCenter(cellX, cellY);
                    return true;
                }
            }
        }
    }

    if (hasFallback) {
        position = fallback;
        return true;
    }
    return false;
}