#pragma once

#include "V2VMessage.hpp"
#include <QObject>
#include <vector>
#include <memory>
#include <unordered_map>
#include <deque>

namespace v2v {

namespace core { class Vehicle; }
namespace network { class InterferenceGraph; }

namespace communication {

/**
 * @brief Statistiques de communication V2V
 */
struct V2VStatistics {
    // Messages
    size_t totalMessagesSent = 0;
    size_t totalMessagesReceived = 0;
    size_t totalMessagesDropped = 0;
    
    // Par type
    size_t camSent = 0;
    size_t denmSent = 0;
    size_t customSent = 0;
    
    // Latence
    double avgLatencyMs = 0.0;
    double maxLatencyMs = 0.0;
    double minLatencyMs = 999999.0;
    
    // Connexions
    size_t activeConnections = 0;
    double avgNeighbors = 0.0;
    
    // Bande passante (messages/sec)
    double throughput = 0.0;
    
    void reset() {
        totalMessagesSent = 0;
        totalMessagesReceived = 0;
        totalMessagesDropped = 0;
        camSent = 0;
        denmSent = 0;
        customSent = 0;
        avgLatencyMs = 0.0;
        maxLatencyMs = 0.0;
        minLatencyMs = 999999.0;
        activeConnections = 0;
        avgNeighbors = 0.0;
        throughput = 0.0;
    }
};

/**
 * @brief Gestionnaire de communication V2V
 * 
 * Responsabilités:
 * - Transmission de messages entre véhicules
 * - Simulation de latence réseau
 * - Gestion des pertes de paquets
 * - Statistiques de communication
 */
class V2VCommunicationManager : public QObject {
    Q_OBJECT
    
public:
    explicit V2VCommunicationManager(QObject* parent = nullptr);
    ~V2VCommunicationManager() override;
    
    /**
     * @brief Définir le graphe d'interférence (pour connaître les voisins)
     */
    void setInterferenceGraph(network::InterferenceGraph* graph);
    
    /**
     * @brief Envoyer un message broadcast (à tous les voisins)
     * @param message Message à envoyer
     * @param maxHops Nombre maximum de sauts (0 = direct uniquement, -1 = illimité)
     * @return Nombre de véhicules ayant reçu le message
     */
    int broadcastMessage(std::shared_ptr<V2VMessage> message, int maxHops = 0);
    
    /**
     * @brief Envoyer un message unicast (à un véhicule spécifique)
     * @param message Message à envoyer
     * @param targetId ID du véhicule destinataire
     * @return true si le message a été délivré
     */
    bool sendMessage(std::shared_ptr<V2VMessage> message, int targetId);
    
    /**
     * @brief Obtenir les messages reçus par un véhicule
     * @param vehicleId ID du véhicule
     * @return Liste des messages reçus depuis le dernier appel
     */
    std::vector<std::shared_ptr<V2VMessage>> getReceivedMessages(int vehicleId);
    
    /**
     * @brief Vider la boîte de réception d'un véhicule
     */
    void clearInbox(int vehicleId);
    
    /**
     * @brief Update (simulation de latence, nettoyage vieux messages)
     * @param deltaTime Temps écoulé depuis dernière update (secondes)
     */
    void update(double deltaTime);
    
    /**
     * @brief Configuration
     */
    void setPacketLossRate(double rate) { m_packetLossRate = rate; }
    void setLatencyMs(double latency) { m_baseLatencyMs = latency; }
    void setMaxMessageAge(double seconds) { m_maxMessageAge = seconds; }
    
    /**
     * @brief Statistiques
     */
    const V2VStatistics& getStatistics() const { return m_stats; }
    void resetStatistics() { m_stats.reset(); }
    
signals:
    /**
     * @brief Signal émis quand un message est transmis avec succès
     */
    void messageTransmitted(int senderId, int receiverId, QString messageType);
    
    /**
     * @brief Signal émis quand un message est perdu
     */
    void messageDropped(int senderId, QString reason);

private:
    struct PendingMessage {
        std::shared_ptr<V2VMessage> message;
        int targetId;
        double deliveryTime;  // Simulation de latence
    };
    
    network::InterferenceGraph* m_interferenceGraph;
    
    // Boîtes de réception par véhicule
    std::unordered_map<int, std::vector<std::shared_ptr<V2VMessage>>> m_inboxes;
    
    // Messages en transit (simulation de latence)
    std::deque<PendingMessage> m_pendingMessages;
    
    // Configuration
    double m_packetLossRate;      // 0.0 = 0%, 1.0 = 100%
    double m_baseLatencyMs;        // Latence de base
    double m_maxMessageAge;        // Âge max d'un message (secondes)
    
    // Statistiques
    V2VStatistics m_stats;
    double m_simulationTime;
    
    // Helpers
    bool simulatePacketLoss() const;
    double calculateLatency() const;
    void cleanOldMessages();
    void updateStatistics();
};

} // namespace communication
} // namespace v2v
