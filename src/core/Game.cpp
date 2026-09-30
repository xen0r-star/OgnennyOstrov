#include "core/Game.h"

#include <Windows.h>
#include <SFML/Window/Event.hpp>
#include <random>

#include "Config.h"
#include "../models/map/MapGenerator.h"
#include "views/MapView.h"

Game::Game() {
    window.create(sf::VideoMode::getDesktopMode(), WINDOW_TITLE, sf::Style::Default);
    window.setFramerateLimit(60);

    ShowWindow(window.getSystemHandle(), SW_MAXIMIZE);

    std::random_device rd;
    map = MapGenerator::generateMap(50, 50, static_cast<int>(rd()));
}

Game::~Game() = default;

void Game::run() {
    sf::Clock clock;

    while (window.isOpen()) {
        const sf::Time deltaTime = clock.restart();

        processEvents();
        update(deltaTime);
        render();
    }
}

void Game::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }

        // For testing
        mapView.handleEvent(event, window);

        // Input Controller
    }
}

void Game::update(sf::Time deltaTime) {
    // Player Move update
}

void Game::render() {
    window.clear(sf::Color(30, 30, 30));
    mapView.drawMap(window, map);
    window.display();
}