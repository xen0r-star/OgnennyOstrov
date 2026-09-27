#include "models/map/MapGenerator.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <queue>
#include <tuple>

MapGenerator::MapGenerator() = default;
MapGenerator::~MapGenerator() = default;

// ----------------------------------------------------------------------
// Point d'entrée public : crée un générateur jetable et lui délègue tout
// le travail. Chaque appel repart d'un état vierge (nouvelle instance),
// donc aucun état ne fuit d'une carte à l'autre.
// ----------------------------------------------------------------------
TileMap MapGenerator::generateMap(const int height, const int width, const int seed) {
    MapGenerator generator;
    return generator.build(height, width, seed);
}

// ----------------------------------------------------------------------
// Aides bas niveau : superposition (structure / décor), orientation des
// tuiles, pathfinding.
// ----------------------------------------------------------------------

void MapGenerator::initGrids(int totalWidth, int totalHeight) {
    grid_ = Grid(totalWidth, totalHeight);
    ground_.assign(totalWidth, std::vector<Tile>(totalHeight, TileType::GROUND_GRASS));
    overlay_.assign(totalWidth, std::vector<OverlayCell>(totalHeight));
}

// Pose LA tuile de structure de la case (mur, porte, clôture, tour de
// garde, chemin, rail...). Une case n'a qu'une seule structure à la fois :
// un nouvel appel remplace le précédent (ex : une porte qui remplace le
// segment de mur posé avant elle).
void MapGenerator::setStructure(int x, int y, Tile tile) {
    auto& layers = overlay_[x][y].layers;
    layers.erase(std::remove_if(layers.begin(), layers.end(),
                 [](const LayerTile& l) { return l.z == ZIndex::Structure; }),
                 layers.end());
    layers.push_back({tile, ZIndex::Structure});
}

// Ajoute une tuile de décor/objet par-dessus, sans toucher à la structure
// existante. Une case peut porter zéro, une, ou plusieurs décorations
// (ex : un chariot posé sur un rail).
void MapGenerator::addDecoration(int x, int y, Tile tile) {
    overlay_[x][y].layers.push_back({tile, ZIndex::Decoration});
}

bool MapGenerator::hasAnyOverlay(int x, int y) const {
    return !overlay_[x][y].layers.empty();
}

// ----------------------------------------------------------------------
// Aides pour l'orientation des tuiles.
//
// Convention déduite des coins de mur (validés visuellement) : le coin
// "haut-gauche" (angle 0) relie les côtés Est et Sud (les deux murs qui
// partent de ce coin vont vers la droite et vers le bas). Une rotation de
// 90° dans le sens horaire fait tourner ce même schéma : {E,S} -> {S,W}
// -> {W,N} -> {N,E}. On applique exactement la même logique aux virages
// de chemin/rail pour rester cohérent avec ce qui fonctionne déjà.
// ----------------------------------------------------------------------
AngleTile MapGenerator::cornerAngle(bool top, bool left) {
    if (top && left)   return ANGLE_0;   // haut-gauche
    if (top && !left)  return ANGLE_90;  // haut-droit
    if (!top && !left) return ANGLE_180; // bas-droit
    return ANGLE_270;                    // bas-gauche
}

AngleTile MapGenerator::sideAngle(bool top, bool right, bool bottom, bool /*left*/) {
    if (top)    return ANGLE_0;
    if (right)  return ANGLE_90;
    if (bottom) return ANGLE_180;
    return ANGLE_270; // gauche
}

// Virage : mêmes paires de directions que cornerAngle, dans le même ordre.
AngleTile MapGenerator::curveAngle(bool n, bool e, bool s, bool w) {
    if (e && s) return ANGLE_0;   // équivalent du coin "haut-gauche"
    if (s && w) return ANGLE_90;
    if (w && n) return ANGLE_180;
    return ANGLE_270; // n && e
}

// Jonction en T (exactement 3 directions connectées, une seule manquante).
// Convention (à ajuster visuellement si besoin, même logique que les
// autres helpers d'angle) : angle 0 = branche manquante au Nord (le T
// relie Est/Sud/Ouest), puis rotation de 90° dans le sens horaire à
// chaque direction manquante suivante : N -> E -> S -> W.
AngleTile MapGenerator::tJunctionAngle(bool n, bool e, bool s, bool /*w*/) {
    if (!n) return ANGLE_0;   // manque Nord   -> relie E, S, W
    if (!e) return ANGLE_90;  // manque Est    -> relie N, S, W
    if (!s) return ANGLE_180; // manque Sud    -> relie N, E, W
    return ANGLE_270;         // manque Ouest  -> relie N, E, S
}

