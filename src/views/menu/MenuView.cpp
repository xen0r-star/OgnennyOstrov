#include "MenuView.h"

#include "Drawing.h"
#include <string>

static const std::string BUTTON_LABELS[4] = {"Jouer", "Multijoueur", "Options", "Quitter"};
static constexpr MenuAction BUTTON_ACTIONS[4] = {
    MenuAction::Play, MenuAction::Multiplayer, MenuAction::Options, MenuAction::Quit
};

// Clickable area of button number i
static sf::FloatRect buttonArea(int i, sf::Vector2f center) {
    return sf::FloatRect(center.x - 170, center.y - 110 + i * 76, 340, 56);
}

MenuAction MenuView::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    const sf::Vector2f center = windowCenter(window);

    if (event.type == sf::Event::MouseMoved) {
        const sf::Vector2f mouse = mouseToView(window, event.mouseMove.x, event.mouseMove.y);
        for (int i = 0; i < 4; i++) {
            if (buttonArea(i, center).contains(mouse)) {
                selection = i;
            }
        }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
        const sf::Vector2f mouse = mouseToView(window, event.mouseButton.x, event.mouseButton.y);
        for (int i = 0; i < 4; i++) {
            if (buttonArea(i, center).contains(mouse)) {
                return BUTTON_ACTIONS[i];
            }
        }
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Down)  selection = (selection + 1) % 4;
        if (event.key.code == sf::Keyboard::Up)    selection = (selection + 3) % 4;
        if (event.key.code == sf::Keyboard::Enter) return BUTTON_ACTIONS[selection];
    }

    return MenuAction::None;
}

void MenuView::draw(sf::RenderWindow& window, const sf::Font& font) {
    const sf::Vector2f center = windowCenter(window);

    drawBackground(window);
    drawText(window, font, "OGNONNY OSTROV", 90, center.x, center.y - 210, WALL_LIGHT, 5);

    for (int i = 0; i < 4; i++) {
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