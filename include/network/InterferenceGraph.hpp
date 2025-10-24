#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <vector>
#include <memory>

namespace v2v {

// Forward declaration
namespace core { class Vehicle; }

namespace network {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

// Point 2D pour R-tree
using Point2D = bg::model::point<double, 2, bg::cs::cartesian>;
using Box = bg::model::box<Point2D>;
using RTreeValue = std::pair<Point2D, int>; // (position, vehicleId)
using RTree = bgi::rtree<RTreeValue, bgi::quadratic<16>>;

/**
 * @brief Graphe d'interférences V2V (véhicule à véhicule)
 * 
 * Caractéristiques:
 * - Graphe dynamique (change à chaque frame)
 * - Deux véhicules sont connectés si leurs zones de transmission se chevauchent
 * - Utilise un R-tree pour recherche spatiale efficace O(log n)
 * - Update incrémental pour éviter reconstruction complète
 */
class InterferenceGraph {
public:
    InterferenceGraph();
    ~InterferenceGraph() = default;

    /**
     * @brief Update complet du graphe (appelé chaque frame ou tous les N frames)
     * @param vehicles Liste des véhicules actifs
     */
    void update(const std::vector<std::shared_ptr<core::Vehicle>>& vehicles);
    
    /**
     * @brief Update incrémental - seulement les véhicules qui ont bougé
     * @param movedVehicles Véhicules dont la position a changé
     */
    void incrementalUpdate(const std::vector<std::shared_ptr<core::Vehicle>>& movedVehicles);
    
    /**
     * @brief Obtenir les voisins d'un véhicule (dans rayon de transmission)
     * @param vehicleId ID du véhicule
     * @return IDs des véhicules connectés
     */
    std::vector<int> getNeighbors(int vehicleId) const;
    
    /**
     * @brief Vérifie si deux véhicules sont connectés
     */
    bool areConnected(int vehicleId1, int vehicleId2) const;
    
    /**
     * @brief Obtenir toutes les connexions actives (pour affichage)
     * @return Paires de (vehicleId1, vehicleId2)
     */
    std::vector<std::pair<int, int>> getAllConnections() const;
    
    /**
     * @brief Statistiques
     */
    size_t getConnectionCount() const { return m_connections.size(); }
    size_t getVehicleCount() const { return m_vehiclePositions.size(); }
    double getAverageConnections() const;
    
    /**
     * @brief Clear
     */
    void clear();

private:
    // R-tree pour recherche spatiale efficace
    std::unique_ptr<RTree> m_rtree;
    
    // Connexions actives: map[vehicleId] -> set<connectedVehicleIds>
    std::unordered_map<int, std::unordered_set<int>> m_connections;
    
    // Positions des véhicules (cache)
    std::unordered_map<int, Point2D> m_vehiclePositions;
    
    // Rayon de transmission par véhicule
    std::unordered_map<int, double> m_transmissionRadii;
    
    /**
     * @brief Reconstruire le R-tree à partir des positions actuelles
     */
    void rebuildRTree();
    
    /**
     * @brief Trouver les voisins d'un véhicule dans le R-tree
     */
    std::vector<int> queryNeighbors(int vehicleId, double radius) const;
    
    /**
     * @brief Calculer distance entre deux véhicules
     */
    double distance(int vehicleId1, int vehicleId2) const;
};

} // namespace network
} // namespace v2v
