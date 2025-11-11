#include "network/PathPlanner.hpp"
#include "utils/Logger.hpp"
#include <boost/graph/astar_search.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <random>
#include <queue>
#include <limits>
#include <cmath>

namespace v2v {
namespace network {

// Heuristique pour A* (distance de Haversine)
class AStarHeuristic : public boost::astar_heuristic<RoadGraphType, double> {
public:
    AStarHeuristic(const RoadGraphType& graph, VertexDescriptor goal)
        : m_graph(graph), m_goal(goal) {}
    
    double operator()(VertexDescriptor v) {
        const auto& nodeV = m_graph[v];
        const auto& nodeGoal = m_graph[m_goal];
        
        // Distance de Haversine
        const double R = 6371000.0; // Rayon de la Terre en mètres
        const double lat1 = nodeV.latitude * M_PI / 180.0;
        const double lat2 = nodeGoal.latitude * M_PI / 180.0;
        const double dLat = (nodeGoal.latitude - nodeV.latitude) * M_PI / 180.0;
        const double dLon = (nodeGoal.longitude - nodeV.longitude) * M_PI / 180.0;
        
        const double a = std::sin(dLat/2) * std::sin(dLat/2) +
                        std::cos(lat1) * std::cos(lat2) *
                        std::sin(dLon/2) * std::sin(dLon/2);
        const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
        
        return R * c;
    }
    
private:
    const RoadGraphType& m_graph;
    VertexDescriptor m_goal;
};

// Visiteur pour détecter la fin de A*
struct FoundGoal {
    bool timedOut;
    FoundGoal(bool timeout = false) : timedOut(timeout) {}
};

class AStarGoalVisitor : public boost::default_astar_visitor {
public:
    AStarGoalVisitor(VertexDescriptor goal, int maxIterations = 5000) 
        : m_goal(goal), m_maxIterations(maxIterations), m_iterations(0) {}
    
