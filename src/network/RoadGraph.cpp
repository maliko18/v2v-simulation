#include "network/RoadGraph.hpp"
#include "utils/Logger.hpp"
#include <limits>
#include <cmath>

namespace v2v {
namespace network {

RoadGraph::RoadGraph() {
    LOG_INFO("RoadGraph created");
}

void RoadGraph::clear() {
    m_graph.clear();
    m_spatialIndex.clear();
}

VertexDescriptor RoadGraph::addNode(double lat, double lon) {
    RoadNode node;
    node.id = boost::num_vertices(m_graph);
    node.latitude = lat;
    node.longitude = lon;
    node.position = QPointF(lon, lat);
    
    return boost::add_vertex(node, m_graph);
}

void RoadGraph::addEdge(VertexDescriptor from, VertexDescriptor to,
                        double length, double speedLimit, const std::string& roadType) {
    RoadEdge edge;
    edge.length = length;
    edge.speedLimit = speedLimit;
    edge.roadType = roadType;
    
    boost::add_edge(from, to, edge, m_graph);
}

VertexDescriptor RoadGraph::getNearestNode(double lat, double lon) const {
    if (m_spatialIndex.empty()) {
        LOG_WARNING("Spatial index is empty");
        return VertexDescriptor();
    }
    
    // Recherche du nœud le plus proche avec early exit
    double minDist = std::numeric_limits<double>::max();
    VertexDescriptor nearest;
    
    // Limite de recherche : si on trouve un nœud à moins de 50m, on arrête
    const double earlyExitThreshold = 0.0005; // ~50m en degrés
    
    for (const auto& spatialNode : m_spatialIndex) {
        // Calcul rapide de distance Manhattan avant Haversine complète
        double dLat = std::abs(lat - spatialNode.lat);
        double dLon = std::abs(lon - spatialNode.lon);
        
        // Skip si clairement trop loin (> 1° = ~111km)
        if (dLat > 1.0 || dLon > 1.0) continue;
        
        double dist = calculateDistance(lat, lon, spatialNode.lat, spatialNode.lon);
        if (dist < minDist) {
            minDist = dist;
            nearest = spatialNode.vertex;
            
            // Early exit si très proche
            if (minDist < earlyExitThreshold) {
                break;
            }
        }
    }
    
    return nearest;
}

size_t RoadGraph::getNodeCount() const {
    return boost::num_vertices(m_graph);
}

size_t RoadGraph::getEdgeCount() const {
    return boost::num_edges(m_graph);
}

void RoadGraph::buildSpatialIndex() {
    LOG_INFO("Building spatial index...");
    m_spatialIndex.clear();
    
    // Parcourir tous les vertices
    auto vertices = boost::vertices(m_graph);
    for (auto it = vertices.first; it != vertices.second; ++it) {
        const RoadNode& node = m_graph[*it];
        
        SpatialNode spatialNode;
        spatialNode.vertex = *it;
        spatialNode.lat = node.latitude;
        spatialNode.lon = node.longitude;
        
        m_spatialIndex.push_back(spatialNode);
    }
    
    LOG_INFO(QString("Spatial index built with %1 nodes").arg(m_spatialIndex.size()));
}

double RoadGraph::calculateDistance(double lat1, double lon1, double lat2, double lon2) const {
    // Formule de Haversine
    const double R = 6371000.0; // Rayon Terre en mètres
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = std::sin(dLat/2) * std::sin(dLat/2) +
               std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) *
               std::sin(dLon/2) * std::sin(dLon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return R * c;
}

} // namespace network
} // namespace v2v
