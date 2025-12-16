#include "visualization/MapView.hpp"
#include "core/SimulationEngine.hpp"
#include "network/RoadGraph.hpp"
#include "network/InterferenceGraph.hpp"
#include "data/TileManager.hpp"
#include "utils/Logger.hpp"
#include <QPainter>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QPointF>
#include <QDateTime>
#include <boost/graph/graph_traits.hpp>
#include <cmath>
#include <algorithm>
#include <unordered_map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace v2v {
namespace visualization {

MapView::MapView(QWidget* parent)
    : QWidget(parent)
    , m_engine(nullptr)
    , m_centerLat(47.75)  // Centre de Mulhouse
    , m_centerLon(7.34)
    , m_zoomLevel(13)  // Zoom plus rapproché pour Mulhouse
    , m_offset(0, 0)
    , m_scale(1.0)
    , m_isDragging(false)
    , m_showVehicles(true)
    , m_showConnections(false)  // Désactivé par défaut pour performance (activer avec 'C')
    , m_showRoadGraph(false)
    , m_showTransmissionRadius(true)  // Cercles bleus activés par défaut (toggle avec 'T')
    , m_vsyncEnabled(false)
    , m_antialiasingEnabled(false)  // Désactivé par défaut pour meilleures performances
    , m_selectedVehicleId(-1)  // Aucun véhicule sélectionné au départ
{
    // Configuration du widget pour performance optimale
    setMinimumSize(800, 600);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // Optimisations de rendu
    setAttribute(Qt::WA_OpaquePaintEvent);  // Pas besoin de clear background
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_PaintOnScreen, false);
    
    // Initialiser le gestionnaire de tuiles OSM
    m_tileManager = std::make_unique<data::TileManager>("osm_cache");
    
    // Connecter le signal de téléchargement pour rafraîchir l'affichage
    connect(m_tileManager.get(), &data::TileManager::tileDownloaded,
            this, [this](int, int, int) { 
                if (!m_isDragging) {  // Ne pas update pendant le drag
                    update(); 
                }
            });
    
    // Précharger les tuiles autour du centre de Mulhouse au démarrage
    m_tileManager->preloadArea(m_centerLat, m_centerLon, m_zoomLevel, 3);
    
    LOG_INFO("MapView created with OSM tile support (Mulhouse)");
}

MapView::~MapView() {
    LOG_INFO("MapView destroyed");
}

void MapView::setSimulationEngine(core::SimulationEngine* engine) {
    m_engine = engine;
}

void MapView::setCenter(double latitude, double longitude) {
    m_centerLat = std::clamp(latitude, -85.0511, 85.0511);  // Limites Web Mercator
    m_centerLon = std::fmod(longitude + 180.0, 360.0) - 180.0;  // Normaliser -180 à 180
    
    // Précharger les tuiles autour de la nouvelle position
    m_tileManager->preloadArea(m_centerLat, m_centerLon, m_zoomLevel, 2);
    
    update();
}

void MapView::setZoomLevel(int level) {
    int oldZoom = m_zoomLevel;
    m_zoomLevel = std::clamp(level, 0, 19);
    
    // Précharger si le zoom a changé
    if (oldZoom != m_zoomLevel) {
        m_tileManager->preloadArea(m_centerLat, m_centerLon, m_zoomLevel, 2);
    }
    
    update();
}

void MapView::setShowVehicles(bool show) {
    m_showVehicles = show;
    update();
}

void MapView::setShowConnections(bool show) {
    m_showConnections = show;
    update();
}

void MapView::setShowRoadGraph(bool show) {
    m_showRoadGraph = show;
    update();
}

void MapView::setVSync(bool enabled) {
    m_vsyncEnabled = enabled;
}

void MapView::setAntialiasing(bool enabled) {
    m_antialiasingEnabled = enabled;
}

