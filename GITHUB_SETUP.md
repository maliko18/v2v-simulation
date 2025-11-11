# 📤 Guide de Publication sur GitHub

## Étape 1 : Créer un dépôt sur GitHub

1. Aller sur **https://github.com**
2. Cliquer sur **"New repository"** (bouton vert)
3. Remplir les informations :
   - **Repository name** : `v2v-simulator`
   - **Description** : `Simulateur haute performance de véhicules communicants avec visualisation OSM`
   - **Visibilité** : 
     - ✅ **Public** (si vous voulez le partager avec tout le monde)
     - 🔒 **Private** (si seulement vos amis doivent y accéder)
   - ❌ **Ne pas cocher** "Add a README file" (on en a déjà un)
   - ❌ **Ne pas cocher** ".gitignore" (on en a déjà un)
   - ✅ **License** : MIT (recommandé)

4. Cliquer sur **"Create repository"**

## Étape 2 : Pusher le code

GitHub vous donnera des commandes. Utilisez celles-ci :

```bash
cd /home/nourine/Documents/reseaux_mobile/v2v-simulator

# Ajouter le remote GitHub
git remote add origin https://github.com/VOTRE_USERNAME/v2v-simulator.git

# Renommer la branche en 'main' (standard GitHub)
git branch -M main

# Pusher le code
git push -u origin main
```

**Note** : Remplacez `VOTRE_USERNAME` par votre nom d'utilisateur GitHub !

## Étape 3 : Donner accès à vos amis

### Si dépôt **Public** 
✅ Rien à faire ! Vos amis peuvent directement cloner avec :
```bash
git clone https://github.com/VOTRE_USERNAME/v2v-simulator.git
```

### Si dépôt **Private**
1. Aller dans **Settings** → **Collaborators**
2. Cliquer sur **"Add people"**
3. Entrer le username GitHub de vos amis
4. Ils recevront une invitation par email

## Étape 4 : Vos amis clonent le projet

```bash
# Cloner
git clone https://github.com/VOTRE_USERNAME/v2v-simulator.git
cd v2v-simulator

# Installer dépendances (Ubuntu/Debian)
sudo apt update
sudo apt install -y build-essential cmake ninja-build
sudo apt install -y qt6-base-dev qt6-positioning-dev
sudo apt install -y libboost-all-dev libtbb-dev libeigen3-dev

# Compiler
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja

# Lancer
./v2v_simulator
```

## 📋 Commandes Git utiles pour le travail collaboratif

### Pour vous (mainteneur)

```bash
# Voir l'état des fichiers
git status

# Ajouter des modifications
git add .

# Commit
git commit -m "Description des changements"

# Pusher vers GitHub
git push

# Créer une nouvelle branche pour une feature
git checkout -b feature/nom-feature
```

### Pour vos amis (contributeurs)

```bash
# Récupérer les dernières modifications
git pull

# Créer une branche pour leur travail
git checkout -b feature/leur-feature

# Faire leurs modifications...

# Commit
git add .
git commit -m "Ajout de leur feature"

# Pusher leur branche
git push origin feature/leur-feature

# Ensuite, ils créent une Pull Request sur GitHub
```

## 🔄 Workflow de collaboration recommandé

### 1. **Branches protégées**
- `main` : Code stable, testé
- `develop` : Code en développement
- `feature/*` : Nouvelles fonctionnalités
- `fix/*` : Corrections de bugs

### 2. **Pull Requests**
Vos amis ne pushent jamais directement sur `main`. Ils :
1. Créent une branche
2. Font leurs modifications
3. Créent une Pull Request
4. Vous reviewez
5. Vous mergez

### 3. **Issues**
Utilisez les GitHub Issues pour :
- 🐛 Reporter des bugs
- ✨ Proposer des features
- 📋 TODO lists
- ❓ Poser des questions

## 📝 Exemple de .git/config

Après avoir ajouté le remote, votre fichier `.git/config` devrait ressembler à :

```ini
[core]
    repositoryformatversion = 0
    filemode = true
    bare = false
    logallrefupdates = true

[remote "origin"]
    url = https://github.com/VOTRE_USERNAME/v2v-simulator.git
    fetch = +refs/heads/*:refs/remotes/origin/*

[branch "main"]
    remote = origin
    merge = refs/heads/main

[user]
    name = Nourine
    email = nourinemalek01@gmail.com
```

## 🎯 Checklist avant de pusher

- [ ] Code compile sans erreurs
- [ ] Pas de fichiers sensibles (mots de passe, clés API)
- [ ] `.gitignore` à jour (pas de binaires, pas de build/)
- [ ] README clair avec instructions
- [ ] Licence ajoutée (MIT recommandé)
- [ ] Email et nom configurés dans Git

## 🆘 Problèmes courants

### Erreur : "Permission denied"
```bash
# Vérifier votre authentication
git remote set-url origin https://VOTRE_USERNAME@github.com/VOTRE_USERNAME/v2v-simulator.git

# Ou utiliser SSH (recommandé)
git remote set-url origin git@github.com:VOTRE_USERNAME/v2v-simulator.git
```

### Erreur : "Repository not found"
Vérifiez :
- Le nom du dépôt est correct
- Vous avez les droits d'accès
- L'URL est correcte

### Fichiers trop gros (>100MB)
GitHub limite à 100MB par fichier. Si vous avez des gros fichiers OSM :
```bash
# Les ajouter à .gitignore
echo "data/osm/*.pbf" >> .gitignore

# Ou utiliser Git LFS
git lfs install
git lfs track "*.pbf"
```

## 🎓 Ressources

- **GitHub Docs** : https://docs.github.com
- **Git Documentation** : https://git-scm.com/doc
- **Git Cheat Sheet** : https://education.github.com/git-cheat-sheet-education.pdf

---

✅ Une fois sur GitHub, partagez simplement l'URL à vos amis :
```
https://github.com/VOTRE_USERNAME/v2v-simulator
```
