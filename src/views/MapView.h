#ifndef GOULAG_MAPVIEW_H
#define GOULAG_MAPVIEW_H

#include "../models/map/MapGenerator.h"
#include <SFML/Graphics.hpp>

class MapView {
public:
    MapView();
    ~MapView();

    void drawMap(sf::RenderWindow& window, const TileMap& map);

    // For testing
    void handleEvent(const sf::Event& event, sf::RenderWindow& window);
private:
    sf::Texture texture;

    static float angleToDegrees(AngleTile angle);
    void drawTile(sf::RenderWindow& window, const Tile& tile, int i, int j) const;

    // For testing
    sf::View view;
    bool isDragging = false;
    sf::Vector2i oldMousePos;
};

#endif // GOULAG_MAPVIEW_H