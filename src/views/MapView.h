#ifndef GOULAG_MAPVIEW_H
#define GOULAG_MAPVIEW_H

#include "../models/map/MapGenerator.h"
#include <SFML/Graphics.hpp>

class MapView {
public:
    MapView();
    ~MapView();

    void drawMap(sf::RenderWindow& window, const TileMap& map);
private:
    sf::Texture texture;

    void drawTile(sf::RenderWindow& window, const Map::Tile& tile, int i, int j) const;

    // For testing
    sf::View view;
    bool viewInitialized = false;
    bool isDragging = false;
    sf::Vector2i oldMousePos;
};

#endif // GOULAG_MAPVIEW_H