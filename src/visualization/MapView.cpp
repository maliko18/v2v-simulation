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
    , m_centerLat(48.08)  // Centre de l'Alsace (Colmar)
    , m_centerLon(7.36)
    , m_zoomLevel(10)  // Zoom réduit pour voir toute l'Alsace
    , m_offset(0, 0)
    , m_scale(1.0)
    , m_isDragging(false)
    , m_showVehicles(true)
    , m_showConnections(false)  // Désactivé par défaut pour performance (activer avec 'C')
    , m_showRoadGraph(false)
    , m_showTransmissionRadius(true)  // Cercles bleus activés par défaut (toggle avec 'T')
    , m_vsyncEnabled(false)
    , m_antialiasingEnabled(false)  // Désactivé par défaut pour meilleures performances
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
    
    // Précharger les tuiles autour du centre de l'Alsace au démarrage
    m_tileManager->preloadArea(m_centerLat, m_centerLon, m_zoomLevel, 3);
    
    LOG_INFO("MapView created with OSM tile support (Alsace region)");
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
        
        // AUGMENTÉ : Afficher TOUS les véhicules si pas de lag
        size_t maxVisible = 2000;  // Afficher jusqu'à 2000 véhicules
        
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
        for (const auto& [vehicle, screenPos] : visibleVehicles) {
            vehicleIdToScreenPos[vehicle->getId()] = screenPos;
        }
        
        // Dessiner les rayons de transmission (cercles autour des véhicules)
        if (m_showTransmissionRadius) {
            for (const auto& [vehicle, screenPos] : visibleVehicles) {
                int radiusMeters = vehicle->getTransmissionRadius();
                double radiusPixels = metersToPixels(radiusMeters, vehicle->getLatitude());
                
                // Dessiner le cercle de rayon avec une couleur semi-transparente
                painter.setPen(QPen(QColor(100, 150, 255, 80), 1.5));  // Bleu clair semi-transparent
                painter.setBrush(QColor(100, 150, 255, 30));  // Remplissage très transparent
                painter.drawEllipse(screenPos, radiusPixels, radiusPixels);
            }
        }
        
        // Dessiner les connexions V2V (edges entre véhicules connectés)
        // OPTIMISÉ: Limiter le nombre de connexions dessinées pour performance
        if (m_showConnections && visibleVehicles.size() < 500) {  // Seulement si < 500 véhicules visibles
            auto* interferenceGraph = m_engine->getInterferenceGraph();
            if (interferenceGraph) {
                // Limiter le nombre de connexions à dessiner (max 2000 pour performance)
                const size_t maxConnectionsToDraw = 2000;
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
        
        // Dessiner les véhicules ULTRA-SIMPLIFIÉ pour supporter 2000 véhicules
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 50, 50));
        
        for (const auto& [vehicle, screenPos] : visibleVehicles) {
            // Simple cercle sans rotation ni flèche
            painter.drawEllipse(screenPos, 4, 4);
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
    
    // Compteur de véhicules si activé
    if (m_showVehicles && m_engine) {
        int vehicleCount = m_engine->getActiveVehicleCount();
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 150, 0, 180));
        painter.drawRoundedRect(width() - 155, 5, 150, 40, 5, 5);
        
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 11, QFont::Bold));
        painter.drawText(width() - 145, 25, QString("🚗 %1 véhicules").arg(vehicleCount));
    }
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
    // Convertir des mètres en pixels à la latitude donnée
    // À l'équateur: 1 degré de latitude ≈ 111,320 mètres
    // La projection Web Mercator conserve les distances à l'équateur mais les étire aux pôles
    
    // Approximation: utiliser la latitude moyenne pour le calcul
    // Plus précis: utiliser la latitude exacte du véhicule
    const double metersPerDegreeLat = 111320.0;  // À l'équateur
    const double metersPerDegreeLon = 111320.0 * std::cos(latitude * M_PI / 180.0);
    
    // Utiliser la moyenne pour un cercle approximatif
    double avgMetersPerDegree = (metersPerDegreeLat + metersPerDegreeLon) / 2.0;
    
    // Convertir mètres en degrés
    double degrees = meters / avgMetersPerDegree;
    
    // Convertir degrés en pixels selon le zoom
    double zoom = std::pow(2.0, m_zoomLevel);
    double pixelsPerDegree = 256.0 * zoom / 360.0;
    
    return degrees * pixelsPerDegree;
}

void MapView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
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
        // Déplacement avec flèches
        case Qt::Key_Left:
            m_centerLon -= panSpeed;
            needsUpdate = true;
            break;
        case Qt::Key_Right:
            m_centerLon += panSpeed;
            needsUpdate = true;
            break;
        case Qt::Key_Up:
            m_centerLat += panSpeed;
            needsUpdate = true;
            break;
        case Qt::Key_Down:
            m_centerLat -= panSpeed;
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

} // namespace visualization
} // namespace v2v
