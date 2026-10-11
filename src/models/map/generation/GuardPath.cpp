#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <algorithm>
#include <random>
#include <vector>
#include <functional>
#include <queue>
#include <utility>

namespace {
    constexpr int WIDTH_MIN = 6;   // distance minimale entre deux chemins parallèles
    constexpr int WIDTH_MAX = 12;  // distance maximale (avant répartition du reste)
    constexpr int MARGIN   = 2;   // bord libre autour de la forme (0 = touche les bords)

    constexpr int EDGE_MARGIN     = 1;   // espace libre avec le bord de la zone de jeu
    constexpr int FREE_COST       = 3;   // coût d'une case vide
    constexpr int TURN_COST       = 2;   // supplément par virage
    constexpr int NEAR_HOUSE_COST = 1;   // supplément pour longer un bâtiment
    constexpr int NEAR_PATH_COST  = 6;   // supplément pour longer un autre chemin
    constexpr int DIRT_COST       = 3;   // supplément pour traverser un champ
    constexpr int RAIL_COST       = 12;  // supplément pour traverser un rail (dernier recours)
}

void MapGenerator::addGuardPaths(TileMap& map, std::mt19937& rng, const Rect& playZone) {
    enum Direction {
        UP = 1,
        RIGHT = 2,
        DOWN = 4,
        LEFT = 8
    };

    std::uniform_int_distribution widthDist(WIDTH_MIN, WIDTH_MAX);

    // ---------- 1. Positions des lignes de la grille ----------
    // pos[0] = positions en x, pos[1] = positions en y.
    // Chaque colonne (ou ligne) a sa propre largeur aléatoire.
    std::vector<int> pos[2];
    int size[2] = { playZone.w - 1 - 2 * MARGIN, playZone.h - 1 - 2 * MARGIN };

    for (int axis = 0; axis < 2; ++axis) {
        // a) On tire des largeurs tant que ça rentre
        std::vector<int> widths;
        int total = 0;
        while (true) {
            int width = widthDist(rng);
            if (total + width > size[axis]) break;
            total += width;
            widths.push_back(width);
        }

        // b) Nombre de colonnes impair (nœud, liaison, nœud, ... nœud)
        if (widths.size() % 2 == 0 && !widths.empty()) {
            total -= widths.back();
            widths.pop_back();
        }
        if (widths.size() < 3) return; // zone trop petite

        // c) On répartit l'espace restant, 1 tuile à la fois, sur toutes les colonnes
        int leftover = size[axis] - total;
        for (int i = 0; i < leftover; ++i) {
            widths[i % widths.size()] += 1;
        }

        // d) Largeurs -> positions
        int sum = 0;
        pos[axis].push_back(0);
        for (int width : widths) {
            sum += width;
            pos[axis].push_back(sum);
        }
    }

    int gw = static_cast<int>(pos[0].size()) - 1; // nombre de colonnes
    int gh = static_cast<int>(pos[1].size()) - 1; // nombre de lignes
    int nw = (gw + 1) / 2;           // nombre de nœuds en largeur
    int nh = (gh + 1) / 2;           // nombre de nœuds en hauteur

    int ox = playZone.x + MARGIN;
    int oy = playZone.y + MARGIN;


    // ---------- 2. Arbre aléatoire (parcours en profondeur) ----------
    int pw = gw + 2;
    std::vector<int> solid(pw * (gh + 2), 0);
    std::vector<int> visited(nw * nh, 0);
    std::vector<int> stackI, stackJ;

    visited[0] = 1;
    solid[1 * pw + 1] = 1;
    stackI.push_back(0);
    stackJ.push_back(0);

    while (!stackI.empty()) {
        int i = stackI.back();
        int j = stackJ.back();

        int candI[4], candJ[4];
        int count = 0;
        for (int k = 0; k < 4; ++k) {
            constexpr int dj[4] = {0, 0, 1, -1};
            constexpr int di[4] = {1, -1, 0, 0};
            int ni = i + di[k];
            int nj = j + dj[k];

            if (ni >= 0 && nj >= 0 && ni < nw && nj < nh && visited[nj * nw + ni] == 0) {
                candI[count] = ni;
                candJ[count] = nj;
                ++count;
            }
        }

        if (count == 0) {
            stackI.pop_back();
            stackJ.pop_back();
            continue;
        }

        std::uniform_int_distribution<int> pick(0, count - 1);
        int c = pick(rng);
        int ni = candI[c];
        int nj = candJ[c];

        visited[nj * nw + ni] = 1;
        solid[(2 * nj + 1) * pw + (2 * ni + 1)] = 1;
        solid[(j + nj + 1) * pw + (i + ni + 1)] = 1;
        stackI.push_back(ni);
        stackJ.push_back(nj);
    }


    // ---------- 3. Contour de l'arbre ----------
    int vw = gw + 1;
    std::vector<int> nextCorner(vw * (gh + 1), -1);

    for (int cy = 0; cy < gh; ++cy) {
        for (int cx = 0; cx < gw; ++cx) {
            if (solid[(cy + 1) * pw + (cx + 1)] == 0) continue;

            int up    = solid[cy * pw + (cx + 1)];
            int down  = solid[(cy + 2) * pw + (cx + 1)];
            int left  = solid[(cy + 1) * pw + cx];
            int right = solid[(cy + 1) * pw + (cx + 2)];

            if (up == 0)    nextCorner[cy * vw + cx]             = cy * vw + (cx + 1);
            if (right == 0) nextCorner[cy * vw + (cx + 1)]       = (cy + 1) * vw + (cx + 1);
            if (down == 0)  nextCorner[(cy + 1) * vw + (cx + 1)] = (cy + 1) * vw + cx;
            if (left == 0)  nextCorner[(cy + 1) * vw + cx]       = cy * vw + cx;
        }
    }

    int start = -1;
    for (int v = 0; v < static_cast<int>(nextCorner.size()); ++v) {
        if (nextCorner[v] >= 0) { start = v; break; }
    }
    if (start < 0) return;


    // ---------- 4. Liste des tuiles de la boucle, dans l'ordre ----------
    std::vector<int> cellsX, cellsY;
    int v = start;
    do {
        int nv = nextCorner[v];

        // Position réelle (en tuiles) du coin de départ et du coin d'arrivée
        int sx = ox + pos[0][v % vw];
        int sy = oy + pos[1][v / vw];
        int ex = ox + pos[0][nv % vw];
        int ey = oy + pos[1][nv / vw];

        int dx = (ex > sx) - (ex < sx); // -1, 0 ou 1
        int dy = (ey > sy) - (ey < sy);

        // On avance jusqu'au prochain coin (il est ajouté par le segment suivant)
        while (sx != ex || sy != ey) {
            cellsX.push_back(sx);
            cellsY.push_back(sy);
            sx += dx;
            sy += dy;
        }
        v = nv;
    } while (v != start);


    // ---------- 5. Poser les tuiles ----------
    int n = static_cast<int>(cellsX.size());
    for (int i = 0; i < n; ++i) {
        int cx = cellsX[i];
        int cy = cellsY[i];

        int neighX[2] = { cellsX[(i + n - 1) % n], cellsX[(i + 1) % n] };
        int neighY[2] = { cellsY[(i + n - 1) % n], cellsY[(i + 1) % n] };

        int mask = 0;
        for (int k = 0; k < 2; ++k) {
            if (neighX[k] > cx)      mask |= RIGHT;
            else if (neighX[k] < cx) mask |= LEFT;
            else if (neighY[k] > cy) mask |= DOWN;
            else                     mask |= UP;
        }

        Map::Tile tile;
        switch (mask) {
            case UP | DOWN:    tile = Tiles::PATH_STRAIGHT_2; tile.modifier.rotation = 0;   break;
            case LEFT | RIGHT: tile = Tiles::PATH_STRAIGHT_2; tile.modifier.rotation = 90;  break;
            case RIGHT | DOWN: tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 0;   break;
            case LEFT | DOWN:  tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 90;  break;
            case LEFT | UP:    tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 180; break;
            case RIGHT | UP:   tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 270; break;
            default: continue;
        }
        addTile(map, cx, cy, tile, Map::ZIndex::Layer1);
    }
}

