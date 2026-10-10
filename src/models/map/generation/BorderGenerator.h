#ifndef GOULAG_BORDERGENERATOR_H
#define GOULAG_BORDERGENERATOR_H

#include "models/map/Tile.h"
#include "models/map/generation/MapTypes.h"

namespace MapGen {

// Murs/miradors du camp + végétation dans la marge extérieure.
void buildMapBorder(TileMap& map, Rng& rng, int margin);

} // namespace MapGen

#endif // GOULAG_BORDERGENERATOR_H
