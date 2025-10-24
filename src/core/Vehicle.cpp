#include "core/Vehicle.hpp"
#include "data/GeometryUtils.hpp"
#include <cmath>

namespace v2v {
namespace core {

Vehicle::Vehicle(int id, QObject* parent)
    : QObject(parent)
    , m_id(id)
    , m_position(0, 0)
    , m_latitude(0.0)
    , m_longitude(0.0)
    , m_speed(0.0)
    , m_direction(0.0)
    , m_transmissionRadius(300)
    , m_isActive(true)
    , m_currentPathIndex(0)
{
}

void Vehicle::setPosition(const QPointF& pos) {
    m_position = pos;
    emit positionChanged(m_id, pos);
}

void Vehicle::setGeoPosition(double lat, double lon) {
    m_latitude = lat;
    m_longitude = lon;
    // Convert to screen coordinates if needed
    emit positionChanged(m_id, m_position);
}

void Vehicle::setSpeed(double speed) {
    m_speed = speed;
    emit speedChanged(m_id, speed);
}

void Vehicle::setDirection(double direction) {
    m_direction = direction;
}

void Vehicle::setTransmissionRadius(int radius) {
    m_transmissionRadius = std::clamp(radius, 100, 500);
}

void Vehicle::setActive(bool active) {
    m_isActive = active;
}

void Vehicle::update(double deltaTime) {
    if (!m_isActive || m_speed <= 0.0) {
        return;
    }
    
    // Simple linear movement
    double dx = m_speed * deltaTime * std::cos(m_direction);
    double dy = m_speed * deltaTime * std::sin(m_direction);
    
    m_position += QPointF(dx, dy);
    emit positionChanged(m_id, m_position);
}

double Vehicle::distanceTo(const Vehicle& other) const {
    return data::GeometryUtils::euclideanDistance(m_position, other.m_position);
}

bool Vehicle::canCommunicateWith(const Vehicle& other) const {
    double dist = distanceTo(other);
    return dist <= m_transmissionRadius;
}

void Vehicle::setPath(const std::vector<QPointF>& path) {
    m_path = path;
    m_currentPathIndex = 0;
}

void Vehicle::clearPath() {
    m_path.clear();
    m_currentPathIndex = 0;
}

bool Vehicle::hasPath() const {
    return !m_path.empty() && m_currentPathIndex < m_path.size();
}

} // namespace core
} // namespace v2v
