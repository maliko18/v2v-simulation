#!/bin/bash

# Script pour télécharger les données OSM de Mulhouse
# Usage: ./download_osm.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_DIR="$SCRIPT_DIR/../data/osm"

mkdir -p "$DATA_DIR"

echo "========================================="
echo "Téléchargement données OSM - Mulhouse"
echo "========================================="

# URL Geofabrik Alsace
ALSACE_URL="http://download.geofabrik.de/europe/france/alsace-latest.osm.pbf"
ALSACE_FILE="$DATA_DIR/alsace-latest.osm.pbf"

# Télécharger l'extract Alsace
if [ ! -f "$ALSACE_FILE" ]; then
    echo "Téléchargement de l'extract Alsace (~25 MB)..."
    wget -O "$ALSACE_FILE" "$ALSACE_URL"
    echo "✓ Téléchargement terminé"
else
    echo "✓ Fichier Alsace déjà présent"
fi

# Extraire la zone de Mulhouse avec osmium
MULHOUSE_FILE="$DATA_DIR/mulhouse.osm.pbf"

if command -v osmium &> /dev/null; then
    echo "Extraction de la zone Mulhouse..."
    osmium extract \
        --bbox 7.30,47.72,7.38,47.78 \
        "$ALSACE_FILE" \
        -o "$MULHOUSE_FILE" \
        --overwrite
    
    echo "✓ Extract Mulhouse créé: $MULHOUSE_FILE"
    
    # Statistiques
    echo ""
    echo "Statistiques du fichier:"
    osmium fileinfo "$MULHOUSE_FILE" | grep -E "nodes|ways"
else
    echo "⚠ osmium-tool n'est pas installé"
    echo "  Vous pouvez l'installer avec: sudo apt install osmium-tool"
    echo "  Pour l'instant, utilisez le fichier Alsace complet"
fi

echo ""
echo "========================================="
echo "✓ Téléchargement terminé"
echo "========================================="
echo "Fichiers disponibles dans: $DATA_DIR"
ls -lh "$DATA_DIR"
