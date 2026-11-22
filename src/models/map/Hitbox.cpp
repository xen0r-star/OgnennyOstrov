#include "models/map/Hitbox.h"

#include <algorithm>

namespace {
    // Point of the segment [a, b] that is the closest to p
    sf::Vector2f closestPointOnSegment(const sf::Vector2f p, const sf::Vector2f a, const sf::Vector2f b) {
        const sf::Vector2f ab = b - a;
        const float lengthSquared = ab.x * ab.x + ab.y * ab.y;
        if (lengthSquared == 0.f) return a;

        const float t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lengthSquared;
        return a + ab * std::clamp(t, 0.f, 1.f);
    }

    // Convex polygon: p is inside if it is on the same side of every edge
    bool isInside(const sf::Vector2f p, const Hitbox::Polygon& polygon) {
        const std::size_t count = polygon.points.size();
        bool left = false;
        bool right = false;

        for (std::size_t i = 0; i < count; ++i) {
            const sf::Vector2f a = polygon.points[i];
            const sf::Vector2f b = polygon.points[(i + 1) % count];

            const float side = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
            if (side > 0.f) left = true;
            if (side < 0.f) right = true;
        }
        return !(left && right);
    }
}

bool Hitbox::circleTouchesPolygon(const sf::Vector2f center, const float radius, const Polygon& polygon) {
    const std::size_t count = polygon.points.size();
    if (count < 3) return false;

    // Case 1: the center is inside the polygon
    if (isInside(center, polygon)) return true;

    // Case 2: an edge of the polygon is closer to the center than the radius
    for (std::size_t i = 0; i < count; ++i) {
        const sf::Vector2f closest = closestPointOnSegment(center, polygon.points[i], polygon.points[(i + 1) % count]);
        const float dx = closest.x - center.x;
        const float dy = closest.y - center.y;
        if (dx * dx + dy * dy < radius * radius) return true;
    }
    return false;
}