void MapView::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    
    // Antialiasing (désactivé pendant le drag pour plus de performance)
    if (m_antialiasingEnabled && !m_isDragging) {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::TextAntialiasing);
    }
    
    // Fond de carte OSM (bleu clair comme l'eau)
    painter.fillRect(rect(), QColor(170, 211, 223));
    
    // Dessiner les tuiles OSM
    drawOSMTiles(painter);
    
    // Dessiner les véhicules si activé
    if (m_showVehicles && m_engine) {
        const auto& vehicles = m_engine->getVehicles();
        
        // Pré-calculer la zone visible pour le culling
        const double margin = 150.0;  // Marge en pixels
        const double minX = -margin;
        const double maxX = width() + margin;
        const double minY = -margin;
        const double maxY = height() + margin;
        
        // Première passe : identifier les véhicules visibles uniquement
        std::vector<std::pair<core::Vehicle*, QPointF>> visibleVehicles;
        
        // ADAPTATIF : Limite basée sur le nombre total de véhicules
        // Augmenté pour afficher tous les véhicules à tous les niveaux de zoom
        size_t maxVisible = (vehicles.size() > 2000) ? 2000 : 5000;

        visibleVehicles.reserve(maxVisible);
        
        for (const auto& vehicle : vehicles) {
            if (!vehicle->isActive()) continue;
            
            QPointF screenPos = latLonToScreen(vehicle->getLatitude(), vehicle->getLongitude());
            
            // Culling: ignorer si hors écran
            if (screenPos.x() < minX || screenPos.x() > maxX ||
                screenPos.y() < minY || screenPos.y() > maxY) {
                continue;
            }
            
            visibleVehicles.emplace_back(vehicle.get(), screenPos);
            
            // Limite stricte pour performances (adaptative)
            if (visibleVehicles.size() >= maxVisible) break;
        }
        
        // Créer une map pour lookup rapide: vehicleId -> screenPos
        std::unordered_map<int, QPointF> vehicleIdToScreenPos;
        vehicleIdToScreenPos.reserve(visibleVehicles.size());
        for (const auto& [vehicle, screenPos] : visibleVehicles) {
            vehicleIdToScreenPos[vehicle->getId()] = screenPos;
        }
        
        // Dessiner les rayons de transmission - TOUJOURS VISIBLE si activé
        if (m_showTransmissionRadius) {
            painter.setPen(QPen(QColor(100, 150, 255, 80), 1.5));
            painter.setBrush(QColor(100, 150, 255, 30));

            for (const auto& [vehicle, screenPos] : visibleVehicles) {
                int radiusMeters = vehicle->getTransmissionRadius();
                double radiusPixels = metersToPixels(radiusMeters, vehicle->getLatitude());
                painter.drawEllipse(screenPos, radiusPixels, radiusPixels);
            }
        }
        
        // Dessiner les connexions V2V - TOUJOURS VISIBLE si activé
        if (m_showConnections) {
            auto* interferenceGraph = m_engine->getInterferenceGraph();
            if (interferenceGraph) {
                // Limite adaptative basée sur le nombre de véhicules
                const size_t maxConnectionsToDraw = (visibleVehicles.size() > 200) ? 500 : 1000;
                size_t connectionsDrawn = 0;
                
                // Dessiner les lignes de connexion (plus épaisses)
                painter.setPen(QPen(QColor(0, 255, 0, 150), 2.0));  // Vert, ligne plus épaisse
                
                // Parcourir seulement les véhicules visibles pour éviter de traiter toutes les connexions
                for (const auto& [vehicle1, screenPos1] : visibleVehicles) {
                    if (connectionsDrawn >= maxConnectionsToDraw) break;
                    
                    int id1 = vehicle1->getId();
                    auto neighbors = interferenceGraph->getNeighbors(id1);
                    
                    for (int id2 : neighbors) {
                        if (connectionsDrawn >= maxConnectionsToDraw) break;
                        
                        // Éviter les doublons (ne dessiner que si id1 < id2)
                        if (id1 >= id2) continue;
                        
                        auto it2 = vehicleIdToScreenPos.find(id2);
                        if (it2 != vehicleIdToScreenPos.end()) {
                            // Ne dessiner que si la distance à l'écran n'est pas trop grande
                            QPointF screenPos2 = it2->second;
                            double screenDist = std::sqrt(std::pow(screenPos1.x() - screenPos2.x(), 2) + 
                                                         std::pow(screenPos1.y() - screenPos2.y(), 2));
                            
                            // Limiter la longueur des lignes dessinées (max 500 pixels)
                            if (screenDist < 500.0) {
                                painter.drawLine(screenPos1, screenPos2);
                                connectionsDrawn++;
                            }
                        }
                    }
                }
            }
        }
        
        // Dessiner les véhicules - rectangles colorés (4 couleurs différentes)
        // Taille du véhicule adaptée au zoom (réduite de 20%)
        double vehicleWidth, vehicleHeight;
        if (m_zoomLevel <= 10) {
            vehicleWidth = 9.6;   // 12 * 0.8
            vehicleHeight = 6.4;  // 8 * 0.8
        } else if (m_zoomLevel <= 12) {
            vehicleWidth = 11.2;  // 14 * 0.8
            vehicleHeight = 7.2;  // 9 * 0.8
        } else if (m_zoomLevel <= 14) {
            vehicleWidth = 12.8;  // 16 * 0.8
            vehicleHeight = 8.0;  // 10 * 0.8
        } else {
            vehicleWidth = 14.4;  // 18 * 0.8
            vehicleHeight = 9.6;  // 12 * 0.8
        }

        // 4 couleurs : noir, gris, blanc, bleu nuit
        static const QColor vehicleColors[4] = {
            QColor(20, 20, 20),      // Noir
            QColor(128, 128, 128),   // Gris
            QColor(245, 245, 245),   // Blanc
            QColor(25, 25, 112)      // Bleu nuit (Midnight Blue)
        };

        for (const auto& [vehicle, screenPos] : visibleVehicles) {
            // Couleur basée sur l'ID du véhicule (mod 4)
            QColor color = vehicleColors[vehicle->getId() % 4];
            
            // Direction du véhicule (radians)
            double direction = vehicle->getDirection();
            
            // Vérifier si ce véhicule est sélectionné
            bool isSelected = (vehicle->getId() == m_selectedVehicleId);
            
            // Sauvegarder l'état du painter
            painter.save();
            
            // Translater et tourner pour orienter le rectangle
            painter.translate(screenPos);
            painter.rotate(direction * 180.0 / M_PI);  // Convertir radians en degrés
            
            // Dessiner le rectangle avec contour (plus épais si sélectionné)
            if (isSelected) {
                // Véhicule sélectionné : contour jaune épais + halo
                painter.setPen(QPen(QColor(255, 215, 0), 3));  // Or
                painter.setBrush(color);
                painter.drawRect(-vehicleWidth/2 - 2, -vehicleHeight/2 - 2, vehicleWidth + 4, vehicleHeight + 4);
            } else {
                painter.setPen(QPen(Qt::black, 1.5));
                painter.setBrush(color);
                painter.drawRect(-vehicleWidth/2, -vehicleHeight/2, vehicleWidth, vehicleHeight);
            }
            
            // Restaurer l'état du painter
            painter.restore();
        }
        
        // Dessiner les informations du véhicule sélectionné
        if (m_selectedVehicleId >= 0) {
            drawVehicleInfo(painter);
        }
    }
    
    // Dessiner le graphe routier si activé
    if (m_showRoadGraph && m_engine) {
        auto* roadGraph = m_engine->getRoadGraph();
        if (roadGraph && roadGraph->getNodeCount() > 0) {
            const auto& graph = roadGraph->getGraph();
            
            // Calculer la bounding box visible pour le culling
            const double margin = 100.0;
            const double minX = -margin;
            const double maxX = width() + margin;
            const double minY = -margin;
            const double maxY = height() + margin;
            
            // Style adaptatif selon le zoom
            QColor roadColor;
            int roadWidth;
            int maxEdgesToDraw;
            
            if (m_zoomLevel < 12) {
                // Zoom faible : routes principales seulement, fines
                roadColor = QColor(0, 0, 255, 150);
                roadWidth = 2;
                maxEdgesToDraw = 1000;
            } else if (m_zoomLevel < 14) {
                // Zoom moyen : plus de routes, moyennes
                roadColor = QColor(0, 0, 255, 180);
                roadWidth = 2;
                maxEdgesToDraw = 3000;
            } else {
                // Zoom élevé : toutes les routes, épaisses
                roadColor = QColor(0, 0, 255, 220);
                roadWidth = 3;
                maxEdgesToDraw = 10000;  // Pas de limite pratique
            }
            
            // Dessiner les arêtes (routes)
            painter.setPen(QPen(roadColor, roadWidth));
            
            int drawnEdges = 0;
            auto [ei, ei_end] = boost::edges(graph);
            for (auto it = ei; it != ei_end && drawnEdges < maxEdgesToDraw; ++it) {
                auto source = boost::source(*it, graph);
                auto target = boost::target(*it, graph);
                
                const auto& nodeSource = graph[source];
                const auto& nodeTarget = graph[target];
                
                QPointF p1 = latLonToScreen(nodeSource.latitude, nodeSource.longitude);
                QPointF p2 = latLonToScreen(nodeTarget.latitude, nodeTarget.longitude);
                
                // Culling: ne dessiner que si au moins un point est visible
                bool p1Visible = (p1.x() >= minX && p1.x() <= maxX && p1.y() >= minY && p1.y() <= maxY);
                bool p2Visible = (p2.x() >= minX && p2.x() <= maxX && p2.y() >= minY && p2.y() <= maxY);
                
                if (p1Visible || p2Visible) {
                    painter.drawLine(p1, p2);
                    drawnEdges++;
                }
            }
            
            // Dessiner les nœuds (intersections) - adaptatif selon zoom
            if (m_zoomLevel >= 13) {  // Réduit de 14 à 13 pour afficher plus tôt
                painter.setPen(QPen(QColor(0, 0, 0), 1));
                painter.setBrush(QColor(255, 200, 0, 255));  // Jaune-orange
                
                int drawnNodes = 0;
                int maxNodes = 500;
                int nodeSize = 3;
                
                // Adapter selon le zoom
                if (m_zoomLevel >= 15) {
                    maxNodes = 2000;
                    nodeSize = 4;
                } else if (m_zoomLevel >= 14) {
                    maxNodes = 1000;
                    nodeSize = 3;
                }
                
                auto [vi, vi_end] = boost::vertices(graph);
                for (auto it = vi; it != vi_end && drawnNodes < maxNodes; ++it) {
                    const auto& node = graph[*it];
                    QPointF p = latLonToScreen(node.latitude, node.longitude);
                    
                    // Culling: ne dessiner que les nœuds visibles
                    if (p.x() >= minX && p.x() <= maxX && p.y() >= minY && p.y() <= maxY) {
                        painter.drawEllipse(p, nodeSize, nodeSize);
                        drawnNodes++;
                    }
                }
            }
        }
    }
    
    // UI overlay (pas affecté par pan/zoom)
    painter.setRenderHint(QPainter::Antialiasing, false);
    
    // Fond semi-transparent pour les infos
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 180));
    painter.drawRoundedRect(5, 5, 250, 110, 5, 5);
    
    // Texte des informations
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(15, 25, "📍 CARTE OSM");
    
    painter.setFont(QFont("Arial", 10));
    painter.drawText(15, 45, QString("Zoom: %1 (molette)").arg(m_zoomLevel));
    painter.drawText(15, 63, QString("Lat: %1").arg(m_centerLat, 0, 'f', 5));
    painter.drawText(15, 81, QString("Lon: %1").arg(m_centerLon, 0, 'f', 5));
    painter.drawText(15, 99, QString("📍 Mulhouse, France"));
    
    // Contrôles
    painter.setPen(QColor(180, 180, 180));
    painter.setFont(QFont("Arial", 9));
    QString controls = "🖱️ Clic: pan | Molette: zoom | ⌨️ Flèches/+/- | H: home | V: véhicules | C: connexions | R: routes";
    painter.drawText(10, height() - 10, controls);
}


