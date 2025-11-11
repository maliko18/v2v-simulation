# 🚗 V2V Simulator - Simulateur de Véhicules Communicants

Simulateur haute performance pour visualiser et analyser les connexions V2V (vehicle-to-vehicle) dans un environnement urbain réel (Mulhouse, France).

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6.4%2B-green.svg)

## ✨ Fonctionnalités

- ✅ **Visualisation interactive** - Carte OSM réelle avec zoom/pan fluide
- ✅ **2000+ véhicules** - Simulation haute performance à 60 FPS
- ✅ **Graphe d'interférences dynamique** - Connexions V2V en temps réel (100-500m)
- ✅ **Contrôle temporel** - Pause, accélération (0.1x à 10x)
- ✅ **Interface moderne** - Panneau de contrôle intuitif avec Qt6
- 🚧 **Pathfinding A*** - Navigation sur graphe routier (en développement)

## 📸 Captures d'écran

![Interface principale](docs/screenshots/main.png)
*Interface avec carte OSM de Mulhouse et panneau de contrôle*

## 🚀 Installation

### Prérequis

- **C++20** compiler (GCC 11+, Clang 14+, MSVC 2022+)
- **CMake** 3.25+
- **Qt6** 6.4+ (Core, Widgets, Network, Positioning)
- **Boost** 1.80+ (Graph, Geometry)
- **Intel TBB** 2021+
- **Eigen3** 3.4+

### Linux (Ubuntu/Debian)

```bash
# Installer les dépendances
sudo apt update
sudo apt install -y build-essential cmake ninja-build
sudo apt install -y qt6-base-dev qt6-positioning-dev
sudo apt install -y libboost-all-dev libtbb-dev libeigen3-dev

# Cloner le projet
git clone https://github.com/VOTRE_USERNAME/v2v-simulator.git
cd v2v-simulator

# Compiler
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja

# Lancer
./v2v_simulator
```

### Windows (avec vcpkg)

```powershell
# Installer vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat

# Installer les dépendances
vcpkg install qt6-base qt6-positioning boost-graph boost-geometry tbb eigen3

# Compiler le projet
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release

# Lancer
Release\v2v_simulator.exe
```

### macOS

```bash
# Avec Homebrew
brew install cmake qt@6 boost tbb eigen

# Compiler
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)
make -j$(sysctl -n hw.ncpu)

# Lancer
./v2v_simulator
```

## 🎮 Utilisation

1. **Lancer l'application**
   ```bash
   ./v2v_simulator
   ```

2. **Contrôles de la carte**
   - 🖱️ **Drag** - Déplacer la carte
   - 🖱️ **Molette** - Zoomer/Dézoomer
   
3. **Simulation**
   - ▶️ **Start** - Démarrer la simulation
   - ⏸️ **Pause** - Mettre en pause
   - ⏹️ **Stop** - Arrêter
   - ↻ **Reset** - Réinitialiser

4. **Paramètres**
   - **Speed** - Vitesse de simulation (0.1x à 10x)
   - **Vehicles** - Nombre de véhicules (10 à 5000)
   - **Radius** - Rayon de transmission V2V (100 à 500m)

## 🏗️ Architecture

```
v2v-simulator/
├── include/              # Headers (.hpp)
│   ├── core/            # Moteur de simulation
│   ├── network/         # Graphes (routier + interférences)
│   ├── visualization/   # Interface Qt + rendu
│   ├── data/            # Parser OSM, tuiles
│   └── utils/           # Logger, Config, Profiler
├── src/                 # Implémentations (.cpp)
├── config/              # Configuration JSON
├── data/                # Données OSM (Mulhouse)
└── CMakeLists.txt       # Configuration CMake
```

## 📚 Technologies

| Composant | Technologie | Usage |
|-----------|-------------|-------|
| **Interface** | Qt 6.4+ | UI, rendu 2D, events |
| **Graphes** | Boost.Graph | Graphe routier, algorithmes |
| **Spatial** | Boost.Geometry R-tree | Indexation O(log n) |
| **Carte** | OpenStreetMap | Tuiles et données routières |
| **Threading** | Intel TBB | Parallélisme |
| **Math** | Eigen3 | Algèbre linéaire |
| **Build** | CMake + Ninja | Compilation rapide |

## 🔧 Configuration

Le fichier `config/mulhouse.json` contient :

```json
{
  "map": {
    "center": {"lat": 47.7508, "lon": 7.3359},
    "zoom": 13
  },
  "simulation": {
    "vehicles": 2000,
    "transmissionRadius": 300,
    "timeScale": 1.0
  }
}
```

## 🤝 Contribution

Les contributions sont les bienvenues !

1. Fork le projet
2. Créer une branche (`git checkout -b feature/amazing-feature`)
3. Commit les changements (`git commit -m 'Add amazing feature'`)
4. Push vers la branche (`git push origin feature/amazing-feature`)
5. Ouvrir une Pull Request

## 📋 TODO

### Haute Priorité
- [ ] Parser OSM complet (libosmium)
- [ ] Pathfinding A* sur graphe routier
- [ ] Véhicules suivent les routes
- [ ] Visualisation connexions V2V (lignes vertes)

### Moyenne Priorité
- [ ] Export données simulation (CSV)
- [ ] Statistiques temps réel (latence, throughput)
- [ ] Mode replay/recording
- [ ] Charger différentes villes

### Basse Priorité
- [ ] Modèle propagation signal réaliste
- [ ] Shaders OpenGL optimisés
- [ ] Multi-threading véhicules
- [ ] Interface web (optionnel)

## 📄 Licence

Ce projet est sous licence MIT. Voir [LICENSE](LICENSE) pour plus de détails.

## 👥 Auteurs

- **Votre Nom** - *Développement initial* - [GitHub](https://github.com/VOTRE_USERNAME)

## 🙏 Remerciements

- OpenStreetMap pour les données cartographiques
- Qt Project pour le framework
- Boost C++ Libraries
- Communauté open-source

## 📞 Contact

Pour toute question ou suggestion :
- 📧 Email: votre.email@example.com
- 💬 Issues: [GitHub Issues](https://github.com/VOTRE_USERNAME/v2v-simulator/issues)

---

**Note** : Projet développé dans le cadre de l'UE Réseaux - UHA - Automne 2025
