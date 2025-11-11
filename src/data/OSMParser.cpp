#include "data/OSMParser.hpp"
#include "network/RoadGraph.hpp"
#include "utils/Logger.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <set>
#include <QPointF>
#include <QFile>
#include <QXmlStreamReader>
#include <cmath>

namespace v2v {
namespace data {

class OSMParser::Impl {
public:
    double minLat = -90.0, minLon = -180.0, maxLat = 90.0, maxLon = 180.0;
    std::set<std::string> roadTypes;
    bool useBoundingBox = false;
};

OSMParser::OSMParser() : m_impl(std::make_unique<Impl>()) {
    // Types de routes par défaut (routes carrossables)
    m_impl->roadTypes = {
        "motorway", "trunk", "primary", "secondary", "tertiary",
        "residential", "living_street", "unclassified",
        "motorway_link", "trunk_link", "primary_link", 
        "secondary_link", "tertiary_link"
    };
}

OSMParser::~OSMParser() = default;

bool OSMParser::loadFile(const std::string& filename, network::RoadGraph* roadGraph) {
    LOG_INFO(QString("Parsing OSM file: %1").arg(QString::fromStdString(filename)));
    
    // Effacer le graphe existant
    roadGraph->clear();
    
    // Si pas de fichier spécifié, générer données de test
    if (filename.empty()) {
        LOG_WARNING("No file specified - generating test data");
        return generateTestGraph(roadGraph);
    }
    
    // Parser le fichier OSM avec Qt XML
    LOG_INFO("Parsing OSM XML...");
    QFile file(QString::fromStdString(filename));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Cannot open file: %1").arg(QString::fromStdString(filename)));
        return generateTestGraph(roadGraph);
    }
    
    QXmlStreamReader xml(&file);
    
    // Maps pour stocker les données
    std::unordered_map<qint64, std::pair<double, double>> osmNodes; // id -> (lat, lon)
    std::unordered_map<qint64, network::VertexDescriptor> nodeMap;  // osmId -> graphVertex
    
    qint64 currentWayId = 0;
    QString currentWayType;
    std::vector<qint64> currentWayNodes;
    
    while (!xml.atEnd()) {
        xml.readNext();
        
        if (xml.isStartElement()) {
            if (xml.name() == QString("node")) {
                qint64 id = xml.attributes().value("id").toLongLong();
                double lat = xml.attributes().value("lat").toDouble();
                double lon = xml.attributes().value("lon").toDouble();
                osmNodes[id] = {lat, lon};
                
            } else if (xml.name() == QString("way")) {
                currentWayId = xml.attributes().value("id").toLongLong();
                currentWayType.clear();
                currentWayNodes.clear();
                
            } else if (xml.name() == QString("nd") && currentWayId != 0) {
                qint64 ref = xml.attributes().value("ref").toLongLong();
                currentWayNodes.push_back(ref);
                
            } else if (xml.name() == QString("tag") && currentWayId != 0) {
                QString key = xml.attributes().value("k").toString();
                QString value = xml.attributes().value("v").toString();
                
                if (key == "highway") {
                    currentWayType = value;
                }
            }
            
        } else if (xml.isEndElement()) {
            if (xml.name() == QString("way") && currentWayId != 0) {
                // Fin du chemin - créer les arêtes si c'est une route
                if (!currentWayType.isEmpty() && 
                    (currentWayType == "motorway" || currentWayType == "trunk" || 
                     currentWayType == "primary" || currentWayType == "secondary" || 
                     currentWayType == "tertiary" || currentWayType == "residential" ||
                     currentWayType == "unclassified" || currentWayType == "service" ||
                     currentWayType.endsWith("_link"))) {
                    
                    // Ajouter les nœuds et créer les arêtes
                    for (size_t i = 0; i < currentWayNodes.size(); ++i) {
                        qint64 nodeId = currentWayNodes[i];
                        
                        // Créer le vertex s'il n'existe pas encore
                        if (nodeMap.find(nodeId) == nodeMap.end() && 
                            osmNodes.find(nodeId) != osmNodes.end()) {
                            auto [lat, lon] = osmNodes[nodeId];
                            nodeMap[nodeId] = roadGraph->addNode(lat, lon);
                        }
                        
                        // Créer l'arête avec le nœud précédent
                        if (i > 0) {
                            qint64 prevNodeId = currentWayNodes[i - 1];
                            if (nodeMap.find(prevNodeId) != nodeMap.end() && 
                                nodeMap.find(nodeId) != nodeMap.end()) {
                                
                                auto [lat1, lon1] = osmNodes[prevNodeId];
                                auto [lat2, lon2] = osmNodes[nodeId];
                                double length = calculateDistance(lat1, lon1, lat2, lon2);
                                
                                // Vitesse selon le type de route
                                double speed = 13.9; // 50 km/h par défaut
                                if (currentWayType == "motorway") speed = 36.1; // 130 km/h
                                else if (currentWayType == "trunk") speed = 30.5; // 110 km/h
                                else if (currentWayType == "primary") speed = 25.0; // 90 km/h
                                else if (currentWayType == "secondary") speed = 22.2; // 80 km/h
                                
                                // Créer des arêtes BIDIRECTIONNELLES (aller et retour)
                                roadGraph->addEdge(nodeMap[prevNodeId], nodeMap[nodeId], 
                                                 length, speed, currentWayType.toStdString());
                                roadGraph->addEdge(nodeMap[nodeId], nodeMap[prevNodeId], 
                                                 length, speed, currentWayType.toStdString());
                            }
                        }
                    }
                }
                
                currentWayId = 0;
            }
        }
    }
    
