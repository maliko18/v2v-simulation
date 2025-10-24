# 🪟 Installation V2V Simulator sur Windows avec CLion

Guide pour installer et compiler le projet V2V Simulator sur Windows avec **CLion** (plus simple que Visual Studio).

## ⚡ Installation Rapide avec CLion

CLion intègre tout : CMake, Git, compilateur. C'est l'IDE le plus facile pour ce projet !

---

## 📋 Étape 1 : Installer les Outils

### 1️⃣ **CLion** (JetBrains - 30 jours gratuit, puis payant ou licence étudiante)

**Télécharger CLion :**
```
https://www.jetbrains.com/clion/download/
```

**Licence Gratuite pour Étudiants :**
```
https://www.jetbrains.com/community/education/#students
```
Avec votre email universitaire, vous avez CLion gratuit !

**Installation :**
- Télécharger l'installateur Windows
- Installer avec les options par défaut
- Au premier lancement, choisir le thème (Darcula recommandé)

### 2️⃣ **MinGW-w64 ou MSVC** (Compilateur C++)

CLion peut utiliser 2 compilateurs sur Windows :

#### **Option A : MinGW-w64** (Recommandé - Plus simple)

CLion peut installer MinGW automatiquement :

1. Lancer CLion
2. **File → Settings → Build, Execution, Deployment → Toolchains**
3. Cliquer sur le **+** et choisir **MinGW**
4. CLion propose de télécharger MinGW automatiquement
5. Cliquer sur **Download** et attendre l'installation

✅ **MinGW installé automatiquement !**

#### **Option B : Visual Studio Build Tools** (Si vous préférez MSVC)

