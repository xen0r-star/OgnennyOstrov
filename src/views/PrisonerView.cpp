#include "PrisonerView.h"
#include "views/Weapons.h"
#include "core/Config.h"
#include "SFML/Graphics/Sprite.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
    constexpr int FRAME_SIZE = 270;           // Skin size (player.png)
    constexpr int SKIN_COUNT = 4;             // Number of skins
    constexpr float REFERENCE_SPEED = 400.f;  // Player speed

    constexpr int HAND_SIZE = 108;            // Hand size (player.png)
    constexpr int HAND_SHEET_Y = FRAME_SIZE;  // Hand position in the sprite sheet

    constexpr float HAND_SIDE = PLAYER_DRAW_SIZE * 0.35f;   // Hand offset
    constexpr float HAND_FORWARD = 32.f;                    // Hand position when idle
    constexpr float HAND_SWING = 0.f;                       // Hand swing amplitude

    constexpr int WEAPON_CELL = 72;                             // Size of one cell in weapons_grid.png
    constexpr float WEAPON_DRAW_SIZE = PLAYER_DRAW_SIZE * 0.9f; // Weapon size on screen

    constexpr float HIDDEN_ALPHA = 110.f;  // Player opacity in a bush (0-255)
    constexpr float HIDE_SPEED   = 10.f;   // Fade speed
}

PrisonerView::PrisonerView() {
    if (!texture.loadFromFile("assets/textures/player.png")) {
        std::cerr << "Could not load assets/textures/player.png" << std::endl;
    }

    texture.setSmooth(true);
    if (!weaponTexture.loadFromFile("assets/textures/weapons.png")) {
        std::cerr << "Could not load assets/textures/weapons.png" << std::endl;
    }
    weaponTexture.setSmooth(true);
}

void PrisonerView::reset(const sf::Vector2f position) {
    lastPosition = position;
    moveAmount = 0.f;
    walkPhase = 0.f;
}

void PrisonerView::update(const sf::Time dt, const Prisoner& prisoner, const bool hidden) {
    const float seconds = std::max(dt.asSeconds(), 0.0001f);
    const sf::Vector2f delta = prisoner.getPosition() - lastPosition;
    lastPosition = prisoner.getPosition();

    const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    const float target = std::min(distance / seconds / REFERENCE_SPEED, 1.f);

    moveAmount += (target - moveAmount) * (1.f - std::exp(-14.f * seconds));
    walkPhase += distance * 0.025f;
    idleTime += seconds;
    hideAmount += ((hidden ? 1.f : 0.f) - hideAmount) * (1.f - std::exp(-HIDE_SPEED * seconds));
}

void PrisonerView::draw(sf::RenderWindow& window, const Prisoner& prisoner, int character) const {
    character = std::clamp(character, 0, SKIN_COUNT - 1);
    const sf::Vector2f pos = prisoner.getPosition();

    const float swing = std::sin(walkPhase) * moveAmount;                      // -1..1
    const float bounce = std::abs(std::sin(walkPhase)) * moveAmount;         //  0..1
    const float breath = std::sin(idleTime * 2.2f) * (1.f - moveAmount);     // Breathing at rest
    const sf::Color tint(255, 255, 255, static_cast<sf::Uint8>(
        255.f + (HIDDEN_ALPHA - 255.f) * hideAmount)
    );

    // Hands
    if (texture.getSize().x >= static_cast<unsigned>(HAND_SIZE * (character + 1)) &&
        texture.getSize().y >= static_cast<unsigned>(HAND_SHEET_Y + HAND_SIZE)) {
        sf::Sprite hand(texture, sf::IntRect(character * HAND_SIZE, HAND_SHEET_Y, HAND_SIZE, HAND_SIZE));
        hand.setOrigin(HAND_SIZE / 2.f, HAND_SIZE / 2.f);

        constexpr float scale = PLAYER_DRAW_SIZE / FRAME_SIZE;
        hand.setScale(scale, scale);
        hand.setColor(tint);

        const sf::Vector2f aim = prisoner.getAimDirection();
        const sf::Vector2f side(-aim.y, aim.x);
        constexpr float radToDeg = 180.f / 3.14159265f;
        hand.setRotation(std::atan2(aim.y, aim.x) * radToDeg - 90.f);

        const sf::Vector2f center(pos.x, pos.y - bounce * 3.f + breath * 1.5f);

        const sf::Vector2f leftHand  = center - side * HAND_SIDE + aim * (HAND_FORWARD + swing * HAND_SWING);
        const sf::Vector2f rightHand = center + side * HAND_SIDE + aim * (HAND_FORWARD - swing * HAND_SWING);

        drawWeapon(window, prisoner.getWeapon(), rightHand, aim);   // l'arme d'abord...
        hand.setPosition(leftHand);
        window.draw(hand);
        hand.setPosition(rightHand);
        window.draw(hand);                                          // ...puis la main par-dessus
    }

    // Body
    if (texture.getSize().x >= static_cast<unsigned>(FRAME_SIZE * (character + 1))) {
        sf::Sprite body(texture, sf::IntRect(character * FRAME_SIZE, 0, FRAME_SIZE, FRAME_SIZE));
        body.setOrigin(FRAME_SIZE / 2.f, FRAME_SIZE / 2.f);
        body.setColor(tint);

        constexpr float scale = PLAYER_DRAW_SIZE / FRAME_SIZE;
        body.setScale(scale * (1.f + 0.015f * breath), scale * (1.f - 0.015f * breath));
        body.setPosition(pos.x, pos.y - bounce * 3.f);
        window.draw(body);
    }
}

void PrisonerView::drawWeapon(sf::RenderWindow& window, const int weapon,
                              const sf::Vector2f handPos, const sf::Vector2f aim) const {
    if (weapon < 0 || weapon >= Weapons::COUNT) return;   // mains nues

    const Weapons::Sprite& s = Weapons::SPRITES[weapon];

    sf::Sprite sprite(weaponTexture, sf::IntRect(s.col * WEAPON_CELL, s.row * WEAPON_CELL,
                                                 WEAPON_CELL, WEAPON_CELL));
    sprite.setOrigin(WEAPON_CELL / 2.f, WEAPON_CELL * s.gripY);

    constexpr float scale = WEAPON_DRAW_SIZE / WEAPON_CELL;
    sprite.setScale(scale, scale);

    constexpr float radToDeg = 180.f / 3.14159265f;
    sprite.setRotation(std::atan2(aim.y, aim.x) * radToDeg + 90.f);

    sprite.setPosition(handPos);
    window.draw(sprite);
}