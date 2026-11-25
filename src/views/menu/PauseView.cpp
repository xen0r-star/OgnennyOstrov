#include "PauseView.h"

#include "Drawing.h"
#include <string>

static const std::string BUTTON_LABELS[3] = {"Reprendre", "Options", "Quitter la partie"};
static constexpr PauseAction BUTTON_ACTIONS[3] = {
    PauseAction::Resume, PauseAction::Options, PauseAction::Quit
};

// Clickable area of button number i
static sf::FloatRect buttonArea(const int i, const sf::Vector2f center) {
    return sf::FloatRect(center.x - 170, center.y - 70 + i * 76, 340, 56);
}

void PauseView::reset() {
    selection = 0;
}

PauseAction PauseView::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    const sf::Vector2f center = windowCenter(window);

    if (event.type == sf::Event::MouseMoved) {
        const sf::Vector2f mouse = mouseToView(window, event.mouseMove.x, event.mouseMove.y);
        for (int i = 0; i < 3; i++) {
            if (buttonArea(i, center).contains(mouse)) selection = i;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
        const sf::Vector2f mouse = mouseToView(window, event.mouseButton.x, event.mouseButton.y);
        for (int i = 0; i < 3; i++) {
            if (buttonArea(i, center).contains(mouse)) return BUTTON_ACTIONS[i];
        }
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Down)   selection = (selection + 1) % 3;
        if (event.key.code == sf::Keyboard::Up)     selection = (selection + 2) % 3;
        if (event.key.code == sf::Keyboard::Enter)  return BUTTON_ACTIONS[selection];
        if (event.key.code == sf::Keyboard::Escape) return PauseAction::Resume;
    }

    return PauseAction::None;
}

void PauseView::draw(sf::RenderWindow& window, const sf::Font& font) const {
    const sf::Vector2f size = window.getView().getSize();
    const sf::Vector2f center = windowCenter(window);

    // Darken the game behind the menu
    drawRectangle(window, center.x - size.x / 2, center.y - size.y / 2, size.x, size.y, sf::Color(0, 0, 0, 140));

    // Panel: same wall + stone look as the menu background
    const float x = center.x - 240;
    const float y = center.y - 190;
    drawRectangle(window, x,      y,      480, 380, INK);
    drawRectangle(window, x + 3,  y + 3,  474, 374, WALL);
    drawRectangle(window, x + 10, y + 10, 460, 360, INK);
    drawRectangle(window, x + 13, y + 13, 454, 354, STONE);
    drawGrid(window, x + 13, y + 13, 454, 354, STONE_LINE);

    drawText(window, font, "Pause", 56, center.x, center.y - 125, WALL_LIGHT, 4);

    for (int i = 0; i < 3; i++) {
        drawButton(window, font, buttonArea(i, center), BUTTON_LABELS[i], i == selection);
    }

    // Small green pawn in front of the selected button
    sf::CircleShape pawn(10);
    pawn.setFillColor(sf::Color(53, 194, 122));
    pawn.setOutlineThickness(4);
    pawn.setOutlineColor(sf::Color::White);
    pawn.setPosition(center.x - 208, buttonArea(selection, center).top + 18);
    window.draw(pawn);
}