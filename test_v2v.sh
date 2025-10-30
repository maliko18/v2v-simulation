#!/bin/bash
# Script de test pour la fonctionnalité V2V

echo "======================================"
echo "Test de la communication V2V"
echo "======================================"
echo ""

# Couleurs
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Vérifier que l'exécutable existe
if [ ! -f "build/v2v_simulator" ]; then
    echo -e "${RED}✗ Exécutable non trouvé${NC}"
    echo "Compilez d'abord: cd build && ninja"
    exit 1
fi

echo -e "${GREEN}✓ Exécutable trouvé${NC}"
echo ""

# Vérifier les symboles V2V dans l'exécutable
echo "Vérification des symboles V2V..."
echo ""

symbols=(
    "V2VCommunicationManager"
    "V2VMessage"
    "CAM"
    "DENM"
    "broadcastMessage"
    "InterferenceGraph"
)

for symbol in "${symbols[@]}"; do
    if nm build/v2v_simulator | grep -q "$symbol"; then
        echo -e "${GREEN}✓${NC} $symbol trouvé"
    else
        echo -e "${RED}✗${NC} $symbol manquant"
    fi
done

echo ""
echo "======================================"
echo "Informations de compilation"
echo "======================================"
echo ""

# Taille de l'exécutable
size=$(du -h build/v2v_simulator | cut -f1)
echo "Taille: $size"

# Dépendances
echo ""
echo "Dépendances principales:"
ldd build/v2v_simulator | grep -E "(Qt6|boost|stdc++)" | head -10

echo ""
echo "======================================"
echo "Lancement du simulateur"
echo "======================================"
echo ""
echo -e "${YELLOW}Instructions:${NC}"
echo "1. Chargez un fichier OSM (Load OSM → mulhouse.osm)"
echo "2. Réglez 100 véhicules (pour tester V2V)"
echo "3. Cliquez Start"
echo "4. Observez la console pour les logs V2V"
echo ""
echo -e "${YELLOW}Logs à vérifier:${NC}"
echo "- 'V2VCommunicationManager created'"
echo "- 'SimulationEngine initialized with V2V communication'"
echo "- Messages CAM envoyés"
echo ""

# Lancer l'application
echo "Lancement de l'application..."
echo ""

cd build
./v2v_simulator
