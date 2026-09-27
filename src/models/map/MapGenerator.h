#ifndef GOULAG_MAPGENERATOR_H
#define GOULAG_MAPGENERATOR_H

#include <functional>
#include <random>
#include <utility>
#include <vector>

enum AngleTile {
    ANGLE_0,
    ANGLE_90,
    ANGLE_180,
    ANGLE_270
};

struct Tile {
    int col = 0;
    int row = 0;
    AngleTile rotation = ANGLE_0;
};

// ----------------------------------------------------------------------
// Système de rendu par z-index : une case peut porter plusieurs tuiles
// empilées (sol, structure, décor), dessinées de la plus basse à la plus
// haute z. Voir MapGenerator::assembleResult().
// ----------------------------------------------------------------------
namespace ZIndex {
    inline constexpr int Ground     = 0;
    inline constexpr int Structure  = 10;
    inline constexpr int Decoration = 20;
}

struct LayerTile {
    Tile tile;
    int z = ZIndex::Ground;
};

struct MapCell {
    std::vector<LayerTile> layers; // trié par z croissant
};

namespace TileType {
    inline constexpr Tile FLOOR_STONE_PLAIN         = {.col = 0, .row = 0};
    inline constexpr Tile FLOOR_STONE_TILES         = {.col = 0, .row = 1};
    inline constexpr Tile FLOOR_STONE_CORNER_DIAG   = {.col = 0, .row = 2};
    inline constexpr Tile FLOOR_STONE_CROSS         = {.col = 0, .row = 3};
    inline constexpr Tile FLOOR_STONE_PATTERN       = {.col = 0, .row = 4};
    inline constexpr Tile FLOOR_STONE_CRACKED       = {.col = 0, .row = 5};
    inline constexpr Tile FLOOR_STONE_CORNER_BEVEL  = {.col = 1, .row = 0};
    inline constexpr Tile FLOOR_WOOD_PLANKS         = {.col = 1, .row = 1};

    inline constexpr Tile GROUND_DIRT               = {.col = 1, .row = 2};
    inline constexpr Tile GROUND_GRASS              = {.col = 1, .row = 3};

    inline constexpr Tile WALL_STRAIGHT             = {.col = 1, .row = 4};
    inline constexpr Tile WALL_CORNER_OUTER         = {.col = 1, .row = 5};
    inline constexpr Tile WALL_STRAIGHT_DIAG        = {.col = 2, .row = 0};
    inline constexpr Tile WALL_WIDE_CURVE_OUTER     = {.col = 2, .row = 1};
    inline constexpr Tile WALL_CURVE_OUTER          = {.col = 2, .row = 2};
    inline constexpr Tile WALL_CURVE_INNER          = {.col = 2, .row = 3};
    inline constexpr Tile WALL_CORNER_INNER         = {.col = 2, .row = 4};
    inline constexpr Tile WALL_ROUNDED_END          = {.col = 2, .row = 5};
    inline constexpr Tile WALL_WINDOWS              = {.col = 3, .row = 0};
    inline constexpr Tile WALL_WEAK                 = {.col = 3, .row = 1};
    inline constexpr Tile WALL_STRAIGHT_SHORT       = {.col = 3, .row = 2};
    inline constexpr Tile WALL_DOORWAY_OPEN         = {.col = 3, .row = 3};
    inline constexpr Tile WALL_DOOR_FRAME           = {.col = 3, .row = 4};
    inline constexpr Tile WALL_DOORWAY_CLOSE        = {.col = 3, .row = 5};
    inline constexpr Tile WALL_BROKEN_DOOR          = {.col = 4, .row = 0};

    inline constexpr Tile RAIL_STRAIGHT             = {.col = 4, .row = 1};
    inline constexpr Tile RAIL_CURVE                = {.col = 4, .row = 2};
    inline constexpr Tile RAIL_CROSSROADS           = {.col = 4, .row = 3};
    inline constexpr Tile CART_WOODEN               = {.col = 4, .row = 4};

    inline constexpr Tile PATH_STRAIGHT_1           = {.col = 4, .row = 5};
    inline constexpr Tile PATH_CURVE_1              = {.col = 5, .row = 0};
    inline constexpr Tile PATH_CROSSROADS           = {.col = 5, .row = 1};
    inline constexpr Tile PATH_T_JUNCTION           = {.col = 8, .row = 5};
    inline constexpr Tile PATH_CURVE_2              = {.col = 5, .row = 2};
    inline constexpr Tile PATH_STRAIGHT_2           = {.col = 5, .row = 3};
    inline constexpr Tile PATH_ARROW                = {.col = 5, .row = 4};
    inline constexpr Tile PATH_END                  = {.col = 5, .row = 5};

    inline constexpr Tile PROP_CAMPFIRE             = {.col = 6, .row = 0};
    inline constexpr Tile PROP_GRASS_TUFT           = {.col = 6, .row = 1};
    inline constexpr Tile PROP_BUSH                 = {.col = 6, .row = 2};

    inline constexpr Tile OBJECT_LARGE_CRATE_WOODEN = {.col = 6, .row = 3};
    inline constexpr Tile OBJECT_LARGE_BARREL_TOP   = {.col = 6, .row = 4};
    inline constexpr Tile OBJECT_SMALL_CRATE_WOODEN = {.col = 6, .row = 5};
    inline constexpr Tile OBJECT_BARRELS_CLUSTER_1  = {.col = 7, .row = 0};
    inline constexpr Tile OBJECT_SMALL_BARREL_TOP   = {.col = 7, .row = 1};
    inline constexpr Tile OBJECT_BARRELS_CLUSTER_2  = {.col = 7, .row = 2};
    inline constexpr Tile OBJECT_TABLE              = {.col = 7, .row = 3};
    inline constexpr Tile OBJECT_CHAIR              = {.col = 7, .row = 4};
    inline constexpr Tile OBJECT_BOARD              = {.col = 7, .row = 5};
    inline constexpr Tile OBJECT_BED                = {.col = 8, .row = 0};
    inline constexpr Tile OBJECT_BED_2              = {.col = 8, .row = 1};

