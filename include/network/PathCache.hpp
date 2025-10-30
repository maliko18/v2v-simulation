#pragma once

#include <vector>
#include <QPointF>
#include <QString>
#include <memory>

namespace v2v {
namespace network {

class RoadGraph;
class PathPlanner;

/**
 * @brief Cache de chemins pré-calculés pour performances optimales
 * 
 * Au lieu de calculer un chemin aléatoire pour chaque véhicule (lent),
 * on pré-calcule un ensemble de chemins réalistes une seule fois.
 */
class PathCache {
public:
    PathCache();
    ~PathCache();
    
    /**
     * @brief Génère et cache un ensemble de chemins typiques
     * @param roadGraph Graphe routier source
     * @param pathPlanner Planificateur de chemins
     * @param numPaths Nombre de chemins à pré-calculer (défaut: 100)
     */
    void generatePaths(RoadGraph* roadGraph, PathPlanner* pathPlanner, int numPaths = 100);
    
    /**
     * @brief Obtient un chemin pré-calculé (rotation circulaire)
     * @return Chemin pré-calculé
     */
    std::vector<QPointF> getPath();
    
    /**
     * @brief Nombre de chemins disponibles
     */
    size_t getPathCount() const { return m_paths.size(); }
    
    /**
     * @brief Vide le cache
     */
    void clear();
    
    /**
     * @brief Sauvegarde les chemins dans un fichier
     */
    bool saveToFile(const QString& filename);
    
    /**
     * @brief Charge les chemins depuis un fichier
     */
    bool loadFromFile(const QString& filename);
    
private:
    std::vector<std::vector<QPointF>> m_paths;
    size_t m_currentIndex;
};

} // namespace network
} // namespace v2v
