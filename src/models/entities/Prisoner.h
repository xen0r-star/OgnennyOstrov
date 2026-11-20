#ifndef GOULAG_PRISONER_H
#define GOULAG_PRISONER_H

#include <SFML/System/Vector2.hpp>
#include <SFML/System/Time.hpp>
#include "models/map/Tile.h"
#include <vector>

class Prisoner {
    // TODO : name, health, STARTING_HEALTH, facing (direction regardee),
    //        kills, attackCooldown (temps avant de pouvoir reattaquer/tirer),
    //        weapon, ammo, currentState, openingTimer (ouverture caisse),
    //        revealedTimer (temps ou tu es visible), phantomTimer (temps ou tu ne peux pas etre repere)
public:
    explicit Prisoner(sf::Vector2f position);

    void setDirection(sf::Vector2f newDirection);
    void update(sf::Time dt, const Map& map);
    bool moveToNearestFreeSpot(sf::Vector2f target, const Map& map);

    [[nodiscard]] const sf::Vector2f& getPosition() const;

private:
    static constexpr float HITBOX_HALF = 30.f;   // hitbox = carre de 60 px autour du centre (taille libre)

    bool canStandAt(sf::Vector2f pos, const Map& map) const;
    static bool isWalkable(int subX, int subY, const Map& map);
    static int toSubcell(float pixel);

    static sf::Vector2f cellCenter(int cellX, int cellY);
    bool canStandInCell(int cellX, int cellY, const Map& map) const;
    bool canWalkToNeighborCell(int cellX, int cellY, int dx, int dy, const Map& map) const;
    int floodFillCells(int startX, int startY, const Map& map, std::vector<char>& visited) const;

    sf::Vector2f position;
    sf::Vector2f direction{0.f, 0.f};
    float speed = 400.f;
};

#endif // GOULAG_PRISONER_H