    inline constexpr Tile PROP_TREE                 = {.col = 8, .row = 2};
    inline constexpr Tile WALL_BARBED_WIRE          = {.col = 8, .row = 3};
    inline constexpr Tile WATCHTOWER                = {.col = 8, .row = 4};
}

using TileMap = std::vector<std::vector<MapCell>>;

class MapGenerator {
public:
    MapGenerator();
    ~MapGenerator();

    static TileMap generateMap(int height, int width, int seed);

private:
    // ------------------------------------------------------------------
    // Types internes du pipeline de génération (implémentation only).
    // ------------------------------------------------------------------
    using Point = std::pair<int, int>;

    enum class Cell : unsigned char {
        Grass, Dirt, HouseFloor, Wall, Door, Path, Rail, Opening
    };

    // Grille logique : un type de case par tuile (herbe, mur, chemin...).
    struct Grid {
        int width = 0, height = 0;
        std::vector<std::vector<Cell>> cell;

        Grid() = default;
        Grid(int w, int h) : width(w), height(h), cell(w, std::vector<Cell>(h, Cell::Grass)) {}

        bool inBounds(int x, int y) const { return x >= 0 && x < width && y >= 0 && y < height; }
        Cell& at(int x, int y) { return cell[x][y]; }
        Cell  at(int x, int y) const { return cell[x][y]; }
    };

    struct Rect {
        int x, y, w, h; // (x, y) = coin haut-gauche, en coordonnées [col, row]
        int right()  const { return x + w - 1; }
        int bottom() const { return y + h - 1; }
    };

    // Une maison = un rectangle + un "type" qui détermine son mobilier
    // (voir placeDecorations()).
    enum HouseType {
        HOUSE_BARRACKS = 0, // lits alignés
        HOUSE_MESS     = 1, // tables / chaises + tonneau
        HOUSE_STORAGE  = 2  // caisses / tonneaux
    };

    struct House : Rect {
        int type = HOUSE_BARRACKS;
    };

    struct Door { int x, y; };

    // Sol (herbe, terre, dallage...) : une seule tuile par case, en z = Ground.
    using GroundGrid = std::vector<std::vector<Tile>>;

    // Superposition (murs, portes, chemins, décor...) : chaque case peut
    // porter plusieurs tuiles empilées (voir setStructure() / addDecoration()).
    struct OverlayCell { std::vector<LayerTile> layers; };
    using OverlayGrid = std::vector<std::vector<OverlayCell>>;

    // Pour chaque case de chemin, indique si elle appartient au réseau
    // "promenade" (style 2) ou "maison" (style 1) — voir autotilePathsUnified().
    using PromenadeFlags = std::vector<std::vector<bool>>;

    // ------------------------------------------------------------------
    // État de la génération en cours. Tout ce qui était auparavant passé
    // en paramètre d'une fonction à l'autre (grille, RNG, maisons,
    // portes...) devient un attribut, rempli au fil du pipeline par
    // build() et lu par les étapes ci-dessous.
    // ------------------------------------------------------------------
    std::mt19937 rng_;
    Grid grid_;
    GroundGrid ground_;
    OverlayGrid overlay_;
    std::vector<House> houses_;
    std::vector<Door> doors_;
    PromenadeFlags isPromenade_;

    int margin_ = 0;                                   // épaisseur de la bande d'arbres
    int playX0_ = 0, playY0_ = 0, playX1_ = 0, playY1_ = 0; // zone jouable (intérieur de la clôture)

    // ------------------------------------------------------------------
    // Orchestration.
    // ------------------------------------------------------------------
    TileMap build(int height, int width, int seed);
    void initGrids(int totalWidth, int totalHeight);
    TileMap assembleResult() const;

    // ------------------------------------------------------------------
    // Aides bas niveau (superposition, orientation, pathfinding).
    // ------------------------------------------------------------------
    void setStructure(int x, int y, Tile tile);
    void addDecoration(int x, int y, Tile tile);
    bool hasAnyOverlay(int x, int y) const;

    static AngleTile cornerAngle(bool top, bool left);
    static AngleTile sideAngle(bool top, bool right, bool bottom, bool left);
    static AngleTile curveAngle(bool n, bool e, bool s, bool w);
    static AngleTile tJunctionAngle(bool n, bool e, bool s, bool w);
    AngleTile randomAngle();
    static bool wantsRandomRotation(const Tile& t);

    std::vector<Point> findRoute(Point start, Point goal,
                                  const std::function<bool(int, int)>& isBlocked) const;
    bool blocksPath(int x, int y) const;

    // ------------------------------------------------------------------
    // Étapes du pipeline de génération, dans leur ordre d'exécution.
    // ------------------------------------------------------------------
    void buildPerimeterFence();
    void placeTreeBorder();
    int  generateBaseTerrain();
    void generateHouses(int housesY0);
    void carveHouseFloors();
    void connectAdjacentHouses();
    void buildWalls();
    void placeDoors();
    void generateHousePaths();
    Point randomBoundaryPoint(int side);
    void generatePromenadePaths();
    void autotilePathsUnified();
    void autotileRails();
    void generateRails();
    void placeDecorations();
};

#endif // GOULAG_MAPGENERATOR_H