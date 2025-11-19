# V2V Simulator - Simulateur de Véhicules Communicants

🚗 Outil de simulation de connexions V2V (véhicule à véhicule) pour la 5G

## 📋 Table des Matières

- [Description](#description)
- [Architecture](#architecture)
- [Prérequis](#prérequis)
- [Installation](#installation)
- [Compilation avec CLion](#compilation-avec-clion)
- [Utilisation](#utilisation)
- [Fonctionnalités](#fonctionnalités)
- [Structure du Projet](#structure-du-projet)
- [Performance](#performance)
- [Roadmap](#roadmap)

---

## 🎯 Description

Ce projet implémente un simulateur haute performance pour visualiser et analyser les connexions V2V dans un environnement urbain. Il utilise:

- **Qt 6** pour l'interface professionnelle et le rendu OpenGL
- **Boost.Graph** pour les algorithmes de graphes
- **Boost.Geometry R-tree** pour l'indexation spatiale efficace
- **libosmium** pour le parsing rapide des données OSM
- **TBB** pour le multi-threading

### Caractéristiques Principales

✅ **2000+ véhicules simulés** à 30 FPS  
✅ **Visualisation interactive** avec zoom/pan  
✅ **Graphe d'interférences dynamique** (V2V)  
✅ **Données réelles OSM** (Mulhouse, Alsace)  
✅ **Architecture optimisée** pour haute performance  
✅ **Tuiles OSM en temps réel** (OpenStreetMap)

---

## 🏗️ Architecture

```
v2v-simulator/
├── include/          # Headers (.hpp)
│   ├── core/         # Vehicle, SimulationEngine
│   ├── network/      # RoadGraph, InterferenceGraph, PathPlanner
│   ├── visualization/# MainWindow, MapView
│   ├── data/         # OSMParser, TileManager, GeometryUtils
│   └── utils/        # Logger
├── src/              # Implémentations (.cpp)
├── data/             # Données OSM (Mulhouse, Alsace)
└── CMakeLists.txt    # Configuration build
```

### Modules Principaux

| Module            | Description                                     | Technologies            |
| ----------------- | ----------------------------------------------- | ----------------------- |
| **Core**          | `Vehicle`, `SimulationEngine`                   | Qt, C++20               |
| **Network**       | `RoadGraph`, `InterferenceGraph`, `PathPlanner` | Boost.Graph, R-tree     |
| **Visualization** | `MainWindow`, `MapView`                         | Qt6, QPainter           |
| **Data**          | `OSMParser`, `TileManager`, `GeometryUtils`     | libosmium, CURL, SQLite |
| **Utils**         | `Logger`                                        | Qt                      |

---

## 🔧 Prérequis

### Système d'Exploitation

- **Linux** (Ubuntu 24.04 LTS recommandé)
- **CLion 2024+** (ou tout IDE supportant CMake)

### Dépendances Systèmes

Toutes les dépendances ont déjà été installées ! Liste complète :

```bash
# Déjà installé lors de la configuration initiale
qt6-base-dev qt6-positioning-dev libqt6opengl6-dev libqt6svg6-dev
cmake ninja-build build-essential
libboost-all-dev libosmium2-dev osmium-tool
libtbb-dev libcurl4-openssl-dev libsqlite3-dev libeigen3-dev
```

### Configuration Matérielle Recommandée

- **CPU:** 4+ cores (Intel i5/Ryzen 5 minimum)
- **RAM:** 8 GB minimum, 16 GB recommandé
- **GPU:** OpenGL 4.3+ (GPU intégré suffisant)
- **Disque:** SSD recommandé pour cache tuiles

---

## 📦 Installation

### 1. Vérifier les Dépendances

```bash
# Vérifier Qt6
qmake6 --version

# Vérifier CMake
cmake --version  # >= 3.25

# Vérifier Boost
dpkg -l | grep libboost

# Vérifier osmium
osmium-tool --version
```

Toutes les dépendances sont déjà installées ✅

### 2. Télécharger Données OSM (Optionnel)

Les données OSM pour Mulhouse et l'Alsace sont déjà incluses dans le dossier `data/`:

- `data/mulhouse.osm` - Zone urbaine de Mulhouse
- `data/alsace_main_roads.osm` - Routes principales d'Alsace

---

## 🚀 Compilation avec CLion

### Méthode 1 : Ouvrir le Projet dans CLion

1. **Ouvrir CLion**
2. **File → Open**
3. Sélectionner le dossier `v2v-simulator/`
4. CLion détecte automatiquement `CMakeLists.txt`
5. Attendre que CMake configure le projet
6. **Build → Build Project** (Ctrl+F9)

### Méthode 2 : Ligne de Commande

```bash
cd v2v-simulator
mkdir build && cd build

# Configuration (Release pour performance)
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..

# Compilation
ninja

# Exécution
./v2v_simulator
```

### Configuration CLion Recommandée

#### Build Type

- **Debug:** Pour développement avec débogueur
  - Flags: `-O0 -g -Wall -Wextra`
- **Release:** Pour performance maximale ⚡
  - Flags: `-O3 -march=native -mtune=native -flto -DNDEBUG`

#### CMake Options

```cmake
-DCMAKE_BUILD_TYPE=Release
-DUSE_CCACHE=ON          # Accélère recompilation
-DBUILD_TESTS=OFF        # Désactiver tests pour l'instant
```

#### Run Configuration

Dans CLion:

1. **Run → Edit Configurations**
2. Ajouter **CMake Application**
3. **Target:** `v2v_simulator`
4. **Working directory:** `$PROJECT_DIR$`

---

## 🎮 Utilisation

### Interface Principale

```
┌────────────────────────────────────────────┐
│ [▶ Start] [⏸ Pause] [↻ Reset]            │ Contrôles
│ Speed: [████──────] 1.0x                   │
│ Vehicles: [2000▼] Radius: [300m▼]         │
├────────────────────────────────────────────┤
│                                            │
│          Carte Interactive (MapView)       │
│          - Zoom: Molette souris           │
│          - Pan: Drag souris               │
│          - Véhicules: Points rouges       │
│          - Connexions V2V: Lignes vertes  │
│          - Tuiles OSM: Fond de carte      │
│                                            │
├────────────────────────────────────────────┤
│ FPS: 30 | Vehicles: 2000 | Connections: ~15k │ Status
└────────────────────────────────────────────┘
```

### Contrôles

| Action                  | Contrôle            |
| ----------------------- | ------------------- |
| **Démarrer simulation** | Bouton Start        |
| **Pause**               | Bouton Pause        |
| **Réinitialiser**       | Bouton Reset        |
| **Zoom +**              | Molette haut        |
| **Zoom -**              | Molette bas         |
| **Pan**                 | Click + Drag souris |
| **Accélérer temps**     | Slider Speed        |
| **Afficher routes**     | Bouton Routes       |
| **Toggle véhicules**    | Touche V            |
| **Toggle connexions**   | Touche C            |
| **Toggle routes**       | Touche R            |
| **Retour Mulhouse**     | Touche H            |

### Configuration

Les paramètres de simulation sont configurés directement dans le code source:

**SimulationEngine** (`src/core/SimulationEngine.cpp`):

- Fréquence de mise à jour: 30 Hz (33ms par frame)
- Nombre de véhicules: configurable via l'UI (10-5000)
- Rayon de transmission V2V: 300m par défaut

**MapView** (`src/visualization/MapView.cpp`):

- Centre par défaut: Alsace (48.08°N, 7.36°E)
- Niveau de zoom initial: 10
- Cache tuiles OSM: `osm_cache/` (généré automatiquement)

---

## ✨ Fonctionnalités

### ✅ Étape 1 : Visualisation Interactive (Implémenté)

- [x] Fenêtre Qt professionnelle
- [x] MapView avec QPainter
- [x] Zoom/Pan interactif
- [x] Interface toolbar + status bar
- [x] Chargement tuiles OSM dynamiques

### ✅ Étape 2 : Graphe Routier (Implémenté)

- [x] Structure `RoadGraph` avec Boost.Graph
- [x] Parsing OSM avec libosmium
- [x] Affichage routes sur la carte
- [x] Nœuds et arêtes du graphe routier

### ✅ Étape 3 : Simulation Véhicules (Implémenté)

- [x] Classe `Vehicle` complète
- [x] `SimulationEngine` avec boucle update 30 Hz
- [x] Contrôle temps (pause, accélération, reset)
- [x] Génération 2000+ véhicules
- [x] Mouvement fluide sur la carte

### ✅ Étape 4 : Graphe d'Interférences (Implémenté)

- [x] `InterferenceGraph` avec R-tree spatial
- [x] Recherche voisins O(log n)
- [x] Update dynamique des connexions
- [x] Affichage connexions V2V (lignes vertes)
- [x] Cercles de transmission autour des véhicules

---

## 📊 Performance

### Métriques Actuelles

| Métrique          | Valeur   | Status      |
| ----------------- | -------- | ----------- |
| **FPS Logique**   | 30 Hz    | ✅          |
| **FPS Affichage** | 15-30    | ✅          |
| **Véhicules**     | 2000+    | ✅          |
| **Update Graphe** | < 5 ms   | ✅ (R-tree) |
| **RAM**           | < 500 MB | ✅          |
| **CPU (1 core)**  | ~30-40%  | ✅          |

### Optimisations Implémentées

✅ **R-tree spatial index** → O(log n) queries  
✅ **Fréquence logique fixe** → 30 Hz (économie CPU)  
✅ **Culling adaptatif** → Dessine selon zoom  
✅ **InterferenceGraph dynamique** → Intervalle adaptatif  
✅ **Compilation optimisée** (`-O3 -march=native -flto`)  
✅ **Frustum culling** → Seulement véhicules visibles
{
PROFILE_SCOPE("GraphUpdate");
// ... code à profiler
}

// Afficher rapport
v2v::utils::Profiler::instance().printReport();

````

---

## 🛠️ Développement

### Ajouter une Nouvelle Fonctionnalité

1. **Header** dans `include/module/`
2. **Implémentation** dans `src/module/`
3. **Ajouter au CMakeLists.txt** (déjà fait pour structure actuelle)
4. **Build** dans CLion

### Déboguer

```bash
# Mode Debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
ninja

# Lancer avec gdb
gdb ./v2v_simulator

# Valgrind (détection fuites mémoire)
valgrind --leak-check=full ./v2v_simulator
````

### Logs

Les logs sont sauvés dans `v2v_simulator.log` et affichés dans la console.

```cpp
LOG_DEBUG("Message de debug");
LOG_INFO("Message informatif");
LOG_WARNING("Avertissement");
LOG_ERROR("Erreur critique");
```

---

## 📝 Roadmap Future

### Améliorations Possibles

- [ ] Pathfinding véhicules sur graphe routier (A\*)
- [ ] Migration vers Qt Quick/QML pour GPU rendering
- [ ] Thread séparé pour simulation (UI toujours fluide)
- [ ] Export données simulation (CSV, JSON)
- [ ] Replay/Recording de simulations
- [ ] Modèle propagation signal plus réaliste

---

## 📚 Références

### Documentation

- [Qt 6 Documentation](https://doc.qt.io/qt-6/)
- [Boost.Graph](https://www.boost.org/doc/libs/release/libs/graph/)
- [Boost.Geometry](https://www.boost.org/doc/libs/release/libs/geometry/)
- [libosmium](https://osmcode.org/libosmium/)
- [OpenStreetMap Wiki](https://wiki.openstreetmap.org/)

### Données

- [Geofabrik Downloads](http://download.geofabrik.de/)
- [OSM Tile Servers](https://wiki.openstreetmap.org/wiki/Tile_servers)

---

## 👥 Auteur

**Projet UHA - UE Réseaux Automne 2025**  
Simulateur V2V pour véhicules communicants 5G

---

## 📄 Licence

Projet académique - UHA (Université de Haute-Alsace)

---

## 🚀 Démarrage Rapide

```bash
# 1. Ouvrir dans CLion
# 2. Attendre configuration CMake
# 3. Build → Build Project
# 4. Run → Run 'v2v_simulator'

# OU en ligne de commande:
cd v2v-simulator
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
ninja
./v2v_simulator
```

**Bon développement ! 🎉**
