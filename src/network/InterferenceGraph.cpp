#include "network/InterferenceGraph.hpp"
#include "core/Vehicle.hpp"
#include "data/GeometryUtils.hpp"
#include "utils/Logger.hpp"
#include <algorithm>
#include <chrono>

namespace v2v {
namespace network {

InterferenceGraph::InterferenceGraph()
    : m_rtree(std::make_unique<RTree>())
{
    LOG_INFO("InterferenceGraph created");
}

void InterferenceGraph::update(const std::vector<std::shared_ptr<core::Vehicle>>& vehicles) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Reserve capacity to avoid rehashing
    const size_t vehicleCount = vehicles.size();
    m_connections.clear();
    m_connections.reserve(vehicleCount);
    m_vehiclePositions.clear();
    m_vehiclePositions.reserve(vehicleCount);
    m_transmissionRadii.clear();
    m_transmissionRadii.reserve(vehicleCount);

    // Build list of active vehicles with their data
    struct VehicleData {
        int id;
        double lat;
        double lon;
        double radius;  // in meters
    };
    std::vector<VehicleData> activeVehicles;
    activeVehicles.reserve(vehicleCount);

    // Update vehicle positions
    for (const auto& vehicle : vehicles) {
        if (!vehicle->isActive()) continue;
        
        int id = vehicle->getId();
        double lat = vehicle->getLatitude();
        double lon = vehicle->getLongitude();
        double radius = static_cast<double>(vehicle->getTransmissionRadius());
        
        Point2D point(lon, lat);  // x=lon, y=lat
        m_vehiclePositions[id] = point;
        m_transmissionRadii[id] = vehicle->getTransmissionRadius();
        
        activeVehicles.push_back({id, lat, lon, radius});
    }
    
    // Rebuild R-tree
    rebuildRTree();
    
    // Constante pour convertir degrés en mètres
    const double metersPerDegree = 111320.0;
    
    // Find connections - LOGIQUE ORIGINALE
    // Connexion quand un véhicule entre dans le rayon de l'autre
    // (distance <= rayon1 ET distance <= rayon2)
    
    for (size_t i = 0; i < activeVehicles.size(); ++i) {
        const auto& v1 = activeVehicles[i];
        std::unordered_set<int> connectedNeighbors;
        
        for (size_t j = 0; j < activeVehicles.size(); ++j) {
            if (i == j) continue;
            
            const auto& v2 = activeVehicles[j];
            
            // Calcul de distance
            double dLat = v2.lat - v1.lat;
            double dLon = v2.lon - v1.lon;
            
            // Convertir en mètres
            double dLatMeters = dLat * metersPerDegree;
            double dLonMeters = dLon * metersPerDegree;
            
            // Distance euclidienne en mètres
            double distMeters = std::sqrt(dLatMeters * dLatMeters + dLonMeters * dLonMeters);
            
            // CONNEXION si un véhicule est dans le rayon de l'autre
            
            double maxRadius = std::max(v1.radius, v2.radius);
            if (distMeters <= maxRadius) {
                connectedNeighbors.insert(v2.id);
            }
        }
        
        m_connections[v1.id] = std::move(connectedNeighbors);
    }

    // Performance logging
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    if (duration.count() > 50) {  // Log if update takes > 50ms
        LOG_WARNING(QString("InterferenceGraph::update took %1 ms for %2 vehicles")
                   .arg(duration.count())
                   .arg(vehicleCount));
    }
}

void InterferenceGraph::incrementalUpdate(const std::vector<std::shared_ptr<core::Vehicle>>& movedVehicles) {
    // TODO: Implement incremental update
    // For now, just call full update
    update(movedVehicles);
}

std::vector<int> InterferenceGraph::getNeighbors(int vehicleId) const {
    auto it = m_connections.find(vehicleId);
    if (it != m_connections.end()) {
        return std::vector<int>(it->second.begin(), it->second.end());
    }
    return std::vector<int>();
}

bool InterferenceGraph::areConnected(int vehicleId1, int vehicleId2) const {
    auto it = m_connections.find(vehicleId1);
    if (it != m_connections.end()) {
        return it->second.count(vehicleId2) > 0;
    }
    return false;
}

std::vector<std::pair<int, int>> InterferenceGraph::getAllConnections() const {
    std::vector<std::pair<int, int>> result;
    
    for (const auto& [id, neighbors] : m_connections) {
        for (int neighbor : neighbors) {
            if (id < neighbor) { // Avoid duplicates
                result.emplace_back(id, neighbor);
            }
        }
    }
    
    return result;
}

double InterferenceGraph::getAverageConnections() const {
    if (m_connections.empty()) return 0.0;
    
    size_t total = 0;
    for (const auto& [id, neighbors] : m_connections) {
        total += neighbors.size();
    }
    
    return static_cast<double>(total) / m_connections.size();
}

void InterferenceGraph::clear() {
    m_connections.clear();
    m_vehiclePositions.clear();
    m_transmissionRadii.clear();
    m_rtree = std::make_unique<RTree>();
}

void InterferenceGraph::rebuildRTree() {
    m_rtree = std::make_unique<RTree>();
    
    for (const auto& [id, pos] : m_vehiclePositions) {
        m_rtree->insert(std::make_pair(pos, id));
    }
}

std::vector<int> InterferenceGraph::queryNeighbors(int vehicleId, double radiusDegrees) const {
    auto posIt = m_vehiclePositions.find(vehicleId);
    if (posIt == m_vehiclePositions.end()) {
        return std::vector<int>();
    }
    
    const Point2D& center = posIt->second;
    Box queryBox(
        Point2D(center.get<0>() - radiusDegrees, center.get<1>() - radiusDegrees),
        Point2D(center.get<0>() + radiusDegrees, center.get<1>() + radiusDegrees)
    );
    
    std::vector<RTreeValue> results;
    m_rtree->query(bgi::intersects(queryBox), std::back_inserter(results));
    
    // Retourner tous les candidats sans pré-filtrage
    // Le filtrage précis sera fait avec distanceInMeters() dans update()
    std::vector<int> neighbors;
    neighbors.reserve(results.size());
    for (const auto& [point, id] : results) {
        if (id != vehicleId) {
            neighbors.push_back(id);
        }
    }
    
    return neighbors;
}

double InterferenceGraph::distance(int vehicleId1, int vehicleId2) const {
    auto pos1 = m_vehiclePositions.find(vehicleId1);
    auto pos2 = m_vehiclePositions.find(vehicleId2);
    
    if (pos1 == m_vehiclePositions.end() || pos2 == m_vehiclePositions.end()) {
        return std::numeric_limits<double>::max();
    }
    
    return bg::distance(pos1->second, pos2->second);
}

double InterferenceGraph::distanceInMeters(int vehicleId1, int vehicleId2) const {
    auto pos1 = m_vehiclePositions.find(vehicleId1);
    auto pos2 = m_vehiclePositions.find(vehicleId2);
    
    if (pos1 == m_vehiclePositions.end() || pos2 == m_vehiclePositions.end()) {
        return std::numeric_limits<double>::max();
    }
    
    // Convert Point2D (x=lon, y=lat) to lat/lon for Haversine
    double lat1 = pos1->second.get<1>();
    double lon1 = pos1->second.get<0>();
    double lat2 = pos2->second.get<1>();
    double lon2 = pos2->second.get<0>();
    
    // Use Haversine distance for accurate meters calculation
    return data::GeometryUtils::haversineDistance(lat1, lon1, lat2, lon2);
}

} // namespace network
} // namespace v2v
