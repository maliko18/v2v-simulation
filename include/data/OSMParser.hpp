#pragma once

#include <string>
#include <memory>
#include <vector>

namespace v2v {

// Forward declaration
namespace network { class RoadGraph; }

namespace data {

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
    
    /**
     * @brief Filtrer par bounding box
     */
    void setBoundingBox(double minLat, double minLon, double maxLat, double maxLon);
    
    /**
     * @brief Types de routes à inclure
     */
    void setRoadTypes(const std::vector<std::string>& types);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    
    double calculateDistance(double lat1, double lon1, double lat2, double lon2) const;
    bool generateTestGraph(network::RoadGraph* roadGraph);
};

} // namespace data
} // namespace v2v
