#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

namespace {
    constexpr double TREE_PERCENT = 1.5;  // rare
    constexpr double BUSH_PERCENT = 5.0;  // assez fréquent
    constexpr double TUFT_PERCENT = 12.0; // très fréquent

    constexpr int CAMPFIRE_MIN       = 1;   // nombre minimal de feux de camp
    constexpr int CAMPFIRE_MAX       = 3;   // nombre maximal
    constexpr int CAMPFIRE_CLEARANCE = 2;   // rayon de cases vides exigé autour
    constexpr int CAMPFIRE_MIN_DIST   = 12;  // distance minimale (en cases) entre deux feux
    constexpr int CAMPFIRE_TRIES     = 500; // nombre d'essais de position

    constexpr int BIG_CLEARANCE = 1;   // rayon de cases vides exigé autour d'un arbre

    struct Style {
        Map::Tile tile;
        float offset;
        float scaleMin;
        float scaleMax;
        bool  big;
    };

    constexpr Style CAMPFIRE {
        .tile = Tiles::PROP_CAMPFIRE,
        .offset = 6.f,
        .scaleMin = 0.9f,
        .scaleMax = 1.1f,
        .big = true
    };

    constexpr Style TREE {
        .tile = Tiles::PROP_TREE,
        .offset = 12.f,
        .scaleMin = 0.85f,
        .scaleMax = 1.25f,
        .big = true
    };

    constexpr Style BUSH {
        .tile = Tiles::PROP_BUSH,
        .offset = 16.f,
        .scaleMin = 0.8f,
        .scaleMax = 1.3f,
        .big = false
    };

    constexpr Style TUFT {
        .tile = Tiles::PROP_GRASS_TUFT,
        .offset = 20.f,
        .scaleMin = 0.7f,
        .scaleMax = 1.2f,
        .big = false
    };
}

void MapGenerator::addDecorations(TileMap& map, std::mt19937& rng, const Rect& playZone) {
    // Variation du Modifier par type : décalage max (±), échelle min et max

    // ---------- 2. Outils ----------
    const int W = playZone.w;
    const int H = playZone.h;
    auto idx = [&](const int x, const int y) { return (y - playZone.y) * W + (x - playZone.x); };
    auto inZone = [&](const int x, const int y) {
        return x >= playZone.x && y >= playZone.y && x < playZone.x + W && y < playZone.y + H;
    };

    // Le sol de la case est-il de l'herbe ?
    auto isGrass = [&](const int x, const int y) {
        for (const Map::Tile& t : map.cells[y][x].layers) {
            if (t.zIndex == Map::ZIndex::Layer0 &&
                t.texture.col == Tiles::GROUND_GRASS.texture.col &&
                t.texture.row == Tiles::GROUND_GRASS.texture.row) {
                return true;
            }
        }
        return false;
    };

    // Rien de posé sur la case (Layer1 à Layer3)
    auto isEmpty = [&](const int x, const int y) {
        return isTileEmpty(map, x, y, Map::ZIndex::Layer1) &&
               isTileEmpty(map, x, y, Map::ZIndex::Layer2) &&
               isTileEmpty(map, x, y, Map::ZIndex::Layer3);
    };

    std::vector<char> big(W * H, 0); // gros éléments déjà posés

    // Cases vides dans un rayon autour de (x, y), et aucun gros élément collé
    auto roomForBig = [&](const int x, const int y, const int radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                const int nx = x + dx;
                const int ny = y + dy;
                if (!inZone(nx, ny)) continue;
                if (!isEmpty(nx, ny) || big[idx(nx, ny)]) return false;
            }
        }
        return true;
    };

    std::uniform_real_distribution<double> roll(0.0, 100.0);
    std::uniform_int_distribution<int> angle(0, 3);
    std::uniform_real_distribution<float> unit(-1.f, 1.f);
    std::uniform_real_distribution<float> unit01(0.f, 1.f);

    // Donne à une tuile sa rotation, son décalage et sa taille, puis la pose
    auto place = [&](const Style& style, const int x, const int y) {
        Map::Tile tile = style.tile;
        tile.modifier.rotation = angle(rng) * 90;
        tile.modifier.offsetX  = unit(rng) * style.offset;
        tile.modifier.offsetY  = unit(rng) * style.offset;
        tile.modifier.scale    = style.scaleMin + unit01(rng) * (style.scaleMax - style.scaleMin);
        addTile(map, x, y, tile, Map::ZIndex::Layer1);
        if (style.big) big[idx(x, y)] = 1;
    };

    int campfireCount = 0;

    // ---------- 3. Feux de camp ----------
    // Posés en premier, à des positions aléatoires, assez loin les uns des autres.
    std::uniform_int_distribution<int> wantedDist(CAMPFIRE_MIN, CAMPFIRE_MAX);
    std::uniform_int_distribution<int> randX(playZone.x, playZone.x + W - 1);
    std::uniform_int_distribution<int> randY(playZone.y, playZone.y + H - 1);
    std::vector<int> fireX, fireY;
    const int wanted = wantedDist(rng);

    for (int attempt = 0; attempt < CAMPFIRE_TRIES && campfireCount < wanted; ++attempt) {
        const int x = randX(rng);
        const int y = randY(rng);
        if (!isGrass(x, y) || !isEmpty(x, y) || !roomForBig(x, y, CAMPFIRE_CLEARANCE)) continue;

        bool farEnough = true;
        for (size_t k = 0; k < fireX.size(); ++k) {
            if (std::max(std::abs(x - fireX[k]), std::abs(y - fireY[k])) < CAMPFIRE_MIN_DIST) {
                farEnough = false;
                break;
            }
        }
        if (!farEnough) continue;

        place(CAMPFIRE, x, y);
        fireX.push_back(x);
        fireY.push_back(y);
        ++campfireCount;
    }

    // ---------- 4. Balayage : arbres, buissons, touffes ----------
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (!isGrass(x, y) || !isEmpty(x, y)) continue;

            const double r = roll(rng);
            const Style* style = nullptr;
            double limit = TREE_PERCENT;
            if (r < limit)                              style = &TREE;
            else if (r < (limit += BUSH_PERCENT))        style = &BUSH;
            else if (r < (limit += TUFT_PERCENT))        style = &TUFT;
            if (!style) continue;

            if (style->big && !roomForBig(x, y, BIG_CLEARANCE)) continue;

            place(*style, x, y);
        }
    }
}