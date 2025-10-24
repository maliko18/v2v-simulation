#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <QPointF>
#include <vector>

namespace v2v {
namespace network {

// Aliases pour Boost.Geometry
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

/**
 * @brief Wrapper autour de Boost.Geometry R-tree pour recherche spatiale
 * 
 * Utilisé pour:
 * - Trouver rapidement les véhicules dans un rayon donné
 * - Trouver le nœud routier le plus proche
 * - Queries spatiales efficaces O(log n) au lieu de O(n)
 */
class SpatialIndex {
public:
    SpatialIndex();
    ~SpatialIndex() = default;
    
    /**
     * @brief Insérer un point avec un ID
     */
    void insert(const QPointF& point, int id);
    
    /**
     * @brief Retirer un point
     */
    void remove(const QPointF& point, int id);
    
    /**
     * @brief Trouver tous les points dans un rayon
     * @param center Centre de recherche
     * @param radius Rayon en unités de coordonnées
     * @return IDs des points trouvés
     */
    std::vector<int> findInRadius(const QPointF& center, double radius) const;
    
    /**
     * @brief Trouver le point le plus proche
     * @param point Point de référence
     * @return ID du point le plus proche, -1 si aucun
     */
    int findNearest(const QPointF& point) const;
    
    /**
     * @brief Clear l'index
     */
    void clear();
    
    /**
     * @brief Nombre d'éléments
     */
    size_t size() const;

private:
    using Point = bg::model::point<double, 2, bg::cs::cartesian>;
    using Value = std::pair<Point, int>;
    using RTree = bgi::rtree<Value, bgi::quadratic<16>>;
    
    std::unique_ptr<RTree> m_rtree;
    
    Point toBoostPoint(const QPointF& p) const;
    QPointF fromBoostPoint(const Point& p) const;
};

} // namespace network
} // namespace v2v
