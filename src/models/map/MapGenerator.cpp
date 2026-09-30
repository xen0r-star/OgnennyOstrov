#include "models/map/MapGenerator.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <queue>
#include <tuple>

MapGenerator::MapGenerator() = default;
MapGenerator::~MapGenerator() = default;

TileMap MapGenerator::generateMap(const int height, const int width, const int seed) {
    MapGenerator generator;
    return generator.build(height, width, seed);
}



void MapGenerator::initGrids(const int totalWidth, const int totalHeight) {
    grid_ = Grid(totalWidth, totalHeight);
    tiles_.assign(totalWidth, std::vector<TileCell>(totalHeight));

    for (int x = 0; x < totalWidth; ++x) {
        for (int y = 0; y < totalHeight; ++y) {
            addTile(x, y, TileType::GROUND_GRASS, ZIndex::Ground);
        }
    }
}

void MapGenerator::addTile(const int x, const int y, const Tile tile, int z) {
    auto& layers = tiles_[x][y].layers;
    if (z != ZIndex::Decoration) {
        layers.erase(std::remove_if(layers.begin(), layers.end(),
                     [z](const LayerTile& l) { return l.z == z; }),
                     layers.end());
    }
    layers.push_back({.tile = tile, .z = z});
}

bool MapGenerator::hasDecoration(const int x, const int y) const {
    for (const auto &[tile, z] : tiles_[x][y].layers) {
        if (z == ZIndex::Decoration) return true;
    }
    return false;
}

Angle MapGenerator::pairAngle(const bool a, const bool b) {
    if (a && b)   return ANGLE_0;
    if (a && !b)  return ANGLE_90;
    if (!a && !b) return ANGLE_180;
    return ANGLE_270;
}

Angle MapGenerator::singleAngle(const bool a, const bool b, const bool c) {
    if (a) return ANGLE_0;
    if (b) return ANGLE_90;
    if (c) return ANGLE_180;
    return ANGLE_270;
}

Angle MapGenerator::randomAngle() {
    static std::uniform_int_distribution d(0, 3);
    switch (d(rng_)) {
        case 0:  return ANGLE_0;
        case 1:  return ANGLE_90;
        case 2:  return ANGLE_180;
        default: return ANGLE_270;
    }
}

bool MapGenerator::wantsRandomRotation(const Tile& t) {
    auto same = [&](const Tile& ref) { return t.col == ref.col && t.row == ref.row; };
    return same(TileType::PROP_CAMPFIRE)            || same(TileType::PROP_GRASS_TUFT) ||
           same(TileType::PROP_BUSH)                || same(TileType::OBJECT_BOARD) ||
           same(TileType::OBJECT_LARGE_CRATE_WOODEN) || same(TileType::OBJECT_LARGE_BARREL_TOP) ||
           same(TileType::OBJECT_SMALL_CRATE_WOODEN)  || same(TileType::OBJECT_SMALL_BARREL_TOP) ||
           same(TileType::OBJECT_BARRELS_CLUSTER_1)   || same(TileType::OBJECT_BARRELS_CLUSTER_2) ||
           same(TileType::PROP_TREE);
}

bool MapGenerator::blocksPath(const int x, const int y) const {
    const Cell c = grid_.at(x, y);
    return c == Cell::Wall || c == Cell::HouseFloor || c == Cell::Door ||
           c == Cell::Opening || c == Cell::Dirt;
}

