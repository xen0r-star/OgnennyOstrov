#include "PlayerController.h"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>


void PlayerController::update(Prisoner& prisoner) {
    sf::Vector2f direction(0.f ,0.f);

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z)||sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) { direction.y -= 1.f; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)||sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) { direction.y += 1.f; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)||sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) { direction.x -= 1.f; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) ||sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) { direction.x += 1.f; }

    // Normalisation : en diagonale (ex: Z+D → (1,-1)), la longueur du vecteur
    // vaut √(x² + y²) = √2 ≈ 1.41 (Pythagore) → le joueur irait 41% plus vite.
    // On divise par la longueur pour la ramener à 1 : même vitesse dans toutes

    const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length != 0.f) {
        direction /= length;
    }

    prisoner.setDirection(direction);
}