void MapView::drawOSMTiles(QPainter& painter) {
    // Calculer quelles tuiles sont visibles
    // Formule OpenStreetMap: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
    
    int zoom = m_zoomLevel;
    int n = 1 << zoom;  // 2^zoom, plus rapide que pow
    
    // Convertir le centre en coordonnées de tuile (projection Web Mercator)
    double centerTileX = (m_centerLon + 180.0) / 360.0 * n;
    double latRad = m_centerLat * M_PI / 180.0;
    double centerTileY = (1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / M_PI) / 2.0 * n;
    
    // Calculer combien de tuiles sont visibles (avec marge)
    int tilesX = (width() / 256) + 3;  // +3 pour assurer couverture complète
    int tilesY = (height() / 256) + 3;
    
    // Offset fractionnaire pour un déplacement fluide
    double offsetX = (centerTileX - std::floor(centerTileX)) * 256.0;
    double offsetY = (centerTileY - std::floor(centerTileY)) * 256.0;
    
    // Dessiner les tuiles
    for (int dx = -tilesX/2; dx <= tilesX/2; dx++) {
        for (int dy = -tilesY/2; dy <= tilesY/2; dy++) {
            int tileX = static_cast<int>(std::floor(centerTileX)) + dx;
            int tileY = static_cast<int>(std::floor(centerTileY)) + dy;
            
            // Vérifier les limites (avec wrapping horizontal)
            if (tileX < 0) tileX += n;
            if (tileX >= n) tileX -= n;
            if (tileY < 0 || tileY >= n) {
                continue;
            }
            
            // Calculer la position d'affichage précise avec sub-pixel
            double screenX = width() / 2.0 + dx * 256.0 - offsetX;
            double screenY = height() / 2.0 + dy * 256.0 - offsetY;
            
            // Obtenir la tuile (télécharge si nécessaire)
            QPixmap tile = m_tileManager->getTile(zoom, tileX, tileY);
            
            if (!tile.isNull()) {
                // Dessiner la tuile avec transformation précise
                painter.drawPixmap(QRectF(screenX, screenY, 256, 256), tile, tile.rect());
            } else {
                // Tuile en cours de téléchargement - afficher un placeholder
                painter.fillRect(QRectF(screenX, screenY, 256, 256), QColor(240, 240, 240));
                
                // Grille pour montrer les limites de tuile
                painter.setPen(QPen(QColor(200, 200, 200), 1));
                painter.drawRect(QRectF(screenX, screenY, 256, 256));
                
                // Texte "Loading..."
                painter.setPen(QColor(150, 150, 150));
                painter.setFont(QFont("Arial", 10));
                painter.drawText(QRectF(screenX, screenY, 256, 256), 
                               Qt::AlignCenter, 
                               QString("Loading...\n%1/%2/%3").arg(zoom).arg(tileX).arg(tileY));
            }
        }
    }
}

