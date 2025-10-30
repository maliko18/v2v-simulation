#include "communication/V2VCommunicationManager.hpp"
#include "network/InterferenceGraph.hpp"
#include "utils/Logger.hpp"
#include <random>
#include <algorithm>

namespace v2v {
namespace communication {

V2VCommunicationManager::V2VCommunicationManager(QObject* parent)
    : QObject(parent)
    , m_interferenceGraph(nullptr)
    , m_packetLossRate(0.05)  // 5% perte par défaut (réaliste pour DSRC)
    , m_baseLatencyMs(10.0)    // 10ms latence de base
    , m_maxMessageAge(5.0)     // Messages valides 5 secondes
    , m_simulationTime(0.0)
{
    LOG_INFO("V2VCommunicationManager created");
}

V2VCommunicationManager::~V2VCommunicationManager() {
    LOG_INFO("V2VCommunicationManager destroyed");
}

void V2VCommunicationManager::setInterferenceGraph(network::InterferenceGraph* graph) {
    m_interferenceGraph = graph;
}

int V2VCommunicationManager::broadcastMessage(std::shared_ptr<V2VMessage> message, int maxHops) {
    if (!m_interferenceGraph) {
        LOG_WARNING("Cannot broadcast: no interference graph set");
        return 0;
    }
    
    int senderId = message->getSenderId();
    auto neighbors = m_interferenceGraph->getNeighbors(senderId);
    
    int deliveredCount = 0;
    
    for (int neighborId : neighbors) {
        // Simuler perte de paquet
        if (simulatePacketLoss()) {
            m_stats.totalMessagesDropped++;
            emit messageDropped(senderId, "Packet loss");
            continue;
        }
        
        // Calculer latence et ajouter à la file d'attente
        double latency = calculateLatency();
        
        PendingMessage pending;
        pending.message = message;
        pending.targetId = neighborId;
        pending.deliveryTime = m_simulationTime + latency / 1000.0;  // ms -> s
        
        m_pendingMessages.push_back(pending);
        deliveredCount++;
        
        // Stats
        m_stats.avgLatencyMs = (m_stats.avgLatencyMs * m_stats.totalMessagesSent + latency) 
                               / (m_stats.totalMessagesSent + 1);
        m_stats.maxLatencyMs = std::max(m_stats.maxLatencyMs, latency);
        m_stats.minLatencyMs = std::min(m_stats.minLatencyMs, latency);
    }
    
    // Stats
    m_stats.totalMessagesSent++;
    switch (message->getType()) {
        case MessageType::CAM: m_stats.camSent++; break;
        case MessageType::DENM: m_stats.denmSent++; break;
        case MessageType::Custom: m_stats.customSent++; break;
    }
    
    return deliveredCount;
}

bool V2VCommunicationManager::sendMessage(std::shared_ptr<V2VMessage> message, int targetId) {
    if (!m_interferenceGraph) {
        LOG_WARNING("Cannot send message: no interference graph set");
        return false;
    }
    
    int senderId = message->getSenderId();
    
    // Vérifier si le destinataire est un voisin direct
    auto neighbors = m_interferenceGraph->getNeighbors(senderId);
    bool isNeighbor = std::find(neighbors.begin(), neighbors.end(), targetId) != neighbors.end();
    
    if (!isNeighbor) {
        m_stats.totalMessagesDropped++;
        emit messageDropped(senderId, "Target not in range");
        return false;
    }
    
    // Simuler perte de paquet
    if (simulatePacketLoss()) {
        m_stats.totalMessagesDropped++;
        emit messageDropped(senderId, "Packet loss");
        return false;
    }
    
    // Calculer latence et ajouter à la file d'attente
    double latency = calculateLatency();
    
    PendingMessage pending;
    pending.message = message;
    pending.targetId = targetId;
    pending.deliveryTime = m_simulationTime + latency / 1000.0;
    
    m_pendingMessages.push_back(pending);
    
    // Stats
    m_stats.totalMessagesSent++;
    m_stats.avgLatencyMs = (m_stats.avgLatencyMs * m_stats.totalMessagesSent + latency) 
                           / (m_stats.totalMessagesSent + 1);
    m_stats.maxLatencyMs = std::max(m_stats.maxLatencyMs, latency);
    m_stats.minLatencyMs = std::min(m_stats.minLatencyMs, latency);
    
    return true;
}

std::vector<std::shared_ptr<V2VMessage>> V2VCommunicationManager::getReceivedMessages(int vehicleId) {
    auto it = m_inboxes.find(vehicleId);
    if (it == m_inboxes.end()) {
        return {};
    }
    
    // Retourner et vider la boîte
    std::vector<std::shared_ptr<V2VMessage>> messages = std::move(it->second);
    it->second.clear();
    
    return messages;
}

void V2VCommunicationManager::clearInbox(int vehicleId) {
    auto it = m_inboxes.find(vehicleId);
    if (it != m_inboxes.end()) {
        it->second.clear();
    }
}

void V2VCommunicationManager::update(double deltaTime) {
    m_simulationTime += deltaTime;
    
    // Délivrer les messages dont la latence est écoulée
    while (!m_pendingMessages.empty() && m_pendingMessages.front().deliveryTime <= m_simulationTime) {
        auto& pending = m_pendingMessages.front();
        
        // Vérifier que le message n'est pas trop vieux
        if (pending.message->getAge() <= m_maxMessageAge * 1000.0) {
            m_inboxes[pending.targetId].push_back(pending.message);
            m_stats.totalMessagesReceived++;
            
            emit messageTransmitted(pending.message->getSenderId(), 
                                  pending.targetId, 
                                  pending.message->toString());
        } else {
            m_stats.totalMessagesDropped++;
            emit messageDropped(pending.message->getSenderId(), "Message too old");
        }
        
        m_pendingMessages.pop_front();
    }
    
    // Nettoyer les vieux messages dans les boîtes de réception
    cleanOldMessages();
    
    // Mettre à jour les statistiques
    updateStatistics();
}

bool V2VCommunicationManager::simulatePacketLoss() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    
    return dis(gen) < m_packetLossRate;
}

double V2VCommunicationManager::calculateLatency() const {
    // Latence variable: base + jitter aléatoire
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::normal_distribution<> jitter(0.0, 2.0);  // +/- 2ms de jitter
    
    double latency = m_baseLatencyMs + jitter(gen);
    return std::max(1.0, latency);  // Au moins 1ms
}

void V2VCommunicationManager::cleanOldMessages() {
    for (auto& [vehicleId, inbox] : m_inboxes) {
        inbox.erase(
            std::remove_if(inbox.begin(), inbox.end(),
                [this](const std::shared_ptr<V2VMessage>& msg) {
                    return msg->getAge() > m_maxMessageAge * 1000.0;
                }),
            inbox.end()
        );
    }
}

void V2VCommunicationManager::updateStatistics() {
    // Calculer le débit (messages/sec)
    static double lastTime = 0.0;
    static size_t lastSentCount = 0;
    
    if (m_simulationTime - lastTime >= 1.0) {
        m_stats.throughput = (m_stats.totalMessagesSent - lastSentCount) / (m_simulationTime - lastTime);
        lastTime = m_simulationTime;
        lastSentCount = m_stats.totalMessagesSent;
    }
    
    // Connexions actives
    if (m_interferenceGraph) {
        m_stats.activeConnections = m_interferenceGraph->getConnectionCount();
        m_stats.avgNeighbors = m_interferenceGraph->getAverageConnections();
    }
}

} // namespace communication
} // namespace v2v
