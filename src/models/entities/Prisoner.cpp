#include "models/entities/Prisoner.h"
#include "core/Config.h"   // pour TILE_STEP
#include "models/map/Tile.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

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


int Prisoner::toSubcell(const float pixel) {
    return static_cast<int>(std::floor(pixel / SUBCELL_SIZE));
}

bool Prisoner::canStandAt(const sf::Vector2f pos, const Map& map) const {
    // On teste une grille de points sur la hitbox, espacés de SUBCELL_SIZE (20 px) au maximum,
    // pour ne jamais "rater" une sous-case bloquée entre deux points.
    // HITBOX_HALF = 30 : 60 / 20 = 3 intervalles -> 4 x 4 = 16 points.
    const int steps = static_cast<int>(std::ceil(2.f * HITBOX_HALF / SUBCELL_SIZE));
    const float width = 2.f * HITBOX_HALF;

    for (int i = 0; i <= steps; ++i) {
        const float px = pos.x - HITBOX_HALF + width * i / steps;
        for (int j = 0; j <= steps; ++j) {
            const float py = pos.y - HITBOX_HALF + width * j / steps;
            if (!isWalkable(toSubcell(px), toSubcell(py), map)) {
                return false;   // un seul point bloqué suffit
            }
        }
    }
    return true;
}

bool Prisoner::isWalkable(const int subX, const int subY, const Map& map) {
    if (subX < 0 || subY < 0) return false;

    const int cellX = subX / MASK_SIZE;   // quelle case de la carte
    const int cellY = subY / MASK_SIZE;

    if (cellX >= map.width || cellY >= map.height) return false;

    const int index = (subY % MASK_SIZE) * MASK_SIZE + (subX % MASK_SIZE);
    return !map.cells[cellY][cellX].blocksMovement[index];
}

sf::Vector2f Prisoner::cellCenter(const int cellX, const int cellY) {
    return {(static_cast<float>(cellX) + 0.5f) * TILE_STEP,
            (static_cast<float>(cellY) + 0.5f) * TILE_STEP};
}

bool Prisoner::canStandInCell(const int cellX, const int cellY, const Map& map) const {
    if (cellX < 0 || cellY < 0 || cellX >= map.width || cellY >= map.height) return false;
    return canStandAt(cellCenter(cellX, cellY), map);
}

// Tests every half sub-cell so a thin obstacle (closed door) can't slip between two tests
bool Prisoner::canWalkToNeighborCell(const int cellX, const int cellY, const int dx, const int dy, const Map& map) const {
    constexpr int steps = TILE_STEP / (SUBCELL_SIZE / 2);
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
    std::vector<int> stack{startY * map.width + startX};
    visited[stack.back()] = 1;
    int count = 0;

    while (!stack.empty()) {
        const int current = stack.back();
        stack.pop_back();
        ++count;

        const int x = current % map.width;
        const int y = current / map.width;
        constexpr int dx[] = {1, -1, 0, 0};
        constexpr int dy[] = {0, 0, 1, -1};

        for (int k = 0; k < 4; ++k) {
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