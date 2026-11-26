#ifndef OGNENNYOSTROV_BOT_H
#define OGNENNYOSTROV_BOT_H

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <random>

#include "models/entities/Prisoner.h"
#include "models/map/Tile.h"

enum class BotState { Patrol, Suspicious, Chase, Attack, Search };

// Un bot = un prisonnier (le corps) + un cerveau qui remplace le clavier
class Bot {
public:
    explicit Bot(unsigned int seed);

    void update(sf::Time dt, const Prisoner& target, const Map& map);

    Prisoner& getBody() { return body; }
    [[nodiscard]] const Prisoner& getBody() const { return body; }
    [[nodiscard]] BotState getState() const { return state; }
    [[nodiscard]] bool isPreparingAttack() const { return windup > 0.f; }

    bool hasHit = false;   // true pendant UNE frame : celle où il touche

private:
    // Une fonction par état
    void patrol(float dt, const Map& map);
    void suspicious(bool sees);
    void chase(bool sees, float distance);
    void attack(float dt, float distance);
    void search(float dt, bool sees);

    // Outils
    void moveTowards(sf::Vector2f point, float speedFactor);
    void lookAt(sf::Vector2f point);
    void stop();
    [[nodiscard]] bool sees(const Prisoner& target, const Map& map) const;
    bool pickPatrolPoint(const Map& map);
    float randomBetween(float min, float max);
    void changeState(BotState newState);

    Prisoner body{{0.f, 0.f}};
    std::mt19937 rng;

    // Cerveau
    BotState state = BotState::Patrol;
    float timeInState = 0.f;
    sf::Vector2f facing{0.f, 1.f};   // où il regarde (longueur 1)
    sf::Vector2f lastSeen;           // mémoire : dernier endroit où il t'a vu

    // Patrouille
    sf::Vector2f patrolPoint;
    bool hasPatrolPoint = false;
    float patrolTime = 0.f;          // depuis combien de temps il marche vers ce point
    float pause = 0.f;               // pause entre deux points

    // Attaque
    float cooldown = 0.f;            // temps avant de pouvoir refrapper
    float windup = -1.f;             // > 0 : il arme son coup
};

#endif // OGNENNYOSTROV_BOT_H