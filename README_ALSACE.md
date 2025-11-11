# 🗺️ Configuration Alsace - Guide d'utilisation

## Fichiers OSM disponibles

Nous avons téléchargé 3 fichiers OSM dans le dossier `data/` :

### 1. **mulhouse.osm** (11 MB) ⭐ Recommandé pour débuter
- **Zone** : Mulhouse uniquement
- **Routes** : Toutes (rues, chemins, autoroutes)
- **Temps de chargement** : ~5 secondes
- **Performances** : Excellentes (2000+ véhicules à 60 FPS)
- **Utilisation** : Tests et développement

### 2. **alsace_main_roads.osm** (50 MB) ⭐⭐ Recommandé pour l'Alsace
- **Zone** : Toute l'Alsace (Mulhouse → Strasbourg)
- **Routes** : Autoroutes, nationales, départementales uniquement
- **Temps de chargement** : ~30 secondes
- **Performances** : Bonnes (2000 véhicules à 50-60 FPS)
- **Utilisation** : **Meilleur compromis performance/couverture**

### 3. **alsace.osm** (701 MB) ⚠️ Fichier très volumineux
- **Zone** : Toute l'Alsace avec TOUTES les routes
- **Routes** : Autoroutes + routes + rues + chemins
- **Temps de chargement** : ~5-10 minutes ⚠️
- **Performances** : Moyennes (risque de lag avec parsing)
- **Utilisation** : Seulement si vous avez besoin de toutes les petites rues

## Configuration appliquée

La simulation est maintenant configurée pour **toute l'Alsace** :

```cpp
// Centre de la carte : Colmar (centre géographique de l'Alsace)
m_centerLat = 48.08°N
m_centerLon = 7.36°E
m_zoomLevel = 10  // Zoom réduit pour voir toute la région

// Zone de génération des véhicules
Latitude:  47.5° à 48.9°  (Mulhouse → Strasbourg)
Longitude: 7.1° à 8.2°    (Vosges → Rhin)
```

## Comment utiliser

### Étape 1 : Lancer l'application
```bash
cd cmake-build-debug
./v2v_simulator
```

### Étape 2 : Charger le fichier OSM
1. Cliquez sur le bouton **"Load OSM"** dans le panneau de gauche
2. Naviguez vers `../data/`
3. Choisissez le fichier selon vos besoins :
   - **alsace_main_roads.osm** ← Recommandé ✅
   - alsace.osm (si vous avez le temps d'attendre)
   - mulhouse.osm (zone limitée)

### Étape 3 : Paramétrer la simulation
- **Nombre de véhicules** : 2000 (fonctionne bien)
- **Rayon de transmission** : 300m
- **Speed** : 1.0x (temps réel)

### Étape 4 : Activer les options visuelles
- ✅ **Show Vehicles** : Activé
- ⚠️ **Show Connections** : Désactiver si lag (auto-désactivé si >200 véhicules visibles)
- ⚠️ **Show Road Graph** : Désactiver pendant simulation (utiliser pour debug uniquement)

### Étape 5 : Démarrer
Cliquez sur **Start** et profitez ! 🚗💨

## Villes couvertes

Avec le fichier **alsace_main_roads.osm**, vous couvrez :

### Haut-Rhin (67)
- ✅ Mulhouse
- ✅ Colmar
- ✅ Saint-Louis
- ✅ Altkirch
- ✅ Thann
- ✅ Guebwiller

### Bas-Rhin (68)
- ✅ Strasbourg
- ✅ Haguenau
- ✅ Sélestat
- ✅ Saverne
- ✅ Obernai
- ✅ Molsheim

### Axes principaux
- 🛣️ A35 (Autoroute Nord-Sud complète)
- 🛣️ A36 (Mulhouse → Belfort)
- 🛣️ N83, N66, N4, etc.

## Performances attendues

| Fichier              | Nœuds estimés | Arêtes estimées | Temps chargement | FPS (2000 véhicules) |
|----------------------|---------------|-----------------|------------------|----------------------|
| mulhouse.osm         | ~2,300        | ~5,000          | 5s               | 60                   |
| alsace_main_roads.osm| ~15,000       | ~30,000         | 30s              | 50-60                |
| alsace.osm           | ~200,000      | ~400,000        | 5-10min          | 40-50 (variable)     |

## Navigation dans la carte

### Raccourcis
- **Clic gauche + Drag** : Déplacer la carte
- **Molette souris** : Zoom in/out
- **Double-clic** : Centrer sur ce point

### Zones intéressantes à explorer
1. **Strasbourg** (48.58°N, 7.75°E) : Grande densité routière
2. **Colmar** (48.08°N, 7.36°E) : Centre de l'Alsace
3. **Mulhouse** (47.75°N, 7.34°E) : Zone industrielle
4. **A35** : Longue autoroute Nord-Sud

## Conseils de performance

### ✅ Pour 60 FPS constant :
1. Utiliser **alsace_main_roads.osm**
2. Limiter à **2000 véhicules maximum**
3. Désactiver "Show Connections" et "Show Road Graph"
4. Garder zoom entre 10-13

### ⚠️ Si vous avez du lag :
1. Réduire le nombre de véhicules (1000-1500)
2. Vérifier que "Show Road Graph" est désactivé
3. Utiliser mulhouse.osm au lieu de alsace.osm
4. Désactiver l'antialiasing dans les paramètres

## Résolution de problèmes

### Le chargement est très long (>2 minutes)
- **Cause** : Vous avez chargé `alsace.osm` (701 MB)
- **Solution** : Patience ! Ou utilisez `alsace_main_roads.osm` à la place

### Les véhicules ne bougent pas après chargement OSM
- **Cause** : Le graphe routier n'est pas connecté
- **Solution** : Vérifiez les logs dans la console, les véhicules devraient être recréés automatiquement

### FPS très bas (<30)
- **Cause 1** : Trop de véhicules pour votre CPU
- **Solution** : Réduire à 1000-1500 véhicules
- **Cause 2** : "Show Road Graph" activé avec alsace.osm
- **Solution** : Désactiver cette option

### La carte est blanche au démarrage
- **Cause** : Les tuiles OSM se téléchargent
- **Solution** : Attendez quelques secondes, elles s'afficheront progressivement

## Améliorations futures possibles

- [ ] Support du format .osm.pbf (plus compact et plus rapide)
- [ ] Indexation spatiale du graphe routier pour culling
- [ ] Cache du graphe parsé pour chargement instantané
- [ ] Level of Detail (LOD) adaptatif selon le zoom
- [ ] Multi-threading du parsing OSM avec TBB

---

**Configuration actuelle** : Centre sur Colmar, zoom 10, zone Alsace complète (47.5-48.9°N, 7.1-8.2°E)

**Fichier recommandé** : `alsace_main_roads.osm` (50 MB, excellent compromis)

**Performances cibles** : 2000 véhicules à 50-60 FPS ✅

*Dernière mise à jour : 30 octobre 2025*
