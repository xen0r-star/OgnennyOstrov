#include "FogView.h"

#include <algorithm>
#include <cmath>
#include <vector>

void FogView::draw(sf::RenderWindow& window, const sf::Vector2f& worldFocus) {
    const sf::View gameView = window.getView();
    const sf::Vector2f size(window.getSize());
    const sf::Vector2f focus(window.mapCoordsToPixel(worldFocus, gameView));

    if (size.y != builtForHeight || size.x != builtForWidth) {
        build(size);
    }

    window.setView(sf::View(sf::FloatRect(0.f, 0.f, size.x, size.y)));
    sf::RenderStates states;
    states.transform.translate(focus);
    window.draw(vertices, states);
    window.setView(gameView);
}

void FogView::build(const sf::Vector2f size) {
    builtForWidth = size.x;
    builtForHeight = size.y;

    std::vector<float> radius;
    std::vector<int> alpha;
    radius.push_back(0.f);                   alpha.push_back(0);
    radius.push_back(CLEAR_RADIUS * size.y); alpha.push_back(0);

    for (int i = 1; i <= FADE_STEPS; ++i) {
        const float t = static_cast<float>(i) / FADE_STEPS;
        const float smooth = t * t * (3.f - 2.f * t);
        radius.push_back((CLEAR_RADIUS + (FOG_RADIUS - CLEAR_RADIUS) * t) * size.y);
        alpha.push_back(static_cast<int>(MAX_ALPHA * smooth));
    }
    radius.push_back((size.x + size.y) * 2.f); alpha.push_back(MAX_ALPHA);

    vertices.setPrimitiveType(sf::Triangles);
    vertices.clear();

    for (std::size_t r = 0; r + 1 < radius.size(); ++r) {
        for (int s = 0; s < SEGMENTS; ++s) {
            constexpr float PI = 3.14159265f;

            const float a0 = PI * 2 * static_cast<float>(s) / SEGMENTS;
            const float a1 = PI * 2 * static_cast<float>(s + 1) / SEGMENTS;

            const sf::Vertex v00(point(radius[r], a0),     color(alpha[r]));
            const sf::Vertex v01(point(radius[r], a1),     color(alpha[r]));
            const sf::Vertex v10(point(radius[r + 1], a0), color(alpha[r + 1]));
            const sf::Vertex v11(point(radius[r + 1], a1), color(alpha[r + 1]));

            vertices.append(v00); vertices.append(v10); vertices.append(v11);
            vertices.append(v00); vertices.append(v11); vertices.append(v01);
        }
    }
}

sf::Vector2f FogView::point(const float r, const float angle) {
    return {std::cos(angle) * r, std::sin(angle) * r};
}

sf::Color FogView::color(const int alpha) {
    return {
        static_cast<sf::Uint8>(FOG_RED),
        static_cast<sf::Uint8>(FOG_GREEN),
        static_cast<sf::Uint8>(FOG_BLUE),
        static_cast<sf::Uint8>(std::clamp(alpha, 0, 255))
    };
}