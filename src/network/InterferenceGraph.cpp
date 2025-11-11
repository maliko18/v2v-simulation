#include "network/InterferenceGraph.hpp"
#include "core/Vehicle.hpp"
#include "data/GeometryUtils.hpp"
#include "utils/Logger.hpp"
#include "utils/Profiler.hpp"
#include <algorithm>

namespace v2v {
namespace network {

InterferenceGraph::InterferenceGraph()
    : m_rtree(std::make_unique<RTree>())
{
    LOG_INFO("InterferenceGraph created");
}

void InterferenceGraph::update(const std::vector<std::shared_ptr<core::Vehicle>>& vehicles) {
    PROFILE_FUNCTION();
    
    // Clear previous state
    m_connections.clear();
    m_vehiclePositions.clear();
    m_transmissionRadii.clear();
    
    // Update vehicle positions
    for (const auto& vehicle : vehicles) {
        if (!vehicle->isActive()) continue;
        
        int id = vehicle->getId();
        QPointF pos = vehicle->getPosition();
        Point2D point(pos.x(), pos.y());
        
        m_vehiclePositions[id] = point;
        m_transmissionRadii[id] = vehicle->getTransmissionRadius();
    }
    
    // Rebuild R-tree
    rebuildRTree();
    
    // Find connections
    // Two vehicles connect if one vehicle's position is inside the other's transmission radius
    // This means: distance <= min(radius1, radius2) OR we check both directions
    // For V2V: both vehicles must be able to reach each other, so distance <= min(radius1, radius2)
    for (const auto& vehicle : vehicles) {
        if (!vehicle->isActive()) continue;
        
        int id = vehicle->getId();
        double radius1 = vehicle->getTransmissionRadius(); // in meters
        
        // Query neighbors with search radius (in degrees, approximate)
        // Convert meters to degrees for search: 1 degree ≈ 111320 meters
        const double metersPerDegree = 111320.0;
        double searchRadiusDegrees = radius1 / metersPerDegree;
        
        auto candidates = queryNeighbors(id, searchRadiusDegrees);
        
        std::unordered_set<int> connectedNeighbors;
        for (int candidateId : candidates) {
            // Find the candidate vehicle to get its radius
            auto candidateIt = std::find_if(vehicles.begin(), vehicles.end(),
                [candidateId](const auto& v) { return v->getId() == candidateId && v->isActive(); });
            
            if (candidateIt == vehicles.end()) continue;
            
            const auto& candidateVehicle = *candidateIt;
            double radius2 = candidateVehicle->getTransmissionRadius(); // in meters
            
            // Calculate actual distance in meters using Haversine
            double distMeters = distanceInMeters(id, candidateId);
            
            // Connect if the distance is within BOTH vehicles' radii
            // This means both vehicles can reach each other (bidirectional communication)
            // Vehicle B is inside Vehicle A's radius AND Vehicle A is inside Vehicle B's radius
            if (distMeters <= radius1 && distMeters <= radius2) {
                connectedNeighbors.insert(candidateId);
            }
        }
        
        m_connections[id] = connectedNeighbors;
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

std::vector<int> InterferenceGraph::queryNeighbors(int vehicleId, double radius) const {
    auto posIt = m_vehiclePositions.find(vehicleId);
    if (posIt == m_vehiclePositions.end()) {
        return std::vector<int>();
    }
    
    const Point2D& center = posIt->second;
    Box queryBox(
        Point2D(center.get<0>() - radius, center.get<1>() - radius),
        Point2D(center.get<0>() + radius, center.get<1>() + radius)
    );
    
    std::vector<RTreeValue> results;
    m_rtree->query(bgi::intersects(queryBox), std::back_inserter(results));
    
    std::vector<int> neighbors;
    for (const auto& [point, id] : results) {
        if (id != vehicleId && distance(vehicleId, id) <= radius) {
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