    void examine_vertex(VertexDescriptor v, const RoadGraphType&) {
        // Vérifier d'abord si on a atteint le but
        if (v == m_goal) {
            throw FoundGoal(false);  // Succès !
        }
        
        // Limiter le nombre d'itérations pour éviter les freezes
        m_iterations++;
        if (m_iterations > m_maxIterations) {
            throw FoundGoal(true);  // Timeout - abandonner
        }
    }
    
private:
    VertexDescriptor m_goal;
    int m_maxIterations;
    mutable int m_iterations;
};

PathPlanner::PathPlanner(RoadGraph* roadGraph)
    : m_roadGraph(roadGraph)
{
}

std::vector<QPointF> PathPlanner::findPath(const QPointF& start, const QPointF& end) {
    if (!m_roadGraph) {
        utils::Logger::instance().warning("[PathPlanner] Graphe routier non initialisé");
        return {};
    }
    
    const auto& graph = m_roadGraph->getGraph();
    
    // Log désactivé pour performances (génère trop de logs avec beaucoup de véhicules)
    // utils::Logger::instance().info(QString("[PathPlanner] Finding path from (%1, %2) to (%3, %4)")
    //                                .arg(start.y(), 0, 'f', 6).arg(start.x(), 0, 'f', 6)
    //                                .arg(end.y(), 0, 'f', 6).arg(end.x(), 0, 'f', 6));
    
    // Trouver les nœuds les plus proches du départ et de l'arrivée
    // Note: QPointF a x=longitude, y=latitude, mais getNearestNode attend (lat, lon)
    auto startVertex = m_roadGraph->getNearestNode(start.y(), start.x());
    auto endVertex = m_roadGraph->getNearestNode(end.y(), end.x());
    
    // Log désactivé pour performances
    // utils::Logger::instance().info(QString("[PathPlanner] Start vertex: %1, End vertex: %2")
    //                                .arg(startVertex).arg(endVertex));
    
    if (startVertex == endVertex) {
        utils::Logger::instance().warning("[PathPlanner] Start and end vertices are the same");
        return {start, end};
    }
    
    // Préparation pour A*
    std::vector<VertexDescriptor> predecessors(boost::num_vertices(graph));
    std::vector<double> distances(boost::num_vertices(graph), std::numeric_limits<double>::max());
    
    // Timeout adaptatif : limiter les itérations pour éviter les freezes
    // Pour Mulhouse (~2300 nœuds) : 10000 itérations = suffisant pour la plupart des chemins
    int maxIterations = std::min(10000, static_cast<int>(boost::num_vertices(graph) * 5));
    
    try {
        // Exécution de A* avec timeout
        AStarHeuristic heuristic(graph, endVertex);
        AStarGoalVisitor visitor(endVertex, maxIterations);
        
        boost::astar_search(
            graph,
            startVertex,
            heuristic,
            boost::predecessor_map(&predecessors[0])
                .distance_map(&distances[0])
                .visitor(visitor)
                .weight_map(boost::get(&RoadEdge::length, graph))
        );
        
    } catch (const FoundGoal& fg) {
        // Vérifier si on a atteint le but ou si c'est un timeout
        if (fg.timedOut) {
            utils::Logger::instance().warning("[PathPlanner] A* timeout - chemin abandonné");
            return {};  // Retourner un chemin vide en cas de timeout
        }
        
        // Chemin trouvé ! Reconstruire le chemin
        std::vector<VertexDescriptor> path;
        VertexDescriptor current = endVertex;
        
        while (current != startVertex) {
            path.push_back(current);
            current = predecessors[current];
        }
        path.push_back(startVertex);
        
        std::reverse(path.begin(), path.end());
        
        // Convertir en QPointF
        std::vector<QPointF> result;
        result.push_back(start); // Point de départ exact
        
        for (const auto& vertex : path) {
            const auto& node = graph[vertex];
            result.push_back(QPointF(node.longitude, node.latitude));
        }
        
        result.push_back(end); // Point d'arrivée exact
        
        utils::Logger::instance().info(QString("[PathPlanner] Chemin trouvé avec %1 points").arg(result.size()));
        return result;
    }
    
    // Aucun chemin trouvé
    utils::Logger::instance().warning("[PathPlanner] Aucun chemin trouvé entre les points");
    return {};
}

std::vector<QPointF> PathPlanner::generateRandomPath(const QPointF& start, double minLength) {
    if (!m_roadGraph) {
        utils::Logger::instance().warning("[PathPlanner] RoadGraph is null");
        return {start};
    }
    
    const auto& graph = m_roadGraph->getGraph();
    
    if (boost::num_vertices(graph) < 2) {
        utils::Logger::instance().warning(QString("[PathPlanner] Not enough vertices: %1").arg(boost::num_vertices(graph)));
        return {start};
    }
    
    // Log désactivé pour performances (génère trop de logs avec beaucoup de véhicules)
    // utils::Logger::instance().info(QString("[PathPlanner] Generating random path from (%1, %2), minLength=%3m")
    //                                .arg(start.y(), 0, 'f', 6)
    //                                .arg(start.x(), 0, 'f', 6)
    //                                .arg(minLength));
    
    // Trouver le nœud de départ le plus proche
    // Note: QPointF a x=longitude, y=latitude, mais getNearestNode attend (lat, lon)
    auto startVertex = m_roadGraph->getNearestNode(start.y(), start.x());
    
    // Log désactivé pour performances
    // utils::Logger::instance().info(QString("[PathPlanner] Start vertex: %1").arg(startVertex));
    
    // Générateur aléatoire
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // Sélectionner un nœud de destination aléatoire suffisamment loin
    std::uniform_int_distribution<> dis(0, boost::num_vertices(graph) - 1);
    VertexDescriptor endVertex;
    
    int attempts = 0;
    const int maxAttempts = 100;  // Augmenté de 50 à 100 pour plus de chances
    double bestDist = 0.0;
    VertexDescriptor bestVertex = boost::vertex(dis(gen), graph);
    
    // Chercher un nœud distant, garder le meilleur trouvé
    while (attempts < maxAttempts) {
        VertexDescriptor candidate = boost::vertex(dis(gen), graph);
        double dist = heuristic(startVertex, candidate);
        
        // Si distance parfaite trouvée, utiliser immédiatement
        if (dist >= minLength) {
            endVertex = candidate;
            break;
        }
        
        // Sinon, garder le plus lointain trouvé
        if (dist > bestDist) {
            bestDist = dist;
            bestVertex = candidate;
        }
        
        attempts++;
    }
    
    // Si aucune distance parfaite, utiliser le meilleur candidat trouvé
    if (attempts >= maxAttempts) {
        endVertex = bestVertex;
        if (bestDist > 0) {
            // Log désactivé pour performances
            // utils::Logger::instance().info(QString("[PathPlanner] Using best found: %1m (requested %2m)")
            //                                .arg(bestDist).arg(minLength));
        }
    }
    
    // Calculer le chemin vers ce nœud
    const auto& endNode = graph[endVertex];
    QPointF endPoint(endNode.longitude, endNode.latitude);
    
    return findPath(start, endPoint);
}

double PathPlanner::heuristic(VertexDescriptor a, VertexDescriptor b) const {
    if (!m_roadGraph) {
        return 0.0;
    }
    
    const auto& graph = m_roadGraph->getGraph();
    const auto& nodeA = graph[a];
    const auto& nodeB = graph[b];
    
    // Distance de Haversine
    const double R = 6371000.0; // Rayon de la Terre en mètres
    const double lat1 = nodeA.latitude * M_PI / 180.0;
    const double lat2 = nodeB.latitude * M_PI / 180.0;
    const double dLat = (nodeB.latitude - nodeA.latitude) * M_PI / 180.0;
    const double dLon = (nodeB.longitude - nodeA.longitude) * M_PI / 180.0;
    
    const double a_val = std::sin(dLat/2) * std::sin(dLat/2) +
                    std::cos(lat1) * std::cos(lat2) *
                    std::sin(dLon/2) * std::sin(dLon/2);
    const double c = 2 * std::atan2(std::sqrt(a_val), std::sqrt(1-a_val));
    
    return R * c;
}

} // namespace network
} // namespace v2v