QPointF MapView::latLonToScreen(double lat, double lon) const {
    // Projection Web Mercator (utilisée par OSM)
    // https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
    
    double zoom = std::pow(2.0, m_zoomLevel);
    
    // Convertir lon en x (0-256 pour une tuile)
    double x = (lon + 180.0) / 360.0 * 256.0 * zoom;
    
    // Convertir lat en y avec projection Mercator
    double latRad = lat * M_PI / 180.0;
    double y = (1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / M_PI) / 2.0 * 256.0 * zoom;
    
    // Centrer sur l'écran
    double centerX = (m_centerLon + 180.0) / 360.0 * 256.0 * zoom;
    double centerLatRad = m_centerLat * M_PI / 180.0;
    double centerY = (1.0 - std::log(std::tan(centerLatRad) + 1.0 / std::cos(centerLatRad)) / M_PI) / 2.0 * 256.0 * zoom;
    
    return QPointF(
        width() / 2.0 + (x - centerX),
        height() / 2.0 + (y - centerY)
    );
}

std::pair<double, double> MapView::screenToLatLon(const QPointF& screen) const {
    // Inverse de la projection Web Mercator
    
    double zoom = std::pow(2.0, m_zoomLevel);
    
    // Récupérer le centre en coordonnées Mercator
    double centerX = (m_centerLon + 180.0) / 360.0 * 256.0 * zoom;
    double centerLatRad = m_centerLat * M_PI / 180.0;
    double centerY = (1.0 - std::log(std::tan(centerLatRad) + 1.0 / std::cos(centerLatRad)) / M_PI) / 2.0 * 256.0 * zoom;
    
    // Convertir écran vers Mercator
    double x = centerX + (screen.x() - width() / 2.0);
    double y = centerY + (screen.y() - height() / 2.0);
    
    // Mercator vers lat/lon
    double lon = x / (256.0 * zoom) * 360.0 - 180.0;
    double n = M_PI - 2.0 * M_PI * y / (256.0 * zoom);
    double lat = 180.0 / M_PI * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
    
    return {lat, lon};
}

