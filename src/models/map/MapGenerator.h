#ifndef GOULAG_MAPGENERATOR_H
#define GOULAG_MAPGENERATOR_H

#include <functional>
#include <random>
#include <utility>
#include <vector>

enum Angle {
    ANGLE_0,
    ANGLE_90,
    ANGLE_180,
    ANGLE_270
};

struct Tile {
    int col = 0;
    int row = 0;
    Angle rotation = ANGLE_0;

    // Constructeur constexpr indispensable pour GCC 8.1 / C++17
    constexpr Tile(int c = 0, int r = 0, Angle rot = ANGLE_0)
        : col(c), row(r), rotation(rot) {}
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
    inline constexpr Tile FLOOR_STONE_PLAIN         = Tile(0, 0);
    inline constexpr Tile FLOOR_STONE_TILES         = Tile(0, 1);
    inline constexpr Tile FLOOR_STONE_CORNER_DIAG   = Tile(0, 2);
    inline constexpr Tile FLOOR_STONE_CROSS         = Tile(0, 3);
    inline constexpr Tile FLOOR_STONE_PATTERN       = Tile(0, 4);
    inline constexpr Tile FLOOR_STONE_CRACKED       = Tile(0, 5);
    inline constexpr Tile FLOOR_STONE_CORNER_BEVEL  = Tile(1, 0);
    inline constexpr Tile FLOOR_WOOD_PLANKS         = Tile(1, 1);

    inline constexpr Tile GROUND_DIRT               = Tile(1, 2);
    inline constexpr Tile GROUND_GRASS              = Tile(1, 3);

    inline constexpr Tile WALL_STRAIGHT             = Tile(1, 4);
    inline constexpr Tile WALL_CORNER_OUTER         = Tile(1, 5);
    inline constexpr Tile WALL_STRAIGHT_DIAG        = Tile(2, 0);
    inline constexpr Tile WALL_WIDE_CURVE_OUTER     = Tile(2, 1);
    inline constexpr Tile WALL_CURVE_OUTER          = Tile(2, 2);
    inline constexpr Tile WALL_CURVE_INNER          = Tile(2, 3);
    inline constexpr Tile WALL_CORNER_INNER         = Tile(2, 4);
    inline constexpr Tile WALL_ROUNDED_END          = Tile(2, 5);
    inline constexpr Tile WALL_WINDOWS              = Tile(3, 0);
    inline constexpr Tile WALL_WEAK                 = Tile(3, 1);
    inline constexpr Tile WALL_STRAIGHT_SHORT       = Tile(3, 2);
    inline constexpr Tile WALL_DOORWAY_OPEN         = Tile(3, 3);
    inline constexpr Tile WALL_DOOR_FRAME           = Tile(3, 4);
    inline constexpr Tile WALL_DOORWAY_CLOSE        = Tile(3, 5);
    inline constexpr Tile WALL_BROKEN_DOOR          = Tile(4, 0);

    inline constexpr Tile RAIL_STRAIGHT             = Tile(4, 1);
    inline constexpr Tile RAIL_CURVE                = Tile(4, 2);
    inline constexpr Tile RAIL_CROSSROADS           = Tile(4, 3);
    inline constexpr Tile CART_WOODEN               = Tile(4, 4);

    inline constexpr Tile PATH_STRAIGHT_1           = Tile(4, 5);
    inline constexpr Tile PATH_CURVE_1              = Tile(5, 0);
    inline constexpr Tile PATH_CROSSROADS           = Tile(5, 1);
    inline constexpr Tile PATH_T_JUNCTION           = Tile(8, 5);
    inline constexpr Tile PATH_CURVE_2              = Tile(5, 2);
    inline constexpr Tile PATH_STRAIGHT_2           = Tile(5, 3);
    inline constexpr Tile PATH_ARROW                = Tile(5, 4);
    inline constexpr Tile PATH_END                  = Tile(5, 5);

    inline constexpr Tile PROP_CAMPFIRE             = Tile(6, 0);
    inline constexpr Tile PROP_GRASS_TUFT           = Tile(6, 1);
    inline constexpr Tile PROP_BUSH                 = Tile(6, 2);

    inline constexpr Tile OBJECT_LARGE_CRATE_WOODEN = Tile(6, 3);
    inline constexpr Tile OBJECT_LARGE_BARREL_TOP   = Tile(6, 4);
    inline constexpr Tile OBJECT_SMALL_CRATE_WOODEN = Tile(6, 5);
    inline constexpr Tile OBJECT_BARRELS_CLUSTER_1  = Tile(7, 0);
    inline constexpr Tile OBJECT_SMALL_BARREL_TOP   = Tile(7, 1);
    inline constexpr Tile OBJECT_BARRELS_CLUSTER_2  = Tile(7, 2);
    inline constexpr Tile OBJECT_TABLE              = Tile(7, 3);
    inline constexpr Tile OBJECT_CHAIR              = Tile(7, 4);
    inline constexpr Tile OBJECT_BOARD              = Tile(7, 5);
    inline constexpr Tile OBJECT_BED                = Tile(8, 0);
    inline constexpr Tile OBJECT_BED_2              = Tile(8, 1);

    inline constexpr Tile PROP_TREE                 = Tile(8, 2);
    inline constexpr Tile WALL_BARBED_WIRE          = Tile(8, 3);
    inline constexpr Tile WATCHTOWER                = Tile(8, 4);
}

using TileMap = std::vector<std::vector<MapCell>>;

class MapGenerator {
public:
    MapGenerator();
    ~MapGenerator();

    static TileMap generateMap(int height, int width, int seed);

private:
    using Point = std::pair<int, int>;

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

    static Angle pairAngle(bool a, bool b);
    static Angle singleAngle(bool a, bool b, bool c);
    Angle randomAngle();
    static bool wantsRandomRotation(const Tile& t);

    std::vector<Point> findRoute(Point start, Point goal,
                                  const std::function<bool(int, int)>& isBlocked) const;
    bool blocksPath(int x, int y) const;

    void buildMapBorder();
    void generateBaseTerrain();
    void generateHouses();
    void carveHouseFloors();
    void connectAdjacentHouses();
    void buildWalls();
    void placeDoors();
    void generateHousePaths();
    Point randomBoundaryPoint(int side);
    void generatePromenadePaths();
    void removePathBlocks();
    void autotilePathsUnified();
    void autotileRails();
    void generateRails();
    void placeDecorations();
};

#endif // GOULAG_MAPGENERATOR_H