AngleTile MapGenerator::randomAngle() {
    static std::uniform_int_distribution<int> d(0, 3);
    switch (d(rng_)) {
        case 0:  return ANGLE_0;
        case 1:  return ANGLE_90;
        case 2:  return ANGLE_180;
        default: return ANGLE_270;
    }
}

// Éléments pour lesquels on veut une orientation aléatoire (pas de sens
// "correct" à respecter, juste de la variété visuelle).
bool MapGenerator::wantsRandomRotation(const Tile& t) {
    auto same = [&](const Tile& ref) { return t.col == ref.col && t.row == ref.row; };
    return same(TileType::PROP_CAMPFIRE)            || same(TileType::PROP_GRASS_TUFT) ||
           same(TileType::PROP_BUSH)                || same(TileType::OBJECT_BOARD) ||
           same(TileType::OBJECT_LARGE_CRATE_WOODEN) || same(TileType::OBJECT_LARGE_BARREL_TOP) ||
           same(TileType::OBJECT_SMALL_CRATE_WOODEN)  || same(TileType::OBJECT_SMALL_BARREL_TOP) ||
           same(TileType::OBJECT_BARRELS_CLUSTER_1)   || same(TileType::OBJECT_BARRELS_CLUSTER_2) ||
           same(TileType::PROP_TREE);
}

// Obstacles communs aux deux réseaux de chemins : murs, sol de maison,
// portes, ouvertures... et les zones de terre, qu'on ne veut jamais voir
// traversées par un chemin.
bool MapGenerator::blocksPath(int x, int y) const {
    Cell c = grid_.at(x, y);
    return c == Cell::Wall || c == Cell::HouseFloor || c == Cell::Door ||
           c == Cell::Opening || c == Cell::Dirt;
}

