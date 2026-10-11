#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <algorithm>
#include <random>
#include <vector>

namespace {
    constexpr int WIDTH_MIN = 10;  // distance minimale entre deux rails parallèles
    constexpr int WIDTH_MAX = 20;  // distance maximale (avant répartition du reste)
    constexpr int MARGIN   = 4;   // bord libre autour de la boucle
    constexpr int COVERAGE = 70;  // % des nœuds utilisés (100 = partout, plus bas = plus de poches vides)
}

void MapGenerator::addRailPaths(TileMap &map, std::mt19937 &rng, const Rect &playZone) {
    // Bits combinables : sert à décrire dans quelles directions part une tuile
    enum Direction {
        UP = 1,
        RIGHT = 2,
        DOWN = 4,
        LEFT = 8
    };

    std::uniform_int_distribution widthDist(WIDTH_MIN, WIDTH_MAX);

    // ---------- 1. Positions des lignes de la grille ----------
    // pos[0] = positions en x, pos[1] = positions en y (relatives au coin de la forme).
    std::vector<int> pos[2];
    int size[2] = { playZone.w - 1 - 2 * MARGIN, playZone.h - 1 - 2 * MARGIN };

    for (int axis = 0; axis < 2; ++axis) {
        // a) Tirer des largeurs tant qu'elles rentrent dans la zone
        std::vector<int> widths;
        int total = 0;
        while (true) {
            int width = widthDist(rng);
            if (total + width > size[axis]) break;
            total += width;
            widths.push_back(width);
        }

        // b) Nombre IMPAIR de colonnes : nœud, liaison, nœud, ..., nœud
        if (widths.size() % 2 == 0 && !widths.empty()) {
            total -= widths.back();
            widths.pop_back();
        }
        if (widths.size() < 3) return; // zone trop petite : pas assez de place pour 2 nœuds

        // c) Répartir l'espace restant, 1 tuile à la fois, pour que la grille remplisse la zone
        int leftover = size[axis] - total;
        for (int i = 0; i < leftover; ++i) {
            widths[i % widths.size()] += 1;
        }

        // d) Largeurs -> positions cumulées
        int sum = 0;
        pos[axis].push_back(0);
        for (int width : widths) {
            sum += width;
            pos[axis].push_back(sum);
        }
    }

    int gw = static_cast<int>(pos[0].size()) - 1; // nombre de colonnes (nœuds + liaisons)
    int gh = static_cast<int>(pos[1].size()) - 1; // nombre de lignes (nœuds + liaisons)
    int nw = (gw + 1) / 2;                        // nombre de nœuds en largeur
    int nh = (gh + 1) / 2;                        // nombre de nœuds en hauteur

    int ox = playZone.x + MARGIN;
    int oy = playZone.y + MARGIN;

    // ---------- 2. Arbre aléatoire (Randomized DFS) ----------
    // solid[] marque les cases de la grille qui font partie de la forme (1 = pleine).
    // Bordure vide de 1 case autour (d'où gw + 2 et les "+ 1") pour lire les voisines sans sortir.
    int pw = gw + 2;
    std::vector<int> solid(pw * (gh + 2), 0);
    std::vector<int> visited(nw * nh, 0);
    std::vector<int> stackI, stackJ;

    // Nombre de nœuds visé : on s'arrête avant d'avoir tout visité pour laisser des poches vides
    int target = std::max(2, nw * nh * COVERAGE / 100);
    int visitedCount = 1;

    // Départ sur un nœud au hasard (les chemins de garde partent du coin, pas les rails)
    std::uniform_int_distribution<int> startI(0, nw - 1);
    std::uniform_int_distribution<int> startJ(0, nh - 1);
    int si = startI(rng);
    int sj = startJ(rng);

    visited[sj * nw + si] = 1;
    solid[(2 * sj + 1) * pw + (2 * si + 1)] = 1;
    stackI.push_back(si);
    stackJ.push_back(sj);

    while (!stackI.empty() && visitedCount < target) {
        int i = stackI.back();
        int j = stackJ.back();

        // Lister les voisins de ce nœud qui ne sont pas encore dans l'arbre
        int candI[4], candJ[4];
        int count = 0;
        for (int k = 0; k < 4; ++k) {
            constexpr int dj[4] = {0, 0, 1, -1};
            constexpr int di[4] = {1, -1, 0, 0};
            int ni = i + di[k];
            int nj = j + dj[k];

            if (ni >= 0 && nj >= 0 && ni < nw && nj < nh && visited[nj * nw + ni] == 0) {
                candI[count] = ni;
                candJ[count] = nj;
                ++count;
            }
        }

        // Cul-de-sac : retour au nœud précédent
        if (count == 0) {
            stackI.pop_back();
            stackJ.pop_back();
            continue;
        }

        std::uniform_int_distribution<int> pick(0, count - 1);
        int c = pick(rng);
        int ni = candI[c];
        int nj = candJ[c];

        // Nœud (i, j) = case (2i + 1, 2j + 1) dans solid[] ; la liaison est pile entre deux nœuds
        visited[nj * nw + ni] = 1;
        ++visitedCount;
        solid[(2 * nj + 1) * pw + (2 * ni + 1)] = 1;   // le nouveau nœud
        solid[(j + nj + 1) * pw + (i + ni + 1)] = 1;   // la liaison entre les deux
        stackI.push_back(ni);
        stackJ.push_back(nj);
    }

    // ---------- 3. Contour de la forme (boundary tracing) ----------
    // On note, pour chaque coin de case, le coin suivant du contour (sens horaire).
    int vw = gw + 1;
    std::vector<int> nextCorner(vw * (gh + 1), -1); // -1 = aucun contour ne part de ce coin

    for (int cy = 0; cy < gh; ++cy) {
        for (int cx = 0; cx < gw; ++cx) {
            if (solid[(cy + 1) * pw + (cx + 1)] == 0) continue;

            int up    = solid[cy * pw + (cx + 1)];
            int down  = solid[(cy + 2) * pw + (cx + 1)];
            int left  = solid[(cy + 1) * pw + cx];
            int right = solid[(cy + 1) * pw + (cx + 2)];

            if (up == 0)    nextCorner[cy * vw + cx]             = cy * vw + (cx + 1);
            if (right == 0) nextCorner[cy * vw + (cx + 1)]       = (cy + 1) * vw + (cx + 1);
            if (down == 0)  nextCorner[(cy + 1) * vw + (cx + 1)] = (cy + 1) * vw + cx;
            if (left == 0)  nextCorner[(cy + 1) * vw + cx]       = cy * vw + cx;
        }
    }

    int start = -1;
    for (int v = 0; v < (int)nextCorner.size(); ++v) {
        if (nextCorner[v] >= 0) { start = v; break; }
    }
    if (start < 0) return;

    // ---------- 4. Liste ordonnée des tuiles de la boucle ----------
    std::vector<int> cellsX, cellsY;
    int v = start;
    do {
        int nv = nextCorner[v];

        // Coin de départ et coin d'arrivée du segment, en coordonnées de la map
        int sx = ox + pos[0][v % vw];
        int sy = oy + pos[1][v / vw];
        int ex = ox + pos[0][nv % vw];
        int ey = oy + pos[1][nv / vw];

        // Direction de marche (-1, 0 ou 1 sur chaque axe)
        int dx = (ex > sx) - (ex < sx);
        int dy = (ey > sy) - (ey < sy);

        // Le coin d'arrivée est ajouté par le segment suivant (pas de doublon)
        while (sx != ex || sy != ey) {
            cellsX.push_back(sx);
            cellsY.push_back(sy);
            sx += dx;
            sy += dy;
        }
        v = nv;
    } while (v != start);

    // ---------- 5. Poser les rails ----------
    // Le "% n" referme la boucle : la voisine de la dernière case est la première.
    int n = static_cast<int>(cellsX.size());
    for (int i = 0; i < n; ++i) {
        int cx = cellsX[i];
        int cy = cellsY[i];

        int neighX[2] = { cellsX[(i + n - 1) % n], cellsX[(i + 1) % n] };
        int neighY[2] = { cellsY[(i + n - 1) % n], cellsY[(i + 1) % n] };

        // Masque de bits : dans quelles directions part le rail depuis cette case ?
        int mask = 0;
        for (int k = 0; k < 2; ++k) {
            if (neighX[k] > cx)      mask |= RIGHT;
            else if (neighX[k] < cx) mask |= LEFT;
            else if (neighY[k] > cy) mask |= DOWN;
            else                     mask |= UP;
        }

        // Mêmes rotations que les chemins (à ajuster si ton tileset de rails est orienté autrement)
        Map::Tile tile;
        switch (mask) {
            case UP | DOWN:    tile = Tiles::RAIL_STRAIGHT; tile.modifier.rotation = 0;   break;
            case LEFT | RIGHT: tile = Tiles::RAIL_STRAIGHT; tile.modifier.rotation = 90;  break;
            case RIGHT | DOWN: tile = Tiles::RAIL_CURVE;    tile.modifier.rotation = 0;   break;
            case LEFT | DOWN:  tile = Tiles::RAIL_CURVE;    tile.modifier.rotation = 90;  break;
            case LEFT | UP:    tile = Tiles::RAIL_CURVE;    tile.modifier.rotation = 180; break;
            case RIGHT | UP:   tile = Tiles::RAIL_CURVE;    tile.modifier.rotation = 270; break;
            default: continue;
        }
        addTile(map, cx, cy, tile, Map::ZIndex::Layer2);
    }
}