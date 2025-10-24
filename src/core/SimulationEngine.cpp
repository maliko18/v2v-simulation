#include "core/SimulationEngine.hpp"
#include "network/RoadGraph.hpp"
#include "network/InterferenceGraph.hpp"
#include "utils/Logger.hpp"
#include <QDateTime>
#include <random>

namespace v2v {
namespace core {

SimulationEngine::SimulationEngine(QObject* parent)
    : QObject(parent)
    , m_state(State::Stopped)
    , m_updateTimer(new QTimer(this))
    , m_timeScale(1.0)
    , m_targetFPS(60)
    , m_currentFPS(0)
    , m_simulationTime(0.0)
    , m_roadGraph(std::make_unique<network::RoadGraph>())
    , m_interferenceGraph(std::make_unique<network::InterferenceGraph>())
    , m_lastUpdateTime(0)
    , m_frameCount(0)
    , m_lastFPSUpdate(0)
{
    // Configure timer pour 60 FPS
    m_updateTimer->setInterval(1000 / m_targetFPS);
    connect(m_updateTimer, &QTimer::timeout, this, &SimulationEngine::updateSimulation);
    
    LOG_INFO("SimulationEngine initialized");
}

SimulationEngine::~SimulationEngine() {
    stop();
}

void SimulationEngine::start() {
    if (m_state == State::Running) {
        return;
    }
    
    m_state = State::Running;
    m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    m_lastFPSUpdate = m_lastUpdateTime;
    m_updateTimer->start();
    
    emit simulationStarted();
    LOG_INFO("Simulation started");
}

void SimulationEngine::pause() {
    if (m_state != State::Running) {
        return;
    }
    
    m_state = State::Paused;
    m_updateTimer->stop();
    
    emit simulationPaused();
    LOG_INFO("Simulation paused");
}

void SimulationEngine::stop() {
    if (m_state == State::Stopped) {
        return;
    }
    
    m_state = State::Stopped;
    m_updateTimer->stop();
    m_simulationTime = 0.0;
    
    emit simulationStopped();
    LOG_INFO("Simulation stopped");
}

void SimulationEngine::reset() {
    stop();
    m_vehicles.clear();
    m_simulationTime = 0.0;
    LOG_INFO("Simulation reset");
}

void SimulationEngine::setTimeScale(double scale) {
    m_timeScale = std::clamp(scale, 0.1, 10.0);
}

void SimulationEngine::setTargetFPS(int fps) {
    m_targetFPS = std::clamp(fps, 30, 120);
    m_updateTimer->setInterval(1000 / m_targetFPS);
}

void SimulationEngine::setVehicleCount(int count) {
    if (count != static_cast<int>(m_vehicles.size())) {
        createVehicles(count);
        emit vehicleCountChanged(count);
    }
}

int SimulationEngine::getActiveVehicleCount() const {
    int count = 0;
    for (const auto& vehicle : m_vehicles) {
        if (vehicle->isActive()) {
            count++;
        }
    }
    return count;
}

void SimulationEngine::updateSimulation() {
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    double deltaTime = (currentTime - m_lastUpdateTime) / 1000.0 * m_timeScale;
    m_lastUpdateTime = currentTime;
    
    // Update vehicles
    updateVehiclePositions(deltaTime);
    
    // Update interference graph
    updateInterferenceGraph();
    
    // Calculate FPS
    calculateFPS();
    
    m_simulationTime += deltaTime;
}

void SimulationEngine::createVehicles(int count) {
    m_vehicles.clear();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> pos_dist(-1000.0, 1000.0);
    std::uniform_real_distribution<> speed_dist(5.0, 20.0); // 5-20 m/s
    std::uniform_real_distribution<> dir_dist(0.0, 2.0 * M_PI);
    
    for (int i = 0; i < count; ++i) {
        auto vehicle = std::make_shared<Vehicle>(i);
        vehicle->setPosition(QPointF(pos_dist(gen), pos_dist(gen)));
        vehicle->setSpeed(speed_dist(gen));
        vehicle->setDirection(dir_dist(gen));
        m_vehicles.push_back(vehicle);
        
        emit vehicleAdded(i);
    }
    
    LOG_INFO(QString("Created %1 vehicles").arg(count));
}

void SimulationEngine::updateVehiclePositions(double deltaTime) {
    for (auto& vehicle : m_vehicles) {
        vehicle->update(deltaTime);
    }
}

void SimulationEngine::updateInterferenceGraph() {
    m_interferenceGraph->update(m_vehicles);
}

void SimulationEngine::calculateFPS() {
    m_frameCount++;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    if (currentTime - m_lastFPSUpdate >= 1000) {
        m_currentFPS = m_frameCount;
        m_frameCount = 0;
        m_lastFPSUpdate = currentTime;
        emit fpsChanged(m_currentFPS);
    }
}

} // namespace core
} // namespace v2v
