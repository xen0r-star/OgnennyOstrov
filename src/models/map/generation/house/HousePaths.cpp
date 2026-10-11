#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <algorithm>
#include <functional>
#include <queue>
#include <random>
#include <utility>
#include <vector>

namespace {
    constexpr int EDGE_MARGIN    = 1;  // espace libre avec le bord de la zone de jeu
    constexpr int FREE_COST      = 3;  // coût d'une case vide
    constexpr int OWN_COST       = 1;  // coût d'une case où passe déjà un chemin de maison (réutilisation)
    constexpr int NEAR_HOUSE_COST = 2;  // supplément pour longer le mur d'un bâtiment
    constexpr int CROSS_COST     = 5;  // supplément pour traverser un chemin de garde / un rail
    constexpr int TURN_COST      = 2;  // supplément par virage
    constexpr int LOOP_PERCENT   = 10; // % de chances d'ajouter une liaison en plus (boucle) entre deux bâtiments proches
    constexpr int LOOP_MAX_DIST   = 14; // distance maximale (en tuiles) pour une liaison en plus

    // Rotations des tuiles. Droits et virages : mêmes conventions que les chemins de garde.
    // T et bout de chemin : rotations calées sur ton tileset.
    constexpr int ROT_T_MISSING_DOWN  = 0;   // T dont la branche manquante est en bas
    constexpr int ROT_T_MISSING_LEFT  = 90;
    constexpr int ROT_T_MISSING_UP    = 180;
    constexpr int ROT_T_MISSING_RIGHT = 270;
    constexpr int ROT_END_UP         = 0;   // bout de chemin ouvert vers le haut
    constexpr int ROT_END_RIGHT      = 90;
    constexpr int ROT_END_DOWN       = 180;
    constexpr int ROT_END_LEFT       = 270;

    // Textures de sol qui comptent comme "intérieur de bâtiment"
    constexpr Map::Texture FLOOR_TEXTURES[] = {
        Tiles::FLOOR_STONE_PLAIN.texture,
        Tiles::FLOOR_STONE_TILES.texture,
        Tiles::FLOOR_STONE_CROSS.texture,
        Tiles::FLOOR_STONE_PATTERN.texture,
        Tiles::FLOOR_STONE_CRACKED.texture,
        Tiles::FLOOR_WOOD_PLANKS.texture,
    };
    // Texture des champs de terre (Layer0)
    constexpr Map::Texture DIRT_TEXTURES[] = {
        Tiles::GROUND_DIRT.texture,
    };
    // Textures des chemins de garde (Layer1)
    constexpr Map::Texture PATH_TEXTURES[] = {
        Tiles::PATH_STRAIGHT_1.texture,
        Tiles::PATH_CURVE_1.texture,
        Tiles::PATH_CROSSROADS.texture,
        Tiles::PATH_T_JUNCTION.texture,
        Tiles::PATH_CURVE_2.texture,
        Tiles::PATH_STRAIGHT_2.texture,
        Tiles::PATH_ARROW.texture,
        Tiles::PATH_END.texture,
    };
    // Textures des rails (Layer2)
    constexpr Map::Texture RAIL_TEXTURES[] = {
        Tiles::RAIL_STRAIGHT.texture,
        Tiles::RAIL_CURVE.texture,
        Tiles::RAIL_CROSSROADS.texture,
        Tiles::CART_WOODEN.texture,
    };
}

