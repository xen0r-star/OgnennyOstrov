#include "models/map/MapGenerator.h"
#include "models/map/Tiles.h"
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <queue>
#include <random>
#include <utility>
#include <vector>

namespace {
    // ---------- Offsets ----------
    // Taille d'une tuile dans l'unité de Modifier::offsetX / offsetY (un demi-décalage = HALF_TILE).
    constexpr float TILE_SIZE = 128.f;
    constexpr float HALF_TILE = TILE_SIZE * 0.5f;

    // Décalage vers le mur pour les objets posés contre un mur (les murs ne font qu'1/3 de tuile).
    // Si un objet mord sur le mur, diminue cette valeur (ex: TILE_SIZE * 0.4f).
    constexpr float WALL_HUG = HALF_TILE;

    // ---------- Directions ----------
    // Index : 0 = haut, 1 = droite, 2 = bas, 3 = gauche  ->  rotation d'un objet "dos au mur" = 90 * index
    constexpr int DX4[4] = { 0, 1, 0, -1 };
    constexpr int DY4[4] = { -1, 0, 1, 0 };

    // Direction vers laquelle la texture de la chaise regarde à rotation 0 (0 = haut).
    constexpr int CHAIR_TEXTURE_FACING = 0;

    // Rotation d'une chaise regardant dans la direction d. Une chaise en face d'une autre regarde
    // dans la direction opposée ((d + 2) & 3) : les deux sont donc toujours symétriques.
    constexpr int chairRotation(const int d) {
        return (((d - CHAIR_TEXTURE_FACING) & 3) * 90) % 360;
    }

    // ---------- Réglages ----------
    // Dortoir
    constexpr int OLD_BED_PERCENT      = 35; // % de vieux lits (BED_2)
    constexpr int DORM_WALL_PERCENT    = 90; // % de lits contre les murs
    constexpr int DORM_ROW_PERCENT     = 90; // % de lits dans les rangées du centre

    // Réfectoire
    constexpr int MESS_GROUP_MIN       = 4;  // Nb de postes (table + 2 chaises) collés avant un trou
    constexpr int MESS_GROUP_MAX       = 8;
    constexpr int MESS_FILL_PERCENT    = 55; // % de trous entre groupes comblés par un poste en plus
    constexpr int MESS_EDGE_PERCENT    = 40; // % de pièces où les postes touchent les murs (sinon 1 tuile libre)
    constexpr int MESS_MISSING_PERCENT = 15; // % de postes auxquels il manque 1 chaise
    constexpr int MESS_MISSING_MAX_ROW = 2;  // Max de chaises manquantes par rangée
    constexpr int BOARD_PERCENT        = 70; // Chance d'avoir un panneau contre un mur

    // Stockage
    constexpr int STORAGE_WALL_PERCENT  = 95; // % d'objets contre les murs
    constexpr int STORAGE_SEED_PERCENT  = 50; // % d'objets isolés au centre
    constexpr int STORAGE_STACK_PERCENT = 80; // % d'objets collés à un autre
    constexpr int STORAGE_SWEEPS        = 5;  // Nb de passes de regroupement
    constexpr int STORAGE_TOP_PERCENT   = 25; // % de gros objets surmontés d'un petit objet (Layer3)
    constexpr int HALF_PERCENT          = 30; // Chance de décaler un objet d'une demi-tuile (entre 2 tuiles)

    // Petites variations des objets de stockage (Modifier)
    constexpr int   STORAGE_ROT_JITTER = 12;    // +/- degrés ajoutés à la rotation (multiple de 90)
    constexpr float STORAGE_JITTER     = 10.f;  // +/- décalage aléatoire (jamais vers le mur)
    constexpr float STORAGE_SCALE_MIN  = 0.92f;
    constexpr float STORAGE_SCALE_MAX  = 1.08f;

    // Textures des portes (Layer2)
    constexpr GameMap::Texture DOOR_TEXTURES[] = {
        Tiles::WALL_DOORWAY_OPEN.texture,
        Tiles::WALL_DOOR_FRAME.texture,
        Tiles::WALL_DOORWAY_CLOSE.texture,
        Tiles::WALL_BROKEN_DOOR.texture,
    };

