#ifndef GOULAG_OPTIONSVIEW_H
#define GOULAG_OPTIONSVIEW_H

#include "core/Settings.h"

#include <SFML/Graphics.hpp>

class OptionsView {
public:
    OptionsView();

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window, Settings& settings);
    void draw(sf::RenderWindow& window, const sf::Font& font, const Settings& settings);

    bool back = false; // becomes true when "Retour" is clicked

private:
    sf::Texture spriteSheet;
};

#endif //GOULAG_OPTIONSVIEW_H