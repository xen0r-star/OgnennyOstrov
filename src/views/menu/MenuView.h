#ifndef GOULAG_MENUVIEW_H
#define GOULAG_MENUVIEW_H

#include <SFML/Graphics.hpp>

enum class MenuAction {
    None,
    Play,
    Multiplayer,
    Options,
    Quit
};

class MenuView {
public:
    // Returns the chosen action (or None)
    MenuAction handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderWindow& window, const sf::Font& font);

private:
    int selection = 0;
};

#endif //GOULAG_MENUVIEW_H