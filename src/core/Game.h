#ifndef GAME_H
#define GAME_H

#include "Settings.h"

#include <SFML/Graphics.hpp>

#include "views/MapView.h"
#include "views/menu/MenuView.h"
#include "views/menu/OptionsView.h"
#include "views/PrisonerView.h"
#include "views/FogView.h"
#include "models/entities/Bot.h"
#include <vector>
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
    void followPlayer(sf::Time dt, bool snap);
    void tryOpenDoor();

    sf::RenderWindow window;
    sf::View camera;
    sf::Vector2f cameraCenter{0.f, 0.f};
    GameState state = GameState::Menu;

    // Menu data
    sf::Font font;
    Settings settings;
    MenuView menu;
    OptionsView options;

    // Game data
    Map map;
    MapView mapView;

    Prisoner prisoner{{0.f, 0.f}};
    PrisonerView prisonerView;
    FogView fogView;
    PlayerController playerController;
    std::vector<Bot> bots;
    std::vector<PrisonerView> botViews;   // une vue par bot (chacun a sa propre animation)
};

#endif // GAME_H