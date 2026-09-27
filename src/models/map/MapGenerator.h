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
    std::vector<LayerTile> layers;
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
    using Point = std::pair<int, int>;

    // Type logique d'une case, utilisé pour le pathfinding et les règles
    // de génération. Indépendant du rendu (voir TileGrid ci-dessous).
    enum class Cell : unsigned char {
        Grass, Dirt, HouseFloor, Wall, Door, Path, Rail, Opening
    };

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
        int x, y, w, h;
        int right()  const { return x + w - 1; }
        int bottom() const { return y + h - 1; }
    };

    enum HouseType {
        HOUSE_BARRACKS = 0,
        HOUSE_MESS     = 1,
        HOUSE_STORAGE  = 2
    };

    struct House : Rect {
        int type = HOUSE_BARRACKS;
    };

    struct Door { int x, y; };

    struct TileCell { std::vector<LayerTile> layers; };
    using TileGrid = std::vector<std::vector<TileCell>>;

    using PromenadeFlags = std::vector<std::vector<bool>>;

    std::mt19937 rng_;
    Grid grid_;
    TileGrid tiles_;
    std::vector<House> houses_;
    std::vector<Door> doors_;
    PromenadeFlags isPromenade_;

    int margin_ = 0;
    int playX0_ = 0, playY0_ = 0, playX1_ = 0, playY1_ = 0;

    TileMap build(int height, int width, int seed);
    void initGrids(int totalWidth, int totalHeight);
    TileMap assembleResult() const;

    void addTile(int x, int y, Tile tile, int z);
    bool hasDecoration(int x, int y) const;

    static AngleTile pairAngle(bool a, bool b);
    static AngleTile singleAngle(bool a, bool b, bool c);
    AngleTile randomAngle();
    static bool wantsRandomRotation(const Tile& t);

    std::vector<Point> findRoute(Point start, Point goal,
                                  const std::function<bool(int, int)>& isBlocked) const;
    bool blocksPath(int x, int y) const;

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