#include "visualization/MapView.hpp"
#include "visualization/MapRenderer.hpp"
#include "visualization/VehicleRenderer.hpp"
#include "visualization/GraphOverlay.hpp"
#include "core/SimulationEngine.hpp"
#include "network/RoadGraph.hpp"
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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace v2v {
namespace visualization {

MapView::MapView(QWidget* parent)
    : QWidget(parent)
    , m_engine(nullptr)
    , m_centerLat(47.7508)  // Mulhouse
    , m_centerLon(7.3359)
    , m_zoomLevel(13)
    , m_offset(0, 0)
    , m_scale(1.0)
    , m_isDragging(false)
    , m_showVehicles(true)
    , m_showConnections(false)  // Désactivé par défaut pour meilleures performances
    , m_showRoadGraph(false)
    , m_vsyncEnabled(false)
    , m_antialiasingEnabled(false)  // Désactivé par défaut pour meilleures performances
    , m_frameCount(0)
    , m_lastFPSTime(0)
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
    
    // Précharger les tuiles autour de Mulhouse au démarrage
    m_tileManager->preloadArea(m_centerLat, m_centerLon, m_zoomLevel, 2);
    
    LOG_INFO("MapView created with OSM tile support");
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
        
        // Compteur pour limiter le nombre de véhicules dessinés (optimisation)
        int drawnVehicles = 0;
        const int maxDrawnVehicles = 500;  // Ne dessiner que les 500 premiers véhicules visibles
        
        // Dessiner les zones de transmission en premier (si connexions activées)
        if (m_showConnections) {
            painter.setPen(Qt::NoPen);
            for (const auto& vehicle : vehicles) {
                if (!vehicle->isActive()) continue;
                
                QPointF screenPos = latLonToScreen(vehicle->getLatitude(), vehicle->getLongitude());
                
                // Vérifier si visible
                if (screenPos.x() < -100 || screenPos.x() > width() + 100 ||
                    screenPos.y() < -100 || screenPos.y() > height() + 100) {
                    continue;
                }
                
                // Rayon de transmission (converti en pixels)
                double radiusMeters = vehicle->getTransmissionRadius();
                double radiusPixels = radiusMeters * std::pow(2.0, m_zoomLevel) / 156543.03392 / std::cos(vehicle->getLatitude() * M_PI / 180.0);
                
                // Zone semi-transparente
                painter.setBrush(QColor(100, 200, 255, 30));
                painter.drawEllipse(screenPos, radiusPixels, radiusPixels);
            }
        }
        
        // Dessiner les véhicules par-dessus
        painter.setPen(QPen(QColor(0, 0, 0), 2));
        painter.setBrush(QColor(255, 50, 50));
        
        for (const auto& vehicle : vehicles) {
            if (!vehicle->isActive()) continue;
            if (drawnVehicles >= maxDrawnVehicles) break;  // Limite atteinte
            
            QPointF screenPos = latLonToScreen(vehicle->getLatitude(), vehicle->getLongitude());
            
            // Vérifier si le véhicule est visible à l'écran (frustum culling)
            if (screenPos.x() < -50 || screenPos.x() > width() + 50 ||
                screenPos.y() < -50 || screenPos.y() > height() + 50) {
                continue;
            }
            
            drawnVehicles++;
            
            // Dessiner le véhicule comme un cercle avec direction
            painter.save();
            painter.translate(screenPos);
            painter.rotate(vehicle->getDirection() * 180.0 / M_PI);
            
            // Corps du véhicule (rectangle arrondi)
            painter.drawEllipse(QPointF(0, 0), 5, 5);
            
            // Indicateur de direction (petit triangle)
            painter.setBrush(QColor(255, 255, 0));
            QPolygonF arrow;
            arrow << QPointF(0, -8) << QPointF(-3, -3) << QPointF(3, -3);
            painter.drawPolygon(arrow);
            
            painter.restore();
        }
    }
    
    // Dessiner le graphe routier si activé
    if (m_showRoadGraph && m_engine) {
        auto* roadGraph = m_engine->getRoadGraph();
        if (roadGraph && roadGraph->getNodeCount() > 0) {
            const auto& graph = roadGraph->getGraph();
            
            // Dessiner les arêtes (routes) - couleur bleu foncé bien visible
            painter.setPen(QPen(QColor(0, 0, 255, 200), 3));
            auto [ei, ei_end] = boost::edges(graph);
            for (auto it = ei; it != ei_end; ++it) {
                auto source = boost::source(*it, graph);
                auto target = boost::target(*it, graph);
                
                const auto& nodeSource = graph[source];
                const auto& nodeTarget = graph[target];
                
                QPointF p1 = latLonToScreen(nodeSource.latitude, nodeSource.longitude);
                QPointF p2 = latLonToScreen(nodeTarget.latitude, nodeTarget.longitude);
                
                // Ne dessiner que les arêtes visibles
                if ((p1.x() >= -50 && p1.x() <= width() + 50 && p1.y() >= -50 && p1.y() <= height() + 50) ||
                    (p2.x() >= -50 && p2.x() <= width() + 50 && p2.y() >= -50 && p2.y() <= height() + 50)) {
                    painter.drawLine(p1, p2);
                }
            }
            
            // Dessiner les nœuds (intersections) - jaune bien visible
            painter.setPen(QPen(QColor(0, 0, 0), 2));
            painter.setBrush(QColor(255, 255, 0, 255));
            auto [vi, vi_end] = boost::vertices(graph);
            for (auto it = vi; it != vi_end; ++it) {
                const auto& node = graph[*it];
                QPointF p = latLonToScreen(node.latitude, node.longitude);
                
                // Ne dessiner que les nœuds visibles
                if (p.x() >= -10 && p.x() <= width() + 10 && p.y() >= -10 && p.y() <= height() + 10) {
                    painter.drawEllipse(p, 4, 4);
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
    
    // FPS counter (calcul toutes les secondes)
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - m_lastFPSTime >= 1000) {
        double fps = m_frameCount * 1000.0 / (currentTime - m_lastFPSTime);
        m_lastFPSTime = currentTime;
        m_frameCount = 0;
        
        // Afficher FPS dans le coin
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 150));
        painter.drawRoundedRect(width() - 90, height() - 35, 85, 30, 5, 5);
        
        painter.setPen(fps >= 55 ? QColor(0, 255, 0) : (fps >= 30 ? QColor(255, 165, 0) : QColor(255, 0, 0)));
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.drawText(width() - 80, height() - 13, QString("FPS: %1").arg(static_cast<int>(fps)));
    }
    
    m_frameCount++;
}

void MapView::renderVehicles() {
    // Cette fonction n'est plus utilisée, le rendu se fait dans paintEvent
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
