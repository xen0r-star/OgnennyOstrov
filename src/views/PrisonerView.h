#ifndef OGNENNYOSTROV_PRISONERVIEW_H
#define OGNENNYOSTROV_PRISONERVIEW_H

#include "models/entities/Prisoner.h"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/Texture.hpp"

class PrisonerView {
public:
    PrisonerView();

    void reset(sf::Vector2f position);
    void update(sf::Time dt, const Prisoner& prisoner, bool hidden);
    void draw(sf::RenderWindow& window, const Prisoner& prisoner, int character) const;


private:
    sf::Texture texture; // assets/textures/player.png
    sf::Texture weaponTexture; // assets/textures/weapons.png

    void drawWeapon(sf::RenderWindow& window, int weapon, sf::Vector2f handPos, sf::Vector2f aim) const;

    sf::Vector2f lastPosition{0.f, 0.f};
    float moveAmount = 0.f;
    float walkPhase = 0.f;
    float idleTime = 0.f;
    float hideAmount = 0.f;
};


#endif //OGNENNYOSTROV_PRISONERVIEW_H
