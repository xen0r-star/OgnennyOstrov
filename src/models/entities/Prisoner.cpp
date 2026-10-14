#include "models/entities/Prisoner.h"
#include "core/Config.h"   // pour TILE_STEP
#include "models/map/MapGenerator.h"
#include <cmath>

Prisoner::Prisoner(const sf::Vector2f position) : position(position) {}

const sf::Vector2f& Prisoner::getPosition() const {
    return position;
}

void Prisoner::setDirection(const sf::Vector2f newDirection) {
    direction = newDirection;
}

void Prisoner::update(const sf::Time dt, const TileMap& map) {
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

bool Prisoner::canStandAt(const sf::Vector2f pos, const TileMap& map) const {
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

bool Prisoner::isWalkable(const int subX, const int subY, const TileMap& map) {
    if (subX < 0 || subY < 0) return false;

    const int cellX = subX / MASK_SIZE;   // quelle case de la carte
    const int cellY = subY / MASK_SIZE;
    if (cellX >= static_cast<int>(map.size()) || cellY >= static_cast<int>(map[0].size())) return false;

    const int index = (subY % MASK_SIZE) * MASK_SIZE + (subX % MASK_SIZE);   // quelle sous-case dans la case
    return !map[cellX][cellY].blocksMovement[index];
}
