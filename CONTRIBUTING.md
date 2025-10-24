# 👥 Guide de Contribution

Merci de contribuer au projet V2V Simulator ! 🚗

## 🚀 Quick Start pour les contributeurs

### 1. Fork et Clone

```bash
# Fork le projet sur GitHub (bouton "Fork" en haut à droite)

# Clone votre fork
git clone https://github.com/VOTRE_USERNAME/v2v-simulator.git
cd v2v-simulator

# Ajouter le remote upstream (pour sync avec l'original)
git remote add upstream https://github.com/nourine/v2v-simulator.git
```

### 2. Créer une branche

```bash
# Toujours créer une branche pour vos modifications
git checkout -b feature/nom-de-votre-feature

# Exemples de noms de branches :
# - feature/osm-parser
# - feature/astar-pathfinding  
# - fix/map-zoom-bug
# - docs/update-readme
```

### 3. Développer

```bash
# Faire vos modifications...

# Tester que ça compile
cd build
ninja
./v2v_simulator

# Vérifier le style de code
# (TODO: ajouter clang-format)
```

### 4. Commit et Push

```bash
# Ajouter vos fichiers
git add .

# Commit avec un message clair
git commit -m "✨ Add OSM parser with libosmium

- Implement loadFile() method
- Parse nodes and ways from .osm.pbf files
- Build RoadGraph structure
- Add unit tests

Closes #12"

# Pusher vers votre fork
git push origin feature/nom-de-votre-feature
```

### 5. Pull Request

1. Aller sur GitHub
2. Cliquer sur **"Compare & pull request"**
3. Remplir la description :
   - Qu'est-ce qui a été fait ?
   - Pourquoi ?
   - Comment tester ?
4. Soumettre la PR

## 📋 TODO List (Pick one!)

### 🔴 Haute Priorité

- [ ] **OSM Parser** - Implémenter `OSMParser::loadFile()` avec libosmium
  - Fichier: `src/data/OSMParser.cpp`
  - Difficulté: ⭐⭐⭐
  - Issue: #1

- [ ] **A* Pathfinding** - Algorithme de recherche de chemin
  - Fichier: `src/network/RoadGraph.cpp`, `src/network/PathPlanner.cpp`
  - Difficulté: ⭐⭐⭐⭐
  - Issue: #2

- [ ] **Véhicules sur routes** - Déplacement réaliste
  - Fichier: `src/core/SimulationEngine.cpp`
  - Difficulté: ⭐⭐⭐
  - Issue: #3

- [ ] **Visualisation V2V** - Afficher connexions
  - Fichier: `src/visualization/MapView.cpp`
  - Difficulté: ⭐⭐
  - Issue: #4

### 🟡 Moyenne Priorité

- [ ] **Tests unitaires** - Ajouter Google Test
- [ ] **Performance** - Profiling et optimisation
- [ ] **Documentation** - Doxygen
- [ ] **CI/CD** - GitHub Actions

### 🟢 Basse Priorité

- [ ] **Export CSV** - Données de simulation
- [ ] **Mode replay** - Rejouer simulation
- [ ] **Multi-villes** - Autres villes que Mulhouse

## 💻 Style de Code

### C++ Style Guide

```cpp
// Namespaces
namespace v2v {
namespace network {

// Classes : PascalCase
class RoadGraph {
public:
    // Méthodes : camelCase
    void addNode(double lat, double lon);
    
private:
    // Membres : m_ prefix + camelCase
    GraphType m_graph;
    int m_nodeCount;
};

// Constantes : UPPERCASE
const int MAX_VEHICLES = 5000;

// Variables locales : camelCase
int vehicleCount = 0;
double simulationTime = 0.0;

} // namespace network
} // namespace v2v
```

### Commits

Format: `<type>(<scope>): <message>`

Types:
- ✨ `feat` : Nouvelle fonctionnalité
- 🐛 `fix` : Correction de bug
- 📚 `docs` : Documentation
- 🎨 `style` : Formatage (pas de changement de code)
- ♻️ `refactor` : Refactoring
- ⚡ `perf` : Performance
- ✅ `test` : Tests
- 🔧 `chore` : Maintenance

Exemples:
```bash
git commit -m "✨ feat(osm): add parser for .osm.pbf files"
git commit -m "🐛 fix(map): correct zoom calculation"
git commit -m "📚 docs(readme): update installation instructions"
git commit -m "⚡ perf(graph): optimize R-tree queries"
```

## 🧪 Tests

```bash
# Compiler avec tests
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
ninja
ctest

# Lancer un test spécifique
./tests/test_roadgraph
```

## 📝 Documentation

Utiliser Doxygen style:

```cpp
/**
 * @brief Trouve le chemin le plus court entre deux nœuds
 * 
 * Utilise l'algorithme A* avec heuristique de distance euclidienne.
 * 
 * @param start Nœud de départ
 * @param end Nœud d'arrivée
 * @return Vecteur de VertexDescriptor représentant le chemin
 * 
 * @throws std::runtime_error Si aucun chemin n'existe
 * 
 * @example
 * ```cpp
 * auto path = roadGraph->getPath(startNode, endNode);
 * for (auto vertex : path) {
 *     // Traiter chaque nœud du chemin
 * }
 * ```
 */
std::vector<VertexDescriptor> getPath(VertexDescriptor start, VertexDescriptor end);
```

## 🔍 Code Review

Avant de soumettre une PR :

- [ ] Le code compile sans warnings
- [ ] Les tests passent
- [ ] Le code est commenté
- [ ] Pas de code mort (commented code)
- [ ] Pas de `TODO` laissés dans le code
- [ ] Performance acceptable (profiling si besoin)
- [ ] Pas de memory leaks (valgrind)

## 🤝 Communication

- 💬 **Discord/Slack** : [Lien à ajouter]
- 📧 **Email** : nourinemalek01@gmail.com
- 🐛 **Issues** : https://github.com/VOTRE_USERNAME/v2v-simulator/issues
- 📖 **Wiki** : https://github.com/VOTRE_USERNAME/v2v-simulator/wiki

## 🎓 Ressources

### OpenStreetMap
- https://wiki.openstreetmap.org/wiki/OSM_file_formats
- https://wiki.openstreetmap.org/wiki/Osmium

### Boost.Graph
- https://www.boost.org/doc/libs/1_83_0/libs/graph/doc/
- https://www.boost.org/doc/libs/1_83_0/libs/graph/doc/astar_search.html

### Qt
- https://doc.qt.io/qt-6/
- https://doc.qt.io/qt-6/qpainter.html

## ⚖️ Licence

En contribuant, vous acceptez que vos contributions soient sous licence MIT.

---

**Merci d'avoir contribué ! 🎉**
