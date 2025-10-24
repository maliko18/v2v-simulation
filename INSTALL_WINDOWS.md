# 🪟 Installation V2V Simulator sur Windows

Guide complet pour installer et compiler le projet V2V Simulator sur Windows 10/11.

## ⚡ Installation Rapide (15-20 minutes)

### 1️⃣ Installer les Outils de Base

#### **Git for Windows**
```
https://git-scm.com/download/win
```
- Télécharger et installer avec les options par défaut
- Vérifier : Ouvrir PowerShell et taper `git --version`

#### **Visual Studio 2022 Community** (Gratuit)
```
https://visualstudio.microsoft.com/fr/downloads/
```
**Lors de l'installation, COCHER :**
- ✅ **Développement Desktop en C++**
- ✅ **Outils CMake C++ pour Windows**
- ✅ **Git for Windows** (si pas déjà installé)

Taille : ~8 GB

#### **Qt 6 Online Installer**
```
https://www.qt.io/download-qt-installer
```
**Lors de l'installation :**
1. Créer un compte Qt (gratuit pour usage personnel)
2. Choisir **Custom Installation**
3. Sélectionner :
   - ✅ **Qt 6.5.3 → MSVC 2019 64-bit**
   - ✅ **Qt Creator** (IDE optionnel mais pratique)
   - ✅ **CMake**
   - ✅ **Ninja**

Installation par défaut : `C:\Qt\6.5.3`

Taille : ~3 GB

---

### 2️⃣ Installer vcpkg (Gestionnaire de Packages)

Ouvrir **PowerShell en Administrateur** :

```powershell
# Aller dans C:\
cd C:\

# Cloner vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Initialiser vcpkg
.\bootstrap-vcpkg.bat
```

**Installer les dépendances C++ :**

```powershell
# Boost Graph
.\vcpkg install boost-graph:x64-windows

# Boost Geometry
.\vcpkg install boost-geometry:x64-windows

# Intel TBB
.\vcpkg install tbb:x64-windows

# Eigen3
.\vcpkg install eigen3:x64-windows
```

⏱️ Durée : 5-10 minutes selon votre connexion

---

### 3️⃣ Cloner le Projet depuis GitHub

