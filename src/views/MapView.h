#ifndef GOULAG_MAPVIEW_H
#define GOULAG_MAPVIEW_H

#include "../models/map/MapGenerator.h"
#include <SFML/Graphics.hpp>

class MapView {
public:
    MapView();
    ~MapView();

    void drawMap(sf::RenderWindow& window, const Map& map) const;
private:
    sf::Texture texture;

    void drawTile(sf::RenderWindow& window, const GameMap::Tile& tile, int x, int y) const;
};

#endif // GOULAG_MAPVIEW_H