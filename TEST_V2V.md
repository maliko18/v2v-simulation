# 🧪 Guide de Test V2V - CLion

## ✅ Ce que vous devez vérifier

### 1️⃣ **Dans CLion - Console de démarrage**

Quand vous lancez l'application (🐛 Debug ou ▶️ Run), vous devriez voir :

```
[INFO] SimulationEngine initialized with V2V communication  ← V2V activé ✅
[INFO] V2VCommunicationManager created                       ← Manager créé ✅
[INFO] InterferenceGraph created                             ← Graph d'interférence OK ✅
[INFO] RoadGraph created
[INFO] MainWindow constructed
[INFO] MapView created with OSM tile support (Alsace region)
```

**✅ Si vous voyez "with V2V communication" → V2V est ACTIF !**

---

### 2️⃣ **Dans l'application - Charger OSM et véhicules**

1. **Cliquez "Load OSM"**
   - Allez dans `data/`
   - Sélectionnez `mulhouse.osm`
   - Attendez 5 secondes

**Console CLion devrait afficher :**
```
[INFO] Loaded OSM: 2337 nodes, 4984 edges
[INFO] Spatial index built with 2337 nodes
```

2. **Réglez le nombre de véhicules**
   - Dans le panneau gauche : **100 véhicules**
   - (Commencez petit pour tester)

3. **Cliquez "Start Simulation"**

**Console CLion devrait afficher :**
```
[INFO] Creating 100 vehicles on road graph with 2337 nodes and 4984 edges
[INFO] Vehicle 0: start at (47.xxxxxx, 7.xxxxxx)
...
[INFO] Generating paths for all 100 vehicles (this may take a few seconds)...
[INFO] Path generation: 50/100 (50%) - 2s elapsed
[INFO] Path generation: 100/100 (100%) - 4s elapsed
[INFO] Path generation complete: 95 paths generated, 5 failed in 4000ms
[INFO] Simulation started
```

---

### 3️⃣ **Pendant la simulation - Logs V2V**

**Toutes les 5 secondes**, vous devriez voir dans la console CLion :

```
[INFO] V2V Stats: 2500 msgs sent, 2450 received, 50 dropped | Latency avg: 10.25ms | Connections: 450 | Neighbors avg: 4.5
[INFO] V2V Stats: 5000 msgs sent, 4900 received, 100 dropped | Latency avg: 10.18ms | Connections: 448 | Neighbors avg: 4.4
[INFO] V2V Stats: 7500 msgs sent, 7350 received, 150 dropped | Latency avg: 10.32ms | Connections: 452 | Neighbors avg: 4.6
```

**📊 Analyse des statistiques :**

| Statistique | Valeur attendue | Signification |
|-------------|----------------|---------------|
| **msgs sent** | Augmente constamment | Messages CAM envoyés (5 Hz × 100 véhicules = 500/s) |
| **received** | ~98% de sent | Messages reçus avec succès |
| **dropped** | ~2-5% de sent | Perte de paquets (normal, 5% configuré) |
| **Latency avg** | 10-15ms | Latence réseau simulée |
| **Connections** | 200-600 | Nombre de connexions V2V actives |
| **Neighbors avg** | 2-8 | Nombre moyen de voisins par véhicule |

**✅ Si ces logs apparaissent → Communication V2V FONCTIONNE !**

---

### 4️⃣ **Tests de performance**

#### Test avec 100 véhicules :
- **FPS attendu** : 60 FPS
- **Logs V2V** : Toutes les 5s
- **Latence** : 10-15ms
- **Voisins** : 2-6 en moyenne

#### Test avec 500 véhicules :
- **FPS attendu** : 40-50 FPS
- **Logs V2V** : Toutes les 5s
- **Messages/s** : ~2500 (500 véhicules × 5 Hz)
- **Voisins** : 3-8 en moyenne

#### Test avec 1000 véhicules :
- **FPS attendu** : 30 FPS
- **Logs V2V** : Toutes les 5s
- **Messages/s** : ~5000 (1000 véhicules × 5 Hz)
- **Voisins** : 4-10 en moyenne

---

### 5️⃣ **Breakpoints utiles dans CLion**

Pour voir V2V en action, mettez des breakpoints :

**Fichier : `src/core/SimulationEngine.cpp`**
- Ligne ~418 : `if (m_simulationTime - m_lastCAMTime >= camInterval)`
  - Vous verrez quand les CAM sont envoyés (toutes les 0.2s)

**Fichier : `src/communication/V2VCommunicationManager.cpp`**
- Ligne ~45 : `int V2VCommunicationManager::broadcastMessage(...)`
  - Vous verrez chaque message broadcast

**Fichier : `src/network/InterferenceGraph.cpp`**
- Ligne ~14 : `void InterferenceGraph::update(...)`
  - Vous verrez l'update du graphe (toutes les 10 frames)

---

### 6️⃣ **Vérifications visuelles**

Dans l'application qui tourne :

✅ **Fenêtre principale :**
- [ ] Carte OSM visible (tuiles Mulhouse)
- [ ] Véhicules visibles (cercles rouges)
- [ ] Véhicules se déplacent sur les routes
- [ ] FPS affiché dans la barre de titre (~30-60 FPS)

✅ **Panneau gauche :**
- [ ] Nombre de véhicules affiché
- [ ] FPS affiché
- [ ] Boutons Start/Pause/Stop fonctionnels

---

### 7️⃣ **Tests négatifs (vérifier robustesse)**

#### Test 1 : Charger OSM sans véhicules
- Load OSM → Start
- **Attendu** : Pas de crash, logs OK

#### Test 2 : Start sans OSM
- Start directement
- **Attendu** : Véhicules créés aléatoirement, simulation fonctionne

#### Test 3 : Changer véhicules pendant simulation
- Start → changer nombre véhicules → Stop → Start
- **Attendu** : Nouveaux véhicules créés, pas de crash

---

### 8️⃣ **Indicateurs de succès**

✅ **V2V fonctionne si :**
1. Log "initialized with V2V communication" au démarrage
2. Logs "V2V Stats" toutes les 5 secondes
3. Messages sent/received augmentent constamment
4. Latence stable (10-15ms)
5. Connexions > 0
6. Neighbors > 0
7. Pas de crash après 1 minute
8. FPS stable (30-60)

❌ **Problèmes possibles :**
- Pas de logs V2V → V2V désactivé
- Messages sent = 0 → InterferenceGraph non mis à jour
- Crash au démarrage → Erreur de compilation
- FPS < 20 → Trop de véhicules

---

### 9️⃣ **Commandes de debug dans CLion**

**Console GDB (si debug) :**
```gdb
# Afficher statistiques V2V
p m_v2vManager->getStatistics()

# Afficher véhicule ID 0
p m_vehicles[0]->getNeighborCount()

# Afficher connexions du graphe
p m_interferenceGraph->getConnectionCount()
```

---

### 🎯 **Résumé rapide**

**Pour tester V2V en 2 minutes :**

1. ▶️ Lancez dans CLion
2. 📁 Load OSM → `mulhouse.osm`
3. 🔢 100 véhicules
4. ▶️ Start
5. 👀 Regardez la console → logs "V2V Stats" toutes les 5s

**Si vous voyez les stats V2V → ✅ ÇA FONCTIONNE !**
