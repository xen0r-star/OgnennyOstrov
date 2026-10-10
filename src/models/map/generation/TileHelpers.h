#ifndef GOULAG_TILEHELPERS_H
#define GOULAG_TILEHELPERS_H

#include "models/map/Tile.h"
#include "models/map/generation/MapTypes.h"

namespace MapGen {

Angle pairAngle(bool a, bool b);
Angle singleAngle(bool a, bool b, bool c);
Angle randomAngle(Rng& rng);

// Même tuile dans l'atlas (col/row), la rotation est ignorée.
bool sameTile(const Tile& a, const Tile& b);
bool wantsRandomRotation(const Tile& t);

} // namespace MapGen

#endif // GOULAG_TILEHELPERS_H