// Algorithme de recherche de chemin (Dijkstra avec pénalité de virage)
std::vector<MapGenerator::Point> MapGenerator::findRoute(
    Point start, Point goal, const std::function<bool(int, int)>& isBlocked) const {
    static const int dx[4] = {1, -1, 0, 0};
    static const int dy[4] = {0, 0, 1, -1};
    const int W = grid_.width, H = grid_.height;
    auto idx = [&](int x, int y) { return y * W + x; };

    std::vector<std::array<int, 4>> dist(
        (size_t)W * H, std::array<int, 4>{INT_MAX, INT_MAX, INT_MAX, INT_MAX});
    std::vector<std::array<Point, 4>> prevPos(
        (size_t)W * H, std::array<Point, 4>{Point{-1, -1}, Point{-1, -1}, Point{-1, -1}, Point{-1, -1}});
    std::vector<std::array<int, 4>> prevDir(
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

            int turnPenalty = (d == nd) ? 0 : 2;
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

// ----------------------------------------------------------------------------
// Étapes du pipeline de génération, dans leur ordre d'exécution.
// ----------------------------------------------------------------------------

// Clôture périmétrique du camp : barbelés + tours de garde à intervalle
// régulier (toujours aux coins, puis tous les "stride" cases sur chaque
// côté). C'est volontairement répétitif, pas aléatoire : une vraie clôture
// de camp est régulière.
void MapGenerator::buildPerimeterFence() {
    const int x0 = margin_, y0 = margin_;
    const int x1 = grid_.width - 1 - margin_, y1 = grid_.height - 1 - margin_;
    if (x1 - x0 < 4 || y1 - y0 < 4) return;

    const int watchtowerStride = 6;
    auto isCorner = [&](int x, int y) {
        return (x == x0 || x == x1) && (y == y0 || y == y1);
    };

    // Côtés horizontaux (haut / bas).
    for (int x = x0; x <= x1; ++x) {
        for (int y : {y0, y1}) {
            grid_.at(x, y) = Cell::Wall;
            ground_[x][y] = TileType::GROUND_GRASS;
            bool tower = isCorner(x, y) || ((x - x0) % watchtowerStride == 0);
            Tile t = tower ? TileType::WATCHTOWER : TileType::WALL_BARBED_WIRE;
            if (!tower) t.rotation = ANGLE_0; // segment horizontal
            setStructure(x, y, t);
        }
    }
    // Côtés verticaux (gauche / droite), sans repasser sur les coins déjà posés.
    for (int y = y0 + 1; y < y1; ++y) {
        for (int x : {x0, x1}) {
            grid_.at(x, y) = Cell::Wall;
            ground_[x][y] = TileType::GROUND_GRASS;
            bool tower = (y - y0) % watchtowerStride == 0;
            Tile t = tower ? TileType::WATCHTOWER : TileType::WALL_BARBED_WIRE;
            if (!tower) t.rotation = ANGLE_90; // segment vertical
            setStructure(x, y, t);
        }
    }
}

// Petite bande de forêt tout autour de la carte, en dehors de la clôture :
// de l'herbe avec des arbres semés aléatoirement (position + rotation).
void MapGenerator::placeTreeBorder() {
    if (margin_ <= 0) return;
    std::uniform_real_distribution<float> chance(0.f, 1.f);

    for (int x = 0; x < grid_.width; ++x) {
        for (int y = 0; y < grid_.height; ++y) {
            bool inMargin = x < margin_ || x >= grid_.width - margin_ ||
                            y < margin_ || y >= grid_.height - margin_;
            if (!inMargin) continue;

            ground_[x][y] = TileType::GROUND_GRASS;
            grid_.at(x, y) = Cell::Wall; // hors zone jouable : bloque routes/rails/maisons

            if (chance(rng_) < 0.4f) {
                Tile t = TileType::PROP_TREE;
                t.rotation = randomAngle();
                addDecoration(x, y, t);
            }
        }
    }
}

// Zones de terre : 2 à 3 GROS rectangles collés les uns aux autres (façon
// petit hameau / cour commune), plutôt qu'un semis de petites parcelles
// répétitives. Renvoie la rangée la plus basse occupée (ou playY0_ - 1 si
// rien n'a été placé), pour que les maisons puissent être décalées en dessous.
int MapGenerator::generateBaseTerrain() {
    const int x0 = playX0_, y0 = playY0_, x1 = playX1_, y1 = playY1_;
    int areaW = x1 - x0 + 1;
    int areaH = y1 - y0 + 1;
    if (areaW < 10 || areaH < 10) return y0 - 1;

    auto fillDirt = [&](const Rect& r) {
        for (int x = std::max(x0, r.x); x <= std::min(x1, r.right()); ++x)
            for (int y = std::max(y0, r.y); y <= std::min(y1, r.bottom()); ++y) {
                grid_.at(x, y) = Cell::Dirt;
                ground_[x][y] = TileType::GROUND_DIRT;
            }
    };

    std::uniform_int_distribution<int> sizeDist(6, std::max(6, std::min(areaW, areaH) / 2));
    std::uniform_int_distribution<int> zoneCountDist(2, 3);
    int zoneCount = zoneCountDist(rng_);

    int w = std::min(sizeDist(rng_), areaW);
    int h = std::min(sizeDist(rng_), areaH);
    Rect first{x0 + std::uniform_int_distribution<int>(0, std::max(0, areaW - w))(rng_),
               y0 + std::uniform_int_distribution<int>(0, std::max(0, areaH - h))(rng_), w, h};
    fillDirt(first);

    std::vector<Rect> zones{first};
    int maxBottom = first.bottom();

    for (int i = 1; i < zoneCount; ++i) {
        const Rect& prev = zones.back();
        int nw = std::min(sizeDist(rng_), areaW);
        int nh = std::min(sizeDist(rng_), areaH);

        Rect next{prev.x, prev.y, nw, nh};
        switch (std::uniform_int_distribution<int>(0, 3)(rng_)) {
            case 0: next.x = prev.right() + 1;  next.y = prev.y;            break; // à droite
            case 1: next.x = prev.x - nw;       next.y = prev.y;            break; // à gauche
            case 2: next.x = prev.x;            next.y = prev.bottom() + 1; break; // en dessous
            default: next.x = prev.x;           next.y = prev.y - nh;       break; // au-dessus
        }
        next.x = std::clamp(next.x, x0, std::max(x0, x1 - next.w + 1));
        next.y = std::clamp(next.y, y0, std::max(y0, y1 - next.h + 1));

        fillDirt(next);
        zones.push_back(next);
        maxBottom = std::max(maxBottom, next.bottom());
    }

    return maxBottom;
}

// Maisons disposées sur une grille régulière d'EMPLACEMENTS (mêmes
// espacements entre rangées/colonnes -> aspect "camp aligné"), mais chaque
// maison a sa propre taille, tirée dans une fourchette assez large, alignée
// sur le coin haut-gauche de son emplacement. On garde donc l'alignement en
// rangées tout en ayant des maisons plus grandes et variées.
void MapGenerator::generateHouses(int housesY0) {
    const int x0 = playX0_, y0 = housesY0, x1 = playX1_, y1 = playY1_;
    std::vector<House> houses;
    int areaW = x1 - x0 + 1;
    int areaH = y1 - y0 + 1;
    if (areaW < 6 || areaH < 6) { houses_ = std::move(houses); return; }

    // Taille max de l'emplacement (détermine l'espacement de la grille) et
    // fourchette de variation individuelle de chaque maison.
    std::uniform_int_distribution<int> slotSizeDist(7, 10);
    const int slotW = slotSizeDist(rng_);
    const int slotH = slotSizeDist(rng_);
    const int spacing = 2; // ruelle entre deux emplacements
    const int minHouseSize = 5;

    std::uniform_int_distribution<int> varyW(std::min(minHouseSize, slotW), slotW);
    std::uniform_int_distribution<int> varyH(std::min(minHouseSize, slotH), slotH);

    int cols = std::max(1, (areaW + spacing) / (slotW + spacing));
    int rows = std::max(1, (areaH + spacing) / (slotH + spacing));

    // On plafonne le nombre total de maisons pour ne pas surcharger les
    // grandes cartes.
    while (rows * cols > 12 && (rows > 1 || cols > 1)) {
        if (rows >= cols && rows > 1) --rows;
        else if (cols > 1) --cols;
    }

    int totalW = cols * slotW + (cols - 1) * spacing;
    int totalH = rows * slotH + (rows - 1) * spacing;
    int offX = x0 + std::max(0, (areaW - totalW) / 2);
    int offY = y0 + std::max(0, (areaH - totalH) / 2);

    int typeCounter = 0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int slotX = offX + c * (slotW + spacing);
            int slotY = offY + r * (slotH + spacing);

            House house;
            house.w = varyW(rng_);
            house.h = varyH(rng_);
            house.x = slotX;
            house.y = slotY;
            if (house.right() > x1 || house.bottom() > y1) continue;

            house.type = typeCounter % 3; // baraquement -> mess -> stockage -> répète
            ++typeCounter;
            houses.push_back(house);
        }
    }
    houses_ = std::move(houses);
}

