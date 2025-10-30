#pragma once

#include <QObject>
#include <QTimer>
#include <vector>
#include <memory>
#include "Vehicle.hpp"

namespace v2v {

// Forward declarations
namespace network { 
    class RoadGraph; 
    class InterferenceGraph;
    class PathPlanner;
}

namespace core {

/**
 * @brief Moteur principal de simulation
 * 
 * Gère:
 * - Création et destruction des véhicules
 * - Boucle de simulation (update loop)
 * - Temps de simulation et accélération
 * - Coordination entre graphe routier et graphe d'interférences
 */
class SimulationEngine : public QObject {
    Q_OBJECT

public:
    enum class State {
        Stopped,
        Running,
        Paused
    };

    explicit SimulationEngine(QObject* parent = nullptr);
    ~SimulationEngine() override;

    // Contrôle de la simulation
    void start();
    void pause();
    void stop();
    void reset();
    
    // Configuration
    void setTimeScale(double scale);  // 1.0 = temps réel, 2.0 = 2x plus rapide
    void setTargetFPS(int fps);
    void setVehicleCount(int count);
    
    // Accès aux données
    std::vector<std::shared_ptr<Vehicle>>& getVehicles() { return m_vehicles; }
    const std::vector<std::shared_ptr<Vehicle>>& getVehicles() const { return m_vehicles; }
    network::RoadGraph* getRoadGraph() const { return m_roadGraph.get(); }
    network::InterferenceGraph* getInterferenceGraph() const { return m_interferenceGraph.get(); }
    network::PathPlanner* getPathPlanner() const { return m_pathPlanner.get(); }
    
    // État
    State getState() const { return m_state; }
    double getTimeScale() const { return m_timeScale; }
    int getActiveVehicleCount() const;
    double getSimulationTime() const { return m_simulationTime; }
    int getCurrentFPS() const { return m_currentFPS; }
    
signals:
    void simulationStarted();
    void simulationPaused();
    void simulationStopped();
    void vehicleAdded(int vehicleId);
    void vehicleRemoved(int vehicleId);
    void fpsChanged(int fps);
    void vehicleCountChanged(int count);
    /** Emitted every simulation update (frame) */
    void tick();

private slots:
    void updateSimulation();
    
private:
    void createVehicles(int count);
    void updateVehiclePositions(double deltaTime);
    void updateInterferenceGraph();
    void calculateFPS();
    
    State m_state;
    QTimer* m_updateTimer;
    double m_timeScale;
    int m_targetFPS;
    int m_currentFPS;
    double m_simulationTime;
    
    std::vector<std::shared_ptr<Vehicle>> m_vehicles;
    std::unique_ptr<network::RoadGraph> m_roadGraph;
    std::unique_ptr<network::InterferenceGraph> m_interferenceGraph;
    std::unique_ptr<network::PathPlanner> m_pathPlanner;
    
    // Performance monitoring
    qint64 m_lastUpdateTime;
    int m_frameCount;
    qint64 m_lastFPSUpdate;
};

} // namespace core
} // namespace v2v
