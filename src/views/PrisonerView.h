//
// Created by amine on 06/10/2026.
//

#ifndef GOULAG_PRISONERVIEW_H
#define GOULAG_PRISONERVIEW_H
#include "models/entities/Prisoner.h"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Texture.hpp"


class PrisonerView {
    public:
    PrisonerView();

    void draw(sf::RenderWindow& window , const Prisoner& prisoner);
    private:
    sf::Texture texture;
    sf::Sprite sprite;
};


#endif //GOULAG_PRISONERVIEW_H
