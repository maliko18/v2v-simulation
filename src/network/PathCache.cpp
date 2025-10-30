#include "network/PathCache.hpp"
#include "network/RoadGraph.hpp"
#include "network/PathPlanner.hpp"
#include "utils/Logger.hpp"
#include <QFile>
#include <QDataStream>
#include <random>

namespace v2v {
namespace network {

PathCache::PathCache()
    : m_currentIndex(0)
{
    LOG_INFO("PathCache created");
}

PathCache::~PathCache() {
    LOG_INFO("PathCache destroyed");
}

void PathCache::generatePaths(RoadGraph* roadGraph, PathPlanner* pathPlanner, int numPaths) {
    if (!roadGraph || !pathPlanner) {
        LOG_WARNING("Cannot generate paths: null roadGraph or pathPlanner");
        return;
    }
    
    clear();
    
    const auto& graph = roadGraph->getGraph();
    size_t nodeCount = boost::num_vertices(graph);
    
    if (nodeCount < 2) {
        LOG_WARNING("Cannot generate paths: not enough nodes");
        return;
    }
    
    LOG_INFO(QString("Generating %1 cached paths...").arg(numPaths));
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> node_dist(0, nodeCount - 1);
    
    int successCount = 0;
    int attempts = 0;
    const int maxAttempts = numPaths * 3;  // Essayer jusqu'à 3x plus
    
    while (successCount < numPaths && attempts < maxAttempts) {
        attempts++;
        
        // Choisir deux nœuds aléatoires
        auto startVertex = boost::vertex(node_dist(gen), graph);
        auto endVertex = boost::vertex(node_dist(gen), graph);
        
        if (startVertex == endVertex) continue;
        
        const auto& startNode = graph[startVertex];
        const auto& endNode = graph[endVertex];
        
        QPointF start(startNode.longitude, startNode.latitude);
        QPointF end(endNode.longitude, endNode.latitude);
        
        // Calculer le chemin
        auto path = pathPlanner->findPath(start, end);
        
        // Filtrer les chemins trop courts (< 500m environ)
        if (path.size() >= 5) {
            m_paths.push_back(path);
            successCount++;
            
            if (successCount % 10 == 0) {
                LOG_INFO(QString("Generated %1/%2 paths...").arg(successCount).arg(numPaths));
            }
        }
    }
    
    LOG_INFO(QString("Path cache ready: %1 paths generated (success rate: %2%)")
             .arg(successCount)
             .arg(successCount * 100 / attempts));
}

std::vector<QPointF> PathCache::getPath() {
    if (m_paths.empty()) {
        LOG_WARNING("PathCache is empty, returning empty path");
        return {};
    }
    
    // Rotation circulaire pour distribuer les chemins
    std::vector<QPointF> path = m_paths[m_currentIndex];
    m_currentIndex = (m_currentIndex + 1) % m_paths.size();
    
    return path;
}

void PathCache::clear() {
    m_paths.clear();
    m_currentIndex = 0;
}

bool PathCache::saveToFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Cannot save path cache to %1").arg(filename));
        return false;
    }
    
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);
    
    // Header
    out << quint32(0xCAFEBABE);  // Magic number
    out << quint32(1);           // Version
    out << quint32(m_paths.size());
    
    // Paths
    for (const auto& path : m_paths) {
        out << quint32(path.size());
        for (const auto& point : path) {
            out << point.x() << point.y();
        }
    }
    
    file.close();
    LOG_INFO(QString("Saved %1 paths to %2").arg(m_paths.size()).arg(filename));
    return true;
}

bool PathCache::loadFromFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARNING(QString("Cannot load path cache from %1").arg(filename));
        return false;
    }
    
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);
    
    clear();
    
    // Header
    quint32 magic, version, pathCount;
    in >> magic >> version >> pathCount;
    
    if (magic != 0xCAFEBABE || version != 1) {
        LOG_ERROR("Invalid path cache file format");
        return false;
    }
    
    // Paths
    for (quint32 i = 0; i < pathCount; ++i) {
        quint32 pointCount;
        in >> pointCount;
        
        std::vector<QPointF> path;
        path.reserve(pointCount);
        
        for (quint32 j = 0; j < pointCount; ++j) {
            double x, y;
            in >> x >> y;
            path.emplace_back(x, y);
        }
        
        m_paths.push_back(path);
    }
    
    file.close();
    LOG_INFO(QString("Loaded %1 paths from %2").arg(m_paths.size()).arg(filename));
    return true;
}

} // namespace network
} // namespace v2v
