#!/bin/bash
# Script de diagnostic pour comparer les configurations
# Exécutez ce script sur les deux machines et comparez les résultats

echo "========================================"
echo "V2V Simulator - Diagnostic Système"
echo "========================================"
echo "Date: $(date)"
echo ""

echo "=== SYSTÈME ==="
uname -a
echo ""

echo "=== CPU ==="
lscpu | grep -E "Model name|CPU\(s\)|Thread|Core|MHz"
echo ""

echo "=== MÉMOIRE ==="
free -h
echo ""

echo "=== GPU ==="
if command -v glxinfo &> /dev/null; then
    glxinfo | grep -E "OpenGL vendor|OpenGL renderer|OpenGL version" 2>/dev/null
else
    echo "glxinfo non installé (installez mesa-utils)"
fi
echo ""

echo "=== PILOTES GPU ==="
if [ -f /proc/driver/nvidia/version ]; then
    cat /proc/driver/nvidia/version
elif command -v nvidia-smi &> /dev/null; then
    nvidia-smi --query-gpu=driver_version,name --format=csv,noheader 2>/dev/null
else
    echo "Pas de pilote NVIDIA détecté (probablement AMD/Intel)"
    lspci | grep -i vga
fi
echo ""

echo "=== QT ==="
if command -v qmake &> /dev/null; then
    qmake --version
elif command -v qmake6 &> /dev/null; then
    qmake6 --version
else
    echo "qmake non trouvé dans PATH"
fi
echo ""

echo "=== BOOST ==="
if [ -f /usr/include/boost/version.hpp ]; then
    grep "define BOOST_VERSION " /usr/include/boost/version.hpp
elif [ -f /usr/local/include/boost/version.hpp ]; then
    grep "define BOOST_VERSION " /usr/local/include/boost/version.hpp
fi
echo ""

echo "=== SESSION GRAPHIQUE ==="
echo "XDG_SESSION_TYPE: $XDG_SESSION_TYPE"
echo "WAYLAND_DISPLAY: $WAYLAND_DISPLAY"
echo "DISPLAY: $DISPLAY"
echo ""

echo "=== LIMITES SYSTÈME ==="
ulimit -a 2>/dev/null | grep -E "open files|max memory|stack size"
echo ""

echo "=== SWAPPINESS ==="
cat /proc/sys/vm/swappiness
echo ""

echo "========================================"
echo "Copiez cette sortie et comparez avec l'autre machine"
echo "========================================"

