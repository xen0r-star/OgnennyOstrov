#ifndef GOULAG_GAME_H
#define GOULAG_GAME_H

#include "Settings.h"

#include <SFML/Graphics.hpp>

#include "views/MapView.h"
#include "views/menu/MenuView.h"
#include "views/menu/OptionsView.h"

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

    sf::RenderWindow window;
    GameState state = GameState::Menu;

    // Menu data
    sf::Font font;
    Settings settings;
    MenuView menu;
    OptionsView options;

    // Game data
    Map map;
    MapView mapView;
};

#endif // GOULAG_GAME_H