#include "core/SimulationEngine.hpp"
#include "network/RoadGraph.hpp"
#include "network/InterferenceGraph.hpp"
#include "network/PathPlanner.hpp"
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
    , m_targetFPS(30)  // Réduit de 60 à 30 FPS pour meilleures performances
    , m_currentFPS(0)
    , m_simulationTime(0.0)
    , m_roadGraph(std::make_unique<network::RoadGraph>())
    , m_interferenceGraph(std::make_unique<network::InterferenceGraph>())
    , m_pathPlanner(nullptr)
    , m_lastUpdateTime(0)
    , m_frameCount(0)
    , m_lastFPSUpdate(0)
{
    // Configure timer pour 30 FPS (meilleure performance)
    m_updateTimer->setInterval(1000 / m_targetFPS);
    connect(m_updateTimer, &QTimer::timeout, this, &SimulationEngine::updateSimulation);
    
    // PathPlanner sera initialisé quand le graphe routier sera chargé
    
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
    
    // Update interference graph (seulement toutes les 5 frames pour optimiser)
    static int frameCounter = 0;
    if (++frameCounter >= 5) {
        updateInterferenceGraph();
        frameCounter = 0;
    }
    
    // Calculate FPS
    calculateFPS();
    
    m_simulationTime += deltaTime;
    
    // Notifier l'UI que la simulation a avancé (permet de redessiner la vue)
    emit tick();
}

void SimulationEngine::createVehicles(int count) {
    m_vehicles.clear();
    
    // Si pas de graphe routier ou pas de PathPlanner, création simple
    if (!m_roadGraph || boost::num_vertices(m_roadGraph->getGraph()) == 0) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> lat_dist(47.73, 47.77); // Autour de Mulhouse
        std::uniform_real_distribution<> lon_dist(7.31, 7.36);
        std::uniform_real_distribution<> speed_dist(10.0, 25.0); // 10-25 m/s (36-90 km/h)
        std::uniform_real_distribution<> dir_dist(0.0, 2.0 * M_PI);
        
        for (int i = 0; i < count; ++i) {
            auto vehicle = std::make_shared<Vehicle>(i);
            double lat = lat_dist(gen);
            double lon = lon_dist(gen);
            vehicle->setGeoPosition(lat, lon);
            vehicle->setPosition(QPointF(lon, lat));
            vehicle->setSpeed(speed_dist(gen));
            vehicle->setDirection(dir_dist(gen));
            m_vehicles.push_back(vehicle);
            
            emit vehicleAdded(i);
        }
        
        LOG_INFO(QString("Created %1 vehicles (simple mode)").arg(count));
        return;
    }
    
    // Initialiser le PathPlanner si nécessaire
    if (!m_pathPlanner) {
        m_pathPlanner = std::make_unique<network::PathPlanner>(m_roadGraph.get());
        LOG_INFO("PathPlanner initialized");
    }
    
    // Création avancée avec chemins sur le graphe routier
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Obtenir les limites du graphe
    const auto& graph = m_roadGraph->getGraph();
    auto [vi, vi_end] = boost::vertices(graph);
    
    if (vi == vi_end) {
        LOG_WARNING("Cannot create vehicles: empty road graph");
        return;
    }
    
    LOG_INFO(QString("Creating %1 vehicles on road graph with %2 nodes and %3 edges")
             .arg(count)
             .arg(boost::num_vertices(graph))
             .arg(boost::num_edges(graph)));
    
    std::uniform_int_distribution<> node_dist(0, boost::num_vertices(graph) - 1);
    std::uniform_real_distribution<> speed_dist(10.0, 25.0); // 10-25 m/s (36-90 km/h)
    
    for (int i = 0; i < count; ++i) {
        auto vehicle = std::make_shared<Vehicle>(i);
        
        // Choisir un nœud de départ aléatoire
        auto startVertex = boost::vertex(node_dist(gen), graph);
        const auto& startNode = graph[startVertex];
        
        QPointF startPos(startNode.longitude, startNode.latitude);
        vehicle->setGeoPosition(startNode.latitude, startNode.longitude);
        vehicle->setPosition(startPos);
        vehicle->setSpeed(speed_dist(gen));
        
        LOG_INFO(QString("Vehicle %1: start at (%2, %3)")
                 .arg(i)
                 .arg(startNode.latitude, 0, 'f', 6)
                 .arg(startNode.longitude, 0, 'f', 6));
        
        // Générer un chemin aléatoire d'au moins 2000m
        auto path = m_pathPlanner->generateRandomPath(startPos, 2000.0);
        if (!path.empty()) {
            vehicle->setPath(path);
            LOG_INFO(QString("Vehicle %1: path with %2 points").arg(i).arg(path.size()));
        } else {
            LOG_WARNING(QString("Vehicle %1: NO PATH GENERATED!").arg(i));
        }
        
        m_vehicles.push_back(vehicle);
        emit vehicleAdded(i);
    }
    
    LOG_INFO(QString("Created %1 vehicles with paths on road network").arg(count));
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