Télécharger :
```
https://visualstudio.microsoft.com/fr/downloads/
```
Installer **"Build Tools pour Visual Studio 2022"** (pas l'IDE complet)
- Cocher : ✅ **Développement Desktop en C++**

### 3️⃣ **Qt 6 pour Windows**

**Télécharger Qt Online Installer :**
```
https://www.qt.io/download-qt-installer
```

**Installation :**
1. Créer un compte Qt (gratuit)
2. Choisir **Custom Installation**
3. Sélectionner :
   - ✅ **Qt 6.5.3** (ou version récente)
   - ✅ **MinGW 11.2.0 64-bit** (si vous utilisez MinGW)
   - OU ✅ **MSVC 2019 64-bit** (si vous utilisez MSVC)
   - ✅ **CMake**
   - ✅ **Ninja**

**Installation par défaut :**
- MinGW : `C:\Qt\6.5.3\mingw_64`
- MSVC : `C:\Qt\6.5.3\msvc2019_64`

Taille : ~3 GB

### 4️⃣ **vcpkg** (Gestionnaire de packages pour Boost, TBB, Eigen)

Ouvrir **PowerShell** :

```powershell
# Aller dans C:\
cd C:\

# Cloner vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Initialiser
.\bootstrap-vcpkg.bat

# Installer les dépendances
.\vcpkg install boost-graph:x64-mingw-dynamic
.\vcpkg install boost-geometry:x64-mingw-dynamic
.\vcpkg install tbb:x64-mingw-dynamic
.\vcpkg install eigen3:x64-mingw-dynamic
```

**Note** : Utilisez `:x64-mingw-dynamic` si MinGW, ou `:x64-windows` si MSVC.

⏱️ Durée : 5-10 minutes

---

## 🔽 Étape 2 : Cloner le Projet depuis GitHub

### **Dans CLion directement :**

1. Ouvrir CLion
2. **Get from VCS** (sur l'écran d'accueil)
3. URL : `https://github.com/maliko18/v2v-simulation.git`
4. Directory : `C:\Users\VotreNom\Documents\v2v-simulation`
5. Cliquer sur **Clone**

✅ Le projet est cloné et ouvert automatiquement dans CLion !

### **Ou avec PowerShell :**

```powershell
cd $HOME\Documents
git clone https://github.com/maliko18/v2v-simulation.git
```

Puis dans CLion : **File → Open** → Sélectionner le dossier `v2v-simulation`

---

## 🔧 Étape 3 : Configurer CLion

### **Configuration Automatique CMake**

CLion détecte automatiquement `CMakeLists.txt` et configure le projet.

**Si besoin de configuration manuelle :**

1. **File → Settings → Build, Execution, Deployment → CMake**

2. Cliquer sur le **+** pour ajouter un profil (si pas déjà fait)

3. **Configuration : Release**

4. **CMake options :**

   **Pour MinGW :**
   ```
   -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/mingw_64
   -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
   -G "Ninja"
   ```

   **Pour MSVC :**
   ```
   -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2019_64
   -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
   -G "Ninja"
   ```

5. **Build directory :** `cmake-build-release` (par défaut)

6. Cliquer sur **OK**

### **Recharger le Projet**

CLion va automatiquement :
- Télécharger les dépendances
- Configurer CMake
- Indexer le code

Attendre que la barre de progression en bas se termine (~2-3 minutes).

---

## 🔨 Étape 4 : Compiler le Projet

### **Dans CLion :**

1. Dans la barre d'outils en haut :
   - Configuration : **v2v_simulator | Release**
   - Cliquer sur le **marteau 🔨** (Build)

2. Ou via menu : **Build → Build Project** (Ctrl+F9)

3. Attendre la fin de la compilation (~2-3 minutes)

✅ La compilation devrait réussir sans erreurs !

**Console de compilation :**
```
[1/44] Building CXX object ...
[2/44] Building CXX object ...
...
[44/44] Linking CXX executable v2v_simulator.exe
Build finished
```

---

## 🚀 Étape 5 : Lancer l'Application

### **Copier les DLLs Qt d'abord !**

**Méthode Automatique (PowerShell) :**

```powershell
# Aller dans le dossier de build
cd $HOME\Documents\v2v-simulation\cmake-build-release

# Copier automatiquement toutes les DLLs Qt nécessaires
C:\Qt\6.5.3\mingw_64\bin\windeployqt.exe v2v_simulator.exe
```

**Pour MSVC :**
```powershell
C:\Qt\6.5.3\msvc2019_64\bin\windeployqt.exe v2v_simulator.exe
```

✅ Les DLLs sont copiées !

### **Lancer depuis CLion :**

1. Cliquer sur le **bouton Run ▶️** (à côté du marteau)
2. Ou via menu : **Run → Run 'v2v_simulator'** (Shift+F10)

🎉 **L'application devrait s'ouvrir avec la carte de Mulhouse !**

### **Lancer manuellement :**

```powershell
cd $HOME\Documents\v2v-simulation\cmake-build-release
.\v2v_simulator.exe
```

---

## 🐛 Problèmes Courants dans CLion

### ❌ "CMake Error: Qt6_DIR not found"

**Solution :**
1. **File → Settings → Build → CMake**
2. Dans **CMake options**, ajouter :
   ```
   -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/mingw_64
   ```
3. Cliquer sur **Reload CMake Project** (icône ↻)

### ❌ "Boost not found"

**Solution :**
1. Vérifier que vcpkg a bien installé Boost :
   ```powershell
   C:\vcpkg\vcpkg list | findstr boost
   ```
2. Ajouter dans CMake options :
   ```
   -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
   ```

### ❌ "Qt6Core.dll introuvable" au lancement

**Solution :** Exécuter `windeployqt.exe` (voir étape 5)

### ❌ "Toolchain is not configured"

**Solution :**
1. **File → Settings → Build → Toolchains**
2. Vérifier que MinGW ou MSVC est bien détecté
3. Si rouge, cliquer sur **Download** pour MinGW

### ❌ Compilation très lente

**Solution :** Activer la compilation parallèle
1. **File → Settings → Build → CMake**
2. Dans **Build options**, ajouter :
   ```
   -j 8
   ```
   (8 = nombre de threads, adapter selon votre CPU)

---

## 🎨 Configuration CLion Recommandée

### **Activer le formatage automatique**

1. **File → Settings → Editor → Code Style → C/C++**
2. **Set from... → Predefined Style → Google**
3. Cocher : ✅ **Reformat code on file save**

### **Activer l'auto-complétion**

1. **File → Settings → Editor → General → Code Completion**
2. Cocher : ✅ **Show suggestions as you type**

### **Thème sombre (optionnel)**

1. **File → Settings → Appearance & Behavior → Appearance**
2. **Theme : Darcula**

---

## 🔄 Workflow de Développement dans CLion

### **1. Modifier le code**
- Éditer les fichiers dans l'éditeur
- L'auto-complétion fonctionne

### **2. Compiler**
- Cliquer sur 🔨 ou Ctrl+F9
- Voir les erreurs dans la console du bas

### **3. Lancer**
- Cliquer sur ▶️ ou Shift+F10
- L'application se lance avec débogueur attaché

### **4. Déboguer**
- Mettre un breakpoint (clic gauche dans la marge)
- Cliquer sur 🐛 (Debug) au lieu de ▶️
- F8 : Step Over
- F7 : Step Into

### **5. Commit Git**
- **VCS → Commit** (Ctrl+K)
- Écrire le message de commit
- **Commit and Push**

---

## 📝 Raccourcis CLion Utiles

| Raccourci | Action |
|-----------|--------|
| `Ctrl+F9` | Build Project |
| `Shift+F10` | Run |
| `Shift+F9` | Debug |
| `Ctrl+K` | Commit |
| `Ctrl+Shift+K` | Push |
| `Alt+1` | Project view |
| `Alt+4` | Run console |
| `Ctrl+Space` | Auto-complete |
| `Ctrl+B` | Go to definition |
| `Ctrl+Shift+F` | Find in project |

---

## 🔄 Mise à Jour du Projet

Pour récupérer les dernières modifications depuis GitHub :

### **Dans CLion :**
1. **VCS → Git → Pull...**
2. Sélectionner **origin/main**
3. Cliquer sur **Pull**

### **Ou PowerShell :**
```powershell
cd $HOME\Documents\v2v-simulation
git pull origin main
```

CLion va automatiquement recharger le projet.

---

## 📊 Structure après Installation

```
C:\Users\VotreNom\Documents\v2v-simulation\
├── include/                    # Headers C++
├── src/                        # Code source
├── config/                     # Configuration JSON
├── cmake-build-release/        # Dossier de build CLion
│   ├── v2v_simulator.exe      ← L'application
│   ├── Qt6Core.dll            ← DLLs Qt (après windeployqt)
│   ├── Qt6Gui.dll
│   └── platforms/
│       └── qwindows.dll
└── CMakeLists.txt             # Configuration CMake
```

---

## 💡 Astuces CLion

### **Voir les TODO dans le code**
- **View → Tool Windows → TODO**
- Liste tous les `// TODO:` du projet

### **Profiler les performances**
- **Run → Profile 'v2v_simulator'**
- Voir quelles fonctions prennent du temps

### **Analyse statique**
- **Code → Inspect Code**
- Détecte les bugs potentiels

---

## 🎮 Utilisation de l'Application

Une fois lancée :

| Contrôle | Action |
|----------|--------|
| 🖱️ **Drag** | Déplacer la carte |
| 🖱️ **Molette** | Zoomer/Dézoomer |
| ▶️ **Start** | Démarrer la simulation |
| ⏸️ **Pause** | Mettre en pause |
| 🎚️ **Speed** | Vitesse (0.1x à 10x) |
| 🔢 **Vehicles** | Nombre de véhicules |
| 📡 **Radius** | Rayon transmission V2V |

---

## 📞 Support

- 🐛 **Bugs** : https://github.com/maliko18/v2v-simulation/issues
- 💬 **Questions** : Ouvrir une issue
- 📧 **Email** : nourinemalek01@gmail.com

---

## ✅ Checklist Installation CLion

- [ ] CLion installé et licence activée
- [ ] MinGW ou MSVC configuré dans CLion
- [ ] Qt 6.5.3 installé avec MinGW/MSVC
- [ ] vcpkg installé avec Boost, TBB, Eigen3
- [ ] Projet cloné depuis GitHub
- [ ] CMake configuré (PREFIX_PATH + TOOLCHAIN_FILE)
- [ ] Projet compilé sans erreurs
- [ ] DLLs Qt copiées avec windeployqt
- [ ] Application se lance avec ▶️
- [ ] Carte OSM de Mulhouse visible

---

## 🎓 Ressources CLion

- **Documentation** : https://www.jetbrains.com/help/clion/
- **Tutoriel CMake** : https://www.jetbrains.com/help/clion/quick-cmake-tutorial.html
- **Débugger** : https://www.jetbrains.com/help/clion/debugging-code.html

---

**Temps total : 30-45 minutes**

CLion rend tout plus simple : pas besoin de gérer manuellement CMake en ligne de commande ! 🚀

Bon développement avec CLion ! 🎉
