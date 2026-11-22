#include "PrisonerView.h"

#include "core/Config.h"
#include "SFML/Graphics/Sprite.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
    constexpr int FRAME_SIZE = 90;           // Skin size (player.png)
    constexpr int SKIN_COUNT = 4;            // Number of skins
    constexpr float REFERENCE_SPEED = 400.f; // Player speed

    constexpr int HAND_SIZE = 36;            // Hand size (player.png)
    constexpr int HAND_SHEET_Y = FRAME_SIZE; // Hand position in the sprite sheet

    constexpr float HAND_SIDE = PLAYER_DRAW_SIZE * 0.35f;   // Hand offset
    constexpr float HAND_FORWARD = 32.f;                    // Hand position when idle
    constexpr float HAND_SWING = 0.f;                       // Hand swing amplitude
}

PrisonerView::PrisonerView() {
    if (!texture.loadFromFile("assets/textures/player.png")) {
        std::cerr << "Could not load assets/textures/player.png" << std::endl;
    }
}

void PrisonerView::reset(const sf::Vector2f position) {
    lastPosition = position;
    moveAmount = 0.f;
    walkPhase = 0.f;
}

void PrisonerView::update(const sf::Time dt, const Prisoner& prisoner) {
    const float seconds = std::max(dt.asSeconds(), 0.0001f);
    const sf::Vector2f delta = prisoner.getPosition() - lastPosition;
    lastPosition = prisoner.getPosition();

    const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    const float target = std::min(distance / seconds / REFERENCE_SPEED, 1.f);

    moveAmount += (target - moveAmount) * (1.f - std::exp(-14.f * seconds));
    walkPhase += distance * 0.025f;
    idleTime += seconds;
}

void PrisonerView::draw(sf::RenderWindow& window, const Prisoner& prisoner, int character) const {
    character = std::clamp(character, 0, SKIN_COUNT - 1);
    const sf::Vector2f pos = prisoner.getPosition();

    const float swing = std::sin(walkPhase) * moveAmount;                      // -1..1
    const float bounce = std::abs(std::sin(walkPhase)) * moveAmount;         //  0..1
    const float breath = std::sin(idleTime * 2.2f) * (1.f - moveAmount);     // Breathing at rest

    // Hands
    if (texture.getSize().x >= static_cast<unsigned>(HAND_SIZE * (character + 1)) &&
        texture.getSize().y >= static_cast<unsigned>(HAND_SHEET_Y + HAND_SIZE)) {
        sf::Sprite hand(texture, sf::IntRect(character * HAND_SIZE, HAND_SHEET_Y, HAND_SIZE, HAND_SIZE));
        hand.setOrigin(HAND_SIZE / 2.f, HAND_SIZE / 2.f);

        constexpr float scale = PLAYER_DRAW_SIZE / FRAME_SIZE;
        hand.setScale(scale, scale);


        const sf::Vector2f aim = prisoner.getAimDirection();
        const sf::Vector2f side(-aim.y, aim.x);
        constexpr float radToDeg = 180.f / 3.14159265f;
        hand.setRotation(std::atan2(aim.y, aim.x) * radToDeg - 90.f);

        const sf::Vector2f center(pos.x, pos.y - bounce * 3.f + breath * 1.5f);

        hand.setPosition(center - side * HAND_SIDE + aim * (HAND_FORWARD + swing * HAND_SWING));
        window.draw(hand);
        hand.setPosition(center + side * HAND_SIDE + aim * (HAND_FORWARD - swing * HAND_SWING));
        window.draw(hand);
    }

    // Body
    if (texture.getSize().x >= static_cast<unsigned>(FRAME_SIZE * (character + 1))) {
        sf::Sprite body(texture, sf::IntRect(character * FRAME_SIZE, 0, FRAME_SIZE, FRAME_SIZE));
        body.setOrigin(FRAME_SIZE / 2.f, FRAME_SIZE / 2.f);

        constexpr float scale = PLAYER_DRAW_SIZE / FRAME_SIZE;
        body.setScale(scale * (1.f + 0.015f * breath), scale * (1.f - 0.015f * breath));
        body.setPosition(pos.x, pos.y - bounce * 3.f);
        window.draw(body);
    }
}