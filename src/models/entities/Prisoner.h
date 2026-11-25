#ifndef OGNENNYOSTROV_PRISONER_H
#define OGNENNYOSTROV_PRISONER_H

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
    void setAimDirection(sf::Vector2f newAimDirection);
    void update(sf::Time dt, const Map& map);
    bool moveToNearestFreeSpot(sf::Vector2f target, const Map& map);

    [[nodiscard]] const sf::Vector2f& getPosition() const;
    [[nodiscard]] const sf::Vector2f& getAimDirection() const;
    [[nodiscard]] bool isHidden(const Map& map) const;

private:
    static constexpr float HITBOX_RADIUS = 30.f; // 60 px

    static bool canStandAt(sf::Vector2f pos, const Map& map);

    static sf::Vector2f cellCenter(int cellX, int cellY);
    [[nodiscard]] bool canStandInCell(int cellX, int cellY, const Map& map) const;
    [[nodiscard]] bool canWalkToNeighborCell(int cellX, int cellY, int dx, int dy, const Map& map) const;
    int floodFillCells(int startX, int startY, const Map& map, std::vector<char>& visited) const;

    sf::Vector2f position;
    sf::Vector2f direction{0.f, 0.f};
    sf::Vector2f aimDirection{0.f, 1.f};
    float speed = 400.f;
};

#endif // OGNENNYOSTROV_PRISONER_H