Ouvrir **PowerShell** (pas besoin d'admin) :

```powershell
# Aller dans vos Documents
cd $HOME\Documents

# Cloner le projet
git clone https://github.com/maliko18/v2v-simulation.git
cd v2v-simulation
```

📁 Le projet est maintenant dans : `C:\Users\VotreNom\Documents\v2v-simulation`

---

### 4️⃣ Compiler le Projet

#### **Option A : Avec Visual Studio (Recommandé)**

1. **Ouvrir Visual Studio 2022**
2. **Fichier → Ouvrir → Dossier...**
3. Sélectionner `C:\Users\VotreNom\Documents\v2v-simulation`
4. Visual Studio détecte automatiquement `CMakeLists.txt`
5. Attendre la configuration CMake (barre de progression en bas)
6. Dans la barre d'outils :
   - Mode : **x64-Release**
   - Cliquer : **Générer → Tout régénérer**
7. Attendre la fin de la compilation (~2-3 minutes)

✅ L'exécutable est dans : `out\build\x64-Release\v2v_simulator.exe`

#### **Option B : Ligne de Commande**

```powershell
cd $HOME\Documents\v2v-simulation

# Créer dossier build
mkdir build
cd build

# Configurer CMake
cmake .. -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.5.3\msvc2019_64" `
  -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake"

# Compiler
cmake --build . --config Release
```

✅ L'exécutable est dans : `build\Release\v2v_simulator.exe`

---

### 5️⃣ Copier les DLLs Qt (Important!)

Windows a besoin des DLLs Qt à côté de l'exécutable.

#### **Méthode Automatique (Recommandée)**

```powershell
# Aller dans le dossier de l'exécutable
cd build\Release

# Utiliser windeployqt pour copier automatiquement toutes les DLLs
C:\Qt\6.5.3\msvc2019_64\bin\windeployqt.exe v2v_simulator.exe
```

✅ Toutes les DLLs nécessaires sont copiées !

#### **Méthode Manuelle (Si problème)**

```powershell
cd build\Release

# Copier les DLLs Qt
copy "C:\Qt\6.5.3\msvc2019_64\bin\Qt6Core.dll" .
copy "C:\Qt\6.5.3\msvc2019_64\bin\Qt6Gui.dll" .
copy "C:\Qt\6.5.3\msvc2019_64\bin\Qt6Widgets.dll" .
copy "C:\Qt\6.5.3\msvc2019_64\bin\Qt6Network.dll" .
copy "C:\Qt\6.5.3\msvc2019_64\bin\Qt6Positioning.dll" .

# Copier le plugin platforms
mkdir platforms
copy "C:\Qt\6.5.3\msvc2019_64\plugins\platforms\qwindows.dll" platforms\
```

---

### 6️⃣ Lancer l'Application ! 🚀

```powershell
cd $HOME\Documents\v2v-simulation\build\Release
.\v2v_simulator.exe
```

🎉 **L'application devrait s'ouvrir avec la carte de Mulhouse !**

---

## 🐛 Problèmes Courants

### ❌ "Qt6Core.dll introuvable"
**Solution** : Exécuter `windeployqt.exe` (voir étape 5)

### ❌ "VCRUNTIME140.dll introuvable"
**Solution** : Installer Visual C++ Redistributable
```
https://aka.ms/vs/17/release/vc_redist.x64.exe
```

### ❌ "CMake ne trouve pas Qt"
**Solution** : Vérifier le chemin dans la commande cmake :
```powershell
-DCMAKE_PREFIX_PATH="C:\Qt\6.5.3\msvc2019_64"
```
Adapter selon votre version Qt installée !

### ❌ "Boost not found"
**Solution** : Vérifier que vcpkg est bien configuré :
```powershell
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
```

### ❌ L'application ne se lance pas
**Solution** : Ouvrir PowerShell dans le dossier Release et lancer :
```powershell
.\v2v_simulator.exe
```
Regarder les erreurs affichées dans le terminal.

---

## 🔄 Mise à Jour du Projet

Pour récupérer les dernières modifications depuis GitHub :

```powershell
cd $HOME\Documents\v2v-simulation

# Récupérer les changements
git pull origin main

# Recompiler
cd build
cmake --build . --config Release
```

---

## 📝 Structure après Installation

```
C:\Users\VotreNom\Documents\v2v-simulation\
├── include/                 # Headers C++
├── src/                     # Code source
├── config/                  # Configuration JSON
├── build/                   # Dossier de compilation
│   └── Release/
│       ├── v2v_simulator.exe    ← L'application
│       ├── Qt6Core.dll          ← DLLs Qt
│       ├── Qt6Gui.dll
│       └── platforms/
│           └── qwindows.dll
└── CMakeLists.txt          # Configuration CMake
```

---

## 🎮 Utilisation

Une fois l'application lancée :

- 🖱️ **Drag** : Déplacer la carte
- 🖱️ **Molette** : Zoomer/Dézoomer
- ▶️ **Start** : Démarrer la simulation
- ⏸️ **Pause** : Mettre en pause
- 🎚️ **Speed** : Ajuster la vitesse (0.1x à 10x)
- 🔢 **Vehicles** : Nombre de véhicules (10 à 5000)
- 📡 **Radius** : Rayon de transmission (100 à 500m)

---

## 🛠️ Développement

### Ouvrir avec Qt Creator (Alternative à Visual Studio)

1. Lancer Qt Creator
2. **Fichier → Ouvrir un fichier ou un projet**
3. Sélectionner `CMakeLists.txt`
4. Configurer le kit **Desktop Qt 6.5.3 MSVC2019 64bit**
5. Cliquer sur le marteau 🔨 pour compiler
6. Cliquer sur la flèche ▶️ pour lancer

---

## 📞 Besoin d'Aide ?

- 🐛 **Bugs** : https://github.com/maliko18/v2v-simulation/issues
- 💬 **Questions** : Ouvrir une issue avec le tag "question"
- 📧 **Contact** : nourinemalek01@gmail.com

---

## ✅ Checklist Installation

- [ ] Visual Studio 2022 installé avec C++
- [ ] Qt 6.5.3 MSVC installé
- [ ] vcpkg installé et configuré
- [ ] Boost, TBB, Eigen3 installés via vcpkg
- [ ] Projet cloné depuis GitHub
- [ ] CMake configuré avec succès
- [ ] Projet compilé sans erreurs
- [ ] DLLs Qt copiées avec windeployqt
- [ ] Application se lance correctement
- [ ] Carte OSM de Mulhouse s'affiche

---

**Temps total d'installation : 30-45 minutes (première fois)**

Bon développement ! 🚗💨
