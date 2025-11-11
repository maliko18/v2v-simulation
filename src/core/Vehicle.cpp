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
    // Mettre à jour la position stockée (x = longitude, y = latitude)
    m_position = QPointF(m_longitude, m_latitude);
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
    
    // Si nous avons un chemin à suivre
    if (hasPath() && m_currentPathIndex < m_path.size()) {
        const QPointF& target = m_path[m_currentPathIndex];

        // target.x() == longitude, target.y() == latitude
        // Calculer la distance en degrés (approx.)
        double dx = target.x() - m_longitude; // delta longitude
        double dy = target.y() - m_latitude;  // delta latitude
        double distToTarget = std::sqrt(dx * dx + dy * dy);

        // Distance que le véhicule peut parcourir pendant deltaTime (convertie en degrés)
        // Approximation: metersPerDegree degrés ~ 111320 m
        const double metersPerDegree = 111320.0; // À l'équateur
        double distanceCanTravelDeg = (m_speed * deltaTime) / metersPerDegree;

        if (distToTarget <= distanceCanTravelDeg * 1.5) {
            // On est arrivé au point, passer au suivant
            m_longitude = target.x();
            m_latitude = target.y();
            m_currentPathIndex++;

            // Si on a atteint la fin du chemin
            if (m_currentPathIndex >= m_path.size()) {
                m_speed = 0.0; // Arrêter le véhicule
            }
        } else {
            // Se déplacer vers le point cible
            m_direction = std::atan2(dy, dx);

            double moveLon = (dx / distToTarget) * distanceCanTravelDeg;
            double moveLat = (dy / distToTarget) * distanceCanTravelDeg;

            m_longitude += moveLon;
            m_latitude += moveLat;
        }
        
        // Mettre à jour la position (x = lon, y = lat)
        m_position = QPointF(m_longitude, m_latitude);
        emit positionChanged(m_id, m_position);
    } else {
        // Mouvement linéaire simple (ancien comportement)
        const double metersPerDegree = 111320.0;
        double distanceCanTravel = (m_speed * deltaTime) / metersPerDegree;
        
        double dx = distanceCanTravel * std::cos(m_direction);
        double dy = distanceCanTravel * std::sin(m_direction);
        
        m_latitude += dy;
        m_longitude += dx;
        m_position = QPointF(m_longitude, m_latitude);
        emit positionChanged(m_id, m_position);
    }
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
