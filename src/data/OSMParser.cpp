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
    // Classe vide pour l'instant (pour extension future)
};

OSMParser::OSMParser() : m_impl(std::make_unique<Impl>()) {
}

OSMParser::~OSMParser() = default;

HighwayType OSMParser::stringToHighwayType(const QString& typeStr) {
    if (typeStr == "motorway") return HighwayType::Motorway;
    if (typeStr == "trunk") return HighwayType::Trunk;
    if (typeStr == "primary") return HighwayType::Primary;
    if (typeStr == "secondary") return HighwayType::Secondary;
    if (typeStr == "tertiary") return HighwayType::Tertiary;
    if (typeStr == "residential") return HighwayType::Residential;
    if (typeStr == "unclassified") return HighwayType::Unclassified;
    if (typeStr == "service") return HighwayType::Service;
    if (typeStr == "motorway_link") return HighwayType::MotorwayLink;
    if (typeStr == "trunk_link") return HighwayType::TrunkLink;
    if (typeStr == "primary_link") return HighwayType::PrimaryLink;
    if (typeStr == "secondary_link") return HighwayType::SecondaryLink;
    if (typeStr == "tertiary_link") return HighwayType::TertiaryLink;
    return HighwayType::Unknown;
}

bool OSMParser::isValidHighwayType(HighwayType type) {
    return type != HighwayType::Unknown;
}

double OSMParser::getDefaultSpeed(HighwayType type) {
    switch (type) {
        case HighwayType::Motorway:
        case HighwayType::MotorwayLink:
            return 36.1; // 130 km/h
        case HighwayType::Trunk:
        case HighwayType::TrunkLink:
            return 30.5; // 110 km/h
        case HighwayType::Primary:
        case HighwayType::PrimaryLink:
            return 25.0; // 90 km/h
        case HighwayType::Secondary:
        case HighwayType::SecondaryLink:
            return 22.2; // 80 km/h
        case HighwayType::Tertiary:
        case HighwayType::TertiaryLink:
        case HighwayType::Residential:
        case HighwayType::Unclassified:
        case HighwayType::Service:
        default:
            return 13.9; // 50 km/h
    }
}

std::string OSMParser::highwayTypeToString(HighwayType type) {
    switch (type) {
        case HighwayType::Motorway: return "motorway";
        case HighwayType::Trunk: return "trunk";
        case HighwayType::Primary: return "primary";
        case HighwayType::Secondary: return "secondary";
        case HighwayType::Tertiary: return "tertiary";
        case HighwayType::Residential: return "residential";
        case HighwayType::Unclassified: return "unclassified";
        case HighwayType::Service: return "service";
        case HighwayType::MotorwayLink: return "motorway_link";
        case HighwayType::TrunkLink: return "trunk_link";
        case HighwayType::PrimaryLink: return "primary_link";
        case HighwayType::SecondaryLink: return "secondary_link";
        case HighwayType::TertiaryLink: return "tertiary_link";
        default: return "unknown";
    }
}

