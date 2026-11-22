#include "PlayerController.h"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>


void PlayerController::update(Prisoner& prisoner) {
    sf::Vector2f direction(0.f ,0.f);

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z)||sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) { direction.y -= 1.f; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)||sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) { direction.y += 1.f; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)||sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) { direction.x -= 1.f; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) ||sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) { direction.x += 1.f; }

    // Normalization: diagonally (e.g., Z+D → (1,-1)), the length of the vector
    // is √(x² + y²) = √2 ≈ 1.41 (Pythagoras) → the player would move 41% faster.
    // We divide by the length to bring it back to 1: same speed in all directions

    const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length != 0.f) {
        direction /= length;
    }

    prisoner.setDirection(direction);
}