#ifndef OGNENNYOSTROV_HITBOX_H
#define OGNENNYOSTROV_HITBOX_H

#include <SFML/System/Vector2.hpp>
#include <vector>

namespace Hitbox {
    struct Polygon {
        std::vector<sf::Vector2f> points;
    };

    bool circleTouchesPolygon(sf::Vector2f center, float radius, const Polygon& polygon);
}

#endif // OGNENNYOSTROV_HITBOX_H