// Sol de maison : croix par défaut, avec quelques dalles craquelées.
void MapGenerator::carveHouseFloors() {
    std::uniform_real_distribution<float> chance(0.f, 1.f);
    for (auto& h : houses_) {
        for (int x = h.x; x <= h.right(); ++x) {
            for (int y = h.y; y <= h.bottom(); ++y) {
                grid_.at(x, y) = Cell::HouseFloor;
                ground_[x][y] = (chance(rng_) < 0.15f) ? TileType::FLOOR_STONE_CRACKED
                                                        : TileType::FLOOR_STONE_TILES;
            }
        }
    }
}

void MapGenerator::connectAdjacentHouses() {
    std::uniform_real_distribution<float> chance(0.f, 1.f);

    for (size_t i = 0; i < houses_.size(); ++i) {
        for (size_t j = i + 1; j < houses_.size(); ++j) {
            House& a = houses_[i];
            House& b = houses_[j];

            if (b.x == a.right() + 2) {
                int start = std::max(a.y + 1, b.y + 1);
                int end   = std::min(a.bottom() - 1, b.bottom() - 1);
                if (end >= start && chance(rng_) < 0.5f) {
                    std::uniform_int_distribution<int> pick(start, end);
                    int y = pick(rng_);
                    for (int x : {a.right(), a.right() + 1, b.x}) {
                        grid_.at(x, y) = Cell::Opening;
                        ground_[x][y] = TileType::FLOOR_STONE_PLAIN;
                    }
                }
            }
            if (b.y == a.bottom() + 2) {
                int start = std::max(a.x + 1, b.x + 1);
                int end   = std::min(a.right() - 1, b.right() - 1);
                if (end >= start && chance(rng_) < 0.5f) {
                    std::uniform_int_distribution<int> pick(start, end);
                    int x = pick(rng_);
                    for (int y : {a.bottom(), a.bottom() + 1, b.y}) {
                        grid_.at(x, y) = Cell::Opening;
                        ground_[x][y] = TileType::FLOOR_STONE_PLAIN;
                    }
                }
            }
        }
    }
}

