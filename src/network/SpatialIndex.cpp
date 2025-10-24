#include "network/SpatialIndex.hpp"

namespace v2v {
namespace network {

SpatialIndex::SpatialIndex()
    : m_rtree(std::make_unique<RTree>())
{
}

void SpatialIndex::insert(const QPointF& point, int id) {
    m_rtree->insert(std::make_pair(toBoostPoint(point), id));
}

void SpatialIndex::remove(const QPointF& point, int id) {
    m_rtree->remove(std::make_pair(toBoostPoint(point), id));
}

std::vector<int> SpatialIndex::findInRadius(const QPointF& center, double radius) const {
    Point centerPoint = toBoostPoint(center);
    
    using Box = bg::model::box<Point>;
    Box queryBox(
        Point(centerPoint.get<0>() - radius, centerPoint.get<1>() - radius),
        Point(centerPoint.get<0>() + radius, centerPoint.get<1>() + radius)
    );
    
    std::vector<Value> results;
    m_rtree->query(bgi::intersects(queryBox), std::back_inserter(results));
    
    std::vector<int> ids;
    for (const auto& [point, id] : results) {
        double dist = bg::distance(centerPoint, point);
        if (dist <= radius) {
            ids.push_back(id);
        }
    }
    
    return ids;
}

int SpatialIndex::findNearest(const QPointF& point) const {
    std::vector<Value> result;
    m_rtree->query(bgi::nearest(toBoostPoint(point), 1), std::back_inserter(result));
    
    return result.empty() ? -1 : result[0].second;
}

void SpatialIndex::clear() {
    m_rtree = std::make_unique<RTree>();
}

size_t SpatialIndex::size() const {
    return m_rtree->size();
}

SpatialIndex::Point SpatialIndex::toBoostPoint(const QPointF& p) const {
    return Point(p.x(), p.y());
}

QPointF SpatialIndex::fromBoostPoint(const Point& p) const {
    return QPointF(p.get<0>(), p.get<1>());
}

} // namespace network
} // namespace v2v