void MapGenerator::addHousePaths(TileMap& map, std::mt19937& rng, const Rect& playZone) {
    // Bits combinables : dans quelles directions part le chemin depuis une tuile.
    // L'indice d (0..3) d'une direction vaut log2 de son bit : UP=0, RIGHT=1, DOWN=2, LEFT=3.
    enum Direction {
        UP = 1,
        RIGHT = 2,
        DOWN = 4,
        LEFT = 8
    };
    constexpr int DX[4] = { 0, 1, 0, -1 };
    constexpr int DY[4] = { -1, 0, 1, 0 };

    // Types de lignes déjà présentes (traversables à angle droit)
    enum Line { GUARD = 1, RAIL = 2 };

    // ---------- 2. Lecture de la map ----------
    const int W = playZone.w;
    const int H = playZone.h;
    auto idx = [&](const int x, const int y) { return (y - playZone.y) * W + (x - playZone.x); };
    auto inZone = [&](const int x, const int y) {
        return x >= playZone.x + EDGE_MARGIN && x < playZone.x + W - EDGE_MARGIN &&
               y >= playZone.y + EDGE_MARGIN && y < playZone.y + H - EDGE_MARGIN;
    };

    // La case (x, y) a-t-elle, sur ce calque, une tuile dont la texture est dans la liste ?
    auto hasTexture = [&](const int x, const int y, const int zIndex, const Map::Texture* list, const int count) {
        for (const Map::Tile& t : map.cells[y][x].layers) {
            if (t.zIndex != zIndex) continue;
            for (int i = 0; i < count; ++i) {
                if (t.texture.col == list[i].col && t.texture.row == list[i].row) return true;
            }
        }
        return false;
    };
    constexpr int floorCount = std::size(FLOOR_TEXTURES);
    constexpr int pathCount  = std::size(PATH_TEXTURES);
    constexpr int railCount  = std::size(RAIL_TEXTURES);

    // Lignes existantes (chemins de garde, rails)
    std::vector<unsigned char> lineKind(W * H, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (hasTexture(x, y, Map::ZIndex::Layer1, PATH_TEXTURES, pathCount)) lineKind[idx(x, y)] |= GUARD;
            if (hasTexture(x, y, Map::ZIndex::Layer2, RAIL_TEXTURES, railCount)) lineKind[idx(x, y)] |= RAIL;
        }
    }

    // Champs de terre : infranchissables pour les chemins de maison
    constexpr int dirtCount = std::size(DIRT_TEXTURES);
    std::vector<unsigned char> dirt(W * H, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (hasTexture(x, y, Map::ZIndex::Layer0, DIRT_TEXTURES, dirtCount)) dirt[idx(x, y)] = 1;
        }
    }

    // ---------- 3. Bâtiments ----------
    struct Anchor { int x, y, outDir; };
    struct House {
        int x, y, w, h;
        std::vector<Anchor> anchors;
    };
    std::vector<House> houses;
    std::vector<int> houseId(W * H, -1);

    auto isFloor = [&](const int x, const int y) {
        if (x < playZone.x || y < playZone.y || x >= playZone.x + W || y >= playZone.y + H) return false;
        return hasTexture(x, y, Map::ZIndex::Layer1, FLOOR_TEXTURES, floorCount);
    };

    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (houseId[idx(x, y)] >= 0 || !isFloor(x, y)) continue;

            // Coin haut-gauche trouvé : on mesure le rectangle
            int w = 0;
            while (isFloor(x + w, y)) ++w;
            int h = 0;
            while (isFloor(x, y + h)) ++h;
            if (w < 3 || h < 3) continue;

            const int id = static_cast<int>(houses.size());
            for (int yy = y; yy < y + h; ++yy) {
                for (int xx = x; xx < x + w; ++xx) {
                    houseId[idx(xx, yy)] = id;
                }
            }
            houses.push_back({ x, y, w, h, {} });
        }
    }

    const int n = static_cast<int>(houses.size());
    if (n < 2) return;

    // Cases collées à un bâtiment (pour éviter de longer les murs)
    std::vector<unsigned char> nearHouse(W * H, 0);
    for (const House& hs : houses) {
        for (int yy = hs.y - 1; yy <= hs.y + hs.h; ++yy) {
            for (int xx = hs.x - 1; xx <= hs.x + hs.w; ++xx) {
                if (xx < playZone.x || yy < playZone.y || xx >= playZone.x + W || yy >= playZone.y + H) continue;
                nearHouse[idx(xx, yy)] = 1;
            }
        }
    }

    // Points d'accroche : cases devant les murs (hors coins). outDir = direction qui s'éloigne du bâtiment.
    auto addAnchor = [&](House& hs, const int ax, const int ay, const int outDir) {
        if (!inZone(ax, ay)) return;
        const int i = idx(ax, ay);
        if (houseId[i] >= 0 || lineKind[i] != 0 || dirt[i]) return;
        hs.anchors.push_back({ ax, ay, outDir });
    };
    for (House& hs : houses) {
        for (int i = 1; i < hs.w - 1; ++i) {
            addAnchor(hs, hs.x + i, hs.y - 1,     0); // devant le mur du haut
            addAnchor(hs, hs.x + i, hs.y + hs.h,  2); // devant le mur du bas
        }
        for (int j = 1; j < hs.h - 1; ++j) {
            addAnchor(hs, hs.x + hs.w, hs.y + j,  1); // devant le mur de droite
            addAnchor(hs, hs.x - 1,    hs.y + j,  3); // devant le mur de gauche
        }
    }

    // ---------- 4. Quelles maisons relier ? ----------
    // Distance (Manhattan) entre les rectangles de deux bâtiments
    auto gapDist = [&](const House& a, const House& b) {
        const int dx = std::max(0, std::max(a.x - (b.x + b.w), b.x - (a.x + a.w)));
        const int dy = std::max(0, std::max(a.y - (b.y + b.h), b.y - (a.y + a.h)));
        return dx + dy;
    };

    std::vector<std::pair<int, int>> links;
    std::vector<char> linked(n * n, 0);
    std::vector<char> inTree(n, 0);
    inTree[0] = 1;

    // Arbre couvrant minimal (Prim) : tout est relié, en commençant par les plus proches
    for (int step = 1; step < n; ++step) {
        int best = 1 << 30, bestA = -1, bestB = -1;
        for (int a = 0; a < n; ++a) {
            if (!inTree[a]) continue;
            for (int b = 0; b < n; ++b) {
                if (inTree[b]) continue;
                const int d = gapDist(houses[a], houses[b]);
                if (d < best) { best = d; bestA = a; bestB = b; }
            }
        }
        inTree[bestB] = 1;
        links.emplace_back(bestA, bestB);
        linked[bestA * n + bestB] = linked[bestB * n + bestA] = 1;
    }

    // Quelques liaisons en plus pour créer des boucles
    std::uniform_int_distribution<int> percent(0, 99);
    for (int a = 0; a < n; ++a) {
        for (int b = a + 1; b < n; ++b) {
            if (linked[a * n + b]) continue;
            if (gapDist(houses[a], houses[b]) > LOOP_MAX_DIST) continue;
            if (percent(rng) >= LOOP_PERCENT) continue;
            links.emplace_back(a, b);
            linked[a * n + b] = linked[b * n + a] = 1;
        }
    }

    // ---------- 5. Tracé des liaisons ----------
    // edge[] : directions (bits) dans lesquelles le chemin continue depuis chaque case. 0 = pas de chemin.
    std::vector<unsigned char> edge(W * H, 0);
    std::vector<unsigned char> targetDoor(W * H, 0); // bit vers le mur, sur les points d'accroche de la destination

    constexpr int INF = 1 << 29;
    std::vector dist(W * H * 4, INF);
    std::vector prev(W * H * 4, -1);

    // Peut-on traverser la ligne (garde / rail) de la case (x, y) en allant dans la direction d ?
    // Oui seulement à angle droit : aucune case voisine du même type dans l'axe du déplacement.
    auto canCross = [&](const int x, const int y, const int d) {
        const int kind = lineKind[idx(x, y)];
        if (kind == (GUARD | RAIL)) return false; // intersection de deux lignes : on évite
        const bool horizontal = (d % 2 == 1);
        const int ax = horizontal ? 1 : 0;
        const int ay = horizontal ? 0 : 1;
        for (int s = -1; s <= 1; s += 2) {
            const int nx = x + s * ax;
            const int ny = y + s * ay;
            if (nx < playZone.x || ny < playZone.y || nx >= playZone.x + W || ny >= playZone.y + H) continue;
            if (lineKind[idx(nx, ny)] & kind) return false;
        }
        return true;
    };

    // Une case contient-elle déjà du chemin (chemin de maison posé, ou chemin de garde) ?
    auto isPathCell = [&](const int x, const int y) {
        if (x < playZone.x || y < playZone.y || x >= playZone.x + W || y >= playZone.y + H) return false;
        const int i = idx(x, y);
        return edge[i] != 0 || (lineKind[i] & GUARD) != 0;
    };

    // Poser un nouveau chemin en (nx, ny) formerait-il un bloc de 2x2 cases de chemin ?
    // On compte aussi les 3 dernières cases du tracé en cours (état s), pas encore dans edge[].
    auto makesBlock = [&](const int nx, const int ny, const int s) {
        int chain[3] = { -1, -1, -1 };
        int t = s;
        for (int k = 0; k < 3 && t != -1; ++k) {
            chain[k] = t >> 2;
            t = prev[t];
        }
        auto occupied = [&](const int x, const int y) {
            if (x < playZone.x || y < playZone.y || x >= playZone.x + W || y >= playZone.y + H) return false;
            if (isPathCell(x, y)) return true;
            const int i = idx(x, y);
            return chain[0] == i || chain[1] == i || chain[2] == i;
        };
        // Les 4 carrés de 2x2 qui contiennent la case (nx, ny)
        for (int ox = -1; ox <= 0; ++ox) {
            for (int oy = -1; oy <= 0; ++oy) {
                bool full = true;
                for (int i = 0; i < 2 && full; ++i) {
                    for (int j = 0; j < 2; ++j) {
                        const int x = nx + ox + i;
                        const int y = ny + oy + j;
                        if (x == nx && y == ny) continue;
                        if (!occupied(x, y)) { full = false; break; }
                    }
                }
                if (full) return true;
            }
        }
        return false;
    };

    for (const auto& [a, b] : links) {
        const House& from = houses[a];
        const House& to   = houses[b];
        if (from.anchors.empty() || to.anchors.empty()) continue;

        std::fill(dist.begin(), dist.end(), INF);
        std::fill(prev.begin(), prev.end(), -1);
        std::fill(targetDoor.begin(), targetDoor.end(), 0);

        for (const Anchor& an : to.anchors) {
            targetDoor[idx(an.x, an.y)] = static_cast<unsigned char>(1 << ((an.outDir + 2) & 3));
        }

        using Entry = std::pair<int, int>; // (coût, état) ; état = case * 4 + direction d'arrivée
        std::priority_queue<Entry, std::vector<Entry>, std::greater<>> queue;
        for (const Anchor& an : from.anchors) {
            const int ai = idx(an.x, an.y);
            if (edge[ai] == 0 && makesBlock(an.x, an.y, -1)) continue; // ce départ ferait un bloc de 2x2
            const int s = ai * 4 + an.outDir;
            dist[s] = 0;
            queue.emplace( 0, s );
        }

        int goal = -1;
        while (!queue.empty()) {
            const auto [cost, s] = queue.top();
            queue.pop();
            if (cost > dist[s]) continue;

            const int cell = s >> 2;
            const int d    = s & 3;
            const int cx   = playZone.x + cell % W;
            const int cy   = playZone.y + cell / W;

            if (targetDoor[cell]) { goal = s; break; }

            const bool onLine = lineKind[cell] != 0;
            for (int turn = 0; turn < 3; ++turn) {
                const int nd = (turn == 0) ? d : (turn == 1 ? (d + 1) & 3 : (d + 3) & 3);
                if (onLine && nd != d) continue; // on traverse une ligne tout droit

                const int nx = cx + DX[nd];
                const int ny = cy + DY[nd];
                if (!inZone(nx, ny)) continue;

                const int ni = idx(nx, ny);
                if (houseId[ni] >= 0) continue;
                if (dirt[ni]) continue;

                int step = (edge[ni] != 0) ? OWN_COST : FREE_COST;
                if (lineKind[ni] != 0) {
                    if (onLine || !canCross(nx, ny, nd)) continue;
                    step += CROSS_COST;
                }
                // Nouvelle case de chemin : interdit si elle forme un bloc de 2x2
                const bool isNewPath = edge[ni] == 0 && !(lineKind[ni] & GUARD);
                if (isNewPath && makesBlock(nx, ny, s)) continue;
                if (nearHouse[ni] && edge[ni] == 0) step += NEAR_HOUSE_COST;
                if (nd != d) step += TURN_COST;

                const int ns = ni * 4 + nd;
                const int nc = cost + step;
                if (nc < dist[ns]) {
                    dist[ns] = nc;
                    prev[ns] = s;
                    queue.emplace( nc, ns );
                }
            }
        }

        if (goal < 0) continue;

        // Reconstruire le chemin (de la destination vers la source)
        std::vector<int> states;
        for (int s = goal; s != -1; s = prev[s]) states.push_back(s);
        std::reverse(states.begin(), states.end());

        for (size_t i = 0; i + 1 < states.size(); ++i) {
            const int ca = states[i] >> 2;
            const int cb = states[i + 1] >> 2;
            const int d  = states[i + 1] & 3; // direction du déplacement a -> b
            edge[ca] |= static_cast<unsigned char>(1 << d);
            edge[cb] |= static_cast<unsigned char>(1 << ((d + 2) & 3));
        }

        // Les deux bouts du chemin s'ouvrent vers le mur de leur bâtiment (future porte)
        const int startOut = states.front() & 3;
        edge[states.front() >> 2] |= static_cast<unsigned char>(1 << ((startOut + 2) & 3));
        edge[goal >> 2] |= targetDoor[goal >> 2];
    }

    // ---------- 6. Fusion fin / gros ----------
    // Une case de chemin fin qui touche (par un côté) un chemin gros sans y être reliée devient grosse
    // et est reliée à lui. On répète tant que ça change : une case devenue grosse peut elle-même
    // se retrouver collée à un autre chemin fin.
    // Les cases qui touchent le gros parce que le chemin le traverse sont déjà reliées (bit dans
    // edge[]) : elles sont ignorées.
    std::vector<unsigned char> thick(W * H, 0);
    for (bool changed = true; changed;) {
        changed = false;
        for (int y = playZone.y; y < playZone.y + H; ++y) {
            for (int x = playZone.x; x < playZone.x + W; ++x) {
                const int i = idx(x, y);
                if (edge[i] == 0 || lineKind[i] != 0 || thick[i]) continue;

                int touching = 0; // directions où un gros est collé sans être relié
                for (int d = 0; d < 4; ++d) {
                    if (edge[i] & (1 << d)) continue; // déjà relié dans cette direction
                    const int nx = x + DX[d];
                    const int ny = y + DY[d];
                    if (nx < playZone.x || ny < playZone.y || nx >= playZone.x + W || ny >= playZone.y + H) continue;
                    const int ni = idx(nx, ny);
                    if ((lineKind[ni] & GUARD) || thick[ni]) touching |= (1 << d);
                }
                if (touching == 0) continue;

                // La case devient grosse et se relie directement à chaque gros collé
                thick[i] = 1;
                edge[i] |= static_cast<unsigned char>(touching);
                for (int d = 0; d < 4; ++d) {
                    if (!(touching & (1 << d))) continue;
                    edge[idx(x + DX[d], y + DY[d])] |= static_cast<unsigned char>(1 << ((d + 2) & 3));
                }
                changed = true;
            }
        }
    }

    // ---------- 7. Poser les tuiles ----------
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            const int i = idx(x, y);
            int mask = edge[i];
            if (mask == 0) continue;

            // Un chemin de garde traversé devient un carrefour : on garde ses propres directions
            if (lineKind[i] & GUARD) {
                for (int d = 0; d < 4; ++d) {
                    const int nx = x + DX[d];
                    const int ny = y + DY[d];
                    if (nx < playZone.x || ny < playZone.y || nx >= playZone.x + W || ny >= playZone.y + H) continue;
                    if (lineKind[idx(nx, ny)] & GUARD) mask |= (1 << d);
                }
            }

            // Gros (style 2) pour les chemins de garde et les parties fusionnées, fin (style 1) sinon
            const bool isThick = thick[i] || (lineKind[i] & GUARD);
            const Map::Tile straight = isThick ? Tiles::PATH_STRAIGHT_2 : Tiles::PATH_STRAIGHT_1;
            const Map::Tile curve    = isThick ? Tiles::PATH_CURVE_2    : Tiles::PATH_CURVE_1;

            Map::Tile tile;
            switch (mask) {
                case UP | DOWN:    tile = straight; tile.modifier.rotation = 0;   break;
                case LEFT | RIGHT: tile = straight; tile.modifier.rotation = 90;  break;
                case RIGHT | DOWN: tile = curve;    tile.modifier.rotation = 0;   break;
                case LEFT | DOWN:  tile = curve;    tile.modifier.rotation = 90;  break;
                case LEFT | UP:    tile = curve;    tile.modifier.rotation = 180; break;
                case RIGHT | UP:   tile = curve;    tile.modifier.rotation = 270; break;

                case RIGHT | DOWN | LEFT: tile = Tiles::PATH_T_JUNCTION; tile.modifier.rotation = ROT_T_MISSING_UP;    break;
                case UP | DOWN | LEFT:    tile = Tiles::PATH_T_JUNCTION; tile.modifier.rotation = ROT_T_MISSING_RIGHT; break;
                case UP | RIGHT | LEFT:   tile = Tiles::PATH_T_JUNCTION; tile.modifier.rotation = ROT_T_MISSING_DOWN;  break;
                case UP | RIGHT | DOWN:   tile = Tiles::PATH_T_JUNCTION; tile.modifier.rotation = ROT_T_MISSING_LEFT;  break;

                case UP | RIGHT | DOWN | LEFT: tile = Tiles::PATH_CROSSROADS; tile.modifier.rotation = 0; break;

                case UP:    tile = Tiles::PATH_END; tile.modifier.rotation = ROT_END_UP;    break;
                case RIGHT: tile = Tiles::PATH_END; tile.modifier.rotation = ROT_END_RIGHT; break;
                case DOWN:  tile = Tiles::PATH_END; tile.modifier.rotation = ROT_END_DOWN;  break;
                case LEFT:  tile = Tiles::PATH_END; tile.modifier.rotation = ROT_END_LEFT;  break;
                default: continue;
            }
            addTile(map, x, y, tile, Map::ZIndex::Layer1);
        }
    }
}