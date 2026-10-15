#include "OptionsView.h"

#include "Drawing.h"
#include "core/Settings.h"

#include <iostream>

// Button areas, relative to the window center
static sf::FloatRect brightnessMinus(const sf::Vector2f c)   { return {c.x - 160, c.y - 165, 56, 56}; }
static sf::FloatRect brightnessPlus(const sf::Vector2f c)    { return {c.x + 104, c.y - 165, 56, 56}; }
static sf::FloatRect volumeMinus(const sf::Vector2f c)       { return {c.x - 160, c.y - 55, 56, 56};  }
static sf::FloatRect volumePlus(const sf::Vector2f c)        { return {c.x + 104, c.y - 55, 56, 56};  }
static sf::FloatRect characterPrevious(const sf::Vector2f c) { return {c.x - 160, c.y + 70, 56, 56};  }
static sf::FloatRect characterNext(const sf::Vector2f c)     { return {c.x + 104, c.y + 70, 56, 56};  }
static sf::FloatRect backButton(const sf::Vector2f c)        { return {c.x - 100, c.y + 210, 200, 56};}

static constexpr int CHARACTER_SIZE = 90;    // size of one frame in the sheet
static constexpr int CHARACTER_COUNT = 4;
static constexpr float PREVIEW_SIZE = 72.f;  // size displayed in the menu

OptionsView::OptionsView() {
    if (!spriteSheet.loadFromFile("assets/characters/player.png")) {
        std::cerr << "Cannot load assets/characters/player.png" << std::endl;
    }
}

void OptionsView::handleEvent(const sf::Event& event, const sf::RenderWindow& window, Settings& settings) {
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) back = true;
    if (event.type != sf::Event::MouseButtonPressed) return;

    const sf::Vector2f c = windowCenter(window);
    const sf::Vector2f mouse = mouseToView(window, event.mouseButton.x, event.mouseButton.y);
    const float x = mouse.x;
    const float y = mouse.y;

    if (brightnessMinus(c).contains(x, y))    settings.changeBrightness(-10);
    if (brightnessPlus(c).contains(x, y))     settings.changeBrightness(+10);
    if (volumeMinus(c).contains(x, y))        settings.changeVolume(-10);
    if (volumePlus(c).contains(x, y))         settings.changeVolume(+10);
    if (characterPrevious(c).contains(x, y))  settings.character = (settings.character + CHARACTER_COUNT - 1) % CHARACTER_COUNT;
    if (characterNext(c).contains(x, y))      settings.character = (settings.character + 1) % CHARACTER_COUNT;
    if (backButton(c).contains(x, y))         back = true;
}

void OptionsView::draw(sf::RenderWindow& window, const sf::Font& font, const Settings& settings) {
    const sf::Vector2f c = windowCenter(window);

    drawBackground(window);
    drawText(window, font, "Options", 46, c.x, c.y - 255, WALL_LIGHT, 3);

    // Brightness
    drawText(window, font, "Luminosité", 24, c.x, c.y - 185);
    drawButton(window, font, brightnessMinus(c), "-");
    drawButton(window, font, brightnessPlus(c), "+");
    drawText(window, font, std::to_string(settings.brightness) + " %", 30, c.x, c.y - 137);

    // Volume
    drawText(window, font, "Son", 24, c.x, c.y - 75);
    drawButton(window, font, volumeMinus(c), "-");
    drawButton(window, font, volumePlus(c), "+");
    drawText(window, font, std::to_string(settings.volume) + " %", 30, c.x, c.y - 27);

    // Character
    drawText(window, font, "Personnage", 24, c.x, c.y + 35);
    drawButton(window, font, characterPrevious(c), "<");
    drawButton(window, font, characterNext(c), ">");

    if (spriteSheet.getSize().x > 0) {  // the image was loaded correctly
        const sf::IntRect frame(settings.character * CHARACTER_SIZE, 0, CHARACTER_SIZE, CHARACTER_SIZE);

        sf::Sprite sprite(spriteSheet, frame);
        sprite.setOrigin(CHARACTER_SIZE / 2.f, CHARACTER_SIZE / 2.f);
        sprite.setScale(PREVIEW_SIZE / CHARACTER_SIZE, PREVIEW_SIZE / CHARACTER_SIZE);
        sprite.setPosition(c.x, c.y + 98);
        window.draw(sprite);
    }

    drawButton(window, font, backButton(c), "Retour");
}