    // Objets de stockage (même ordre que les poids dans storagePick)
    constexpr GameMap::Tile STORAGE_TILES[] = {
        Tiles::OBJECT_LARGE_CRATE_WOODEN,
        Tiles::OBJECT_LARGE_BARREL_TOP,
        Tiles::OBJECT_SMALL_CRATE_WOODEN,
        Tiles::OBJECT_BARRELS_CLUSTER_1,
        Tiles::OBJECT_SMALL_BARREL_TOP,
        Tiles::OBJECT_BARRELS_CLUSTER_2,
    };

    bool sameTex(const GameMap::Texture& a, const GameMap::Texture& b) {
        return a.col == b.col && a.row == b.row;
    }
}

void MapGenerator::addHouseDecorations(Map& map, std::mt19937& rng, const Rect& playZone) {
    // ---------- 1. Lectures de la carte ----------
    auto inZone = [&](const int x, const int y) {
        return x >= playZone.x && y >= playZone.y &&
               x < playZone.x + playZone.w && y < playZone.y + playZone.h;
    };

    auto isDoor = [&](const int x, const int y) {
        if (!inZone(x, y)) return false;
        for (const GameMap::Tile& t : map.cells[y][x].layers) {
            if (t.zIndex != GameMap::ZIndex::Layer2) continue;
            for (const GameMap::Texture& d : DOOR_TEXTURES) {
                if (sameTex(t.texture, d)) return true;
            }
        }
        return false;
    };

    std::uniform_int_distribution<int> percent(0, 99);
    std::uniform_int_distribution<int> angle(0, 3);
    std::discrete_distribution<int>    storagePick({ 20.0, 15.0, 20.0, 12.0, 15.0, 12.0 });

    // ---------- 2. Décoration d'un bâtiment ----------
    auto decorate = [&](const int x, const int y, const int w, const int h, const GameMap::HouseRole role) {
        // Intérieur = empreinte sans les murs. Coordonnées locales (lx, ly) dans [0, iw) x [0, ih)
        const int iw = w - 2;
        const int ih = h - 2;
        const int ox = x + 1;
        const int oy = y + 1;

        // occ : 0 = libre, 1 = objet, 2 = réservé (passage devant une porte)
        std::vector<char> occ(iw * ih, 0);
        auto at = [&](const int lx, const int ly) -> char& { return occ[ly * iw + lx]; };
        auto inside = [&](const int lx, const int ly) { return lx >= 0 && ly >= 0 && lx < iw && ly < ih; };

        // Passage devant chaque porte : la tuile intérieure juste derrière reste toujours libre
        for (int lx = 0; lx < iw; ++lx) {
            if (isDoor(ox + lx, y))         at(lx, 0)      = 2;
            if (isDoor(ox + lx, y + h - 1)) at(lx, ih - 1) = 2;
        }
        for (int ly = 0; ly < ih; ++ly) {
            if (isDoor(x, oy + ly))         at(0, ly)      = 2;
            if (isDoor(x + w - 1, oy + ly)) at(iw - 1, ly) = 2;
        }

        // Toutes les tuiles non occupées par un objet doivent rester d'un seul tenant :
        // le joueur peut ainsi aller de la porte à n'importe quel point libre de la pièce.
        auto connected = [&]() {
            int total = 0, start = -1;
            for (int i = 0; i < iw * ih; ++i) {
                if (occ[i] == 1) continue;
                ++total;
                if (start < 0) start = i;
            }
            if (total == 0) return true;

            std::vector<char> seen(iw * ih, 0);
            std::queue<int> q;
            q.push(start);
            seen[start] = 1;
            int reached = 0;
            while (!q.empty()) {
                const int c = q.front();
                q.pop();
                ++reached;
                const int cx = c % iw;
                const int cy = c / iw;
                for (int d = 0; d < 4; ++d) {
                    const int nx = cx + DX4[d];
                    const int ny = cy + DY4[d];
                    if (!inside(nx, ny)) continue;
                    const int ni = ny * iw + nx;
                    if (seen[ni] || occ[ni] == 1) continue;
                    seen[ni] = 1;
                    q.push(ni);
                }
            }
            return reached == total;
        };

        // Réserve des tuiles pour un objet ; annule tout si l'une est prise ou si ça coupe la circulation
        auto claim = [&](const std::initializer_list<std::pair<int, int>> cells) {
            for (const auto& [cx, cy] : cells) {
                if (!inside(cx, cy) || at(cx, cy) != 0) return false;
            }
            for (const auto& [cx, cy] : cells) at(cx, cy) = 1;
            if (connected()) return true;
            for (const auto& [cx, cy] : cells) at(cx, cy) = 0;
            return false;
        };

        auto put = [&](GameMap::Tile tile, const int lx, const int ly, const int zIndex,
                       const int rotation, const float offX = 0.f, const float offY = 0.f) {
            tile.modifier.rotation = rotation;
            tile.modifier.offsetX  = offX;
            tile.modifier.offsetY  = offY;
            addTile(map, ox + lx, oy + ly, tile, zIndex);
        };

        // Index (0..3) du mur le plus proche
        auto nearestWall = [&](const int lx, const int ly) {
            const int dist[4] = { ly, iw - 1 - lx, ih - 1 - ly, lx };
            int best = 0;
            for (int k = 1; k < 4; ++k) if (dist[k] < dist[best]) best = k;
            return best;
        };
        auto onRing = [&](const int lx, const int ly) {
            return lx == 0 || ly == 0 || lx == iw - 1 || ly == ih - 1;
        };

        // Sens (-1/0/1) dans lequel une tuile contre un mur doit être poussée pour toucher ce mur
        // (les coins poussent dans les deux axes ; deux murs opposés s'annulent)
        auto wallPush = [&](const int lx, const int ly) {
            const int px = (lx == 0 ? -1 : 0) + (lx == iw - 1 ? 1 : 0);
            const int py = (ly == 0 ? -1 : 0) + (ly == ih - 1 ? 1 : 0);
            return std::pair<int, int>{ px, py };
        };

        // Tuiles de l'anneau (contre les murs, dans l'ordre de balayage) et du centre (mélangées)
        std::vector<std::pair<int, int>> ring, center;
        for (int ly = 0; ly < ih; ++ly) {
            for (int lx = 0; lx < iw; ++lx) {
                (onRing(lx, ly) ? ring : center).emplace_back(lx, ly);
            }
        }
        std::shuffle(center.begin(), center.end(), rng);

        // ---------- DORTOIR : lits ----------
        if (role == GameMap::HouseRole::Dorm) {
            // Règles : (1) un lit n'a des voisins que sur UN axe (gauche/droite OU haut/bas) -> que des lignes ;
            //         (2) deux lits collés ont toujours la même rotation.
            std::vector<int> bed(iw * ih, -1); // -1 = pas de lit, sinon k (mur vers lequel la tête est tournée)
            auto bedAt = [&](const int lx, const int ly) { return inside(lx, ly) ? bed[ly * iw + lx] : -1; };
            auto axes = [&](const int lx, const int ly) {
                int m = 0;
                if (bedAt(lx - 1, ly) >= 0 || bedAt(lx + 1, ly) >= 0) m |= 1;
                if (bedAt(lx, ly - 1) >= 0 || bedAt(lx, ly + 1) >= 0) m |= 2;
                return m;
            };

            // k = mur vers lequel la tête de lit est tournée
            auto placeBed = [&](const int lx, const int ly, const int k) {
                if (!inside(lx, ly) || at(lx, ly) != 0) return;

                // Même rotation que les lits voisins
                for (int d = 0; d < 4; ++d) {
                    const int nk = bedAt(lx + DX4[d], ly + DY4[d]);
                    if (nk >= 0 && nk != k) return;
                }

                // Pas de lit voisin sur les deux axes (ni pour ce lit, ni pour ses voisins)
                bed[ly * iw + lx] = k;
                bool ok = axes(lx, ly) != 3;
                for (int d = 0; d < 4 && ok; ++d) {
                    if (bedAt(lx + DX4[d], ly + DY4[d]) >= 0) ok = axes(lx + DX4[d], ly + DY4[d]) != 3;
                }
                bed[ly * iw + lx] = -1;
                if (!ok || !claim({ { lx, ly } })) return;

                bed[ly * iw + lx] = k;
                const GameMap::Tile tile = (percent(rng) < OLD_BED_PERCENT) ? Tiles::OBJECT_BED_2
                                                                            : Tiles::OBJECT_BED;
                put(tile, lx, ly, GameMap::ZIndex::Layer2, 90 * k);
            };

            // a) Lits contre les murs, tête de lit contre le mur
            for (const auto& [lx, ly] : ring) {
                if (percent(rng) < DORM_WALL_PERCENT) placeBed(lx, ly, nearestWall(lx, ly));
            }

            // b) Rangées au centre : lignes de lits séparées par une tuile de couloir
            //    (1 tuile libre le long des lits des murs, et 1 tuile entre deux rangées)
            const bool rowsHorizontal = percent(rng) < 50;
            const int outerN = rowsHorizontal ? ih : iw;
            const int innerN = rowsHorizontal ? iw : ih;
            for (int o = 2; o <= outerN - 3; o += 2) {
                const bool first = percent(rng) < 50;
                const int k = rowsHorizontal ? (first ? 0 : 2) : (first ? 3 : 1);
                for (int i = 2; i <= innerN - 3; ++i) {
                    if (percent(rng) >= DORM_ROW_PERCENT) continue;
                    placeBed(rowsHorizontal ? i : o, rowsHorizontal ? o : i, k);
                }
            }
        }

        // ---------- STOCKAGE : caisses et tonneaux ----------
        if (role == GameMap::HouseRole::Storage) {
            std::uniform_real_distribution<float> jit(-1.f, 1.f);
            std::uniform_real_distribution<float> scaleDist(STORAGE_SCALE_MIN, STORAGE_SCALE_MAX);
            std::uniform_int_distribution<int>    rotJit(-STORAGE_ROT_JITTER, STORAGE_ROT_JITTER);

            struct Placed { int lx, ly; float offX, offY; bool large; };
            std::vector<Placed> placed;

            // Pose un objet avec de petites variations : rotation, échelle et décalage.
            // (px, py) = sens du mur contre lequel il est posé : le décalage aléatoire s'en éloigne toujours.
            auto putItem = [&](GameMap::Tile tile, const int lx, const int ly, const float offX, const float offY,
                               const int px, const int py, const int zIndex) {
                tile.modifier.scale = scaleDist(rng);
                const int rotation = ((angle(rng) * 90 + rotJit(rng)) % 360 + 360) % 360;
                float jx = jit(rng) * STORAGE_JITTER;
                float jy = jit(rng) * STORAGE_JITTER;
                if (px != 0) jx = -std::abs(jx) * static_cast<float>(px);
                if (py != 0) jy = -std::abs(jy) * static_cast<float>(py);
                put(tile, lx, ly, zIndex, rotation, offX + jx, offY + jy);
            };

            auto placeItem = [&](const int lx, const int ly) {
                const int kind = storagePick(rng);
                const GameMap::Tile tile = STORAGE_TILES[kind];
                const bool large = (kind == 0 || kind == 1);

                if (onRing(lx, ly)) {
                    // Contre un mur : poussé d'une demi-tuile vers le mur
                    const auto [px, py] = wallPush(lx, ly);
                    const float hugX = px * WALL_HUG;
                    const float hugY = py * WALL_HUG;

                    // Parfois glissé d'une demi-tuile le long du mur (entre 2 tuiles voisines)
                    if (percent(rng) < HALF_PERCENT && (px != 0) != (py != 0)) {
                        const int k = nearestWall(lx, ly);
                        int dirs[2] = { (k + 1) & 3, (k + 3) & 3 };
                        std::shuffle(std::begin(dirs), std::end(dirs), rng);
                        for (const int d : dirs) {
                            if (!claim({ { lx, ly }, { lx + DX4[d], ly + DY4[d] } })) continue;
                            const float ox2 = DX4[d] * HALF_TILE + hugX;
                            const float oy2 = DY4[d] * HALF_TILE + hugY;
                            putItem(tile, lx, ly, ox2, oy2, px, py, GameMap::ZIndex::Layer2);
                            placed.push_back({ lx, ly, ox2, oy2, large });
                            return;
                        }
                    }
                    if (claim({ { lx, ly } })) {
                        putItem(tile, lx, ly, hugX, hugY, px, py, GameMap::ZIndex::Layer2);
                        placed.push_back({ lx, ly, hugX, hugY, large });
                    }
                    return;
                }

                // Au centre : parfois à cheval sur 2 tuiles
                if (percent(rng) < HALF_PERCENT) {
                    int dirs[4] = { 0, 1, 2, 3 };
                    std::shuffle(std::begin(dirs), std::end(dirs), rng);
                    for (const int d : dirs) {
                        if (!claim({ { lx, ly }, { lx + DX4[d], ly + DY4[d] } })) continue;
                        const float ox2 = DX4[d] * HALF_TILE;
                        const float oy2 = DY4[d] * HALF_TILE;
                        putItem(tile, lx, ly, ox2, oy2, 0, 0, GameMap::ZIndex::Layer2);
                        placed.push_back({ lx, ly, ox2, oy2, large });
                        return;
                    }
                }
                if (claim({ { lx, ly } })) {
                    putItem(tile, lx, ly, 0.f, 0.f, 0, 0, GameMap::ZIndex::Layer2);
                    placed.push_back({ lx, ly, 0.f, 0.f, large });
                }
            };

            // a) Objets contre les murs + objets isolés au centre
            for (const auto& [lx, ly] : ring)   if (percent(rng) < STORAGE_WALL_PERCENT) placeItem(lx, ly);
            for (const auto& [lx, ly] : center) if (percent(rng) < STORAGE_SEED_PERCENT) placeItem(lx, ly);

            // b) Regroupement : on empile à côté d'objets déjà posés (piles / rangées)
            std::vector<std::pair<int, int>> all = ring;
            all.insert(all.end(), center.begin(), center.end());
            for (int sweep = 0; sweep < STORAGE_SWEEPS; ++sweep) {
                std::shuffle(all.begin(), all.end(), rng);
                for (const auto& [lx, ly] : all) {
                    if (at(lx, ly) != 0) continue;
                    bool touches = false;
                    for (int d = 0; d < 4 && !touches; ++d) {
                        const int nx = lx + DX4[d];
                        const int ny = ly + DY4[d];
                        touches = inside(nx, ny) && at(nx, ny) == 1;
                    }
                    if (touches && percent(rng) < STORAGE_STACK_PERCENT) placeItem(lx, ly);
                }
            }

            // c) Un petit objet posé par-dessus certains gros objets (Layer3)
            for (const Placed& p : placed) {
                if (!p.large || percent(rng) >= STORAGE_TOP_PERCENT) continue;
                const GameMap::Tile small = (percent(rng) < 50) ? Tiles::OBJECT_SMALL_CRATE_WOODEN
                                                                : Tiles::OBJECT_SMALL_BARREL_TOP;
                GameMap::Tile tile = small;
                tile.modifier.scale = scaleDist(rng) * 0.9f;
                put(tile, p.lx, p.ly, GameMap::ZIndex::Layer3,
                    ((angle(rng) * 90 + rotJit(rng)) % 360 + 360) % 360,
                    p.offX + jit(rng) * STORAGE_JITTER, p.offY + jit(rng) * STORAGE_JITTER);
            }
        }

        // ---------- RÉFECTOIRE : tables et chaises ----------
        if (role == GameMap::HouseRole::Mess) {
            // Un "poste" = 2 tuiles voisines A et B, avec une table (Layer3, au-dessus des chaises)
            // décalée d'une demi-tuile vers B, et une chaise (Layer2) sur A et/ou B regardant la table.
            //   vertical   : table pivotée de 0°,  chaises en haut / en bas
            //   horizontal : table pivotée de 90°, chaises à gauche / à droite
            // Jamais de table sans chaise, jamais de chaise sans table : si une chaise manque,
            // l'autre est toujours là.
            auto placeSet = [&](const int ax, const int ay, const bool horizontal, int* missingInRow) {
                const int bx = ax + (horizontal ? 1 : 0);
                const int by = ay + (horizontal ? 0 : 1);
                if (!claim({ { ax, ay }, { bx, by } })) return false;

                // Parfois une chaise manque (au plus MESS_MISSING_MAX_ROW par rangée)
                bool chairA = true, chairB = true;
                if (missingInRow && *missingInRow < MESS_MISSING_MAX_ROW && percent(rng) < MESS_MISSING_PERCENT) {
                    (percent(rng) < 50 ? chairA : chairB) = false;
                    ++*missingInRow;
                }

                const int dirAB = horizontal ? 1 : 2; // A regarde vers B (droite ou bas), B regarde vers A
                if (chairA) put(Tiles::OBJECT_CHAIR, ax, ay, GameMap::ZIndex::Layer2, chairRotation(dirAB));
                if (chairB) put(Tiles::OBJECT_CHAIR, bx, by, GameMap::ZIndex::Layer2, chairRotation((dirAB + 2) & 3));
                put(Tiles::OBJECT_TABLE, ax, ay, GameMap::ZIndex::Layer3,
                    horizontal ? 90 : 0,
                    horizontal ? HALF_TILE : 0.f,
                    horizontal ? 0.f : HALF_TILE);
                return true;
            };

            // Rangées de postes séparées par 1 tuile de couloir, avec des trous tous les 4-8 postes ;
            // ensuite, une bonne partie des trous est comblée par un poste supplémentaire.
            // La vérification de circulation garantit qu'il reste toujours un passage.
            const int m = (percent(rng) < MESS_EDGE_PERCENT) ? 0 : 1; // 0 = contre les murs, 1 = couloir autour
            std::uniform_int_distribution<int> groupDist(MESS_GROUP_MIN, MESS_GROUP_MAX);
            auto fillRows = [&](const bool horizontal) {
                const int outerN = horizontal ? iw : ih; // sens de la longueur d'un poste
                const int innerN = horizontal ? ih : iw; // sens dans lequel les postes s'alignent
                int count = 0;
                std::vector<std::pair<int, int>> gaps;   // (o, i) des trous laissés entre groupes

                for (int o = m; o + 1 <= outerN - 1 - m; o += 3) {
                    int missing = 0;
                    int i = m;
                    while (i <= innerN - 1 - m) {
                        const int group = groupDist(rng);
                        for (int g = 0; g < group && i <= innerN - 1 - m; ++g, ++i) {
                            if (placeSet(horizontal ? o : i, horizontal ? i : o, horizontal, &missing)) ++count;
                        }
                        if (i <= innerN - 1 - m) gaps.emplace_back(o, i);
                        ++i; // trou entre deux groupes
                    }
                }

                for (const auto& [o, i] : gaps) {
                    if (percent(rng) >= MESS_FILL_PERCENT) continue;
                    if (placeSet(horizontal ? o : i, horizontal ? i : o, horizontal, nullptr)) ++count;
                }
                return count;
            };

            const bool horizontalFirst = percent(rng) < 50;
            if (fillRows(horizontalFirst) == 0) fillRows(!horizontalFirst); // petite pièce : autre sens

            // Panneau contre un mur
            if (percent(rng) < BOARD_PERCENT) {
                std::vector<std::pair<int, int>> spots = ring;
                std::shuffle(spots.begin(), spots.end(), rng);
                for (const auto& [lx, ly] : spots) {
                    if (!claim({ { lx, ly } })) continue;
                    const auto [px, py] = wallPush(lx, ly);
                    put(Tiles::OBJECT_BOARD, lx, ly, GameMap::ZIndex::Layer2,
                        90 * nearestWall(lx, ly), px * WALL_HUG, py * WALL_HUG);
                    break;
                }
            }
        }
    };

    // ---------- 3. Décoration de chaque bâtiment (liste remplie par buildHouseFloors) ----------
    for (const GameMap::HouseInfo& house : map.houses) {
        if (house.w < 3 || house.h < 3) continue;
        decorate(house.x, house.y, house.w, house.h, house.role);
    }
}