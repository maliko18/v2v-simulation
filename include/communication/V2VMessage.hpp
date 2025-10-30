#pragma once

#include <QString>
#include <QPointF>
#include <chrono>
#include <memory>

namespace v2v {
namespace communication {

/**
 * @brief Type de message V2V
 */
enum class MessageType {
    CAM,    // Cooperative Awareness Message (position, vitesse, direction)
    DENM,   // Decentralized Environmental Notification Message (événements)
    Custom  // Messages personnalisés
};

/**
 * @brief Priorité du message
 */
enum class MessagePriority {
    Low,
    Normal,
    High,
    Emergency
};

/**
 * @brief Classe de base pour tous les messages V2V
 */
class V2VMessage {
public:
    V2VMessage(int senderId, MessageType type, MessagePriority priority = MessagePriority::Normal)
        : m_senderId(senderId)
        , m_type(type)
        , m_priority(priority)
        , m_timestamp(std::chrono::steady_clock::now())
        , m_hopCount(0)
        , m_messageId(generateMessageId())
    {}
    
    virtual ~V2VMessage() = default;
    
    // Getters
    int getSenderId() const { return m_senderId; }
    MessageType getType() const { return m_type; }
    MessagePriority getPriority() const { return m_priority; }
    auto getTimestamp() const { return m_timestamp; }
    int getHopCount() const { return m_hopCount; }
    QString getMessageId() const { return m_messageId; }
    
    // Pour multi-hop
    void incrementHopCount() { m_hopCount++; }
    
    // Calcul de l'âge du message (en ms)
    double getAge() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(now - m_timestamp).count();
    }
    
    // Sérialisation (pour envoi réseau simulé)
    virtual QString toString() const = 0;
    
protected:
    int m_senderId;
    MessageType m_type;
    MessagePriority m_priority;
    std::chrono::steady_clock::time_point m_timestamp;
    int m_hopCount;
    QString m_messageId;
    
private:
    static QString generateMessageId() {
        static int counter = 0;
        return QString("MSG_%1_%2")
            .arg(std::chrono::steady_clock::now().time_since_epoch().count())
            .arg(++counter);
    }
};

/**
 * @brief CAM - Cooperative Awareness Message
 * Envoyé périodiquement (1-10 Hz) pour informer les voisins de sa position/état
 */
class CAM : public V2VMessage {
public:
    CAM(int senderId, 
        const QPointF& position, 
        double speed, 
        double heading,
        double acceleration = 0.0)
        : V2VMessage(senderId, MessageType::CAM, MessagePriority::Normal)
        , m_position(position)
        , m_speed(speed)
        , m_heading(heading)
        , m_acceleration(acceleration)
    {}
    
    // Getters
    QPointF getPosition() const { return m_position; }
    double getSpeed() const { return m_speed; }
    double getHeading() const { return m_heading; }
    double getAcceleration() const { return m_acceleration; }
    
    QString toString() const override {
        return QString("CAM[%1]: pos(%2,%3) speed=%4 heading=%5")
            .arg(m_senderId)
            .arg(m_position.x(), 0, 'f', 6)
            .arg(m_position.y(), 0, 'f', 6)
            .arg(m_speed, 0, 'f', 2)
            .arg(m_heading, 0, 'f', 2);
    }
    
private:
    QPointF m_position;
    double m_speed;
    double m_heading;
    double m_acceleration;
};

/**
 * @brief DENM - Decentralized Environmental Notification Message
 * Envoyé lors d'événements (accident, freinage d'urgence, obstacle, etc.)
 */
class DENM : public V2VMessage {
public:
    enum class EventType {
        HardBraking,        // Freinage d'urgence
        Accident,           // Accident
        RoadObstacle,       // Obstacle sur la route
        SlipperyRoad,       // Route glissante
        TrafficJam,         // Embouteillage
        EmergencyVehicle,   // Véhicule d'urgence
        Custom
    };
    
    DENM(int senderId,
         EventType eventType,
         const QPointF& eventLocation,
         const QString& description = "")
        : V2VMessage(senderId, MessageType::DENM, MessagePriority::High)
        , m_eventType(eventType)
        , m_eventLocation(eventLocation)
        , m_description(description)
        , m_validUntil(std::chrono::steady_clock::now() + std::chrono::seconds(60))
    {}
    
    // Getters
    EventType getEventType() const { return m_eventType; }
    QPointF getEventLocation() const { return m_eventLocation; }
    QString getDescription() const { return m_description; }
    
    bool isValid() const {
        return std::chrono::steady_clock::now() < m_validUntil;
    }
    
    QString toString() const override {
        return QString("DENM[%1]: event=%2 at(%3,%4) - %5")
            .arg(m_senderId)
            .arg(eventTypeToString(m_eventType))
            .arg(m_eventLocation.x(), 0, 'f', 6)
            .arg(m_eventLocation.y(), 0, 'f', 6)
            .arg(m_description);
    }
    
private:
    EventType m_eventType;
    QPointF m_eventLocation;
    QString m_description;
    std::chrono::steady_clock::time_point m_validUntil;
    
    static QString eventTypeToString(EventType type) {
        switch (type) {
            case EventType::HardBraking: return "HardBraking";
            case EventType::Accident: return "Accident";
            case EventType::RoadObstacle: return "RoadObstacle";
            case EventType::SlipperyRoad: return "SlipperyRoad";
            case EventType::TrafficJam: return "TrafficJam";
            case EventType::EmergencyVehicle: return "EmergencyVehicle";
            case EventType::Custom: return "Custom";
        }
        return "Unknown";
    }
};

/**
 * @brief Message personnalisé
 */
class CustomMessage : public V2VMessage {
public:
    CustomMessage(int senderId, const QString& payload, MessagePriority priority = MessagePriority::Normal)
        : V2VMessage(senderId, MessageType::Custom, priority)
        , m_payload(payload)
    {}
    
    QString getPayload() const { return m_payload; }
    
    QString toString() const override {
        return QString("CUSTOM[%1]: %2").arg(m_senderId).arg(m_payload);
    }
    
private:
    QString m_payload;
};

} // namespace communication
} // namespace v2v
