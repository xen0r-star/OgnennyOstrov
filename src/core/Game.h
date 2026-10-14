#ifndef GOULAG_GAME_H
#define GOULAG_GAME_H

#include "views/MapView.h"
#include <SFML/Graphics.hpp>

#include "controllers/client/PlayerController.h"
#include "views/PrisonerView.h"
#include "models/entities/Prisoner.h"

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
    sf::View camera;
    TileMap map;
    MapView mapView;

    Prisoner prisoner;
    PrisonerView prisonerView;
    PlayerController playerController;
};

#endif // GOULAG_GAME_H
