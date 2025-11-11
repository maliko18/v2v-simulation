#pragma once

#include "RoadGraph.hpp"
#include <vector>
#include <QPointF>

namespace v2v {
namespace network {

/**
 * @brief Planificateur de chemin utilisant A* ou Dijkstra
 */
class PathPlanner {
public:
    PathPlanner(RoadGraph* roadGraph);
    ~PathPlanner() = default;
    
    /**
     * @brief Calculer le plus court chemin entre deux points
     * @param start Point de départ
     * @param end Point d'arrivée
     * @return Liste de points formant le chemin, vide si aucun chemin trouvé
     */
    std::vector<QPointF> findPath(const QPointF& start, const QPointF& end);
    
    /**
     * @brief Calculer un chemin aléatoire pour un véhicule
     * @param start Point de départ
     * @param minLength Longueur minimale du chemin en mètres
     * @return Chemin aléatoire
     */
    std::vector<QPointF> generateRandomPath(const QPointF& start, double minLength);

private:
    RoadGraph* m_roadGraph;
    
    double heuristic(VertexDescriptor a, VertexDescriptor b) const;
};

} // namespace network
} // namespace v2v