void MapGenerator::buildWalls() {
    for (auto& h : houses_) {
        for (int x = h.x; x <= h.right(); ++x) {
            for (int y = h.y; y <= h.bottom(); ++y) {
                bool top = (y == h.y), bottom = (y == h.bottom());
                bool left = (x == h.x), right = (x == h.right());
                bool border = top || bottom || left || right;
                if (!border) continue;
                if (grid_.at(x, y) == Cell::Opening) continue;

                bool isCorner = (left || right) && (top || bottom);
                grid_.at(x, y) = Cell::Wall;

                Tile t;
                if (isCorner) {
                    t = TileType::WALL_CORNER_OUTER;
                    t.rotation = cornerAngle(top, left);
                } else {
                    t = TileType::WALL_STRAIGHT;
                    t.rotation = sideAngle(top, right, bottom, left);
                }
                setStructure(x, y, t);
            }
        }
    }
}

void MapGenerator::placeDoors() {
    std::vector<Door> doors;

    for (auto& h : houses_) {
        std::vector<Point> candidates;
        for (int x = h.x + 1; x < h.right(); ++x)
            if (grid_.at(x, h.bottom()) == Cell::Wall) candidates.emplace_back(x, h.bottom());

        bool onBottom = !candidates.empty();
        if (!onBottom) {
            for (int x = h.x + 1; x < h.right(); ++x)
                if (grid_.at(x, h.y) == Cell::Wall) candidates.emplace_back(x, h.y);
        }
        if (candidates.empty()) continue;

        std::uniform_int_distribution<int> pick(0, (int)candidates.size() - 1);
        auto [dx, dy] = candidates[pick(rng_)];
        grid_.at(dx, dy) = Cell::Door;

        Tile t = TileType::WALL_DOORWAY_OPEN;
        t.rotation = onBottom ? ANGLE_180 : ANGLE_0;
        setStructure(dx, dy, t); // remplace le segment de mur posé par buildWalls()

        doors.push_back({dx, dy});
    }
    doors_ = std::move(doors);
}

// ----------------------------------------------------------------------------
// Chemins.
//
// Deux réseaux distincts, qui ne se mélangent jamais dans leur jeu de
// tuiles, et qui évitent tous les deux les zones de terre (Cell::Dirt) :
//   - Réseau "maison" (isPromenade = false) : relie les portes entre elles.
//     Toujours en style 1 (PATH_STRAIGHT_1 / PATH_CURVE_1 / PATH_CROSSROADS
//     / PATH_T_JUNCTION).
//   - Réseau "promenade" (isPromenade = true) : une ou deux routes qui
//     traversent la zone jouable d'un bord à l'autre, en style 2
//     (PATH_STRAIGHT_2 / PATH_CURVE_2 / PATH_ARROW). Les deux extrémités
//     touchent toujours le bord de la zone jouable (donc "sortent du
//     cadre" plutôt que de s'arrêter en pleine herbe) : jamais de
//     cul-de-sac, et on n'utilise donc jamais PATH_END.
//
// Les deux réseaux partagent la même case Cell::Path (pour la détection de
// connectivité), mais chaque case garde en mémoire (isPromenade_) quel jeu
// de tuiles lui appliquer.
// ----------------------------------------------------------------------------

