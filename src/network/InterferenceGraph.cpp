#include "network/InterferenceGraph.hpp"
#include "core/Vehicle.hpp"
#include "utils/Logger.hpp"
#include "utils/Profiler.hpp"

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
    for (const auto& vehicle : vehicles) {
        if (!vehicle->isActive()) continue;
        
        int id = vehicle->getId();
        double radius = vehicle->getTransmissionRadius();
        
        auto neighbors = queryNeighbors(id, radius);
        m_connections[id] = std::unordered_set<int>(neighbors.begin(), neighbors.end());
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

} // namespace network
} // namespace v2v
