#include "core/Game.h"

#include <SFML/Window/Event.hpp>
#include <iostream>
#include <random>
#include <ctime>
#include <cmath>
#include <algorithm>
#include "views/Weapons.h"
#include "../models/map/MapGenerator.h"
#include "Config.h"
#include "core/Settings.h"
#include "views/MapView.h"
#include "views/menu/Drawing.h"
#include "models/map/Tiles.h"
#include "views/menu/MenuView.h"

#ifdef _WIN32
    #include <Windows.h>
#endif
namespace {
    constexpr int BOT_COUNT = 3;
    constexpr int BOT_DAMAGE = 10;

    // Empêche deux personnages de se chevaucher : on les écarte l'un de l'autre
    void separate(Prisoner& a, Prisoner& b, const Map& map) {
        const sf::Vector2f d = b.getPosition() - a.getPosition();
        const float distance = std::sqrt(d.x * d.x + d.y * d.y);
        const float minDistance = Prisoner::getRadius() * 2.f;
        if (distance >= minDistance) return;   // ils ne se touchent pas

        const sf::Vector2f normal = distance > 0.001f ? d / distance : sf::Vector2f(1.f, 0.f);
        const float overlap = minDistance - distance;

        const sf::Vector2f moved = a.pushBy(-normal * (overlap / 2.f), map);
        const float movedLength = std::sqrt(moved.x * moved.x + moved.y * moved.y);
        b.pushBy(normal * (overlap - movedLength), map);
    }
}
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

    prisoner = Prisoner({0.f, 0.f});   // nouveau prisonnier : vie pleine

    const sf::Vector2f mapCenter(static_cast<float>(map.width * TILE_STEP) / 2.f,
                                 static_cast<float>(map.height * TILE_STEP) / 2.f);
    if (!prisoner.moveToNearestFreeSpot(mapCenter, map)) {
        std::cerr << "No free spot found to spawn the prisoner" << std::endl;
    }

    // Les bots apparaissent en cercle, à 12 cases du joueur
    bots.clear();
    botViews.clear();
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> randomAngle(0.f, 6.2832f);

    for (int i = 0; i < BOT_COUNT; ++i) {
        Bot bot(static_cast<unsigned int>(seed) + i + 1);
        const float angle = randomAngle(rng);
        const sf::Vector2f spot = mapCenter + sf::Vector2f(std::cos(angle), std::sin(angle)) * (5.f * TILE_STEP);

        bot.getBody().moveToNearestFreeSpot(spot, map);
        bot.getBody().setWeapon(Weapons::AXE);
        bots.push_back(bot);

        botViews.emplace_back("assets/textures/bot.png");
        botViews.back().reset(bot.getBody().getPosition());
    }

    updateCamera();
    followPlayer(sf::Time::Zero, true); // Snap the camera to the player position at the start of the game
    prisonerView.reset(prisoner.getPosition());
    prisoner.setWeapon(Weapons::LONG_SWORD);
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
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Escape) {
                        state = GameState::Menu;
                        resetScreenView();
                    }
                    else if (event.key.code == sf::Keyboard::E) {
                        tryOpenDoor();
                    }
                    else if (event.key.code == sf::Keyboard::Tab) {
                        prisoner.setWeapon((prisoner.getWeapon() + 1) % Weapons::COUNT);
                    }
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

        // Les bots réfléchissent et bougent
        for (Bot& bot : bots) {
            bot.update(deltaTime, prisoner, map);
            if (bot.hasHit) prisoner.takeDamage(BOT_DAMAGE);
        }

        // Collisions entre personnages : joueur <-> bots, puis bots <-> bots
        for (std::size_t i = 0; i < bots.size(); ++i) {
            separate(prisoner, bots[i].getBody(), map);
            for (std::size_t j = i + 1; j < bots.size(); ++j) {
                separate(bots[i].getBody(), bots[j].getBody(), map);
            }
        }

        if (prisoner.isDead()) {   // mort : retour au menu
            state = GameState::Menu;
            resetScreenView();
            return;
        }

        followPlayer(deltaTime, false); // Smoothly follow the player
        prisonerView.update(deltaTime, prisoner);
        for (std::size_t i = 0; i < bots.size(); ++i) {
            botViews[i].update(deltaTime, bots[i].getBody());
        }
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
            for (std::size_t i = 0; i < bots.size(); ++i) {
                botViews[i].draw(window, bots[i].getBody(), 0);
            }
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
namespace {
    constexpr float DOOR_REACH = TILE_STEP * 1.2f;

    bool sameTexture(const GameMap::Tile& a, const GameMap::Tile& b) {
        return a.texture.col == b.texture.col && a.texture.row == b.texture.row;
    }
}

void Game::tryOpenDoor() {
    const sf::Vector2f pos = prisoner.getPosition();

    const int px = static_cast<int>(std::floor(pos.x / TILE_STEP));
    const int py = static_cast<int>(std::floor(pos.y / TILE_STEP));

    GameMap::Tile* door = nullptr;
    int doorX = 0, doorY = 0;
    float bestDistance = DOOR_REACH * DOOR_REACH;

    for (int y = py - 1; y <= py + 1; ++y) {
        for (int x = px - 1; x <= px + 1; ++x) {
            if (x < 0 || y < 0 || x >= map.width || y >= map.height) continue;

            for (GameMap::Tile& tile : map.cells[y][x].layers) {
                if (!sameTexture(tile, Tiles::WALL_DOORWAY_CLOSE) &&
                    !sameTexture(tile, Tiles::WALL_DOORWAY_OPEN)) continue;

                const float dx = static_cast<float>(x * TILE_STEP) + TILE_SIZE / 2.f - pos.x;
                const float dy = static_cast<float>(y * TILE_STEP) + TILE_SIZE / 2.f - pos.y;
                const float distance = dx * dx + dy * dy;

                if (distance < bestDistance) {
                    bestDistance = distance;
                    door = &tile;
                    doorX = x;
                    doorY = y;
                }
            }
        }
    }

    if (door == nullptr) return;

    if (sameTexture(*door, Tiles::WALL_DOORWAY_OPEN)) {
        if (doorX == px && doorY == py) return;
        door->texture = Tiles::WALL_DOORWAY_CLOSE.texture;
    } else {
        door->texture = Tiles::WALL_DOORWAY_OPEN.texture;
    }

    MapGenerator::buildCellCollision(map, doorX, doorY);
}