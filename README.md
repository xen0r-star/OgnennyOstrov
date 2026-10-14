# Ognenny Ostrov (nom de code : *Goulag*)

Jeu 2D en **C++17 / SFML 2.5.1**, vue du dessus, sur une carte de camp générée procéduralement (baraquements, murs, portes, chemins, barbelés, miradors).
Le projet est découpé en **client** (affichage + jeu) et **serveur** (multijoueur, encore vide).

> État actuel : le client ouvre une fenêtre, génère une carte 50×50 aléatoire et l'affiche. On peut déplacer la caméra en glissant avec le clic gauche. Pas encore de joueur, ni de réseau.

---

## 1. Lancer le projet

- **IDE** : CLion (CMake). SFML est fourni dans `deps/SFML` → rien à installer.
- **Compilateur** : MinGW (GCC) sous Windows (le code utilise `<Windows.h>`).
- **Cibles CMake** :
  - `GoulagClient` → le jeu (`src/main.cpp`)
  - `GoulagServer` → le serveur (`src/main_server.cpp`, affiche juste "Hello World!" pour l'instant)
- Après compilation, CMake copie automatiquement les **DLL SFML** et le dossier **`assets/`** à côté de l'exécutable (sinon la texture `map.png` ne se charge pas).

**Code::Blocks** (optionnel) : `python tools/gen_cbp.py` génère `Goulag.cbp` à partir de `tools/Goulag.cbp.in` en listant tous les fichiers de `src/`.

---

## 2. Arborescence

```
OgnennyOstrov/
├── CMakeLists.txt          # Build : lib partagée + client + serveur
├── assets/
│   └── textures/map.png    # Tileset (tuiles 128×128 px)
├── deps/SFML/              # SFML 2.5.1 précompilée (headers, .a, .dll)
├── tools/                  # Générateur de projet Code::Blocks
└── src/
    ├── main.cpp            # Point d'entrée CLIENT
    ├── main_server.cpp     # Point d'entrée SERVEUR
    ├── core/               # Boucle de jeu, config
    │   ├── Config.h        # Constantes globales (titre fenêtre…)
    │   ├── Game.h/.cpp     # Classe principale du client
    │   └── GameServer.h/.cpp  # (vide)
    ├── models/             # DONNÉES + LOGIQUE (pas d'affichage, partagé client/serveur)
    │   └── map/MapGenerator.h/.cpp
    ├── views/              # AFFICHAGE (SFML graphics)
    │   └── MapView.h/.cpp
    ├── controllers/
    │   ├── client/         # (vide) gestion des inputs joueur
    │   └── server/         # (vide) logique côté serveur
    ├── network/            # (vide) communication client ↔ serveur
    └── utils/              # (vide) fonctions utilitaires
```

### Architecture : MVC + client/serveur

| Couche | Rôle | Compilé dans |
|---|---|---|
| `models/` | Les données du jeu et les règles (carte, plus tard joueurs…) | **les deux** (lib `GoulagShared`) |
| `network/`, `utils/` | Code commun | **les deux** |
| `views/` | Dessiner les modèles à l'écran | client seulement |
| `controllers/client/` | Clavier/souris → actions | client seulement |
| `controllers/server/` | Valider/appliquer les actions des joueurs | serveur seulement |

**Règle d'or** : un fichier dans `models/` ne doit **jamais** inclure `SFML/Graphics.hpp`, car le serveur ne linke pas la partie graphique (seulement `sfml-system` et `sfml-network`).

Les fichiers `.cpp` sont trouvés automatiquement par CMake (`GLOB_RECURSE`) : pour ajouter un fichier, il suffit de le créer dans le bon dossier puis de **recharger CMake**.

---

## 3. Comment ça tourne (client)

```
main()
 └── Game game;            // constructeur : crée la fenêtre + génère la carte
 └── game.run();           // boucle infinie tant que la fenêtre est ouverte
        ├── processEvents()  // lit les events SFML (fermeture, souris…)
        ├── update(dt)       // logique du jeu (vide pour l'instant)
        └── render()         // clear → mapView.drawMap() → display
```

- `Game` possède : la fenêtre (`sf::RenderWindow`), la carte (`TileMap map`) et sa vue (`MapView mapView`).
- `dt` (deltaTime) = temps écoulé depuis la dernière frame → à utiliser pour que les déplacements ne dépendent pas des FPS.
- Le jeu est limité à 60 FPS.

---

## 4. La carte : structures de données clés

Tout est défini dans `models/map/MapGenerator.h`.

- **`Tile`** = une case du tileset : `col`, `row` dans `map.png` + une `rotation` (`ANGLE_0/90/180/270`).
- **`TileType::XXX`** = catalogue nommé de toutes les tuiles (`GROUND_GRASS`, `WALL_STRAIGHT`, `PATH_CURVE_1`, `WATCHTOWER`…). Utilise ces noms plutôt que des coordonnées en dur.
- **`LayerTile`** = une `Tile` + un **z-index** (`ZIndex::Ground = 0`, `Structure = 10`, `Decoration = 20`).
- **`MapCell`** = une case de la carte = plusieurs couches empilées (ex : herbe + mur + tonneau).
- **`TileMap`** = `vector<vector<MapCell>>` → accès **`map[x][y]`** (colonne d'abord, puis ligne).

### Génération (`MapGenerator::generateMap(height, width, seed)`)

Même `seed` ⇒ même carte (utile pour le multijoueur : le serveur n'aura qu'à envoyer la seed).

Le travail se fait en 2 temps :
1. une **grille logique** (`Grid` de `Cell` : `Grass`, `Dirt`, `HouseFloor`, `Wall`, `Door`, `Path`, `Rail`…) qui décrit *ce qu'il y a* ;
2. une **grille de tuiles** qui décrit *comment le dessiner* (choix de la tuile + rotation selon les voisins = **autotiling**).

Étapes dans `build()` :

1. `buildMapBorder()` – mur d'enceinte + marge de végétation autour
2. `generateBaseTerrain()` – herbe / terre
3. `generateHouses()` – place des bâtiments (`BARRACKS`, `MESS`, `STORAGE`)
4. `carveHouseFloors()` + `connectAdjacentHouses()` – sols, fusion des maisons collées
5. `buildWalls()` + `placeDoors()` – murs (coins, courbes…) et portes
6. `generateHousePaths()` – chemins entre les portes (pathfinding `findRoute`)
7. `removePathBlocks()` + `autotilePathsUnified()` – nettoyage et choix des tuiles de chemin
8. `assembleResult()` – conversion vers le `TileMap` final

⚠️ **Désactivés pour l'instant** (commentés dans `build()`) : `generatePromenadePaths()`, `generateRails()`, `placeDecorations()`.

---

## 5. L'affichage : `MapView`

- Charge `assets/textures/map.png` (1152×770 px, tuiles de **128 px**).
- `drawTile()` découpe la bonne tuile (`IntRect` à partir de `col`/`row`), la tourne autour de son centre et la place à `i * TILE_STEP`.
  - `TILE_STEP = 120` (128 − 8) → les tuiles **se chevauchent de 8 px** pour éviter les joints visibles.
- `drawMap()` dessine en **3 passes** (Ground → Structure → Decoration) pour que les objets soient toujours au-dessus du sol.
- Au premier dessin, la caméra (`sf::View`) est dézoomée pour que toute la carte tienne à l'écran.
- `handleEvent()` : glisser-déposer clic gauche = déplacer la caméra (marqué *For testing*).

---

## 6. Conventions

- Headers en `.h` avec garde `#ifndef GOULAG_XXX_H`.
- Includes relatifs à `src/` : `#include "core/Game.h"`, `#include "models/map/MapGenerator.h"`.
- Membres privés du générateur suffixés par `_` (`rng_`, `grid_`…).
- Commits : `type: message` (`refactor:`, `dep:`…).
- Compatibilité **GCC 8.1 / C++17** (d'où le constructeur `constexpr` de `Tile`).

---

## 7. Ce qui reste à faire (pistes)

À réfléchir avant de coder — dans quel dossier va chaque élément ?

- [ ] **Joueur** : un modèle (position, vitesse…), une vue (sprite), un contrôleur client (clavier → déplacement), et `Game::update(dt)` pour le faire bouger.
- [ ] **Collisions** : savoir si une case est franchissable (indice : la grille logique `Cell` du générateur sait déjà où sont les murs).
- [ ] **Caméra** qui suit le joueur au lieu du glisser-souris de test.
- [ ] **Réactiver** promenades, rails et décorations dans `build()`.
- [ ] **Serveur** : `GameServer` + `network/` avec `sf::TcpListener` / `sf::UdpSocket` / `sf::Packet`.
- [ ] Remplir `assets/audio`, `assets/fonts`, `docs/`.

### Points d'attention

- `Game.cpp` inclut `<Windows.h>` (pour maximiser la fenêtre) → le client ne compile que sous Windows.
- La texture est chargée en chemin **relatif** : si elle ne s'affiche pas, vérifie le *working directory* de la configuration d'exécution dans CLion.
