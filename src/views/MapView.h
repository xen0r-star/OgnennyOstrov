#ifndef OGNENNYOSTROV_MAPVIEW_H
#define OGNENNYOSTROV_MAPVIEW_H

    #include "../models/map/MapGenerator.h"
#include <SFML/Graphics.hpp>

class MapView {
public:
    MapView();
    ~MapView();

    void drawGround(sf::RenderWindow& window, const Map& map) const;  // below the player
    void drawOverlay(sf::RenderWindow& window, const Map& map) const; // above the player: bushes, then Layer2/3
private:
    enum class Filter { All, RailOnly, NoRail, BushOnly, NoBush };

    sf::Texture texture;

    void drawPass(sf::RenderWindow& window, const Map& map, int z, Filter filter) const;
    void drawTile(sf::RenderWindow& window, const GameMap::Tile& tile, int x, int y) const;
};

#endif // OGNENNYOSTROV_MAPVIEW_H