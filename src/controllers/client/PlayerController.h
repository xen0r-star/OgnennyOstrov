#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include "models/entities/Prisoner.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>


class PlayerController {
public:
  static void update(Prisoner& prisoner, const sf::RenderWindow& window, const sf::View& camera);
};


#endif //PLAYERCONTROLLER_H