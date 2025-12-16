#pragma once

#include <string>
#include <memory>
#include <vector>
#include <QString>

namespace v2v {

// Forward declaration
namespace network { class RoadGraph; }

namespace data {

/**
 * @brief Types de routes OSM supportées
 */
enum class HighwayType {
    Motorway,       // Autoroute
    Trunk,          // Route nationale
    Primary,        // Route principale
    Secondary,      // Route secondaire
    Tertiary,       // Route tertiaire
    Residential,    // Route résidentielle
    Unclassified,   // Route non classifiée
    Service,        // Route de service
    MotorwayLink,   // Bretelle d'autoroute
    TrunkLink,      // Bretelle de nationale
    PrimaryLink,    // Bretelle de route principale
    SecondaryLink,  // Bretelle de route secondaire
    TertiaryLink,   // Bretelle de route tertiaire
    Unknown         // Type inconnu ou non supporté
};

/**
 * @brief Parser de fichiers OSM (PBF ou XML)
 * 
 * Utilise libosmium pour parsing rapide
 */
class OSMParser {
public:
    OSMParser();
    ~OSMParser();
    
    /**
     * @brief Charger un fichier OSM et construire le graphe routier
     * @param filename Chemin vers fichier .osm.pbf ou .osm
     * @param roadGraph Graphe routier à remplir
     * @return true si succès
     */
    bool loadFile(const std::string& filename, network::RoadGraph* roadGraph);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    
    double calculateDistance(double lat1, double lon1, double lat2, double lon2) const;
    bool generateTestGraph(network::RoadGraph* roadGraph);
    
    /**
     * @brief Convertir un string OSM en HighwayType
     * @param typeStr String du type highway OSM
     * @return HighwayType correspondant
     */
    static HighwayType stringToHighwayType(const QString& typeStr);
    
    /**
     * @brief Vérifier si le type de route est valide pour la simulation
     * @param type Type de route
     * @return true si valide, false sinon
     */
    static bool isValidHighwayType(HighwayType type);
    
    /**
     * @brief Obtenir la vitesse par défaut selon le type de route
     * @param type Type de route
     * @return Vitesse en m/s
     */
    static double getDefaultSpeed(HighwayType type);
    
    /**
     * @brief Convertir un HighwayType en string pour le graphe
     * @param type Type de route
     * @return String représentant le type
     */
    static std::string highwayTypeToString(HighwayType type);
};

} // namespace data
} // namespace v2v