std::vector<MapGenerator::Point> MapGenerator::findRoute(
    Point start, Point goal, const std::function<bool(int, int)>& isBlocked) const {
    static const int dx[4] = {1, -1, 0, 0};
    static const int dy[4] = {0, 0, 1, -1};
    const int W = grid_.width, H = grid_.height;
    auto idx = [&](int x, int y) { return y * W + x; };

    std::vector dist(
        (size_t)W * H, std::array<int, 4>{INT_MAX, INT_MAX, INT_MAX, INT_MAX});
    std::vector prevPos(
        (size_t)W * H, std::array<Point, 4>{Point{-1, -1}, Point{-1, -1}, Point{-1, -1}, Point{-1, -1}});
    std::vector prevDir(
        (size_t)W * H, std::array<int, 4>{-1, -1, -1, -1});

    using State = std::tuple<int, int, int, int>; // cost, x, y, dir
    std::priority_queue<State, std::vector<State>, std::greater<>> pq;

    for (int d = 0; d < 4; ++d) {
        dist[idx(start.first, start.second)][d] = 0;
        pq.emplace(0, start.first, start.second, d);
    }

    while (!pq.empty()) {
        auto [cost, x, y, d] = pq.top();
        pq.pop();
        if (cost > dist[idx(x, y)][d]) continue;
        if (x == goal.first && y == goal.second) break;

        for (int nd = 0; nd < 4; ++nd) {
            int nx = x + dx[nd], ny = y + dy[nd];
            if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;

            bool isGoal = (nx == goal.first && ny == goal.second);
            if (!isGoal && isBlocked(nx, ny)) continue;

            const int turnPenalty = (d == nd) ? 0 : 2;
            int newCost = cost + 1 + turnPenalty;
            if (newCost < dist[idx(nx, ny)][nd]) {
                dist[idx(nx, ny)][nd] = newCost;
                prevPos[idx(nx, ny)][nd] = {x, y};
                prevDir[idx(nx, ny)][nd] = d;
                pq.emplace(newCost, nx, ny, nd);
            }
        }
    }

    int bestDir = -1, bestCost = INT_MAX;
    for (int d = 0; d < 4; ++d) {
        int c = dist[idx(goal.first, goal.second)][d];
        if (c < bestCost) { bestCost = c; bestDir = d; }
    }
    if (bestDir == -1) return {};

    std::vector<Point> path;
    int x = goal.first, y = goal.second, d = bestDir;
    while (!(x == start.first && y == start.second)) {
        path.emplace_back(x, y);
        Point p = prevPos[idx(x, y)][d];
        int pd = prevDir[idx(x, y)][d];
        if (p.first == -1) break;
        x = p.first; y = p.second; d = pd;
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}



void MapGenerator::buildMapBorder() {
    const int x0 = margin_, y0 = margin_;
    const int x1 = grid_.width - 1 - margin_, y1 = grid_.height - 1 - margin_;
    if (x1 - x0 < 4 || y1 - y0 < 4) return;

    const int watchtowerStride = 6;
    auto isCorner = [&](int x, int y) {
        return (x == x0 || x == x1) && (y == y0 || y == y1);
    };

    // Horizontal wall (top / bottom)
    for (int x = x0; x <= x1; ++x) {
        for (int y : {y0, y1}) {
            grid_.at(x, y) = Cell::Wall;
            bool tower = isCorner(x, y) || ((x - x0) % watchtowerStride == 0);
            Tile t = tower ? TileType::WATCHTOWER : TileType::WALL_BARBED_WIRE;
            if (!tower) t.rotation = ANGLE_0;
            addTile(x, y, t, ZIndex::Structure);
        }
    }
    // Vertical wall (left / right)
    for (int y = y0 + 1; y < y1; ++y) {
        for (int x : {x0, x1}) {
            grid_.at(x, y) = Cell::Wall;
            bool tower = (y - y0) % watchtowerStride == 0;
            Tile t = tower ? TileType::WATCHTOWER : TileType::WALL_BARBED_WIRE;
            if (!tower) t.rotation = ANGLE_90;
            addTile(x, y, t, ZIndex::Structure);
        }
    }


    // Add vegetation
    std::uniform_real_distribution chance(0.f, 1.f);

    constexpr Tile vegetationProps[] = {
        TileType::PROP_TREE, TileType::PROP_TREE, TileType::PROP_TREE, TileType::PROP_TREE,
        TileType::PROP_BUSH,
        TileType::PROP_GRASS_TUFT, TileType::PROP_GRASS_TUFT, TileType::PROP_GRASS_TUFT,
    };
    std::uniform_int_distribution propPick(0, (int)(sizeof(vegetationProps) / sizeof(Tile)) - 1);

    for (int x = 0; x < grid_.width; ++x) {
        for (int y = 0; y < grid_.height; ++y) {
            bool inMargin = x < margin_ || x >= grid_.width - margin_ ||
                            y < margin_ || y >= grid_.height - margin_;
            if (!inMargin) continue;

            grid_.at(x, y) = Cell::Wall;
            Tile t = vegetationProps[propPick(rng_)];

            if (chance(rng_) < 0.4f) {
                t.rotation = randomAngle();
                addTile(x, y, t, ZIndex::Decoration);
            }
        }
    }
}

void MapGenerator::generateBaseTerrain() {
    const int x0 = playX0_, y0 = playY0_, x1 = playX1_, y1 = playY1_;
    const int areaW = x1 - x0 + 1;
    const int areaH = y1 - y0 + 1;
    if (areaW < 10 || areaH < 10) return;

    auto fillDirt = [&](const Rect& r) {
        for (int x = std::max(x0, r.x); x <= std::min(x1, r.right()); ++x)
            for (int y = std::max(y0, r.y); y <= std::min(y1, r.bottom()); ++y) {
                if (grid_.at(x, y) == Cell::Dirt) continue;
                grid_.at(x, y) = Cell::Dirt;
                addTile(x, y, TileType::GROUND_DIRT, ZIndex::Ground);
            }
    };

    const int maxSize = std::max(3, std::min(areaW, areaH) / 4);
    std::uniform_int_distribution sizeDist(3, maxSize);
    const int w   = std::min(sizeDist(rng_), areaW);
    const int h   = std::min(sizeDist(rng_), areaH);
    const int gap = std::uniform_int_distribution(1, 2)(rng_);

    const bool horizontal = std::uniform_int_distribution(0, 1)(rng_) == 0;

    const int mainSize  = horizontal ? areaW : areaH;
    const int crossSize = horizontal ? areaH : areaW;
    const int mainBase  = horizontal ? x0 : y0;
    const int crossBase = horizontal ? y0 : x0;
    const int mainLen   = horizontal ? w : h;
    const int crossLen  = horizontal ? h : w;

    int count = std::uniform_int_distribution(2, 3)(rng_);
    while (count > 1 && count * mainLen + (count - 1) * gap > mainSize) --count;

    const int total = count * mainLen + (count - 1) * gap;
    int pos      = mainBase  + std::uniform_int_distribution(0, mainSize  - total)(rng_);
    int crossPos = crossBase + std::uniform_int_distribution(0, crossSize - crossLen)(rng_);

    int maxBottom = y0 - 1;
    for (int i = 0; i < count; ++i) {
        Rect r = horizontal ? Rect{pos, crossPos, w, h}
                            : Rect{crossPos, pos, w, h};
        fillDirt(r);
        maxBottom = std::max(maxBottom, r.bottom());
        pos += mainLen + gap;
    }
}

void MapGenerator::generateHouses() {
    const int x0 = playX0_, y0 = playY0_, x1 = playX1_, y1 = playY1_;
    const int areaW = x1 - x0 + 1;
    const int areaH = y1 - y0 + 1;
    houses_.clear();
    if (areaW < 5 || areaH < 5) return;

    constexpr int spacing   = 2;
    constexpr int maxRows   = 4;
    constexpr int maxHouses = 10;

    long sx = 0, sy = 0, n = 0;
    for (int x = x0; x <= x1; ++x)
        for (int y = y0; y <= y1; ++y)
            if (grid_.at(x, y) == Cell::Dirt) { sx += x; sy += y; ++n; }
    const int cx = n ? static_cast<int>(sx / n) : (x0 + x1) / 2;
    const int cy = n ? static_cast<int>(sy / n) : (y0 + y1) / 2;

    struct Size { int w, h; };
    std::vector<House> best;

    for (int attempt = 0; attempt < 6 && best.size() < 3; ++attempt) {
        const int dirtGap = (attempt < 4) ? 1 : 0;
        const int shrink  = std::min(attempt, 3);

        std::uniform_int_distribution wDist(std::max(5, 6 - shrink), std::max(5, 9 - 2 * shrink));
        std::uniform_int_distribution hDist(5, std::max(5, 8 - 2 * shrink));
        Size sizes[3];
        for (auto& s : sizes) {
            s.w = std::min(wDist(rng_), areaW);
            s.h = std::min(hDist(rng_), areaH);
        }

        auto touchesDirt = [&](int x, int y, int w, int h) {
            for (int i = std::max(x0, x - dirtGap); i <= std::min(x1, x + w - 1 + dirtGap); ++i)
                for (int j = std::max(y0, y - dirtGap); j <= std::min(y1, y + h - 1 + dirtGap); ++j)
                    if (grid_.at(i, j) == Cell::Dirt) return true;
            return false;
        };

        std::array<int, 3> order{0, 1, 2};
        std::shuffle(order.begin(), order.end(), rng_);
        std::vector<int> rowTypes;
        int blockH = 0;
        for (int r = 0; r < maxRows; ++r) {
            const int t = order[r % 3];
            const int add = sizes[t].h + (rowTypes.empty() ? 0 : spacing);
            if (blockH + add > areaH) break;
            blockH += add;
            rowTypes.push_back(t);
        }
        if (rowTypes.empty()) continue;

        std::vector<House> attemptBest;
        int bestDist = INT_MAX;
        for (int by = y0; by <= y1 - blockH + 1; ++by) {
            std::vector<House> cand;
            int y = by;
            for (int t : rowTypes) {
                const Size s = sizes[t];
                const int cols   = std::max(1, (areaW + spacing) / (s.w + spacing));
                const int totalW = cols * s.w + (cols - 1) * spacing;
                const int offX   = x0 + (areaW - totalW) / 2;
                for (int c = 0; c < cols; ++c) {
                    House h;
                    h.x = offX + c * (s.w + spacing);
                    h.y = y;
                    h.w = s.w;
                    h.h = s.h;
                    h.type = t;
                    if (h.right() > x1 || h.bottom() > y1) continue;
                    if (touchesDirt(h.x, h.y, h.w, h.h)) continue;
                    cand.push_back(h);
                }
                y += s.h + spacing;
            }
            const int dist = std::abs(by + blockH / 2 - cy);
            const int cnt  = std::min<int>(cand.size(), maxHouses);
            const int bcnt = std::min<int>(attemptBest.size(), maxHouses);
            if (cnt > bcnt || (cnt == bcnt && dist < bestDist)) {
                attemptBest = std::move(cand);
                bestDist = dist;
            }
        }
        if (attemptBest.size() > best.size()) best = std::move(attemptBest);
    }

    if (static_cast<int>(best.size()) > maxHouses) {
        auto dist2 = [&](const House& h) {
            const int dx = h.x + h.w / 2 - cx, dy = h.y + h.h / 2 - cy;
            return dx * dx + dy * dy;
        };
        std::sort(best.begin(), best.end(),
                  [&](const House& a, const House& b) { return dist2(a) < dist2(b); });
        best.resize(maxHouses);
    }
    houses_ = std::move(best);
}

void MapGenerator::carveHouseFloors() {
    std::uniform_real_distribution chance(0.f, 1.f);
    for (auto& h : houses_) {
        for (int x = h.x; x <= h.right(); ++x) {
            for (int y = h.y; y <= h.bottom(); ++y) {
                grid_.at(x, y) = Cell::HouseFloor;
                Tile floor = (chance(rng_) < 0.15f) ? TileType::FLOOR_STONE_CRACKED
                                                     : TileType::FLOOR_STONE_TILES;
                addTile(x, y, floor, ZIndex::Ground);
            }
        }
    }
}

void MapGenerator::connectAdjacentHouses() {
    std::uniform_real_distribution chance(0.f, 1.f);

    for (size_t i = 0; i < houses_.size(); ++i) {
        for (size_t j = i + 1; j < houses_.size(); ++j) {
            House& a = houses_[i];
            House& b = houses_[j];

            if (b.x == a.right() + 2) {
                int start = std::max(a.y + 1, b.y + 1);
                int end   = std::min(a.bottom() - 1, b.bottom() - 1);
                if (end >= start && chance(rng_) < 0.5f) {
                    std::uniform_int_distribution pick(start, end);
                    int y = pick(rng_);
                    for (int x : {a.right(), a.right() + 1, b.x}) {
                        grid_.at(x, y) = Cell::Opening;
                        addTile(x, y, TileType::FLOOR_STONE_PLAIN, ZIndex::Ground);
                    }
                }
            }
            if (b.y == a.bottom() + 2) {
                int start = std::max(a.x + 1, b.x + 1);
                int end   = std::min(a.right() - 1, b.right() - 1);
                if (end >= start && chance(rng_) < 0.5f) {
                    std::uniform_int_distribution pick(start, end);
                    int x = pick(rng_);
                    for (int y : {a.bottom(), a.bottom() + 1, b.y}) {
                        grid_.at(x, y) = Cell::Opening;
                        addTile(x, y, TileType::FLOOR_STONE_PLAIN, ZIndex::Ground);
                    }
                }
            }
        }
    }
}

void MapGenerator::buildWalls() {
    struct Style { int windowPeriod; float weakChance; Tile corner; };
    const Style styles[3] = {
        {3, 0.04f, TileType::WALL_CORNER_OUTER},
        {4, 0.10f, TileType::WALL_CORNER_OUTER},
        {2, 0.02f, TileType::WALL_CORNER_OUTER},
    };
    std::uniform_real_distribution chance(0.f, 1.f);

    for (auto& h : houses_) {
        const Style& st = styles[h.type % 3];
        for (int x = h.x; x <= h.right(); ++x) {
            for (int y = h.y; y <= h.bottom(); ++y) {
                bool top = y == h.y, bottom = y == h.bottom();
                bool left = x == h.x, right = x == h.right();
                bool border = top || bottom || left || right;
                if (!border) continue;
                if (grid_.at(x, y) == Cell::Opening) continue;

                bool isCorner = (left || right) && (top || bottom);
                grid_.at(x, y) = Cell::Wall;

                Tile t;
                if (isCorner) {
                    t = st.corner;
                    t.rotation = pairAngle(top, left);
                } else {
                    const bool horizontal = top || bottom;
                    const int len = horizontal ? h.w : h.h;
                    const int i   = horizontal ? x - h.x : y - h.y;
                    const bool inner = i >= 2 && i <= len - 3;

                    if (inner && st.windowPeriod > 0 && (i - len / 2) % st.windowPeriod == 0)
                        t = TileType::WALL_WINDOWS;
                    else if (chance(rng_) < st.weakChance)
                        t = TileType::WALL_WEAK;
                    else
                        t = TileType::WALL_STRAIGHT;
                    t.rotation = singleAngle(top, right, bottom);
                }
                addTile(x, y, t, ZIndex::Structure);
            }
        }
    }
}

void MapGenerator::placeDoors() {
    std::vector<Door> doors;
    std::uniform_real_distribution chance(0.f, 1.f);

    constexpr int DX[4] = {0, 1, 0, -1};
    constexpr int DY[4] = {-1, 0, 1, 0};

    std::array<int, 3> sidePerType, offsetPerType;
    for (int t = 0; t < 3; ++t) {
        sidePerType[t]   = std::uniform_int_distribution(0, 3)(rng_);
        offsetPerType[t] = std::uniform_int_distribution(-1, 1)(rng_);
    }

    auto pickDoor = [&](const House& h, int side, int offset) -> Point {
        std::vector<Point> cells;
        switch (side) {
            case 0:  for (int x = h.x + 1; x < h.right();  ++x) cells.emplace_back(x, h.y);       break;
            case 1:  for (int y = h.y + 1; y < h.bottom(); ++y) cells.emplace_back(h.right(), y); break;
            case 2:  for (int x = h.x + 1; x < h.right();  ++x) cells.emplace_back(x, h.bottom()); break;
            default: for (int y = h.y + 1; y < h.bottom(); ++y) cells.emplace_back(h.x, y);       break;
        }
        cells.erase(std::remove_if(cells.begin(), cells.end(), [&](const Point& p) {
            if (grid_.at(p.first, p.second) != Cell::Wall) return true;
            const int ox = p.first + DX[side], oy = p.second + DY[side];
            if (!grid_.inBounds(ox, oy)) return true;
            const Cell c = grid_.at(ox, oy);
            return c == Cell::Wall || c == Cell::HouseFloor || c == Cell::Opening || c == Cell::Door;
        }), cells.end());
        if (cells.empty()) return {-1, -1};

        const bool horizontal = (side == 0 || side == 2);
        const int target = horizontal ? h.x + (h.w - 1) / 2 + offset
                                      : h.y + (h.h - 1) / 2 + offset;
        Point best = cells[0];
        int bestD = INT_MAX;
        for (const Point& p : cells) {
            const int d = std::abs((horizontal ? p.first : p.second) - target);
            if (d < bestD) { bestD = d; best = p; }
        }
        return best;
    };

    auto placeDoor = [&](int x, int y, int side) {
        grid_.at(x, y) = Cell::Door;
        const float r = chance(rng_);
        Tile t = r < 0.45f ? TileType::WALL_DOORWAY_OPEN
               : r < 0.75f ? TileType::WALL_DOOR_FRAME
               : r < 0.92f ? TileType::WALL_DOORWAY_CLOSE
                           : TileType::WALL_BROKEN_DOOR;
        t.rotation = singleAngle(side == 0, side == 1, side == 2);
        addTile(x, y, t, ZIndex::Structure);
        doors.push_back({.x = x, .y = y});
    };

    for (auto& h : houses_) {
        const int type = static_cast<int>(h.type) % 3;
        int side = sidePerType[type];
        Point p = pickDoor(h, side, offsetPerType[type]);

        for (int k = 1; k < 4 && p.first < 0; ++k) {
            side = (sidePerType[type] + k) % 4;
            p = pickDoor(h, side, 0);
        }
        if (p.first < 0) continue;
        placeDoor(p.first, p.second, side);

        if (chance(rng_) < 0.2f) {
            const int back = (side + 2) % 4;
            const Point q = pickDoor(h, back, 0);
            if (q.first >= 0) placeDoor(q.first, q.second, back);
        }
    }
    doors_ = std::move(doors);
}

void MapGenerator::generateHousePaths() {
    if (doors_.size() < 2) return;

    auto exteriorOf = [&](const Door& d) -> Point {
        constexpr int DX[4] = {0, 1, 0, -1};
        constexpr int DY[4] = {-1, 0, 1, 0};
        for (int k = 0; k < 4; ++k) {
            const int nx = d.x + DX[k], ny = d.y + DY[k];
            if (!grid_.inBounds(nx, ny)) continue;
            const Cell c = grid_.at(nx, ny);
            if (c == Cell::Wall || c == Cell::HouseFloor || c == Cell::Door || c == Cell::Opening) continue;
            return {nx, ny};
        }
        return {d.x, d.y + 1};
    };

    auto blocked = [this](const int x, const int y) { return blocksPath(x, y); };

    std::vector connected(doors_.size(), false);
    connected[0] = true;
    for (size_t step = 1; step < doors_.size(); ++step) {
        int bestI = -1, bestJ = -1, bestDist = INT_MAX;
        for (size_t i = 0; i < doors_.size(); ++i) {
            if (!connected[i]) continue;
            for (size_t j = 0; j < doors_.size(); ++j) {
                if (connected[j]) continue;
                int d = std::abs(doors_[i].x - doors_[j].x) + std::abs(doors_[i].y - doors_[j].y);
                if (d < bestDist) { bestDist = d; bestI = (int)i; bestJ = (int)j; }
            }
        }
        if (bestJ == -1) break;

        Point a = exteriorOf(doors_[bestI]);
        Point b = exteriorOf(doors_[bestJ]);
        auto route = findRoute(a, b, blocked);
        for (auto& [x, y] : route) {
            if (grid_.at(x, y) == Cell::Grass) {
                grid_.at(x, y) = Cell::Path;
            }
        }
        connected[bestJ] = true;
    }
}

MapGenerator::Point MapGenerator::randomBoundaryPoint(int side) {
    switch (side) {
        case 0: return {std::uniform_int_distribution(playX0_, playX1_)(rng_), playY0_};
        case 1: return {playX1_, std::uniform_int_distribution(playY0_, playY1_)(rng_)};
        case 2: return {std::uniform_int_distribution(playX0_, playX1_)(rng_), playY1_};
        default: return {playX0_, std::uniform_int_distribution(playY0_, playY1_)(rng_)};
    }
}

void MapGenerator::generatePromenadePaths() {
    if (playX1_ - playX0_ < 10 || playY1_ - playY0_ < 10) return;

    auto blocked = [this](int x, int y) { return blocksPath(x, y); };

    int roadCount = std::uniform_int_distribution(1, 2)(rng_);
    for (int i = 0; i < roadCount; ++i) {
        int sideA = std::uniform_int_distribution(0, 3)(rng_);
        int sideB = (sideA + 1 + std::uniform_int_distribution(0, 2)(rng_)) % 4;

        Point a = randomBoundaryPoint(sideA);
        Point b = randomBoundaryPoint(sideB);
        if (blocked(a.first, a.second) || blocked(b.first, b.second)) continue;

        auto route = findRoute(a, b, blocked);
        if (route.empty()) continue;

        for (auto& [x, y] : route) {
            if (grid_.at(x, y) == Cell::Grass) {
                grid_.at(x, y) = Cell::Path;
                isPromenade_[x][y] = true;
            }
        }
    }
}

void MapGenerator::removePathBlocks() {
    constexpr int DX[4] = {0, 1, 0, -1};
    constexpr int DY[4] = {-1, 0, 1, 0};

    auto pathLike = [&](int x, int y) {
        return grid_.inBounds(x, y) &&
               (grid_.at(x, y) == Cell::Path || grid_.at(x, y) == Cell::Door);
    };

    auto countComponents = [&]() {
        std::vector<char> seen(grid_.width * grid_.height, 0);
        std::vector<Point> stack;
        int comps = 0;
        for (int x = 0; x < grid_.width; ++x)
            for (int y = 0; y < grid_.height; ++y) {
                if (!pathLike(x, y) || seen[x * grid_.height + y]) continue;
                ++comps;
                seen[x * grid_.height + y] = 1;
                stack.assign(1, Point{x, y});
                while (!stack.empty()) {
                    const Point p = stack.back();
                    stack.pop_back();
                    for (int k = 0; k < 4; ++k) {
                        const int nx = p.first + DX[k], ny = p.second + DY[k];
                        if (!pathLike(nx, ny) || seen[nx * grid_.height + ny]) continue;
                        seen[nx * grid_.height + ny] = 1;
                        stack.emplace_back(nx, ny);
                    }
                }
            }
        return comps;
    };

    auto neighbours = [&](const Point& p) {
        int n = 0;
        for (int k = 0; k < 4; ++k) n += pathLike(p.first + DX[k], p.second + DY[k]);
        return n;
    };

    const int components = countComponents();
    bool changed = true;
    for (int pass = 0; changed && pass < 10; ++pass) {
        changed = false;
        for (int x = 0; x + 1 < grid_.width; ++x) {
            for (int y = 0; y + 1 < grid_.height; ++y) {
                const Point cells[4] = {{x, y}, {x + 1, y}, {x, y + 1}, {x + 1, y + 1}};
                bool full = true;
                for (const Point& c : cells)
                    if (grid_.at(c.first, c.second) != Cell::Path) { full = false; break; }
                if (!full) continue;

                int order[4] = {0, 1, 2, 3};
                std::sort(order, order + 4, [&](int a, int b) {
                    return neighbours(cells[a]) < neighbours(cells[b]);
                });
                for (int idx : order) {
                    const Point p = cells[idx];
                    grid_.at(p.first, p.second) = Cell::Grass;
                    if (countComponents() <= components) { changed = true; break; }
                    grid_.at(p.first, p.second) = Cell::Path;
                }
            }
        }
    }
}

void MapGenerator::autotilePathsUnified() {
    for (int x = 0; x < grid_.width; ++x) {
        for (int y = 0; y < grid_.height; ++y) {
            if (grid_.at(x, y) != Cell::Path) continue;

            bool styleB = isPromenade_[x][y];
            const Tile straightTile = styleB ? TileType::PATH_STRAIGHT_2 : TileType::PATH_STRAIGHT_1;
            const Tile curveTile    = styleB ? TileType::PATH_CURVE_2    : TileType::PATH_CURVE_1;

            auto pathLike = [&](int nx, int ny) {
                return grid_.inBounds(nx, ny) &&
                       (grid_.at(nx, ny) == Cell::Path || grid_.at(nx, ny) == Cell::Door);
            };
            bool n = pathLike(x, y - 1), s = pathLike(x, y + 1);
            bool e = pathLike(x + 1, y), w = pathLike(x - 1, y);
            int count = n + s + e + w;

            Tile t;
            if (count == 4) {
                t = styleB ? TileType::PATH_ARROW : TileType::PATH_CROSSROADS;
            } else if (count == 3) {
                if (styleB) {
                    t = TileType::PATH_ARROW;
                } else {
                    t = TileType::PATH_T_JUNCTION;
                    t.rotation = singleAngle(!s, !w, !n);
                }
            } else if (count == 2) {
                bool vertical = n && s;
                bool horizontal = e && w;
                if (vertical || horizontal) {
                    t = straightTile;
                    t.rotation = vertical ? ANGLE_0 : ANGLE_90;
                } else {
                    t = curveTile;
                    t.rotation = pairAngle(s, e);
                }
            } else if (count == 1) {
                t = straightTile;
                t.rotation = (n || s) ? ANGLE_0 : ANGLE_90;
            } else {
                t = straightTile;
            }
            addTile(x, y, t, ZIndex::Structure);
        }
    }
}

void MapGenerator::autotileRails() {
    for (int x = 0; x < grid_.width; ++x) {
        for (int y = 0; y < grid_.height; ++y) {
            if (grid_.at(x, y) != Cell::Rail) continue;

            auto isRail = [&](int nx, int ny) {
                return grid_.inBounds(nx, ny) && grid_.at(nx, ny) == Cell::Rail;
            };
            bool n = isRail(x, y - 1), s = isRail(x, y + 1);
            bool e = isRail(x + 1, y), w = isRail(x - 1, y);
            int count = n + s + e + w;

            Tile t;
            if (count >= 3) {
                t = TileType::RAIL_CROSSROADS;
            } else if (count == 2) {
                bool vertical = n && s;
                bool horizontal = e && w;
                if (vertical || horizontal) {
                    t = TileType::RAIL_STRAIGHT;
                    t.rotation = vertical ? ANGLE_0 : ANGLE_90;
                } else {
                    t = TileType::RAIL_CURVE;
                    t.rotation = pairAngle(s, e);
                }
            } else if (count == 1) {
                t = TileType::RAIL_STRAIGHT;
                t.rotation = (n || s) ? ANGLE_0 : ANGLE_90;
            } else {
                t = TileType::RAIL_STRAIGHT;
            }
            addTile(x, y, t, ZIndex::Structure);
        }
    }
}

void MapGenerator::generateRails() {
    std::uniform_real_distribution chance(0.f, 1.f);
    if (chance(rng_) > 0.3f) return;
    if (playX1_ <= playX0_ || playY1_ <= playY0_) return;

    auto blocked = [&](int x, int y) {
        Cell c = grid_.at(x, y);
        return c == Cell::Wall || c == Cell::HouseFloor || c == Cell::Door ||
               c == Cell::Opening || c == Cell::Path;
    };

    std::uniform_int_distribution edgeY(playY0_, playY1_);
    std::uniform_int_distribution lengthDist(5, 10);

    bool fromLeft = chance(rng_) < 0.5f;
    Point a = fromLeft ? Point{playX0_, edgeY(rng_)} : Point{playX1_, edgeY(rng_)};
    int length = lengthDist(rng_);
    int bx = fromLeft ? std::min(playX1_, a.first + length)
                      : std::max(playX0_, a.first - length);
    Point b = {bx, edgeY(rng_)};

    auto route = findRoute(a, b, blocked);
    if (route.empty()) return;

    for (auto& [x, y] : route) {
        if (grid_.at(x, y) == Cell::Grass || grid_.at(x, y) == Cell::Dirt) {
            grid_.at(x, y) = Cell::Rail;
        }
    }
    autotileRails();

    auto [cx, cy] = route.back();
    if (grid_.at(cx, cy) == Cell::Rail) {
        addTile(cx, cy, TileType::CART_WOODEN, ZIndex::Decoration);
    }
}

void MapGenerator::placeDecorations() {
    std::uniform_real_distribution chance(0.f, 1.f);

    auto place = [&](int x, int y, Tile tile) {
        if (wantsRandomRotation(tile)) tile.rotation = randomAngle();
        addTile(x, y, tile, ZIndex::Decoration);
    };

    const Tile outdoorProps[] = {
        TileType::PROP_BUSH, TileType::PROP_BUSH,
        TileType::PROP_GRASS_TUFT, TileType::PROP_GRASS_TUFT, TileType::PROP_GRASS_TUFT,
    };
    std::uniform_int_distribution propPick(0, (int)(sizeof(outdoorProps) / sizeof(Tile)) - 1);

    for (int x = 0; x < grid_.width; ++x) {
        for (int y = 0; y < grid_.height; ++y) {
            Cell c = grid_.at(x, y);
            if (c != Cell::Grass && c != Cell::Dirt) continue;
            if (chance(rng_) < 0.06f) {
                place(x, y, outdoorProps[propPick(rng_)]);
            }
        }
    }

    for (int x = 0; x < grid_.width; ++x) {
        for (int y = 0; y < grid_.height; ++y) {
            Cell c = grid_.at(x, y);
            if (c != Cell::Grass && c != Cell::Dirt) continue;
            if (hasDecoration(x, y)) continue;
            if (chance(rng_) < 0.05f) {
                place(x, y, TileType::PROP_BUSH);
            }
        }
    }

    for (int x = 0; x < grid_.width; ++x) {
        for (int y = 0; y < grid_.height; ++y) {
            Cell c = grid_.at(x, y);
            if (c != Cell::Grass && c != Cell::Dirt) continue;
            if (hasDecoration(x, y)) continue;
            if (chance(rng_) < 0.08f) {
                place(x, y, TileType::PROP_GRASS_TUFT);
            }
        }
    }

    const Tile outdoorStockpile[] = {
        TileType::OBJECT_LARGE_CRATE_WOODEN, TileType::OBJECT_LARGE_BARREL_TOP,
        TileType::OBJECT_BARRELS_CLUSTER_1, TileType::OBJECT_BARRELS_CLUSTER_2,
        TileType::OBJECT_SMALL_CRATE_WOODEN, TileType::OBJECT_SMALL_BARREL_TOP
    };
    std::uniform_int_distribution stockpilePick(0, (int)(sizeof(outdoorStockpile) / sizeof(Tile)) - 1);
    int stockpileCount = std::clamp(grid_.width * grid_.height / 100, 4, 14);
    std::uniform_int_distribution anyX(0, std::max(0, grid_.width - 1));
    std::uniform_int_distribution anyY(0, std::max(0, grid_.height - 1));

    for (int i = 0; i < stockpileCount; ++i) {
        for (int tries = 0; tries < 25; ++tries) {
            int x = anyX(rng_), y = anyY(rng_);
            Cell c = grid_.at(x, y);
            if ((c == Cell::Grass || c == Cell::Dirt) && !hasDecoration(x, y)) {
                place(x, y, outdoorStockpile[stockpilePick(rng_)]);
                break;
            }
        }
    }

    for (int tries = 0; tries < 15; ++tries) {
        int x = anyX(rng_), y = anyY(rng_);
        if (grid_.at(x, y) == Cell::Grass && !hasDecoration(x, y)) {
            place(x, y, TileType::PROP_CAMPFIRE);
            break;
        }
    }

    const Tile storageProps[] = {
        TileType::OBJECT_LARGE_CRATE_WOODEN, TileType::OBJECT_LARGE_BARREL_TOP,
        TileType::OBJECT_BARRELS_CLUSTER_1, TileType::OBJECT_BARRELS_CLUSTER_2,
        TileType::OBJECT_SMALL_CRATE_WOODEN, TileType::OBJECT_SMALL_BARREL_TOP
    };
    std::uniform_int_distribution<int> storagePick(0, (int)(sizeof(storageProps) / sizeof(Tile)) - 1);

    for (auto& h : houses_) {
        Point keepClear{-1, -1};
        for (auto& d : doors_) {
            if (d.x >= h.x && d.x <= h.right() && d.y >= h.y && d.y <= h.bottom()) {
                constexpr int DX[4] = {0, 1, 0, -1};
                constexpr int DY[4] = {-1, 0, 1, 0};
                for (int k = 0; k < 4; ++k) {
                    const int nx = d.x + DX[k], ny = d.y + DY[k];
                    if (grid_.inBounds(nx, ny) && grid_.at(nx, ny) == Cell::HouseFloor)
                        keepClear = {nx, ny};
                }
            }
        }

        if (h.type == HOUSE_BARRACKS) {
            bool altRow = false;
            for (int y = h.y + 1; y < h.bottom(); ++y) {
                Tile bed = altRow ? TileType::OBJECT_BED_2 : TileType::OBJECT_BED;
                for (int x = h.x + 1; x < h.right(); ++x) {
                    if (grid_.at(x, y) != Cell::HouseFloor) continue;
                    if (x == keepClear.first && y == keepClear.second) continue;
                    place(x, y, bed);
                }
                altRow = !altRow;
            }
        } else if (h.type == HOUSE_MESS) {
            for (int x = h.x + 1; x < h.right(); ++x) {
                bool tableColumn = ((x - (h.x + 1)) % 2 == 0);
                for (int y = h.y + 1; y < h.bottom(); ++y) {
                    if (grid_.at(x, y) != Cell::HouseFloor) continue;
                    if (x == keepClear.first && y == keepClear.second) continue;
                    place(x, y, tableColumn ? TileType::OBJECT_TABLE : TileType::OBJECT_CHAIR);
                }
            }
            int bx = h.right() - 1;
            int by = h.bottom() - 1;
            if (grid_.at(bx, by) == Cell::HouseFloor && !(bx == keepClear.first && by == keepClear.second)) {
                place(bx, by, TileType::OBJECT_BARRELS_CLUSTER_1);
            }
        } else { // HOUSE_STORAGE
            for (int x = h.x + 1; x < h.right(); ++x) {
                for (int y = h.y + 1; y < h.bottom(); ++y) {
                    if (grid_.at(x, y) != Cell::HouseFloor) continue;
                    if (x == keepClear.first && y == keepClear.second) continue;
                    if (chance(rng_) < 0.35f) {
                        place(x, y, storageProps[storagePick(rng_)]);
                    }
                }
            }
        }
    }
}



TileMap MapGenerator::assembleResult() const {
    TileMap result(grid_.width, std::vector<MapCell>(grid_.height));
    for (int x = 0; x < grid_.width; ++x) {
        for (int y = 0; y < grid_.height; ++y) {
            auto layers = tiles_[x][y].layers;
            std::stable_sort(layers.begin(), layers.end(),
                              [](const LayerTile& a, const LayerTile& b) { return a.z < b.z; });
            result[x][y].layers = std::move(layers);
        }
    }
    return result;
}

TileMap MapGenerator::build(const int height, const int width, const int seed) {
    rng_ = std::mt19937(static_cast<unsigned int>(seed));
    margin_ = 5;

    const int border = margin_ + 1;

    const int totalWidth  = width  + 2 * border;
    const int totalHeight = height + 2 * border;
    initGrids(totalWidth, totalHeight);

    playX0_ = border;
    playY0_ = border;
    playX1_ = border + width - 1;
    playY1_ = border + height - 1;

    buildMapBorder();

    generateBaseTerrain();
    generateHouses();

    carveHouseFloors();
    connectAdjacentHouses();

    buildWalls();
    placeDoors();

    isPromenade_.assign(totalWidth, std::vector(totalHeight, false));
    generateHousePaths();
    // generatePromenadePaths();
    removePathBlocks();
    autotilePathsUnified();

    // generateRails();
    // placeDecorations();

    return assembleResult();
}