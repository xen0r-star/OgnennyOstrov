#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <algorithm>
#include <random>
#include <vector>
#include <iostream>

namespace {
    constexpr int GAP         = 2;   // espace libre entre un bâtiment et les rails / autres bâtiments
    constexpr int PATH_GAP     = 0;   // espace libre avec les chemins de garde (0 = peut être collé)
    constexpr int EDGE_MARGIN  = 2;   // espace libre avec le bord de la zone (doit rester >= gap)
    constexpr int STEP        = 2;   // positions aléatoires multiples de 2 : aligne un peu l'ensemble
    constexpr int TRIES       = 150; // nombre d'essais par bâtiment avant d'abandonner
    constexpr int ALIGN_CHANCE = 35;  // % de chances de coller le bâtiment à un voisin déjà posé

    // Zone de camp : % de la largeur et de la hauteur de la map où on a le droit de construire.
    constexpr int CAMP_RATIO = 90;

    // Remplissage : on génère des rôles jusqu'à viser ce % de la surface libre du camp.
    constexpr int FILL_PERCENT = 55;
    // Nombre d'échecs consécutifs en passe 0 avant de considérer la zone saturée.
    constexpr int MAX_FAILS = 12;

    // Couverture du chemin de garde (passe 1 uniquement)
    constexpr int MAX_PATH_OVERLAP = 14; // nombre maximal de tuiles de chemin couvertes par un bâtiment
    constexpr int CROSS_PERCENT   = 15; // % maximal de bâtiments posés qui ont le droit de couvrir le chemin

    // Tailles par rôle (murs compris), dans l'ordre MESS, STORAGE, DORM
    constexpr int MIN_W[3] = { 10, 5, 6 };
    constexpr int MAX_W[3] = { 14, 8, 10 };
    constexpr int MIN_H[3] = { 7,  5, 5 };
    constexpr int MAX_H[3] = { 10, 7, 8 };

    // Surface moyenne par rôle (sert à estimer combien de bâtiments générer)
    constexpr int AVG_AREA[3] = { 100, 40, 52 };

    // Sols : réfectoire = FLOOR_STONE_CROSS, autres = FLOOR_STONE_TILES.
    // Une partie des FLOOR_STONE_TILES est remplacée par FLOOR_STONE_CRACKED.
    // Un bâtiment carré de côté impair reçoit un FLOOR_STONE_PATTERN au centre.
    constexpr int CRACKED_PERCENT = 15;
}

