#include "core/SimulationEngine.hpp"
#include "network/RoadGraph.hpp"
#include "network/InterferenceGraph.hpp"
#include "network/PathPlanner.hpp"
#include "communication/V2VCommunicationManager.hpp"
#include "communication/V2VMessage.hpp"
#include "utils/Logger.hpp"
#include <QDateTime>
#include <random>
#include <chrono>

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
    , m_v2vManager(std::make_unique<communication::V2VCommunicationManager>())
    , m_v2vEnabled(true)   // V2V activé par défaut
    , m_camFrequency(5.0)  // 5 Hz = 5 messages CAM/seconde
    , m_lastCAMTime(0.0)
    , m_lastUpdateTime(0)
    , m_frameCount(0)
    , m_lastFPSUpdate(0)
    , m_nextVehicleToGeneratePath(0)
    , m_maxVehiclesWithPaths(1000)
{
    // Configure timer pour 30 FPS (meilleure performance)
    m_updateTimer->setInterval(1000 / m_targetFPS);
    connect(m_updateTimer, &QTimer::timeout, this, &SimulationEngine::updateSimulation);
    
    // Lier V2VManager avec InterferenceGraph
    m_v2vManager->setInterferenceGraph(m_interferenceGraph.get());
    
    // PathPlanner sera initialisé quand le graphe routier sera chargé
    
    LOG_INFO("SimulationEngine initialized with V2V communication");
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
    
    // NE PLUS générer de chemins pendant la simulation (déplacé au démarrage)
    // generatePathsProgressively();
    
    // ✅ RÉACTIVÉ avec optimisation: Update interference graph toutes les 10 frames
    // Pour 2000 véhicules @ 30 FPS = update toutes les ~333ms (acceptable)
    if (m_v2vEnabled) {
        static int frameCounter = 0;
        if (++frameCounter >= 10) {
            updateInterferenceGraph();
            frameCounter = 0;
        }
        
        // Update V2V communication
        updateV2VCommunication();
    }
    // }
    
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
        // Zone géographique de Mulhouse
        std::uniform_real_distribution<> lat_dist(47.70, 47.80);  // Mulhouse centre
        std::uniform_real_distribution<> lon_dist(7.30, 7.40);
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
    
    int successCount = 0;
    
    // Timer pour éviter blocage avec gros fichiers OSM
    auto startTime = std::chrono::steady_clock::now();
    const int maxTimeSeconds = 60; // Maximum 60 secondes pour créer tous les véhicules
    
    for (int i = 0; i < count; ++i) {
        // Vérifier le timeout tous les 10 véhicules
        if (i % 10 == 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime).count();
            if (elapsed > maxTimeSeconds) {
                LOG_WARNING(QString("Vehicle creation timeout after %1s, created %2/%3 vehicles")
                           .arg(elapsed).arg(i).arg(count));
                break;
            }
        }
        
        auto vehicle = std::make_shared<Vehicle>(i);
        
        // Choisir un nœud de départ aléatoire
        auto startVertex = boost::vertex(node_dist(gen), graph);
        const auto& startNode = graph[startVertex];
        
        QPointF startPos(startNode.longitude, startNode.latitude);
        vehicle->setGeoPosition(startNode.latitude, startNode.longitude);
        vehicle->setPosition(startPos);
        vehicle->setSpeed(speed_dist(gen));
        
        // Log seulement les 10 premiers véhicules
        if (i < 10) {
            LOG_INFO(QString("Vehicle %1: start at (%2, %3)")
                     .arg(i)
                     .arg(startNode.latitude, 0, 'f', 6)
                     .arg(startNode.longitude, 0, 'f', 6));
        }
        
        // Ajouter le véhicule maintenant (pathfinding sera fait après)
        m_vehicles.push_back(vehicle);
        emit vehicleAdded(i);
        successCount++;
        
        // Afficher la progression tous les 50 véhicules
        if ((i + 1) % 50 == 0) {
            LOG_INFO(QString("Creating vehicles: %1/%2 (%3%)")
                     .arg(i + 1).arg(count)
                     .arg((i + 1) * 100 / count));
        }
    }
    
    LOG_INFO(QString("Created %1 vehicles on road network").arg(successCount));
    
    // Générer les chemins MAINTENANT (avant la simulation) pour éviter les freezes
    LOG_INFO(QString("Generating paths for all %1 vehicles (this may take a few seconds)...").arg(successCount));
    
    int pathsGenerated = 0;
    int pathsFailed = 0;
    auto pathStartTime = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < m_vehicles.size(); ++i) {
        auto& vehicle = m_vehicles[i];
        
        QPointF startPos(vehicle->getLongitude(), vehicle->getLatitude());
        auto path = m_pathPlanner->generateRandomPath(startPos, 500.0);
        
        if (!path.empty()) {
            vehicle->setPath(path);
            pathsGenerated++;
        } else {
            pathsFailed++;
        }
        
        // Afficher la progression tous les 200 véhicules
        if ((i + 1) % 200 == 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - pathStartTime).count();
            LOG_INFO(QString("Path generation: %1/%2 (%3%) - %4s elapsed")
                     .arg(i + 1).arg(m_vehicles.size())
                     .arg((i + 1) * 100 / m_vehicles.size())
                     .arg(elapsed));
        }
    }
    
    auto pathEndTime = std::chrono::steady_clock::now();
    auto pathDuration = std::chrono::duration_cast<std::chrono::milliseconds>(pathEndTime - pathStartTime).count();
    
    LOG_INFO(QString("Path generation complete: %1 paths generated, %2 failed in %3ms (avg %4ms/path)")
             .arg(pathsGenerated)
             .arg(pathsFailed)
             .arg(pathDuration)
             .arg(pathsGenerated > 0 ? pathDuration / pathsGenerated : 0));
    
    // Marquer comme terminé
    m_nextVehicleToGeneratePath = m_vehicles.size();
    m_maxVehiclesWithPaths = pathsGenerated;
}

