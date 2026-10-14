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

    static float angleToDegrees(Angle angle);
    void drawTile(sf::RenderWindow& window, const Tile& tile, int i, int j) const;
};

#endif // GOULAG_MAPVIEW_H