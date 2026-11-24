#ifndef OGNENNYOSTROV_FOGVIEW_H
#define OGNENNYOSTROV_FOGVIEW_H

#include <SFML/Graphics.hpp>

class FogView {
public:
    static constexpr float CLEAR_RADIUS = 0.30f;
    static constexpr float FOG_RADIUS = 0.85f;
    static constexpr int MAX_ALPHA = 225;
    static constexpr int FOG_RED = 8, FOG_GREEN = 10, FOG_BLUE = 18; // #080A12

    void draw(sf::RenderWindow& window, const sf::Vector2f& worldFocus);

private:
    static constexpr int SEGMENTS = 48;
    static constexpr int FADE_STEPS = 10;

    void build(sf::Vector2f size);
    static sf::Vector2f point(float r, float angle);
    static sf::Color color(int alpha);

    sf::VertexArray vertices;
    float builtForWidth = 0.f;
    float builtForHeight = 0.f;
};

#endif // OGNENNYOSTROV_FOGVIEW_H