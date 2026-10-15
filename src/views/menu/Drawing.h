#ifndef GOULAG_DRAWING_H
#define GOULAG_DRAWING_H

#include <SFML/Graphics.hpp>
#include <string>

// Game colors
const sf::Color GRASS(200, 213, 203);
const sf::Color GRASS_LINE(180, 195, 184);
const sf::Color STONE(90, 103, 111);
const sf::Color STONE_LINE(75, 87, 94);
const sf::Color WALL(163, 139, 62);
const sf::Color WALL_LIGHT(194, 168, 88);
const sf::Color INK(42, 38, 32);
const sf::Color WOOD(122, 82, 66);
const sf::Color WOOD_LIGHT(148, 102, 84);
const sf::Color WOOD_DARK(61, 42, 34);
const sf::Color PARCHMENT(243, 236, 217);

inline void drawRectangle(
    sf::RenderWindow& window,
    const float x, const float y,
    const float width, const float height,
    const sf::Color color
) {
    sf::RectangleShape shape(sf::Vector2f(width, height));
    shape.setPosition(x, y);
    shape.setFillColor(color);
    window.draw(shape);
}

// Text centered on (x, y)
inline void drawText(
    sf::RenderWindow& window,
    const sf::Font& font, const std::string& message,
    const unsigned int size, const float x, const float y,
    const sf::Color color = PARCHMENT, const float outline = 0
) {
    sf::Text text(sf::String::fromUtf8(message.begin(), message.end()), font, size);
    text.setFillColor(color);
    text.setOutlineColor(INK);
    text.setOutlineThickness(outline);

    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width / 2, bounds.top + bounds.height / 2);
    text.setPosition(x, y);
    window.draw(text);
}

// Wooden button with a shadow and a label
inline void drawButton(
    sf::RenderWindow& window,
    const sf::Font& font, const sf::FloatRect area,
    const std::string& message, const bool hovered = false
) {
    drawRectangle(window, area.left, area.top + 5, area.width, area.height, WOOD_DARK);
    drawRectangle(window, area.left, area.top, area.width, area.height, INK);
    drawRectangle(window, area.left + 3, area.top + 3, area.width - 6, area.height - 6,
                  hovered ? WOOD_LIGHT : WOOD);
    drawText(window, font, message, 30, area.left + area.width / 2, area.top + area.height / 2);
}

inline void drawGrid(
    sf::RenderWindow& window,
    const float x, const float y,
    const float width, const float height,
    const sf::Color color
) {
    for (float i = 0; i < width; i += 44) {
        drawRectangle(window, x + i, y, 2, height, color);
    }

    for (float j = 0; j < height; j += 44) {
        drawRectangle(window, x, y + j, width, 2, color);
    }
}

// Center of the window in pixels: every menu is laid out relative to it
inline sf::Vector2f windowCenter(const sf::RenderWindow& window) {
    const sf::Vector2u size = window.getSize();
    return sf::Vector2f(size.x / 2.f, size.y / 2.f);
}

// Background shared by the menus: grass + stone room surrounded by a wall
inline void drawBackground(sf::RenderWindow& window) {
    const sf::Vector2u size = window.getSize();
    const sf::Vector2f c = windowCenter(window);

    window.clear(GRASS);
    drawGrid(window, 0, 0, static_cast<float>(size.x), static_cast<float>(size.y), GRASS_LINE);

    const float x = c.x - 252;
    const float y = c.y - 322;
    drawRectangle(window, x,      y,      504, 644, INK);
    drawRectangle(window, x + 3,  y + 3,  498, 638, WALL);
    drawRectangle(window, x + 19, y + 19, 466, 606, INK);
    drawRectangle(window, x + 22, y + 22, 460, 600, STONE);
    drawGrid(window, x + 22, y + 22, 460, 600, STONE_LINE);
}

inline void drawBrightnessVeil(sf::RenderWindow& window, const int brightness) {
    const sf::View gameView = window.getView();
    const sf::Vector2f size(window.getSize());
    window.setView(sf::View(sf::FloatRect(0.f, 0.f, size.x, size.y)));

    sf::RectangleShape veil(size);
    veil.setFillColor(sf::Color(0, 0, 0, 255 * (100 - brightness) / 100));
    window.draw(veil);

    window.setView(gameView);
}

#endif //GOULAG_DRAWING_H