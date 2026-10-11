#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

namespace {
    constexpr int CLEARANCE  = 1;  // allée d'herbe entre un champ et un chemin / bâtiment / rail
    constexpr int FIELD_GAP   = 1;  // allée d'herbe entre deux champs
    constexpr int EDGE_MARGIN = 1;  // allée d'herbe avec le bord de la zone de jeu
    constexpr int BIG_PERCENT = 100; // on tire parmi les tailles dont la surface >= ce % de la plus grande qui rentre

    struct Size { int w, h; };
    constexpr Size SIZES[] = {
        { .w = 10, .h = 8 },
        { .w = 8, .h = 10 },
        { .w = 8, .h = 6 },
        { .w = 6, .h = 8 },
        { .w = 6, .h = 5 },
        { .w = 5, .h = 6 },
    };
}

void MapGenerator::buildDirtZones(TileMap& map, std::mt19937& rng, const Rect& playZone) {
    constexpr int sizeCount = std::size(SIZES);

    // ---------- 2. Cases occupées ----------
    const int W = playZone.w;
    const int H = playZone.h;
    auto idx = [&](const int x, const int y) { return (y - playZone.y) * W + (x - playZone.x); };

    // obstacle[] : quelque chose est déjà posé (chemin, bâtiment, rail)
    std::vector<char> obstacle(W * H, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (!isTileEmpty(map, x, y, Map::ZIndex::Layer1) ||
                !isTileEmpty(map, x, y, Map::ZIndex::Layer2) ||
                !isTileEmpty(map, x, y, Map::ZIndex::Layer3)) {
                obstacle[idx(x, y)] = 1;
            }
        }
    }

    // blocked[] : case interdite aux champs (obstacle + allée autour + bord + champs déjà posés)
    std::vector<char> blocked(W * H, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            // Bord de la zone
            if (x < playZone.x + EDGE_MARGIN || x >= playZone.x + W - EDGE_MARGIN ||
                y < playZone.y + EDGE_MARGIN || y >= playZone.y + H - EDGE_MARGIN) {
                blocked[idx(x, y)] = 1;
            }
            if (!obstacle[idx(x, y)]) continue;

            // Allée autour de l'obstacle
            for (int dy = -CLEARANCE; dy <= CLEARANCE; ++dy) {
                for (int dx = -CLEARANCE; dx <= CLEARANCE; ++dx) {
                    const int nx = x + dx;
                    const int ny = y + dy;
                    if (nx < playZone.x || ny < playZone.y ||
                        nx >= playZone.x + W || ny >= playZone.y + H) continue;
                    blocked[idx(nx, ny)] = 1;
                }
            }
        }
    }

    // Le rectangle (x, y, w, h) est-il entièrement libre ?
    auto canPlace = [&](const int x, const int y, const int w, const int h) {
        if (x + w > playZone.x + W || y + h > playZone.y + H) return false;
        for (int yy = y; yy < y + h; ++yy) {
            for (int xx = x; xx < x + w; ++xx) {
                if (blocked[idx(xx, yy)]) return false;
            }
        }
        return true;
    };

    // ---------- 3. Balayage et pose des champs ----------
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (blocked[idx(x, y)]) continue;

            // Tailles qui rentrent avec ce coin en haut à gauche
            Size fit[sizeCount];
            int fitCount = 0;
            int bestArea = 0;
            for (const Size& s : SIZES) {
                if (!canPlace(x, y, s.w, s.h)) continue;
                fit[fitCount++] = s;
                bestArea = std::max(bestArea, s.w * s.h);
            }
            if (fitCount == 0) continue;

            // On garde les plus grandes (>= bigPercent % de la meilleure) et on en tire une
            Size pool[sizeCount];
            int poolCount = 0;
            for (int i = 0; i < fitCount; ++i) {
                if (fit[i].w * fit[i].h * 100 >= bestArea * BIG_PERCENT) pool[poolCount++] = fit[i];
            }
            std::uniform_int_distribution<int> pick(0, poolCount - 1);
            const Size chosen = pool[pick(rng)];

            // Poser la terre (Layer0 : remplace l'herbe)
            for (int yy = y; yy < y + chosen.h; ++yy) {
                for (int xx = x; xx < x + chosen.w; ++xx) {
                    addTile(map, xx, yy, Tiles::GROUND_DIRT, Map::ZIndex::Layer0);
                }
            }

            // Bloquer le champ + l'allée autour
            for (int yy = y - FIELD_GAP; yy < y + chosen.h + FIELD_GAP; ++yy) {
                for (int xx = x - FIELD_GAP; xx < x + chosen.w + FIELD_GAP; ++xx) {
                    if (xx < playZone.x || yy < playZone.y ||
                        xx >= playZone.x + W || yy >= playZone.y + H) continue;
                    blocked[idx(xx, yy)] = 1;
                }
            }
        }
    }
}