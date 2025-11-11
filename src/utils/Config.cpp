#include "utils/Config.hpp"
#include "utils/Logger.hpp"
#include <QFile>
#include <QJsonArray>

namespace v2v {
namespace utils {

Config& Config::instance() {
    static Config instance;
    return instance;
}

bool Config::load(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARNING(QString("Could not open config file: %1").arg(filename));
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        return false;
    }
    
    m_root = doc.object();
    LOG_INFO(QString("Config loaded from: %1").arg(filename));
    return true;
}

bool Config::save(const QString& filename) const {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Could not save config file: %1").arg(filename));
        return false;
    }
    
    QJsonDocument doc(m_root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    LOG_INFO(QString("Config saved to: %1").arg(filename));
    return true;
}

QJsonValue Config::get(const QString& key) const {
    return m_root.value(key);
}

void Config::set(const QString& key, const QJsonValue& value) {
    m_root[key] = value;
}

QString Config::getString(const QString& key, const QString& defaultValue) const {
    return m_root.value(key).toString(defaultValue);
}

int Config::getInt(const QString& key, int defaultValue) const {
    return m_root.value(key).toInt(defaultValue);
}

double Config::getDouble(const QString& key, double defaultValue) const {
    return m_root.value(key).toDouble(defaultValue);
}

bool Config::getBool(const QString& key, bool defaultValue) const {
    return m_root.value(key).toBool(defaultValue);
}

Config::SimulationConfig Config::getSimulationConfig() const {
    QJsonObject simObj = m_root.value("simulation").toObject();
    
    SimulationConfig config;
    config.initialVehicles = simObj.value("initial_vehicles").toInt(2000);
    config.timeAcceleration = simObj.value("time_acceleration").toDouble(1.0);
    config.targetFPS = simObj.value("target_fps").toInt(60);
    config.transmissionRadius = simObj.value("transmission_radius_m").toInt(300);
    
    return config;
}

Config::MapConfig Config::getMapConfig() const {
    QJsonObject zoneObj = m_root.value("zone").toObject();
    QJsonObject centerObj = zoneObj.value("center").toObject();
    
    MapConfig config;
    config.centerLat = centerObj.value("lat").toDouble(47.7508);
    config.centerLon = centerObj.value("lon").toDouble(7.3359);
    config.zoomLevel = 13;
    config.osmFile = "";
    
    return config;
}

void Config::setSimulationConfig(const SimulationConfig& config) {
    QJsonObject simObj;
    simObj["initial_vehicles"] = config.initialVehicles;
    simObj["time_acceleration"] = config.timeAcceleration;
    simObj["target_fps"] = config.targetFPS;
    simObj["transmission_radius_m"] = config.transmissionRadius;
    
    m_root["simulation"] = simObj;
}

void Config::setMapConfig(const MapConfig& config) {
    QJsonObject zoneObj;
    QJsonObject centerObj;
    centerObj["lat"] = config.centerLat;
    centerObj["lon"] = config.centerLon;
    zoneObj["center"] = centerObj;
    
    m_root["zone"] = zoneObj;
}

} // namespace utils
} // namespace v2v