// Réseau "maison" : relie les portes les plus proches entre elles, une par
// une, toujours avec le style 1 (voir en-tête de section). Ne traverse
// jamais une zone de terre.
void MapGenerator::generateHousePaths() {
    if (doors_.size() < 2) return;

    auto exteriorOf = [&](const Door& d) -> Point {
        Cell above = grid_.inBounds(d.x, d.y - 1) ? grid_.at(d.x, d.y - 1) : Cell::Wall;
        if (above != Cell::Wall && above != Cell::HouseFloor) return {d.x, d.y - 1};
        return {d.x, d.y + 1};
    };

    auto blocked = [this](int x, int y) { return blocksPath(x, y); };

    std::vector<bool> connected(doors_.size(), false);
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

// Un point tiré au hasard sur un des 4 côtés de la zone jouable.
MapGenerator::Point MapGenerator::randomBoundaryPoint(int side) {
    switch (side) {
        case 0: return {std::uniform_int_distribution<int>(playX0_, playX1_)(rng_), playY0_}; // haut
        case 1: return {playX1_, std::uniform_int_distribution<int>(playY0_, playY1_)(rng_)}; // droite
        case 2: return {std::uniform_int_distribution<int>(playX0_, playX1_)(rng_), playY1_}; // bas
        default: return {playX0_, std::uniform_int_distribution<int>(playY0_, playY1_)(rng_)}; // gauche
    }
}

// Réseau "promenade" : 1 à 2 routes qui traversent la zone jouable d'un
// bord à l'autre (jamais une boucle/cercle fermé). Comme les deux
// extrémités touchent toujours le bord de la zone jouable, il n'y a jamais
// de cul-de-sac : si une route ne trouve pas de chemin, elle est
// simplement annulée plutôt que de laisser un tronçon incomplet.
void MapGenerator::generatePromenadePaths() {
    if (playX1_ - playX0_ < 10 || playY1_ - playY0_ < 10) return; // pas assez de place

    auto blocked = [this](int x, int y) { return blocksPath(x, y); };

    int roadCount = std::uniform_int_distribution<int>(1, 2)(rng_);
    for (int i = 0; i < roadCount; ++i) {
        int sideA = std::uniform_int_distribution<int>(0, 3)(rng_);
        int sideB = (sideA + 1 + std::uniform_int_distribution<int>(0, 2)(rng_)) % 4; // un côté différent

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
            // Si la case appartient déjà au réseau "maison", on la laisse
            // telle quelle : la route s'y raccorde simplement (jonction),
            // ce qui n'est pas un cul-de-sac.
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
                // Carrefour complet : même tuile (symétrique) dans les deux styles.
                t = styleB ? TileType::PATH_ARROW : TileType::PATH_CROSSROADS;
            } else if (count == 3) {
                // Jonction en T : style 1 (réseau maison) a une vraie tuile de
                // T dédiée et orientable ; le style 2 (promenade) n'en a pas,
                // on garde donc PATH_ARROW pour lui.
                if (styleB) {
                    t = TileType::PATH_ARROW;
                } else {
                    t = TileType::PATH_T_JUNCTION;
                    t.rotation = tJunctionAngle(n, e, s, w);
                }
            } else if (count == 2) {
                bool vertical = n && s;
                bool horizontal = e && w;
                if (vertical || horizontal) {
                    t = straightTile;
                    t.rotation = vertical ? ANGLE_0 : ANGLE_90;
                } else {
                    t = curveTile;
                    t.rotation = curveAngle(n, e, s, w);
                }
            } else if (count == 1) {
                // Jamais de tuile "cul-de-sac" : on prolonge en ligne droite.
                t = straightTile;
                t.rotation = (n || s) ? ANGLE_0 : ANGLE_90;
            } else {
                t = straightTile;
            }
            setStructure(x, y, t);
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
                    t.rotation = curveAngle(n, e, s, w);
                }
            } else if (count == 1) {
                t = TileType::RAIL_STRAIGHT;
                t.rotation = (n || s) ? ANGLE_0 : ANGLE_90;
            } else {
                t = TileType::RAIL_STRAIGHT;
            }
            setStructure(x, y, t);
        }
    }
}

// Rails : plus rares (30% des cartes) et courts (5 à 10 cases depuis un
// bord de la zone jouable, pas du bord physique de la carte, pour ne pas
// traverser la clôture / la bande d'arbres).
void MapGenerator::generateRails() {
    std::uniform_real_distribution<float> chance(0.f, 1.f);
    if (chance(rng_) > 0.3f) return;
    if (playX1_ <= playX0_ || playY1_ <= playY0_) return;

    auto blocked = [&](int x, int y) {
        Cell c = grid_.at(x, y);
        return c == Cell::Wall || c == Cell::HouseFloor || c == Cell::Door ||
               c == Cell::Opening || c == Cell::Path;
    };

    std::uniform_int_distribution<int> edgeY(playY0_, playY1_);
    std::uniform_int_distribution<int> lengthDist(5, 10);

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

    // Le chariot est un DÉCOR posé par-dessus le rail : avec un simple
    // slot d'overlay, il remplacerait la tuile de rail en dessous, qui
    // deviendrait invisible. Avec addDecoration(), les deux coexistent
    // (rail en z=Structure, chariot en z=Decoration par-dessus).
    auto [cx, cy] = route.back();
    if (grid_.at(cx, cy) == Cell::Rail) {
        addDecoration(cx, cy, TileType::CART_WOODEN);
    }
}

