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

    // OPTIMIZATION: Build a lookup map for O(1) access instead of O(n) find_if
    std::unordered_map<int, std::shared_ptr<core::Vehicle>> vehicleLookup;
    vehicleLookup.reserve(vehicleCount);

    // Update vehicle positions
    for (const auto& vehicle : vehicles) {
        if (!vehicle->isActive()) continue;
        
        int id = vehicle->getId();
        QPointF pos = vehicle->getPosition();
        Point2D point(pos.x(), pos.y());
        
        m_vehiclePositions[id] = point;
        m_transmissionRadii[id] = vehicle->getTransmissionRadius();
        vehicleLookup[id] = vehicle;  // O(1) lookup
    }
    
    // Rebuild R-tree with bulk loading for better performance
    rebuildRTree();
    
    // OPTIMIZATION: Pre-compute meters per degree (constant for this region)
    const double metersPerDegree = 111320.0;

    // Find connections using O(1) lookup instead of O(n) find_if
    for (const auto& [id, vehicle] : vehicleLookup) {
        double radius1 = m_transmissionRadii[id]; // in meters

        // Query neighbors with search radius (in degrees, approximate)
        double searchRadiusDegrees = radius1 / metersPerDegree;
        
        auto candidates = queryNeighbors(id, searchRadiusDegrees);
        
        std::unordered_set<int> connectedNeighbors;
        connectedNeighbors.reserve(candidates.size());

        for (int candidateId : candidates) {
            // OPTIMIZATION: O(1) lookup instead of O(n) find_if
            auto candidateIt = vehicleLookup.find(candidateId);
            if (candidateIt == vehicleLookup.end()) continue;

            double radius2 = m_transmissionRadii[candidateId]; // in meters

            // Calculate actual distance in meters using Haversine
            double distMeters = distanceInMeters(id, candidateId);
            
            // Connect if the distance is within BOTH vehicles' radii
            if (distMeters <= radius1 && distMeters <= radius2) {
                connectedNeighbors.insert(candidateId);
            }
        }
        
        m_connections[id] = std::move(connectedNeighbors);
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