void MapGenerator::correctionsGuardPaths(TileMap& map, std::mt19937& rng, const Rect& playZone) {
    (void)rng;

    enum Direction {
        UP = 1,
        RIGHT = 2,
        DOWN = 4,
        LEFT = 8
    };
    constexpr int DX[4] = { 0, 1, 0, -1 };
    constexpr int DY[4] = { -1, 0, 1, 0 };
    constexpr int INF             = 1 << 29;

    // ---------- 1. Outils ----------
    const int W = playZone.w;
    const int H = playZone.h;
    const int N = W * H;
    auto idx = [&](const int x, const int y) { return (y - playZone.y) * W + (x - playZone.x); };
    auto inBounds = [&](const int x, const int y) {
        return x >= playZone.x && y >= playZone.y && x < playZone.x + W && y < playZone.y + H;
    };
    auto inZone = [&](const int x, const int y) {
        return x >= playZone.x + EDGE_MARGIN && x < playZone.x + W - EDGE_MARGIN &&
               y >= playZone.y + EDGE_MARGIN && y < playZone.y + H - EDGE_MARGIN;
    };
    auto sameTex = [](const Map::Texture& a, const Map::Texture& b) {
        return a.col == b.col && a.row == b.row;
    };

    // Directions ouvertes d'une tuile de chemin de garde (mêmes conventions que addGuardPaths)
    auto guardMask = [&](const Map::Tile& t) -> int {
        const int rot = ((t.modifier.rotation % 360) + 360) % 360;
        if (sameTex(t.texture, Tiles::PATH_STRAIGHT_2.texture)) {
            return (rot % 180 == 0) ? (UP | DOWN) : (LEFT | RIGHT);
        }
        if (sameTex(t.texture, Tiles::PATH_CURVE_2.texture)) {
            switch (rot) {
                case 0:   return RIGHT | DOWN;
                case 90:  return LEFT | DOWN;
                case 180: return LEFT | UP;
                default:  return RIGHT | UP;
            }
        }
        return 0;
    };

    // ---------- 2. Lecture de la map ----------
    std::vector<int>  cur(N, 0);        // directions ouvertes de chaque case de chemin
    std::vector<char> pathCell(N, 0);   // la case fait partie du chemin de garde
    std::vector<char> blocked(N, 0);    // bâtiment ou autre objet : infranchissable
    std::vector<char> railCell(N, 0);   // rail (franchissable en dernier recours)
    std::vector<char> dirt(N, 0);       // champ de terre
    std::vector<char> changed(N, 0);    // tuile à réécrire

    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            const int i = idx(x, y);
            for (const Map::Tile& t : map.cells[y][x].layers) {
                if (t.zIndex == Map::ZIndex::Layer1) cur[i] |= guardMask(t);
                if (t.zIndex == Map::ZIndex::Layer0 && sameTex(t.texture, Tiles::GROUND_DIRT.texture)) dirt[i] = 1;
            }
            if (cur[i] != 0) pathCell[i] = 1;
            else if (!isTileEmpty(map, x, y, Map::ZIndex::Layer1)) blocked[i] = 1; // sol de maison, etc.
            if (!isTileEmpty(map, x, y, Map::ZIndex::Layer3)) blocked[i] = 1;
            if (!isTileEmpty(map, x, y, Map::ZIndex::Layer2)) railCell[i] = 1;
        }
    }

    // Cases collées (8 voisines) à un bâtiment
    std::vector<char> nearHouse(N, 0);
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            if (!blocked[idx(x, y)]) continue;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (inBounds(x + dx, y + dy)) nearHouse[idx(x + dx, y + dy)] = 1;
                }
            }
        }
    }

    // ---------- 3. Tronçons (union-find) ----------
    std::vector<int> parent(N);
    for (int i = 0; i < N; ++i) parent[i] = i;
    auto find = [&](int a) {
        while (parent[a] != a) {
            parent[a] = parent[parent[a]];
            a = parent[a];
        }
        return a;
    };
    auto unite = [&](const int a, const int b) { parent[find(a)] = find(b); };

    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            const int i = idx(x, y);
            if (!pathCell[i]) continue;
            for (int d = 1; d <= 2; ++d) { // RIGHT et DOWN suffisent
                if (!(cur[i] & (1 << d))) continue;
                const int nx = x + DX[d];
                const int ny = y + DY[d];
                if (!inBounds(nx, ny)) continue;
                const int ni = idx(nx, ny);
                if (cur[ni] & (1 << ((d + 2) & 3))) unite(i, ni);
            }
        }
    }

    // ---------- 4. Bouts cassés ----------
    // Une direction ouverte dont la voisine ne répond pas = le chemin a été coupé (maison posée dessus).
    // On retire ces directions : need[] = nombre de liaisons à refaire sur la case.
    std::vector<int> stubMask(N, 0);
    std::vector<int> need(N, 0);
    std::vector<int> needCells;

    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            const int i = idx(x, y);
            if (!pathCell[i]) continue;
            for (int d = 0; d < 4; ++d) {
                const int bit = 1 << d;
                if (!(cur[i] & bit)) continue;
                const int nx = x + DX[d];
                const int ny = y + DY[d];
                const bool linked = inBounds(nx, ny) && (cur[idx(nx, ny)] & (1 << ((d + 2) & 3)));
                if (!linked) {
                    stubMask[i] |= bit;
                    ++need[i];
                }
            }
            if (need[i] > 0) needCells.push_back(i);
        }
    }
    if (needCells.empty()) return; // la boucle est intacte
    for (const int c : needCells) {
        cur[c] &= ~stubMask[c];
        changed[c] = 1;
    }

    // Traverser un rail à angle droit : aucune case de rail voisine dans l'axe du déplacement
    auto canCrossRail = [&](const int x, const int y, const int d) {
        const bool horizontal = (d % 2 == 1);
        const int ax = horizontal ? 1 : 0;
        const int ay = horizontal ? 0 : 1;
        for (int s = -1; s <= 1; s += 2) {
            const int nx = x + s * ax;
            const int ny = y + s * ay;
            if (inBounds(nx, ny) && railCell[idx(nx, ny)]) return false;
        }
        return true;
    };

    // ---------- 5. Contournements ----------
    std::vector<int> dist(N * 4);
    std::vector<int> prev(N * 4);

    for (const int startCell : needCells) {
        while (need[startCell] > 0) {
            // Tant qu'il reste un autre tronçon, on se relie à lui ; sinon on ferme la boucle
            bool otherChain = false;
            for (const int c : needCells) {
                if (need[c] > 0 && find(c) != find(startCell)) { otherChain = true; break; }
            }
            auto isGoal = [&](const int c) {
                return c != startCell && need[c] > 0 && (!otherChain || find(c) != find(startCell));
            };
            // Case de chemin déjà posée (hors départ et cibles) collée à (x, y) ?
            auto touchesPath = [&](const int x, const int y) {
                for (int d = 0; d < 4; ++d) {
                    const int nx = x + DX[d];
                    const int ny = y + DY[d];
                    if (!inBounds(nx, ny)) continue;
                    const int ni = idx(nx, ny);
                    if (pathCell[ni] && ni != startCell && need[ni] == 0) return true;
                }
                return false;
            };

            int goal = -1;
            // Essai 1 : sans toucher aux rails. Essai 2 : en les traversant si nécessaire.
            for (int attempt = 0; attempt < 2 && goal < 0; ++attempt) {
                const bool allowRail = (attempt == 1);

                std::fill(dist.begin(), dist.end(), INF);
                std::fill(prev.begin(), prev.end(), -1);

                using Entry = std::pair<int, int>; // (coût, état) ; état = case * 4 + direction
                std::priority_queue<Entry, std::vector<Entry>, std::greater<>> queue;
                for (int d = 0; d < 4; ++d) {
                    const int s = startCell * 4 + d;
                    dist[s] = 0;
                    queue.emplace(0, s);
                }

                while (!queue.empty()) {
                    const auto [cost, s] = queue.top();
                    queue.pop();
                    if (cost > dist[s]) continue;

                    const int cell = s >> 2;
                    const int d    = s & 3;
                    if (cell != startCell && isGoal(cell)) { goal = s; break; }

                    const int cx = playZone.x + cell % W;
                    const int cy = playZone.y + cell / W;
                    const bool onRail = railCell[cell] && !pathCell[cell];

                    for (int turn = 0; turn < 3; ++turn) {
                        const int nd = (turn == 0) ? d : (turn == 1 ? (d + 1) & 3 : (d + 3) & 3);
                        if (onRail && nd != d) continue; // un rail se traverse tout droit

                        const int nx = cx + DX[nd];
                        const int ny = cy + DY[nd];
                        if (!inZone(nx, ny)) continue;

                        const int ni = idx(nx, ny);
                        if (blocked[ni]) continue;

                        int step = FREE_COST;
                        if (pathCell[ni]) {
                            if (!isGoal(ni)) continue; // on ne marche pas sur un chemin existant
                        } else {
                            if (railCell[ni]) {
                                if (!allowRail || onRail || !canCrossRail(nx, ny, nd)) continue;
                                step += RAIL_COST;
                            }
                            if (dirt[ni])               step += DIRT_COST;
                            if (nearHouse[ni])          step += NEAR_HOUSE_COST;
                            if (touchesPath(nx, ny))    step += NEAR_PATH_COST;
                        }
                        if (nd != d) step += TURN_COST;

                        const int ns = ni * 4 + nd;
                        const int nc = cost + step;
                        if (nc < dist[ns]) {
                            dist[ns] = nc;
                            prev[ns] = s;
                            queue.emplace(nc, ns);
                        }
                    }
                }
            }

            if (goal < 0) break; // aucun contournement possible : on laisse la case telle quelle

            // Reconstruire le tracé et ouvrir les directions de chaque case
            std::vector<int> states;
            for (int s = goal; s != -1; s = prev[s]) states.push_back(s);
            std::reverse(states.begin(), states.end());

            for (size_t i = 0; i + 1 < states.size(); ++i) {
                const int ca = states[i] >> 2;
                const int cb = states[i + 1] >> 2;
                const int d  = states[i + 1] & 3; // direction du déplacement a -> b
                cur[ca] |= 1 << d;
                cur[cb] |= 1 << ((d + 2) & 3);
                changed[ca] = changed[cb] = 1;
                pathCell[ca] = pathCell[cb] = 1;
            }

            const int goalCell = goal >> 2;
            --need[startCell];
            --need[goalCell];
            unite(startCell, goalCell);
        }
    }

    // ---------- 6. Réécriture des tuiles ----------
    for (int y = playZone.y; y < playZone.y + H; ++y) {
        for (int x = playZone.x; x < playZone.x + W; ++x) {
            const int i = idx(x, y);
            if (!changed[i] || need[i] != 0) continue; // inchangée, ou contournement raté : on ne touche pas

            Map::Tile tile;
            switch (cur[i]) {
                case UP | DOWN:    tile = Tiles::PATH_STRAIGHT_2; tile.modifier.rotation = 0;   break;
                case LEFT | RIGHT: tile = Tiles::PATH_STRAIGHT_2; tile.modifier.rotation = 90;  break;
                case RIGHT | DOWN: tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 0;   break;
                case LEFT | DOWN:  tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 90;  break;
                case LEFT | UP:    tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 180; break;
                case RIGHT | UP:   tile = Tiles::PATH_CURVE_2;    tile.modifier.rotation = 270; break;
                default: continue;
            }
            addTile(map, x, y, tile, Map::ZIndex::Layer1);
        }
    }
}
