#ifndef GOULAG_HOUSELAYOUT_H
#define GOULAG_HOUSELAYOUT_H

#include <vector>

#include "models/map/Tile.h"
#include "models/map/generation/MapTypes.h"

namespace MapGen {

// Calcule l'emplacement des bâtiments (plan uniquement, ne modifie pas la map).
std::vector<House> generateHouses(const TileMap& map, Rng& rng, const Rect& play);

void carveHouseFloors(TileMap& map, Rng& rng, const std::vector<House>& houses);      // Sols intérieurs
void connectAdjacentHouses(TileMap& map, Rng& rng, const std::vector<House>& houses); // Ouvertures entre bâtiments voisins

} // namespace MapGen

#endif // GOULAG_HOUSELAYOUT_H
