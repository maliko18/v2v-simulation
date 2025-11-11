#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <QPointF>
#include <vector>
#include <memory>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace v2v {
namespace network {

/**
 * @brief Structure représentant un nœud routier
 */
struct RoadNode {
    int id;
    double latitude;
    double longitude;
    QPointF position;
};

/**
 * @brief Structure représentant une arête (segment de route)
 */
struct RoadEdge {
    double length;        // Mètres
    double speedLimit;    // m/s
    std::string roadType; // "motorway", "primary", "residential", etc.
    std::string name;
};

// Type de graphe Boost
using RoadGraphType = boost::adjacency_list<
    boost::vecS,           // OutEdgeList
    boost::vecS,           // VertexList  
    boost::bidirectionalS, // Directed
    RoadNode,              // VertexProperty
    RoadEdge               // EdgeProperty
>;

using VertexDescriptor = boost::graph_traits<RoadGraphType>::vertex_descriptor;
using EdgeDescriptor = boost::graph_traits<RoadGraphType>::edge_descriptor;

/**
 * @brief Graphe routier pour navigation des véhicules
 * 
 * Construit à partir de données OSM
 * Utilise Boost.Graph pour les algorithmes (Dijkstra, A*)
 */
class RoadGraph {
public:
    RoadGraph();
    ~RoadGraph() = default;

    // Construction du graphe
    void clear();
    
    // Ajout de nœuds/arêtes
    VertexDescriptor addNode(double lat, double lon);
    void addEdge(VertexDescriptor from, VertexDescriptor to, 
                 double length, double speedLimit, const std::string& roadType);
    
    // Requêtes
    VertexDescriptor getNearestNode(double lat, double lon) const;
    
    // Statistiques
    size_t getNodeCount() const;
    size_t getEdgeCount() const;
    
    // Accès au graphe
    const RoadGraphType& getGraph() const { return m_graph; }
    RoadGraphType& getGraph() { return m_graph; }
    
    // Index spatial
    void buildSpatialIndex();
    
private:
    RoadGraphType m_graph;
    
    // Spatial index pour recherche rapide
    struct SpatialNode {
        VertexDescriptor vertex;
        double lat, lon;
    };
    std::vector<SpatialNode> m_spatialIndex;
    
    double calculateDistance(double lat1, double lon1, double lat2, double lon2) const;
};

} // namespace network
} // namespace v2v
