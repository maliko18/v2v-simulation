#!/bin/bash
# Script de lancement optimisé pour V2V Simulator
# Utiliser ce script si vous avez des problèmes de performance avec 2000+ véhicules

# Aller dans le dossier du projet
cd "$(dirname "$0")"

# =============================================
# OPTIMISATIONS CRITIQUES POUR VOTRE MACHINE
# =============================================

# FORCER X11 au lieu de Wayland (plus stable pour Qt avec AMD)
export QT_QPA_PLATFORM=xcb
export GDK_BACKEND=x11

# Désactiver le scaling HiDPI
export QT_SCALE_FACTOR=1
export QT_AUTO_SCREEN_SCALE_FACTOR=0

# Optimisations de rendu Qt
export QSG_RENDER_LOOP=basic
export QT_QUICK_BACKEND=software

# Désactiver VSync pour maximiser FPS
export vblank_mode=0

# Utiliser le GPU dédié AMD (RX 6700S) au lieu du GPU intégré
export DRI_PRIME=1

# Optimiser l'allocation mémoire
export MALLOC_ARENA_MAX=2

# Afficher la configuration
echo "========================================"
echo "V2V Simulator - Mode Optimisé"
echo "========================================"
echo "Platform: $QT_QPA_PLATFORM (X11 forcé)"
echo "GPU: Dédié (DRI_PRIME=1)"
echo "VSync: Disabled"
echo ""

# Vérifier si le binaire existe
if [ -f "cmake-build-debug/v2v_simulator" ]; then
    echo "Lancement depuis cmake-build-debug..."
    ./cmake-build-debug/v2v_simulator "$@"
elif [ -f "build/v2v_simulator" ]; then
    echo "Lancement depuis build..."
    ./build/v2v_simulator "$@"
else
    echo "ERREUR: Binaire v2v_simulator non trouvé!"
    echo "Compilez le projet d'abord avec: cmake --build cmake-build-debug"
    exit 1
fi