double MapView::metersToPixels(double meters, double latitude) const {
    // Convertir des mètres en pixels
    // Utiliser la même logique que InterferenceGraph pour cohérence
    
    // 1 degré de latitude = 111320 mètres (constant)
    const double metersPerDegreeLat = 111320.0;
    
    // Convertir mètres en degrés
    double degrees = meters / metersPerDegreeLat;
    
    // Convertir degrés en pixels selon le niveau de zoom
    // Au niveau z, il y a 2^z tuiles de 256 pixels sur 360°
    double n = std::pow(2.0, m_zoomLevel);
    double pixelsPerDegree = (256.0 * n) / 360.0;
    
    return degrees * pixelsPerDegree;
}

void MapView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Essayer de sélectionner un véhicule d'abord
        int vehicleId = findVehicleAtPosition(event->pos());
        if (vehicleId >= 0) {
            m_selectedVehicleId = vehicleId;
            update();
            return;  // Ne pas commencer le drag si on clique sur un véhicule
        }
        
        // Sinon, commencer le drag
        m_isDragging = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    } else if (event->button() == Qt::RightButton) {
        // Désélectionner avec clic droit
        m_selectedVehicleId = -1;
        update();
    }
}

void MapView::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        
        // Mettre à jour le centre de la carte en fonction du déplacement
        double zoom = std::pow(2.0, m_zoomLevel);
        double pixelsPerDegree = 256.0 * zoom / 360.0;
        
        // Déplacer le centre (inverser X car on déplace la carte, pas la vue)
        m_centerLon -= delta.x() / pixelsPerDegree;
        
        // Pour latitude, prendre en compte la projection Mercator
        double centerLatRad = m_centerLat * M_PI / 180.0;
        double centerY = (1.0 - std::log(std::tan(centerLatRad) + 1.0 / std::cos(centerLatRad)) / M_PI) / 2.0 * 256.0 * zoom;
        centerY -= delta.y();  // Inverser Y car l'axe Y de l'écran va vers le bas
        double n = M_PI - 2.0 * M_PI * centerY / (256.0 * zoom);
        m_centerLat = 180.0 / M_PI * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
        
        m_lastMousePos = event->pos();
        update();
    }
}

void MapView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        setCursor(Qt::ArrowCursor);
        
        // Précharger les tuiles autour de la nouvelle position
        m_tileManager->preloadArea(m_centerLat, m_centerLon, m_zoomLevel, 2);
        
        update();  // Force un dernier update avec antialiasing
    }
}

void MapView::wheelEvent(QWheelEvent* event) {
    // Sauvegarder la position de la souris en lat/lon
    QPointF mousePos = event->position();
    auto [oldLat, oldLon] = screenToLatLon(mousePos);
    
    // Changer le zoom
    int delta = event->angleDelta().y();
    int oldZoom = m_zoomLevel;
    
    if (delta > 0 && m_zoomLevel < 19) {
        setZoomLevel(m_zoomLevel + 1);
    } else if (delta < 0 && m_zoomLevel > 1) {
        setZoomLevel(m_zoomLevel - 1);
    }
    
    // Ajuster le centre pour que le point sous la souris reste au même endroit
    // (zoom vers la position de la souris)
    auto [newLat, newLon] = screenToLatLon(mousePos);
    m_centerLat += (oldLat - newLat);
    m_centerLon += (oldLon - newLon);
    
    // Précharger les tuiles au nouveau niveau de zoom si changé
    if (oldZoom != m_zoomLevel) {
        m_tileManager->preloadArea(m_centerLat, m_centerLon, m_zoomLevel, 2);
    }
    
    update();
}

