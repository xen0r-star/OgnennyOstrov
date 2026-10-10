#include "models/map/generation/DecorationGenerator.h"

#include <algorithm>
#include <iterator>

#include "models/map/Tiles.h"
#include "models/map/generation/TileHelpers.h"
#include "models/map/generation/TileMapUtils.h"

namespace MapGen {

namespace {

constexpr int DX[4] = {0, 1, 0, -1};
constexpr int DY[4] = {-1, 0, 1, 0};

// Sol extérieur libre (pas de chemin, rail, mur...)
bool isNextToDoor(const TileMap& map, const int x, const int y) {
    for (int k = 0; k < 4; ++k)
        if (structureAt(map, x + DX[k], y + DY[k]) == DOOR) return true;
    return false;
}

} // namespace

void placeDecorations(TileMap& map, Rng& rng, const Rect& play, const std::vector<House>& houses) {
    std::uniform_real_distribution chance(0.f, 1.f);

    auto place = [&](int x, int y, Tile tile) {
        if (wantsRandomRotation(tile)) tile.rotation = randomAngle(rng);
        addTile(map, x, y, tile, ZIndex::Layer2);
    };

    // Parcourt toute la zone jouable
    auto forEachPlayCell = [&](auto&& fn) {
        for (int x = play.x; x <= play.right(); ++x)
            for (int y = play.y; y <= play.bottom(); ++y)
                fn(x, y);
    };

    // --- Végétation extérieure ---
    const Tile outdoorProps[] = {
        Tiles::PROP_BUSH, Tiles::PROP_BUSH,
        Tiles::PROP_GRASS_TUFT, Tiles::PROP_GRASS_TUFT, Tiles::PROP_GRASS_TUFT,
    };
    std::uniform_int_distribution propPick(0, (int)std::size(outdoorProps) - 1);

    forEachPlayCell([&](int x, int y) {
        if (isOpenGround(map, x, y) && chance(rng) < 0.06f) place(x, y, outdoorProps[propPick(rng)]);
    });
    forEachPlayCell([&](int x, int y) {
        if (isOpenGround(map, x, y) && !hasDecoration(map, x, y) && chance(rng) < 0.05f)
            place(x, y, Tiles::PROP_BUSH);
    });
    forEachPlayCell([&](int x, int y) {
        if (isOpenGround(map, x, y) && !hasDecoration(map, x, y) && chance(rng) < 0.08f)
            place(x, y, Tiles::PROP_GRASS_TUFT);
    });

    // --- Stocks extérieurs ---
    const Tile storageProps[] = {
        Tiles::OBJECT_LARGE_CRATE_WOODEN, Tiles::OBJECT_LARGE_BARREL_TOP,
        Tiles::OBJECT_BARRELS_CLUSTER_1, Tiles::OBJECT_BARRELS_CLUSTER_2,
        Tiles::OBJECT_SMALL_CRATE_WOODEN, Tiles::OBJECT_SMALL_BARREL_TOP
    };
    std::uniform_int_distribution stockpilePick(0, (int)std::size(storageProps) - 1);
    const int stockpileCount = std::clamp(play.w * play.h / 100, 4, 14);
    std::uniform_int_distribution anyX(play.x, play.right());
    std::uniform_int_distribution anyY(play.y, play.bottom());

    for (int i = 0; i < stockpileCount; ++i) {
        for (int tries = 0; tries < 25; ++tries) {
            int x = anyX(rng), y = anyY(rng);
            if (isOpenGround(map, x, y) && !hasDecoration(map, x, y)) {
                place(x, y, storageProps[stockpilePick(rng)]);
                break;
            }
        }
    }

    // --- Feu de camp ---
    for (int tries = 0; tries < 15; ++tries) {
        int x = anyX(rng), y = anyY(rng);
        if (isOpenGround(map, x, y) && groundAt(map, x, y) == GRASS && !hasDecoration(map, x, y)) {
            place(x, y, Tiles::PROP_CAMPFIRE);
            break;
        }
    }

    // --- Intérieur des bâtiments ---
    // La case devant une porte reste libre : on la détecte directement via les tuiles de porte.
    auto isFreeFloor = [&](int x, int y) {
        return !hasStructure(map, x, y) && groundAt(map, x, y) == HOUSE
            && !isNextToDoor(map, x, y);
    };

    for (const House& h : houses) {
        if (h.type == HOUSE_BARRACKS) {
            bool altRow = false;
            for (int y = h.y + 1; y < h.bottom(); ++y) {
                Tile bed = altRow ? Tiles::OBJECT_BED_2 : Tiles::OBJECT_BED;
                for (int x = h.x + 1; x < h.right(); ++x)
                    if (isFreeFloor(x, y)) place(x, y, bed);
                altRow = !altRow;
            }
        } else if (h.type == HOUSE_MESS) {
            for (int x = h.x + 1; x < h.right(); ++x) {
                bool tableColumn = ((x - (h.x + 1)) % 2 == 0);
                for (int y = h.y + 1; y < h.bottom(); ++y)
                    if (isFreeFloor(x, y))
                        place(x, y, tableColumn ? Tiles::OBJECT_TABLE : Tiles::OBJECT_CHAIR);
            }
            const int bx = h.right() - 1, by = h.bottom() - 1;
            if (isFreeFloor(bx, by)) place(bx, by, Tiles::OBJECT_BARRELS_CLUSTER_1);
        } else { // HOUSE_STORAGE
            for (int x = h.x + 1; x < h.right(); ++x)
                for (int y = h.y + 1; y < h.bottom(); ++y)
                    if (isFreeFloor(x, y) && chance(rng) < 0.35f)
                        place(x, y, storageProps[std::uniform_int_distribution<int>(0, (int)std::size(storageProps) - 1)(rng)]);
        }
    }
}

} // namespace MapGen
