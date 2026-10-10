#ifndef GOULAG_HOUSESTRUCTURE_H
#define GOULAG_HOUSESTRUCTURE_H

#include <vector>

#include "models/map/Tile.h"
#include "models/map/generation/MapTypes.h"

namespace MapGen {

void buildWalls(TileMap& map, Rng& rng, const std::vector<House>& houses); // Murs, coins, fenêtres
void placeDoors(TileMap& map, Rng& rng, const std::vector<House>& houses); // Portes

} // namespace MapGen

#endif // GOULAG_HOUSESTRUCTURE_H