void MapView::onSimulationUpdate() {
    update(); // Trigger repaint
}

void MapView::keyPressEvent(QKeyEvent* event) {
    const double panSpeed = 0.01; // Degrés de latitude/longitude
    bool needsUpdate = false;
    
    switch (event->key()) {
        // Navigation véhicules avec flèches (si pas de modificateur) ou Page Up/Down
        case Qt::Key_Left:
            if (event->modifiers() == Qt::NoModifier && m_selectedVehicleId >= 0) {
                selectPreviousVehicle();
                needsUpdate = true;
            } else {
                m_centerLon -= panSpeed;
                needsUpdate = true;
            }
            break;
        case Qt::Key_Right:
            if (event->modifiers() == Qt::NoModifier && m_selectedVehicleId >= 0) {
                selectNextVehicle();
                needsUpdate = true;
            } else {
                m_centerLon += panSpeed;
                needsUpdate = true;
            }
            break;
        case Qt::Key_Up:
            if (event->modifiers() == Qt::ShiftModifier) {
                selectPreviousVehicle();
            } else {
                m_centerLat += panSpeed;
            }
            needsUpdate = true;
            break;
        case Qt::Key_Down:
            if (event->modifiers() == Qt::ShiftModifier) {
                selectNextVehicle();
            } else {
                m_centerLat -= panSpeed;
            }
            needsUpdate = true;
            break;
            
        // Page Up/Down pour naviguer entre véhicules
        case Qt::Key_PageUp:
            selectPreviousVehicle();
            needsUpdate = true;
            break;
        case Qt::Key_PageDown:
            selectNextVehicle();
            needsUpdate = true;
            break;
            
        // Escape pour désélectionner
        case Qt::Key_Escape:
            m_selectedVehicleId = -1;
            needsUpdate = true;
            break;
            
        // Zoom avec + / -
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            if (m_zoomLevel < 19) {
                setZoomLevel(m_zoomLevel + 1);
                needsUpdate = true;
            }
            break;
        case Qt::Key_Minus:
            if (m_zoomLevel > 1) {
                setZoomLevel(m_zoomLevel - 1);
                needsUpdate = true;
            }
            break;
            
        // Retour à Mulhouse avec 'H' (Home)
        case Qt::Key_H:
            setCenter(47.7508, 7.3359);
            setZoomLevel(13);
            needsUpdate = true;
            break;
            
        // Toggle affichage véhicules avec 'V'
        case Qt::Key_V:
            setShowVehicles(!m_showVehicles);
            needsUpdate = true;
            break;
            
        // Toggle affichage connexions avec 'C'
        case Qt::Key_C:
            setShowConnections(!m_showConnections);
            needsUpdate = true;
            break;
            
        // Toggle affichage graphe routier avec 'R'
        case Qt::Key_R:
            setShowRoadGraph(!m_showRoadGraph);
            needsUpdate = true;
            break;
            
        // Toggle cercles de transmission avec 'T'
        case Qt::Key_T:
            m_showTransmissionRadius = !m_showTransmissionRadius;
            LOG_INFO(m_showTransmissionRadius ? 
                "Transmission radius circles: ON" : "Transmission radius circles: OFF");
            needsUpdate = true;
            break;
            
        // Toggle antialiasing avec 'A'
        case Qt::Key_A:
            setAntialiasing(!m_antialiasingEnabled);
            needsUpdate = true;
            break;
            
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    
    if (needsUpdate) {
        update();
    }
}

// ============================================================================
// Fonctions de sélection de véhicule
// ============================================================================

int MapView::findVehicleAtPosition(const QPointF& screenPos) const {
    if (!m_engine) return -1;
    
    const auto& vehicles = m_engine->getVehicles();
    const double clickRadius = 20.0;  // Rayon de clic en pixels
    
    for (const auto& vehicle : vehicles) {
        if (!vehicle->isActive()) continue;
        
        QPointF vehicleScreen = latLonToScreen(vehicle->getLatitude(), vehicle->getLongitude());
        double dist = std::sqrt(std::pow(screenPos.x() - vehicleScreen.x(), 2) +
                               std::pow(screenPos.y() - vehicleScreen.y(), 2));
        
        if (dist <= clickRadius) {
            return vehicle->getId();
        }
    }
    
    return -1;  // Aucun véhicule trouvé
}

void MapView::selectNextVehicle() {
    if (!m_engine) return;
    
    const auto& vehicles = m_engine->getVehicles();
    if (vehicles.empty()) return;
    
    if (m_selectedVehicleId < 0) {
        // Aucun véhicule sélectionné, sélectionner le premier visible
        m_selectedVehicleId = vehicles[0]->getId();
        for (const auto& v : vehicles) {
            if (v->getId() == m_selectedVehicleId) {
                setCenter(v->getLatitude(), v->getLongitude());
                break;
            }
        }
        return;
    }
    
    // Trouver la position écran du véhicule actuellement sélectionné
    QPointF currentScreenPos;
    core::Vehicle* currentVehicle = nullptr;
    for (const auto& v : vehicles) {
        if (v->getId() == m_selectedVehicleId) {
            currentVehicle = v.get();
            currentScreenPos = latLonToScreen(v->getLatitude(), v->getLongitude());
            break;
        }
    }
    
    if (!currentVehicle) return;
    
    // Chercher le véhicule le plus proche à DROITE (screenX plus grand)
    int bestId = -1;
    double bestScore = std::numeric_limits<double>::max();
    
    for (const auto& v : vehicles) {
        if (v->getId() == m_selectedVehicleId) continue;
        
        QPointF screenPos = latLonToScreen(v->getLatitude(), v->getLongitude());
        double dx = screenPos.x() - currentScreenPos.x();
        double dy = screenPos.y() - currentScreenPos.y();
        
        // Seulement les véhicules à droite (dx > 0)
        if (dx > 5.0) {  // Au moins 5 pixels à droite
            // Score = distance, mais pénaliser fortement les véhicules trop haut/bas
            double score = std::abs(dx) + std::abs(dy) * 2.0;
            if (score < bestScore) {
                bestScore = score;
                bestId = v->getId();
            }
        }
    }
    
    // Si aucun véhicule à droite, wrap vers le plus à gauche
    if (bestId < 0) {
        double leftMostX = std::numeric_limits<double>::max();
        for (const auto& v : vehicles) {
            QPointF screenPos = latLonToScreen(v->getLatitude(), v->getLongitude());
            if (screenPos.x() < leftMostX) {
                leftMostX = screenPos.x();
                bestId = v->getId();
            }
        }
    }
    
    if (bestId >= 0) {
        m_selectedVehicleId = bestId;
        // Centrer sur le nouveau véhicule
        for (const auto& v : vehicles) {
            if (v->getId() == m_selectedVehicleId) {
                setCenter(v->getLatitude(), v->getLongitude());
                break;
            }
        }
    }
}

void MapView::selectPreviousVehicle() {
    if (!m_engine) return;
    
    const auto& vehicles = m_engine->getVehicles();
    if (vehicles.empty()) return;
    
    if (m_selectedVehicleId < 0) {
        // Aucun véhicule sélectionné, sélectionner le dernier
        m_selectedVehicleId = vehicles.back()->getId();
        for (const auto& v : vehicles) {
            if (v->getId() == m_selectedVehicleId) {
                setCenter(v->getLatitude(), v->getLongitude());
                break;
            }
        }
        return;
    }
    
    // Trouver la position écran du véhicule actuellement sélectionné
    QPointF currentScreenPos;
    core::Vehicle* currentVehicle = nullptr;
    for (const auto& v : vehicles) {
        if (v->getId() == m_selectedVehicleId) {
            currentVehicle = v.get();
            currentScreenPos = latLonToScreen(v->getLatitude(), v->getLongitude());
            break;
        }
    }
    
    if (!currentVehicle) return;
    
    // Chercher le véhicule le plus proche à GAUCHE (screenX plus petit)
    int bestId = -1;
    double bestScore = std::numeric_limits<double>::max();
    
    for (const auto& v : vehicles) {
        if (v->getId() == m_selectedVehicleId) continue;
        
        QPointF screenPos = latLonToScreen(v->getLatitude(), v->getLongitude());
        double dx = currentScreenPos.x() - screenPos.x();
        double dy = screenPos.y() - currentScreenPos.y();
        
        // Seulement les véhicules à gauche (dx > 0 signifie screenPos.x < current)
        if (dx > 5.0) {  // Au moins 5 pixels à gauche
            // Score = distance, mais pénaliser fortement les véhicules trop haut/bas
            double score = std::abs(dx) + std::abs(dy) * 2.0;
            if (score < bestScore) {
                bestScore = score;
                bestId = v->getId();
            }
        }
    }
    
    // Si aucun véhicule à gauche, wrap vers le plus à droite
    if (bestId < 0) {
        double rightMostX = -std::numeric_limits<double>::max();
        for (const auto& v : vehicles) {
            QPointF screenPos = latLonToScreen(v->getLatitude(), v->getLongitude());
            if (screenPos.x() > rightMostX) {
                rightMostX = screenPos.x();
                bestId = v->getId();
            }
        }
    }
    
    if (bestId >= 0) {
        m_selectedVehicleId = bestId;
        // Centrer sur le nouveau véhicule
        for (const auto& v : vehicles) {
            if (v->getId() == m_selectedVehicleId) {
                setCenter(v->getLatitude(), v->getLongitude());
                break;
            }
        }
    }
}

void MapView::drawVehicleInfo(QPainter& painter) {
    if (!m_engine || m_selectedVehicleId < 0) return;
    
    // Trouver le véhicule sélectionné
    const auto& vehicles = m_engine->getVehicles();
    core::Vehicle* selectedVehicle = nullptr;
    
    for (const auto& v : vehicles) {
        if (v->getId() == m_selectedVehicleId) {
            selectedVehicle = v.get();
            break;
        }
    }
    
    if (!selectedVehicle) {
        m_selectedVehicleId = -1;  // Véhicule n'existe plus
        return;
    }
    
    // Obtenir les voisins connectés
    auto* interferenceGraph = m_engine->getInterferenceGraph();
    int neighborCount = 0;
    if (interferenceGraph) {
        neighborCount = interferenceGraph->getNeighbors(m_selectedVehicleId).size();
    }
    
    // Dessiner le panneau d'information en bas à droite
    const int panelWidth = 250;
    const int panelHeight = 160;
    const int panelX = width() - panelWidth - 15;
    const int panelY = height() - panelHeight - 15;
    
    // Fond semi-transparent
    painter.setPen(QPen(QColor(50, 50, 50), 2));
    painter.setBrush(QColor(0, 0, 0, 200));
    painter.drawRoundedRect(panelX, panelY, panelWidth, panelHeight, 8, 8);
    
    // Titre
    painter.setPen(Qt::white);
    QFont titleFont("Arial", 12, QFont::Bold);
    painter.setFont(titleFont);
    painter.drawText(panelX + 10, panelY + 22, QString("🚗 Véhicule #%1").arg(m_selectedVehicleId));
    
    // Ligne de séparation
    painter.setPen(QPen(QColor(100, 100, 100), 1));
    painter.drawLine(panelX + 10, panelY + 30, panelX + panelWidth - 10, panelY + 30);
    
    // Informations
    QFont infoFont("Arial", 10);
    painter.setFont(infoFont);
    painter.setPen(QColor(200, 200, 200));
    
    int textY = panelY + 48;
    const int lineHeight = 20;
    
    // Position
    painter.drawText(panelX + 15, textY, 
        QString("📍 Position: %1°, %2°")
            .arg(selectedVehicle->getLatitude(), 0, 'f', 5)
            .arg(selectedVehicle->getLongitude(), 0, 'f', 5));
    textY += lineHeight;
    
    // Vitesse (convertir m/s en km/h)
    double speedKmh = selectedVehicle->getSpeed() * 3.6;
    painter.drawText(panelX + 15, textY, 
        QString("🚀 Vitesse: %1 km/h").arg(speedKmh, 0, 'f', 1));
    textY += lineHeight;
    
    // Direction (convertir radians en degrés)
    double directionDeg = selectedVehicle->getDirection() * 180.0 / M_PI;
    QString directionName;
    if (directionDeg >= -22.5 && directionDeg < 22.5) directionName = "Nord";
    else if (directionDeg >= 22.5 && directionDeg < 67.5) directionName = "Nord-Est";
    else if (directionDeg >= 67.5 && directionDeg < 112.5) directionName = "Est";
    else if (directionDeg >= 112.5 && directionDeg < 157.5) directionName = "Sud-Est";
    else if (directionDeg >= 157.5 || directionDeg < -157.5) directionName = "Sud";
    else if (directionDeg >= -157.5 && directionDeg < -112.5) directionName = "Sud-Ouest";
    else if (directionDeg >= -112.5 && directionDeg < -67.5) directionName = "Ouest";
    else directionName = "Nord-Ouest";
    
    painter.drawText(panelX + 15, textY, 
        QString("🧭 Direction: %1 (%2°)").arg(directionName).arg(directionDeg, 0, 'f', 0));
    textY += lineHeight;
    
    // Rayon de transmission
    painter.drawText(panelX + 15, textY, 
        QString("📡 Rayon: %1 m").arg(selectedVehicle->getTransmissionRadius()));
    textY += lineHeight;
    
    // Connexions V2V
    painter.drawText(panelX + 15, textY, 
        QString("🔗 Connexions: %1").arg(neighborCount));
    
    // Instructions en bas du panneau
    painter.setPen(QColor(120, 120, 120));
    QFont smallFont("Arial", 8);
    painter.setFont(smallFont);
    painter.drawText(panelX + 10, panelY + panelHeight - 8, 
        "←→ changer | Esc désélectionner");
}

} // namespace visualization
} // namespace v2v