bool OSMParser::loadFile(const std::string& filename, network::RoadGraph* roadGraph) {
    
    //-------Etape 1 : Log et nettoyage---------
    //---------------------------------
 
    // Message de log pour savoir q'on commence
    LOG_INFO(QString("Parsing OSM file: %1").arg(QString::fromStdString(filename)));
    
    // Effacer le graphe existant
    roadGraph->clear();
    

    //-------Etape 2 : Gestion fichier vide ---------

    // Si pas de fichier spécifié, générer données de test
    if (filename.empty()) {
        LOG_WARNING("No file specified - generating test data");
        return generateTestGraph(roadGraph);
    }


    //-------Etape 3 : Ouverture de fichier ---------
    // Parser le fichier OSM avec Qt XML
    LOG_INFO("Parsing OSM XML...");
    QFile file(QString::fromStdString(filename));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Cannot open file: %1").arg(QString::fromStdString(filename)));
        return generateTestGraph(roadGraph);
    }
    
    // lecteur du fichier : QXmlStreamReader
    //objet qui vas parser le xml
    QXmlStreamReader xml(&file);



    //-------Etape 4 : Structure de donnees ---------
    
    // Maps pour stocker les données table de hashage
    // 1. osmNodes = Catalogue de tout les noeuds disponibles sert a stocker toutes le spositions gps des noeuds
    std::unordered_map<qint64, std::pair<double, double>> osmNodes; // id -> (lat, lon)
    // 2. nodeMap = Numérotation des noeuds déjà posées  evite les doublons
    //Sert à : Savoir quels nœuds OSM ont déjà été ajoutés au graphe
    std::unordered_map<qint64, network::VertexDescriptor> nodeMap;  // osmId -> graphVertex
    
    // numero de la route en cours de parsing 
    qint64 currentWayId = 0;

    // type de la route en cours de parsing
    HighwayType currentWayType = HighwayType::Unknown;

    // liste des noeuds de la route en cours de parsing
    std::vector<qint64> currentWayNodes;
    

    //-----------Etape 5 : Boucle de Parsing XML OSM -------------
    //tant qu'il reste du xml a lire 
    while (!xml.atEnd()) {
        // Lire le prochain élément tokens
        xml.readNext();
        
        //si c'est une balise ouvrante <..> 
        if (xml.isStartElement()) {
            // si c'est un noeud <node>
            if (xml.name() == QString("node")) {
                // Extraire les attributs  id, lat, lon
                qint64 id = xml.attributes().value("id").toLongLong();
                double lat = xml.attributes().value("lat").toDouble();
                double lon = xml.attributes().value("lon").toDouble();

                // les stocker dans la table de hashage  osmNodes
                osmNodes[id] = {lat, lon};
                
            // si c'est un chemin <way> une route
            } else if (xml.name() == QString("way")) {
                currentWayId = xml.attributes().value("id").toLongLong();
                // Reintialise toujours les variables temporaire
                currentWayType = HighwayType::Unknown; // réinitialiser le type de route
                currentWayNodes.clear(); // vider la liste des noeuds de la route
            
            // si c'est un noeud de chemin <nd> on extrait le ref
            // Si on lit <nd> en dehors d'un way → ignore
            } else if (xml.name() == QString("nd") && currentWayId != 0) {
                qint64 ref = xml.attributes().value("ref").toLongLong();
                // Ajouter le noeud de chemin à la liste
                currentWayNodes.push_back(ref);
               
                // if highway tag memorisé 
            } else if (xml.name() == QString("tag") && currentWayId != 0) {
                QString key = xml.attributes().value("k").toString();
                QString value = xml.attributes().value("v").toString();
                
                if (key == "highway") {
                    currentWayType = stringToHighwayType(value);
                }
            }
        // si c'est une balise fermante </..> on traite la fin des ways </way> et construire le graphe
        } else if (xml.isEndElement()) {

            if (xml.name() == QString("way") && currentWayId != 0) {
                // Fin du chemin - créer les arêtes si c'est une route valide
                if (isValidHighwayType(currentWayType)) {
                    
                    // Ajouter les nœuds et créer les arêtes
                    for (size_t i = 0; i < currentWayNodes.size(); ++i) {
                        // Traiter chaque nœud  vertex de la route
                        // Recuperer l'id du noeud courant
                        qint64 nodeId = currentWayNodes[i];
                        
                        // Créer le vertex s'il n'existe pas encore
                        // si ce noeud n as pas encore été ajouté au graphe et ce noeud existe dans osmNodes
                        if (nodeMap.find(nodeId) == nodeMap.end() && 
                            osmNodes.find(nodeId) != osmNodes.end()) {
                            // recuperer les positions gps du noeud
                            auto [lat, lon] = osmNodes[nodeId];
                            // ajouter le noeud au graphe
                            nodeMap[nodeId] = roadGraph->addNode(lat, lon);
                        }
                        
                        // Créer l'arête avec le nœud précédent
                        if (i > 0) {

                            // Recuperer l'id du noeud precedent
                            qint64 prevNodeId = currentWayNodes[i - 1];
                            // verifier que les deux noeuds existent  dans le graphe
                            //Parfois un nœud OSM est référencé dans un <way> mais n'existe pas dans osmNodes
                            if (nodeMap.find(prevNodeId) != nodeMap.end() && 
                                nodeMap.find(nodeId) != nodeMap.end()) {
                                

                                // recupérer les positions gps des deux noeuds
                                auto [lat1, lon1] = osmNodes[prevNodeId];
                                auto [lat2, lon2] = osmNodes[nodeId];
                                // Calculer la distance entre les deux noeuds en metres
                                double length = calculateDistance(lat1, lon1, lat2, lon2);
                                
                                // Vitesse selon le type de route
                                double speed = getDefaultSpeed(currentWayType);
                                
                                // Créer des arêtes BIDIRECTIONNELLES (aller et retour)
                                roadGraph->addEdge(nodeMap[prevNodeId], nodeMap[nodeId], 
                                                 length, speed, highwayTypeToString(currentWayType));
                                roadGraph->addEdge(nodeMap[nodeId], nodeMap[prevNodeId], 
                                                 length, speed, highwayTypeToString(currentWayType));
                            }
                        }
                    }
                }
                
                currentWayId = 0;
            }
        }
    }
    
    file.close();
    
    // Vérifier les erreurs de parsing XML
    if (xml.hasError()) {
        LOG_ERROR(QString("XML parsing error: %1").arg(xml.errorString()));
        // au lieu de planter generer le graphe de test
        return generateTestGraph(roadGraph);
    }
    
    // Compter les nœuds et arêtes dans le graphe
    int nodeCount = roadGraph->getNodeCount();
    int edgeCount = roadGraph->getEdgeCount();
    
    // Vérifier si le graphe est vide juste pour la sécurité
    if (nodeCount == 0) {
        LOG_WARNING("No valid road data found in OSM file - generating test data");
        return generateTestGraph(roadGraph);
    }
    
    LOG_INFO(QString("OSM file parsed successfully: %1 nodes, %2 edges").arg(nodeCount).arg(edgeCount));

    // Construire l'index spatial pour recherches rapides
    // structure de donnee pour accelerer la recherche du noeud le plus proche
    // struvture de donne pour trouver rapidement le noeud le plus proche
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

} // namespace data
} // namespace v2v