void MapGenerator::placeDecorations() {
    std::uniform_real_distribution<float> chance(0.f, 1.f);

    // Pose un décor, avec une rotation aléatoire pour les types qui le
    // demandent (voir wantsRandomRotation).
    auto place = [&](int x, int y, Tile tile) {
        if (wantsRandomRotation(tile)) tile.rotation = randomAngle();
        addDecoration(x, y, tile);
    };

    // --- Extérieur : buissons, touffes d'herbe, caisses/tonneaux épars ---
    const Tile outdoorProps[] = {
        TileType::PROP_BUSH, TileType::PROP_BUSH,
        TileType::PROP_GRASS_TUFT, TileType::PROP_GRASS_TUFT, TileType::PROP_GRASS_TUFT,
        TileType::OBJECT_SMALL_CRATE_WOODEN, TileType::OBJECT_SMALL_BARREL_TOP
    };
    std::uniform_int_distribution<int> propPick(0, (int)(sizeof(outdoorProps) / sizeof(Tile)) - 1);

    for (int x = 0; x < grid_.width; ++x) {
        for (int y = 0; y < grid_.height; ++y) {
            Cell c = grid_.at(x, y);
            if (c != Cell::Grass && c != Cell::Dirt) continue;
            if (chance(rng_) < 0.06f) {
                place(x, y, outdoorProps[propPick(rng_)]);
            }
        }
    }

    // Passe dédiée : buissons supplémentaires (il en manquait trop). On
    // inclut aussi la terre (Dirt), qui en manquait totalement puisque
    // cette passe ne couvrait avant que l'herbe.
    for (int x = 0; x < grid_.width; ++x) {
        for (int y = 0; y < grid_.height; ++y) {
            Cell c = grid_.at(x, y);
            if (c != Cell::Grass && c != Cell::Dirt) continue;
            if (hasAnyOverlay(x, y)) continue;
            if (chance(rng_) < 0.05f) {
                place(x, y, TileType::PROP_BUSH);
            }
        }
    }

    // Passe dédiée : touffes d'herbe sur l'herbe et la terre restantes.
    for (int x = 0; x < grid_.width; ++x) {
        for (int y = 0; y < grid_.height; ++y) {
            Cell c = grid_.at(x, y);
            if (c != Cell::Grass && c != Cell::Dirt) continue;
            if (hasAnyOverlay(x, y)) continue;
            if (chance(rng_) < 0.08f) {
                place(x, y, TileType::PROP_GRASS_TUFT);
            }
        }
    }

    // Amas de caisses/tonneaux en extérieur : plusieurs, bien visibles,
    // dispersés sur la carte (pas seulement au hasard dans la boucle
    // générique ci-dessus, qui n'en posait presque jamais).
    const Tile outdoorStockpile[] = {
        TileType::OBJECT_LARGE_CRATE_WOODEN, TileType::OBJECT_LARGE_BARREL_TOP,
        TileType::OBJECT_BARRELS_CLUSTER_1, TileType::OBJECT_BARRELS_CLUSTER_2,
        TileType::OBJECT_SMALL_CRATE_WOODEN, TileType::OBJECT_SMALL_BARREL_TOP
    };
    std::uniform_int_distribution<int> stockpilePick(0, (int)(sizeof(outdoorStockpile) / sizeof(Tile)) - 1);
    int stockpileCount = std::clamp(grid_.width * grid_.height / 100, 4, 14);
    std::uniform_int_distribution<int> anyX(0, std::max(0, grid_.width - 1));
    std::uniform_int_distribution<int> anyY(0, std::max(0, grid_.height - 1));

    for (int i = 0; i < stockpileCount; ++i) {
        for (int tries = 0; tries < 25; ++tries) {
            int x = anyX(rng_), y = anyY(rng_);
            Cell c = grid_.at(x, y);
            if ((c == Cell::Grass || c == Cell::Dirt) && !hasAnyOverlay(x, y)) {
                place(x, y, outdoorStockpile[stockpilePick(rng_)]);
                break;
            }
        }
    }

    // Un feu de camp isolé si on trouve une case d'herbe libre.
    for (int tries = 0; tries < 15; ++tries) {
        int x = anyX(rng_), y = anyY(rng_);
        if (grid_.at(x, y) == Cell::Grass && !hasAnyOverlay(x, y)) {
            place(x, y, TileType::PROP_CAMPFIRE);
            break;
        }
    }

    // --- Intérieur : mobilier selon le type de maison ---
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
                keepClear = (d.y == h.bottom()) ? Point{d.x, d.y - 1} : Point{d.x, d.y + 1};
            }
        }

        if (h.type == HOUSE_BARRACKS) {
            // Lits alignés en rangées régulières ; on alterne les deux
            // variantes de lit pour distinguer visuellement chaque rangée.
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
            // Colonnes alternées table / chaise, plus un tonneau au fond.
            for (int x = h.x + 1; x < h.right(); ++x) {
                bool tableColumn = ((x - (h.x + 1)) % 2 == 0);
                for (int y = h.y + 1; y < h.bottom(); ++y) {
                    if (grid_.at(x, y) != Cell::HouseFloor) continue;
                    if (x == keepClear.first && y == keepClear.second) continue;
                    place(x, y, tableColumn ? TileType::OBJECT_TABLE : TileType::OBJECT_CHAIR);
                }
            }
            int bx = h.right() - 1, by = h.bottom() - 1;
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

// Assemble le résultat final : sol (z=Ground) + toutes les couches de
// superposition, triées par z croissant pour un rendu correct.
TileMap MapGenerator::assembleResult() const {
    TileMap result(grid_.width, std::vector<MapCell>(grid_.height));
    for (int x = 0; x < grid_.width; ++x) {
        for (int y = 0; y < grid_.height; ++y) {
            auto& cell = result[x][y];
            cell.layers.push_back({ground_[x][y], ZIndex::Ground});
            for (auto& layer : overlay_[x][y].layers) {
                cell.layers.push_back(layer);
            }
            std::stable_sort(cell.layers.begin(), cell.layers.end(),
                              [](const LayerTile& a, const LayerTile& b) { return a.z < b.z; });
        }
    }
    return result;
}

// ----------------------------------------------------------------------------
// Orchestration : enchaîne toutes les étapes ci-dessus dans l'ordre.
//
// NOTE IMPORTANTE : (height, width) désignent la taille de la zone JOUABLE
// (intérieur de la clôture). La clôture (barbelés + tours de garde) et la
// bande d'arbres qui l'entoure sont ajoutées EN PLUS de cette taille : la
// carte réellement renvoyée est donc plus grande que height*width. Par
// exemple pour un appel generateMap(50, 50, seed), avec une marge d'arbres
// de 2 cases, la carte retournée fait 56x56 (50 + 2*(marge 2 + clôture 1)).
// ----------------------------------------------------------------------------
TileMap MapGenerator::build(const int height, const int width, const int seed) {
    rng_ = std::mt19937(static_cast<unsigned int>(seed));

    // Largeur de la bande d'arbres ; la clôture (1 case) s'ajoute toujours
    // par-dessus, tant que la zone jouable est assez grande pour ça.
    if (std::min(width, height) >= 20)      margin_ = 2;
    else if (std::min(width, height) >= 8)  margin_ = 1;
    else                                     margin_ = 0;

    bool hasFence = (width >= 4 && height >= 4);
    const int fenceThickness = hasFence ? 1 : 0;
    const int border = margin_ + fenceThickness; // ajouté de CHAQUE côté

    const int totalWidth  = width  + 2 * border;
    const int totalHeight = height + 2 * border;
    initGrids(totalWidth, totalHeight);

    // Zone jouable, en coordonnées de la carte totale.
    playX0_ = border;
    playY0_ = border;
    playX1_ = border + width - 1;
    playY1_ = border + height - 1;

    if (hasFence) {
        buildPerimeterFence();
        if (margin_ > 0) placeTreeBorder();
    }

    int dirtBottom = generateBaseTerrain();

    // Les maisons démarrent sous les zones de terre pour ne pas s'y
    // superposer.
    int housesY0 = std::min(playY1_, std::max(playY0_, dirtBottom + 2));

    generateHouses(housesY0);
    carveHouseFloors();
    connectAdjacentHouses();

    buildWalls();
    placeDoors();

    // Réseau "maison" (style 1 uniquement) puis réseau "promenade" (routes
    // traversantes, style 2), tuilés ensemble à la fin pour rester
    // cohérents. Aucun des deux ne traverse les zones de terre.
    isPromenade_.assign(totalWidth, std::vector<bool>(totalHeight, false));
    generateHousePaths();
    generatePromenadePaths();
    autotilePathsUnified();

    generateRails();
    placeDecorations();

    return assembleResult();
}