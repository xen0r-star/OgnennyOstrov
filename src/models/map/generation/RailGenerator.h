#ifndef GOULAG_RAILGENERATOR_H
#define GOULAG_RAILGENERATOR_H

#include "models/map/Tile.h"
#include "models/map/generation/MapTypes.h"

namespace MapGen {

void generateRails(TileMap& map, Rng& rng, const Rect& play);

} // namespace MapGen

#endif // GOULAG_RAILGENERATOR_H
