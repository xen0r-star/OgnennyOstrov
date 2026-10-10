#ifndef GOULAG_TERRAINGENERATOR_H
#define GOULAG_TERRAINGENERATOR_H

#include "models/map/Tile.h"
#include "models/map/generation/MapTypes.h"

namespace MapGen {

// Zones de terre battue (cours du camp).
void generateBaseTerrain(TileMap& map, Rng& rng, const Rect& play);

} // namespace MapGen

#endif // GOULAG_TERRAINGENERATOR_H
