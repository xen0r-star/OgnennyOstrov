#ifndef OGNENNYOSTROV_PAUSEVIEW_H
#define OGNENNYOSTROV_PAUSEVIEW_H

#include <SFML/Graphics.hpp>

enum class PauseAction {
    None,
    Resume,
    Options,
    Quit
};

class PauseView {
public:
    // Returns the chosen action (or None)
    PauseAction handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderWindow& window, const sf::Font& font) const;
    void reset(); // Selects "Reprendre" again

private:
    int selection = 0;
};

#endif //OGNENNYOSTROV_PAUSEVIEW_H