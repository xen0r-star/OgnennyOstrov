#ifndef GOULAG_PATHGENERATOR_H
#define GOULAG_PATHGENERATOR_H

#include "models/map/Tile.h"
#include "models/map/generation/MapTypes.h"

namespace MapGen {

void generateHousePaths(TileMap& map);                                // Chemins entre les portes
void generatePromenadePaths(TileMap& map, Rng& rng, const Rect& play); // Grandes routes bord à bord
void removePathBlocks(TileMap& map);                                  // Supprime les blocs 2x2 de chemin
void autotilePathsUnified(TileMap& map);                              // Choix des tuiles (droit/courbe/croisement)

} // namespace MapGen

#endif // GOULAG_PATHGENERATOR_H
