#!/bin/bash
# Script pour télécharger des données OSM d'une zone personnalisée
# Usage: ./download_osm.sh <nom_fichier> <lat_min> <lon_min> <lat_max> <lon_max> [route_type]

set -e

# Couleurs pour output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Fonction d'aide
show_help() {
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${NC}        ${GREEN}V2V Simulator - OSM Data Downloader${NC}                    ${BLUE}║${NC}"
    echo -e "${BLUE}╚══════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${YELLOW}Usage:${NC}"
    echo "  $0 <nom_fichier> <lat_min> <lon_min> <lat_max> <lon_max> [route_type]"
    echo ""
    echo -e "${YELLOW}Arguments:${NC}"
    echo "  nom_fichier  : Nom du fichier de sortie (sans extension .osm)"
    echo "  lat_min      : Latitude minimale (sud)"
    echo "  lon_min      : Longitude minimale (ouest)"
    echo "  lat_max      : Latitude maximale (nord)"
    echo "  lon_max      : Longitude maximale (est)"
    echo "  route_type   : Type de routes à inclure (optionnel)"
    echo ""
    echo -e "${YELLOW}Types de routes disponibles:${NC}"
    echo "  all          : Toutes les routes (défaut)"
    echo "  main         : Routes principales (autoroutes, nationales, départementales)"
    echo "  motorway     : Autoroutes uniquement"
    echo "  urban        : Routes urbaines (rues en ville)"
    echo ""
    echo -e "${YELLOW}Exemples:${NC}"
    echo ""
    echo -e "${GREEN}# Toute l'Alsace avec toutes les routes:${NC}"
    echo "  $0 alsace 47.4 7.0 49.1 8.3"
    echo ""
    echo -e "${GREEN}# Strasbourg et environs (routes principales):${NC}"
    echo "  $0 strasbourg 48.5 7.6 48.7 7.9 main"
    echo ""
    echo -e "${GREEN}# Paris intra-muros (toutes routes):${NC}"
    echo "  $0 paris 48.815 2.225 48.902 2.469"
    echo ""
    echo -e "${GREEN}# Région Grand Est (routes principales seulement):${NC}"
    echo "  $0 grand_est 47.4 4.8 49.6 8.3 main"
    echo ""
    echo -e "${YELLOW}Zones prédéfinies:${NC}"
    echo "  alsace       : 47.4, 7.0, 49.1, 8.3"
    echo "  mulhouse     : 47.7, 7.28, 47.8, 7.38"
    echo "  strasbourg   : 48.52, 7.68, 48.62, 7.82"
    echo "  colmar       : 48.04, 7.32, 48.12, 7.40"
    echo ""
}

# Vérifier les arguments
if [ "$#" -lt 5 ] && [ "$1" != "--help" ] && [ "$1" != "-h" ]; then
    show_help
    exit 1
fi

if [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
    show_help
    exit 0
fi

# Parser les arguments
FILENAME="$1"
LAT_MIN="$2"
LON_MIN="$3"
LAT_MAX="$4"
LON_MAX="$5"
ROUTE_TYPE="${6:-all}"

# Construire le filtre de routes selon le type
case "$ROUTE_TYPE" in
    all)
        HIGHWAY_FILTER="way[highway]"
        TYPE_DESC="toutes routes"
        ;;
    main)
        HIGHWAY_FILTER="way[highway~\"^(motorway|trunk|primary|secondary)$\"]"
        TYPE_DESC="routes principales"
        ;;
    motorway)
        HIGHWAY_FILTER="way[highway~\"^(motorway|motorway_link)$\"]"
        TYPE_DESC="autoroutes"
        ;;
    urban)
        HIGHWAY_FILTER="way[highway~\"^(residential|tertiary|unclassified|service)$\"]"
        TYPE_DESC="routes urbaines"
        ;;
    *)
        echo -e "${RED}✗ Type de route invalide: $ROUTE_TYPE${NC}"
        echo "Types valides: all, main, motorway, urban"
        exit 1
        ;;
esac

# Construire l'URL Overpass API
BBOX="[bbox:$LAT_MIN,$LON_MIN,$LAT_MAX,$LON_MAX]"
QUERY="$BBOX;($HIGHWAY_FILTER;node(w););out body;"
URL="https://overpass-api.de/api/interpreter?data=$QUERY"

OUTPUT_FILE="$FILENAME.osm"

echo -e "${BLUE}╔══════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║${NC}  ${GREEN}Téléchargement des données OSM${NC}                              ${BLUE}║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${YELLOW}Configuration:${NC}"
echo -e "  Fichier      : ${GREEN}$OUTPUT_FILE${NC}"
echo -e "  Zone         : ${GREEN}($LAT_MIN, $LON_MIN) → ($LAT_MAX, $LON_MAX)${NC}"
echo -e "  Type routes  : ${GREEN}$TYPE_DESC${NC}"
echo ""
echo -e "${YELLOW}Bounding Box:${NC}"
echo -e "  Nord  : ${GREEN}$LAT_MAX°${NC}"
echo -e "  Sud   : ${GREEN}$LAT_MIN°${NC}"
echo -e "  Est   : ${GREEN}$LON_MAX°${NC}"
echo -e "  Ouest : ${GREEN}$LON_MIN°${NC}"
echo ""
echo -e "${BLUE}Téléchargement en cours...${NC}"
echo ""

# Télécharger avec wget et barre de progression
if wget -O "$OUTPUT_FILE" "$URL" 2>&1 | grep --line-buffered -oP '\d+%|\d+[KMG]'; then
    echo ""
    FILE_SIZE=$(du -h "$OUTPUT_FILE" | cut -f1)
    echo -e "${GREEN}✓ Téléchargement réussi !${NC}"
    echo -e "  Fichier : ${GREEN}$OUTPUT_FILE${NC}"
    echo -e "  Taille  : ${GREEN}$FILE_SIZE${NC}"
    echo ""
    
    # Compter les nœuds et les ways (approximatif)
    NODE_COUNT=$(grep -c '<node ' "$OUTPUT_FILE" || echo "?")
    WAY_COUNT=$(grep -c '<way ' "$OUTPUT_FILE" || echo "?")
    
    echo -e "${YELLOW}Statistiques approximatives:${NC}"
    echo -e "  Nœuds : ${GREEN}~$NODE_COUNT${NC}"
    echo -e "  Ways  : ${GREEN}~$WAY_COUNT${NC}"
    echo ""
    echo -e "${BLUE}Le fichier est prêt à être utilisé dans V2V Simulator !${NC}"
    echo -e "${YELLOW}Chargez-le via: ${NC}${GREEN}Load OSM → $OUTPUT_FILE${NC}"
    echo ""
else
    echo ""
    echo -e "${RED}✗ Erreur lors du téléchargement${NC}"
    exit 1
fi
