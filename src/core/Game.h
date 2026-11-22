#ifndef GAME_H
#define GAME_H

#include "Settings.h"

#include <SFML/Graphics.hpp>

#include "views/MapView.h"
#include "views/menu/MenuView.h"
#include "views/menu/OptionsView.h"
#include "views/PrisonerView.h"

#include "controllers/client/PlayerController.h"
#include "models/entities/Prisoner.h"

enum class GameState {
    Menu,
    Options,
    Playing,
};

class Game {
public:
    Game();
    ~Game();

    void run();
private:
    void processEvents();
    void update(sf::Time deltaTime);
    void render();

    void startGame();
    void resetScreenView();

    void updateCamera();

    sf::RenderWindow window;
    sf::View camera;
    GameState state = GameState::Menu;

    // Menu data
    sf::Font font;
    Settings settings;
    MenuView menu;
    OptionsView options;

    // Game data
    Map map;
    MapView mapView;

    Prisoner prisoner;
    PrisonerView prisonerView;
    PlayerController playerController;
};

#endif // GAME_H