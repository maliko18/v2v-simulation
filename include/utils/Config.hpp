#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <memory>

namespace v2v {
namespace utils {

/**
 * @brief Gestionnaire de configuration JSON
 */
class Config {
public:
    static Config& instance();
    
    bool load(const QString& filename);
    bool save(const QString& filename) const;
    
    // Accesseurs génériques
    QJsonValue get(const QString& key) const;
    void set(const QString& key, const QJsonValue& value);
    
    // Accesseurs typés
    QString getString(const QString& key, const QString& defaultValue = "") const;
    int getInt(const QString& key, int defaultValue = 0) const;
    double getDouble(const QString& key, double defaultValue = 0.0) const;
    bool getBool(const QString& key, bool defaultValue = false) const;
    
    // Configuration spécifique V2V
    struct SimulationConfig {
        int initialVehicles;
        double timeAcceleration;
        int targetFPS;
        int transmissionRadius;
    };
    
    struct MapConfig {
        double centerLat;
        double centerLon;
        int zoomLevel;
        QString osmFile;
    };
    
    SimulationConfig getSimulationConfig() const;
    MapConfig getMapConfig() const;
    
    void setSimulationConfig(const SimulationConfig& config);
    void setMapConfig(const MapConfig& config);

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    QJsonObject m_root;
};

} // namespace utils
} // namespace v2v