void MapGenerator::buildHouseFloors(TileMap& map, std::mt19937& rng, const Rect& playZone) {
    enum Role { MESS = 0, STORAGE = 1, DORM = 2 };

    // Bits qui décrivent le contenu d'une case (combinables)
    enum Cell { PATH = 1, RAIL = 2, HOUSE = 4 };

    // ---------- 2. Zone de camp ----------
    // Rectangle tiré au hasard dans la zone de jeu, à l'intérieur des marges de bord.
    int areaW = playZone.w - 2 * EDGE_MARGIN;
    int areaH = playZone.h - 2 * EDGE_MARGIN;
    if (areaW < 16 || areaH < 16) return; // zone trop petite pour construire

    int campW = std::min(areaW, std::max(16, areaW * CAMP_RATIO / 100));
    int campH = std::min(areaH, std::max(16, areaH * CAMP_RATIO / 100));

    std::uniform_int_distribution campXDist(0, areaW - campW);
    std::uniform_int_distribution campYDist(0, areaH - campH);
    int campX = playZone.x + EDGE_MARGIN + campXDist(rng);
    int campY = playZone.y + EDGE_MARGIN + campYDist(rng);

    // ---------- 3. Contenu des cases ----------
    // cell[] a la taille de la zone de jeu. L'index d'une case (x, y) est
    // (y - zone.y) * zone.w + (x - zone.x).
    std::vector cell(playZone.w * playZone.h, 0);
    int freeCount = 0; // cases libres à l'intérieur de la zone de camp

    for (int y = playZone.y; y < playZone.y + playZone.h; ++y) {
        for (int x = playZone.x; x < playZone.x + playZone.w; ++x) {
            int index = (y - playZone.y) * playZone.w + (x - playZone.x);

            // Layer1 = chemins de garde, Layer2 = rails.
            // (isTileEmpty renvoie true quand la case est vide, d'où le "!")
            if (!isTileEmpty(map, x, y, Map::ZIndex::Layer1)) cell[index] |= PATH;
            if (!isTileEmpty(map, x, y, Map::ZIndex::Layer2)) cell[index] |= RAIL;

            bool inCamp = x >= campX && x < campX + campW && y >= campY && y < campY + campH;
            if (inCamp && cell[index] == 0) ++freeCount;
        }
    }

    int pathCells = 0, railCells = 0;
    for (int c : cell) {
        if (c & PATH) ++pathCells;
        if (c & RAIL) ++railCells;
    }

    // ---------- 4. Liste des bâtiments à placer ----------
    // On tire des rôles jusqu'à atteindre la surface visée (fillPercent % de la surface libre).
    // La passe 0 s'arrête de toute façon quand la zone est saturée (maxFails).
    std::discrete_distribution rolePick({ 10.0, 25.0, 65.0 }); // MESS, STORAGE, DORM

    int targetArea = freeCount * FILL_PERCENT / 100;
    std::vector<int> roles;
    roles.push_back(MESS); // au moins un réfectoire
    int area = AVG_AREA[MESS];
    while (area < targetArea) {
        int r = rolePick(rng);
        roles.push_back(r);
        area += AVG_AREA[r];
    }

    // Les plus gros d'abord : ce sont les plus difficiles à caser (MESS = 0 < STORAGE = 1 < DORM = 2)
    std::stable_sort(roles.begin(), roles.end());

    // ---------- 5. Placement ----------
    // Bâtiments déjà posés : sert à aligner les suivants
    std::vector<int> houseX, houseY, houseW, houseH, houseRole;
    std::vector<int> failedRoles; // bâtiments sans place en passe 0, retentés en passe 1
    int countByRole[3] = { 0, 0, 0 };

    std::uniform_int_distribution percent(0, 99);
    std::uniform_int_distribution coin(0, 1);

    int crossCount  = 0; // bâtiments posés qui couvrent une partie du chemin
    int crossBudget = 0; // nombre maximal autorisé (calculé au début de la passe 1)

    for (int pass = 0; pass < 2; ++pass) {
        const std::vector<int> list = (pass == 0) ? roles : failedRoles;
        const bool allowPath = (pass == 1);

        if (pass == 1) {
            crossBudget = 1 + static_cast<int>(houseX.size()) * CROSS_PERCENT / 100;
        }

        int fails = 0; // échecs consécutifs (passe 0)

        for (int role : list) {
            if (pass == 0 && fails >= MAX_FAILS) break; // zone saturée

            std::uniform_int_distribution wDist(MIN_W[role], MAX_W[role]);
            std::uniform_int_distribution hDist(MIN_H[role], MAX_H[role]);
            bool placed = false;

            for (int attempt = 0; attempt < TRIES && !placed; ++attempt) {
                int w = wDist(rng);
                int h = hDist(rng);

                // a) Position candidate.
                // En passe 1, on ne tente que des positions alignées sur un voisin :
                // c'est l'alignement qui justifie de couvrir le chemin.
                bool aligned = (pass == 1) || (!houseX.empty() && percent(rng) < ALIGN_CHANCE);
                if (aligned && houseX.empty()) break; // rien à quoi s'aligner

                int x, y;
                if (aligned) {
                    // On colle le bâtiment à droite ou en dessous d'un bâtiment déjà posé
                    std::uniform_int_distribution pickHouse(0, static_cast<int>(houseX.size()) - 1);
                    int k = pickHouse(rng);
                    if (coin(rng) == 0) {
                        x = houseX[k] + houseW[k] + GAP;
                        y = houseY[k];
                    } else {
                        x = houseX[k];
                        y = houseY[k] + houseH[k] + GAP;
                    }
                } else {
                    // Position au hasard dans la zone de camp, multiple de "step"
                    int rangeX = (campW - w) / STEP;
                    int rangeY = (campH - h) / STEP;
                    if (rangeX < 0 || rangeY < 0) continue; // bâtiment trop grand pour le camp
                    std::uniform_int_distribution xDist(0, rangeX);
                    std::uniform_int_distribution yDist(0, rangeY);
                    x = campX + STEP * xDist(rng);
                    y = campY + STEP * yDist(rng);
                }

                // b) Le bâtiment doit rester entièrement dans la zone de camp
                if (x < campX || y < campY) continue;
                if (x + w > campX + campW || y + h > campY + campH) continue;

                // c) Vérifications.
                //    (la zone de camp est à au moins edgeMargin >= gap du bord : on ne sort jamais du tableau)
                int flags = 0;      // RAIL | HOUSE dans le rectangle agrandi de "gap"
                int pathNear = 0;   // PATH dans le rectangle agrandi de "pathGap"
                int pathTiles = 0;  // PATH sous le bâtiment lui-même

                for (int yy = y - GAP; yy < y + h + GAP; ++yy) {
                    for (int xx = x - GAP; xx < x + w + GAP; ++xx) {
                        int c = cell[(yy - playZone.y) * playZone.w + (xx - playZone.x)];
                        flags |= (c & (RAIL | HOUSE));

                        bool inside = xx >= x && xx < x + w && yy >= y && yy < y + h;
                        bool near   = xx >= x - PATH_GAP && xx < x + w + PATH_GAP &&
                                      yy >= y - PATH_GAP && yy < y + h + PATH_GAP;

                        if (inside && (c & PATH)) ++pathTiles;
                        if (near && (c & PATH))   ++pathNear;
                    }
                }

                if (flags) continue;                                      // jamais sur un rail, jamais collé à un autre bâtiment
                if (!allowPath && pathNear > 0) continue;                 // passe 0 : on évite les chemins
                if (pathTiles > MAX_PATH_OVERLAP) continue;                 // ne pas manger tout le chemin
                if (pathTiles > 0 && crossCount >= crossBudget) continue; // quota atteint

                // d) Marquer les cases comme occupées. Les dalles du sol sont posées plus bas,
                //    une fois tous les bâtiments placés (le placement ne dépend pas du type de sol).
                for (int yy = y; yy < y + h; ++yy) {
                    for (int xx = x; xx < x + w; ++xx) {
                        cell[(yy - playZone.y) * playZone.w + (xx - playZone.x)] |= HOUSE;
                    }
                }

                houseX.push_back(x);
                houseY.push_back(y);
                houseW.push_back(w);
                houseH.push_back(h);
                houseRole.push_back(role);
                ++countByRole[role];
                if (pathTiles > 0) ++crossCount;
                placed = true;
            }

            if (placed) fails = 0; else ++fails;
            if (!placed && pass == 0 && failedRoles.size() < 6) failedRoles.push_back(role);
        }
    }

    // ---------- 6. Pose des sols ----------
    for (size_t k = 0; k < houseX.size(); ++k) {
        const int x = houseX[k];
        const int y = houseY[k];
        const int w = houseW[k];
        const int h = houseH[k];
        const bool isMess = (houseRole[k] == MESS);

        for (int yy = y; yy < y + h; ++yy) {
            for (int xx = x; xx < x + w; ++xx) {
                Map::Tile tile = isMess ? Tiles::FLOOR_STONE_CROSS : Tiles::FLOOR_STONE_TILES;
                // 15 % des dalles FLOOR_STONE_TILES sont fissurées
                if (!isMess && percent(rng) < CRACKED_PERCENT) tile = Tiles::FLOOR_STONE_CRACKED;
                addTile(map, xx, yy, tile, Map::ZIndex::Layer1);
            }
        }

        // Bâtiment carré de côté impair : une dalle spéciale pile au centre
        if (w == h && w % 2 == 1) {
            addTile(map, x + w / 2, y + h / 2, Tiles::FLOOR_STONE_PATTERN, Map::ZIndex::Layer1);
        }
    }
}