void SimulationEngine::updateVehiclePositions(double deltaTime) {
    for (auto& vehicle : m_vehicles) {
        vehicle->update(deltaTime);
    }
}

void SimulationEngine::generatePathsProgressively() {
    // Génère des chemins pour quelques véhicules à chaque frame
    // pour éviter de bloquer l'UI
    if (!m_pathPlanner || m_nextVehicleToGeneratePath >= m_maxVehiclesWithPaths) {
        return;  // Déjà terminé
    }
    
    // IMPORTANT : Attendre 1 seconde seulement pour démarrer rapidement
    if (m_simulationTime < 1.0) {
        return;  // Attendre que la simulation soit stable
    }
    
    // Générer BEAUCOUP de chemins par frame - adaptatif selon le nombre de véhicules
    int pathsPerFrame = 20;  // Base : 20 chemins/frame
    if (m_vehicles.size() > 500) {
        pathsPerFrame = 50;  // Si >500 véhicules : 50 chemins/frame
    }
    if (m_vehicles.size() > 1000) {
        pathsPerFrame = 100;  // Si >1000 véhicules : 100 chemins/frame
    }
    if (m_vehicles.size() > 1500) {
        pathsPerFrame = 150;  // Si >1500 véhicules : 150 chemins/frame
    }
    
    int generated = 0;
    
    while (m_nextVehicleToGeneratePath < m_maxVehiclesWithPaths && 
           generated < pathsPerFrame &&
           m_nextVehicleToGeneratePath < m_vehicles.size()) {
        
        auto& vehicle = m_vehicles[m_nextVehicleToGeneratePath];
        
        // Vérifier si le véhicule n'a pas déjà un chemin
        if (!vehicle->hasPath()) {
            QPointF startPos(vehicle->getLongitude(), vehicle->getLatitude());
            // Distance adaptée pour Mulhouse (500m donne des trajets intéressants)
            auto path = m_pathPlanner->generateRandomPath(startPos, 500.0);
            if (!path.empty()) {
                vehicle->setPath(path);
            }
        }
        
        m_nextVehicleToGeneratePath++;
        generated++;
    }
    
    // Log progression tous les 100 véhicules
    static size_t lastLogged = 0;
    if (m_nextVehicleToGeneratePath / 100 > lastLogged) {
        lastLogged = m_nextVehicleToGeneratePath / 100;
        LOG_INFO(QString("Path generation progress: %1/%2 (%3%)")
                 .arg(m_nextVehicleToGeneratePath)
                 .arg(m_maxVehiclesWithPaths)
                 .arg(m_nextVehicleToGeneratePath * 100 / m_maxVehiclesWithPaths));
    }
    
    // Log final
    if (m_nextVehicleToGeneratePath >= m_maxVehiclesWithPaths) {
        LOG_INFO(QString("Path generation complete: %1 vehicles have paths")
                 .arg(m_maxVehiclesWithPaths));
    }
}

