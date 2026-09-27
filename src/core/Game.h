#ifndef GOULAG_GAME_H
#define GOULAG_GAME_H

#include "views/MapView.h"
#include <SFML/Graphics.hpp>

class Game {
public:
    Game();
    ~Game();

    void run();
private:
    void processEvents();
    void update(sf::Time deltaTime);
    void render();

    sf::RenderWindow window;
    TileMap map;
    MapView mapView;
};

#endif // GOULAG_GAME_H
