#ifndef GOULAG_DECORATIONGENERATOR_H
#define GOULAG_DECORATIONGENERATOR_H

#include <vector>

#include "models/map/Tile.h"
#include "models/map/generation/MapTypes.h"

namespace MapGen {

void placeDecorations(TileMap& map, Rng& rng, const Rect& play, const std::vector<House>& houses);

} // namespace MapGen

#endif // GOULAG_DECORATIONGENERATOR_H
