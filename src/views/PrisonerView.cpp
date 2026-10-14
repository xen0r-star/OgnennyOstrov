//
// Created by amine on 06/10/2026.
//

#include "PrisonerView.h"

#include <iostream>

PrisonerView::PrisonerView() {
    if (!texture.loadFromFile("assets/textures/prisoner_red.png")) {
        std::cerr << "Could not load prisoner_red.png" << std::endl;
        return;
    }

    sprite.setTexture(texture);

    const sf::Vector2u size = texture.getSize();
    sprite.setOrigin(size.x / 2.f, size.y / 2.f);
    sprite.setScale(0.3f, 0.3f);
}

void PrisonerView::draw(sf::RenderWindow& window, const Prisoner& prisoner) {
    sprite.setPosition(prisoner.getPosition());
    window.draw(sprite);
}
