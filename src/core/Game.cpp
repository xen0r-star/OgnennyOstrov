#include "core/Game.h"

#include <SFML/Window/Event.hpp>
#include <iostream>
#include <random>
#include <ctime>
#include <cmath>
#include <algorithm>

#include "../models/map/MapGenerator.h"
#include "Config.h"
#include "core/Settings.h"
#include "views/MapView.h"
#include "views/menu/Drawing.h"

#ifdef _WIN32
    #include <Windows.h>
#endif

Game::~Game() = default;

Game::Game() {
    #ifdef _WIN32
        window.create(sf::VideoMode::getDesktopMode(), WINDOW_TITLE, sf::Style::Default);
        ShowWindow(window.getSystemHandle(), SW_MAXIMIZE);
    #else
        window.create(sf::VideoMode::getDesktopMode(), WINDOW_TITLE, sf::Style::Fullscreen);
    #endif

    window.setVerticalSyncEnabled(true);

    if (!font.loadFromFile("assets/fonts/LilitaOne-Regular.ttf")) {
        std::cerr << "Cannot load assets/fonts/LilitaOne-Regular.ttf" << std::endl;
        window.close();
    }
}

void Game::startGame() {
    const int seed = static_cast<int>(std::time(nullptr));
    map = MapGenerator::generateMap(65, 65, seed);

    const sf::Vector2f mapCenter(static_cast<float>(map.width * TILE_STEP) / 2.f,
                                 static_cast<float>(map.height * TILE_STEP) / 2.f);
    if (!prisoner.moveToNearestFreeSpot(mapCenter, map)) {
        std::cerr << "No free spot found to spawn the prisoner" << std::endl;
    }

    updateCamera();
    followPlayer(sf::Time::Zero, true); // Snap the camera to the player position at the start of the game
    prisonerView.reset(prisoner.getPosition());
    state = GameState::Playing;
}

void Game::updateCamera() {
    const float width = static_cast<float>(window.getSize().x);
    const float height = std::max(1.f, static_cast<float>(window.getSize().y));

    camera.setSize(CAMERA_VISIBLE_HEIGHT * width / height, CAMERA_VISIBLE_HEIGHT);
}

void Game::followPlayer(const sf::Time dt, const bool snap) {
    const sf::Vector2f target = prisoner.getPosition();

    if (snap) {
        cameraCenter = target;
    } else {
        // Smoothly interpolate the camera position towards the target position
        const float t = 1.f - std::exp(-CAMERA_SMOOTHNESS * dt.asSeconds());
        cameraCenter += (target - cameraCenter) * t;
    }

    const float pixel = camera.getSize().y / std::max(1.f, static_cast<float>(window.getSize().y));
    camera.setCenter(std::round(cameraCenter.x / pixel) * pixel,
                     std::round(cameraCenter.y / pixel) * pixel);
}

void Game::run() {
    sf::Clock clock;

    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();
        deltaTime = std::min(deltaTime, sf::seconds(0.05f));

        processEvents();
        update(deltaTime);
        render();
    }
}

void Game::processEvents() {
    if (state != GameState::Playing) resetScreenView();

    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
            continue;
        }

        if (event.type == sf::Event::Resized) {
            updateCamera();
        }

        switch (state) {
            case GameState::Menu:
                switch (menu.handleEvent(event, window)) {
                    case MenuAction::Play:        startGame(); break;
                    case MenuAction::Multiplayer: break;
                    case MenuAction::Options:     state = GameState::Options; break;
                    case MenuAction::Quit:        window.close(); break;
                    case MenuAction::None:        break;
                }
                break;

            case GameState::Options:
                options.handleEvent(event, window, settings);
                break;

            case GameState::Playing:
                if (event.type == sf::Event::KeyPressed &&
                    event.key.code == sf::Keyboard::Escape) {
                    state = GameState::Menu;   // back to the menu
                    resetScreenView();
                }
                break;
        }
    }
}

void Game::update(sf::Time deltaTime) {
    if (state == GameState::Options && options.back) {
        options.back = false;
        state = GameState::Menu;
    }

    if (state == GameState::Playing) {
        playerController.update(prisoner, window, camera);
        prisoner.update(deltaTime, map);
        followPlayer(deltaTime, false); // Smoothly follow the player
        prisonerView.update(deltaTime, prisoner);
    }
}


void Game::resetScreenView() {
    const sf::Vector2f size(window.getSize());
    const float scale = size.y / 1200.f; // the menu is designed for a 1200px high screen

    window.setView(sf::View(sf::FloatRect(0.f, 0.f, size.x / scale, size.y / scale)));
}

void Game::render() {
    window.clear(sf::Color(30, 30, 30));

    switch (state) {
        case GameState::Menu:
            resetScreenView();
            menu.draw(window, font);
            break;

        case GameState::Playing:
            window.setView(camera);

            mapView.drawMap(window, map);
            prisonerView.draw(window, prisoner, settings.character);
            fogView.draw(window, prisoner.getPosition());
            break;

        case GameState::Options:
            resetScreenView();
            options.draw(window, font, settings);
            break;
    }

    drawBrightnessVeil(window, settings.brightness);
    window.display();
}