    file.close();
    
    if (xml.hasError()) {
        LOG_ERROR(QString("XML parsing error: %1").arg(xml.errorString()));
        return generateTestGraph(roadGraph);
    }
    
    int nodeCount = roadGraph->getNodeCount();
    int edgeCount = roadGraph->getEdgeCount();
    
    if (nodeCount == 0) {
        LOG_WARNING("No valid road data found in OSM file - generating test data");
        return generateTestGraph(roadGraph);
    }
    
    LOG_INFO(QString("OSM file parsed successfully: %1 nodes, %2 edges").arg(nodeCount).arg(edgeCount));
    roadGraph->buildSpatialIndex();
    return true;
}

bool OSMParser::generateTestGraph(network::RoadGraph* roadGraph) {
    try {
        LOG_INFO("Generating test graph (10x10 grid)...");
        
        // GÉNÉRATION DE DONNÉES DE TEST pour Mulhouse
        // Centre : 47.7508°N, 7.3359°E
        double centerLat = 47.7508;
        double centerLon = 7.3359;
        
        // Créer une grille de routes pour tester
        const int gridSize = 10;
        double spacing = 0.005; // ~500m
        
        // Utiliser un vecteur 2D au lieu de VLA
        std::vector<std::vector<network::VertexDescriptor>> grid(gridSize, 
            std::vector<network::VertexDescriptor>(gridSize));
        
        // Créer les nœuds
        for (int i = 0; i < gridSize; ++i) {
            for (int j = 0; j < gridSize; ++j) {
                double lat = centerLat + (i - gridSize/2) * spacing;
                double lon = centerLon + (j - gridSize/2) * spacing;
                grid[i][j] = roadGraph->addNode(lat, lon);
            }
        }
        
        // Créer les arêtes (routes horizontales et verticales)
        for (int i = 0; i < gridSize; ++i) {
            for (int j = 0; j < gridSize; ++j) {
                // Arête horizontale
                if (j < gridSize - 1) {
                    double length = calculateDistance(
                        centerLat + (i - gridSize/2) * spacing,
                        centerLon + (j - gridSize/2) * spacing,
                        centerLat + (i - gridSize/2) * spacing,
                        centerLon + (j + 1 - gridSize/2) * spacing
                    );
                    
                    std::string roadType = (i % 3 == 0) ? "primary" : "residential";
                    double speed = (roadType == "primary") ? 25.0 : 13.9; // m/s
                    
                    roadGraph->addEdge(grid[i][j], grid[i][j+1], length, speed, roadType);
                    roadGraph->addEdge(grid[i][j+1], grid[i][j], length, speed, roadType);
                }
                
                // Arête verticale
                if (i < gridSize - 1) {
                    double length = calculateDistance(
                        centerLat + (i - gridSize/2) * spacing,
                        centerLon + (j - gridSize/2) * spacing,
                        centerLat + (i + 1 - gridSize/2) * spacing,
                        centerLon + (j - gridSize/2) * spacing
                    );
                    
                    std::string roadType = (j % 3 == 0) ? "secondary" : "residential";
                    double speed = (roadType == "secondary") ? 19.4 : 13.9; // m/s
                    
                    roadGraph->addEdge(grid[i][j], grid[i+1][j], length, speed, roadType);
                    roadGraph->addEdge(grid[i+1][j], grid[i][j], length, speed, roadType);
                }
            }
        }
        
        LOG_INFO(QString("Test graph generated: %1 nodes, %2 edges")
                 .arg(roadGraph->getNodeCount())
                 .arg(roadGraph->getEdgeCount()));
        
        // Construire l'index spatial
        roadGraph->buildSpatialIndex();
        
        LOG_INFO("Test road graph built successfully (Mulhouse area)");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Error generating test graph: %1").arg(e.what()));
        return false;
    }
}

double OSMParser::calculateDistance(double lat1, double lon1, double lat2, double lon2) const {
    // Formule de Haversine
    const double R = 6371000.0; // Rayon Terre en mètres
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = std::sin(dLat/2) * std::sin(dLat/2) +
               std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) *
               std::sin(dLon/2) * std::sin(dLon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return R * c;
}

void OSMParser::setBoundingBox(double minLat, double minLon, double maxLat, double maxLon) {
    m_impl->minLat = minLat;
    m_impl->minLon = minLon;
    m_impl->maxLat = maxLat;
    m_impl->maxLon = maxLon;
    m_impl->useBoundingBox = true;
}

void OSMParser::setRoadTypes(const std::vector<std::string>& types) {
    m_impl->roadTypes = std::set<std::string>(types.begin(), types.end());
}

} // namespace data
} // namespace v2v
