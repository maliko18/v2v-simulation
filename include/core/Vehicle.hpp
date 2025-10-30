#pragma once

#include <QObject>
#include <QPointF>
#include <memory>
#include <vector>

namespace v2v {
namespace core {

/**
 * @brief Représente un véhicule dans la simulation V2V
 * 
 * Propriétés:
 * - Position géographique (lat/lon)
 * - Vitesse et direction
 * - Rayon de transmission (100-500m)
 * - État de connexion aux autres véhicules
 */
class Vehicle : public QObject {
    Q_OBJECT

public:
    explicit Vehicle(int id, QObject* parent = nullptr);
    ~Vehicle() override = default;

    // Getters
    int getId() const { return m_id; }
    QPointF getPosition() const { return m_position; }
    double getLatitude() const { return m_latitude; }
    double getLongitude() const { return m_longitude; }
    double getSpeed() const { return m_speed; }
    double getDirection() const { return m_direction; }
    int getTransmissionRadius() const { return m_transmissionRadius; }
    bool isActive() const { return m_isActive; }
    
    // Setters
    void setPosition(const QPointF& pos);
    void setGeoPosition(double lat, double lon);
    void setSpeed(double speed);
    void setDirection(double direction);
    void setTransmissionRadius(int radius);
    void setActive(bool active);
    
    // Méthodes de simulation
    void update(double deltaTime);
    double distanceTo(const Vehicle& other) const;
    bool canCommunicateWith(const Vehicle& other) const;
    
    // Gestion du chemin
    void setPath(const std::vector<QPointF>& path);
    void clearPath();
    bool hasPath() const;
    
    // Communication V2V
    void setNeighbors(const std::vector<int>& neighbors);
    const std::vector<int>& getNeighbors() const { return m_connectedVehicles; }
    int getNeighborCount() const { return m_connectedVehicles.size(); }
    
    // État pour messages CAM
    double getAcceleration() const { return m_acceleration; }
    void setAcceleration(double accel) { m_acceleration = accel; }
    
signals:
    void positionChanged(int vehicleId, QPointF newPosition);
    void speedChanged(int vehicleId, double newSpeed);
    void connectionEstablished(int vehicleId, int otherId);
    void connectionLost(int vehicleId, int otherId);

private:
    int m_id;
    QPointF m_position;          // Position écran (x, y)
    double m_latitude;           // Position GPS
    double m_longitude;
    double m_speed;              // m/s
    double m_direction;          // Radians (0 = Nord)
    double m_acceleration;       // m/s² (pour messages CAM)
    int m_transmissionRadius;    // Mètres (100-500)
    bool m_isActive;
    
    // Chemin à suivre
    std::vector<QPointF> m_path;
    size_t m_currentPathIndex;
    
    // Connectivité (voisins V2V)
    std::vector<int> m_connectedVehicles;
};

} // namespace core
} // namespace v2v