void SimulationEngine::calculateFPS() {
    m_frameCount++;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // Mettre à jour FPS toutes les 500ms pour réactivité
    if (currentTime - m_lastFPSUpdate >= 500) {
        m_currentFPS = m_frameCount * 2;  // *2 car on mesure sur 0.5s au lieu de 1s
        m_frameCount = 0;
        m_lastFPSUpdate = currentTime;
        emit fpsChanged(m_currentFPS);
    }
}

void SimulationEngine::updateInterferenceGraph() {
    if (!m_interferenceGraph) return;
    
    // Update avec tous les véhicules actifs
    m_interferenceGraph->update(m_vehicles);
    
    // Mettre à jour les voisins de chaque véhicule
    for (auto& vehicle : m_vehicles) {
        if (!vehicle->isActive()) continue;
        
        auto neighbors = m_interferenceGraph->getNeighbors(vehicle->getId());
        vehicle->setNeighbors(neighbors);
    }
}

void SimulationEngine::updateV2VCommunication() {
    if (!m_v2vManager) return;
    
    // Update du gestionnaire (traitement messages en attente)
    m_v2vManager->update(m_simulationTime);
    
    // Envoyer des messages CAM périodiquement
    double camInterval = 1.0 / m_camFrequency;  // ex: 5 Hz = 0.2s
    if (m_simulationTime - m_lastCAMTime >= camInterval) {
        m_lastCAMTime = m_simulationTime;
        
        int totalSent = 0;
        
        // Chaque véhicule envoie un CAM à ses voisins
        for (const auto& vehicle : m_vehicles) {
            if (!vehicle->isActive()) continue;
            
            // Créer message CAM
            auto cam = std::make_shared<communication::CAM>(
                vehicle->getId(),
                vehicle->getPosition(),
                vehicle->getSpeed(),
                vehicle->getDirection(),
                vehicle->getAcceleration()
            );
            
            // Broadcast aux voisins (direct uniquement, maxHops=0)
            int sent = m_v2vManager->broadcastMessage(cam, 0);
            totalSent += sent;
        }
        
        // Log V2V stats toutes les 5 secondes
        static double lastStatsLog = 0.0;
        if (m_simulationTime - lastStatsLog >= 5.0) {
            lastStatsLog = m_simulationTime;
            const auto& stats = m_v2vManager->getStatistics();
            LOG_INFO(QString("V2V Stats: %1 msgs sent, %2 received, %3 dropped | "
                           "Latency avg: %4ms | Connections: %5 | Neighbors avg: %6")
                     .arg(stats.totalMessagesSent)
                     .arg(stats.totalMessagesReceived)
                     .arg(stats.totalMessagesDropped)
                     .arg(stats.avgLatencyMs, 0, 'f', 2)
                     .arg(stats.activeConnections)
                     .arg(stats.avgNeighbors, 0, 'f', 1));
        }
    }
    
    // TODO: Traiter les messages reçus par chaque véhicule
    // Pour l'instant, on les collecte juste pour les statistiques
    for (const auto& vehicle : m_vehicles) {
        if (!vehicle->isActive()) continue;
        
        auto messages = m_v2vManager->getReceivedMessages(vehicle->getId());
        // Les véhicules pourraient réagir aux messages ici
        // Ex: freinage si DENM reçu, ajuster vitesse selon CAM voisins, etc.
    }
}

} // namespace core
} // namespace v2v
