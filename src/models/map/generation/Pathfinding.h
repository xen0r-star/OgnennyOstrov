#ifndef GOULAG_PATHFINDING_H
#define GOULAG_PATHFINDING_H

#include <functional>
#include <vector>

#include "models/map/Tile.h"
#include "models/map/generation/MapTypes.h"

namespace MapGen {

// Dijkstra avec pénalité de virage (favorise les chemins droits).
std::vector<Point> findRoute(const TileMap& map, Point start, Point goal,
                             const std::function<bool(int, int)>& isBlocked);

} // namespace MapGen

#endif // GOULAG_PATHFINDING_H
