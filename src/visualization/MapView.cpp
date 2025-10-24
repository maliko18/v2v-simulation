#include "visualization/MapView.hpp"
#include "visualization/MapRenderer.hpp"
#include "visualization/VehicleRenderer.hpp"
#include "visualization/GraphOverlay.hpp"
#include "core/SimulationEngine.hpp"
#include "data/TileManager.hpp"
#include "utils/Logger.hpp"
#include <QPainter>
#include <QPaintEvent>
#include <QPointF>
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
    , m_showConnections(true)
    , m_showRoadGraph(false)
    , m_vsyncEnabled(false)
    , m_antialiasingEnabled(true)
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
    
    LOG_INFO("MapView created with OSM tile support");
}

MapView::~MapView() {
    LOG_INFO("MapView destroyed");
}

void MapView::setSimulationEngine(core::SimulationEngine* engine) {
    m_engine = engine;
}

void MapView::setCenter(double latitude, double longitude) {
    m_centerLat = latitude;
    m_centerLon = longitude;
    update();
}

void MapView::setZoomLevel(int level) {
    m_zoomLevel = std::clamp(level, 0, 19);
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
    }
    
    // Fond de carte OSM (bleu clair comme l'eau)
    painter.fillRect(rect(), QColor(170, 211, 223));
    
    // Dessiner les tuiles OSM
    drawOSMTiles(painter);
    
    // Dessiner les véhicules
    if (m_showVehicles && m_engine) {
        const auto& vehicles = m_engine->getVehicles();
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 100, 230));
        
        for (const auto& vehicle : vehicles) {
            QPointF pos = vehicle->getPosition();
            QPointF screenPos = latLonToScreen(pos.y(), pos.x()); // lat, lon
            painter.drawEllipse(screenPos, 4, 4);
        }
    }
    
    // UI overlay (pas affecté par pan/zoom)
    painter.setPen(Qt::white);
    painter.setBrush(QColor(0, 0, 0, 150));
    painter.drawRect(5, 5, 200, 95);
    
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 11));
    painter.drawText(10, 25, QString("Zoom: %1").arg(m_zoomLevel));
    painter.drawText(10, 45, QString("Lat: %1").arg(m_centerLat, 0, 'f', 5));
    painter.drawText(10, 65, QString("Lon: %1").arg(m_centerLon, 0, 'f', 5));
    painter.drawText(10, 85, QString("Mulhouse, France"));
    
    m_frameCount++;
}

void MapView::renderVehicles() {
    // Cette fonction n'est plus utilisée, le rendu se fait dans paintEvent
}

void MapView::drawOSMTiles(QPainter& painter) {
    // Calculer quelles tuiles sont visibles
    // Formule OpenStreetMap: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
    
    int zoom = m_zoomLevel;
    double n = std::pow(2.0, zoom);
    
    // Convertir le centre en coordonnées de tuile
    double centerTileX = (m_centerLon + 180.0) / 360.0 * n;
    double centerTileY = (1.0 - std::log(std::tan(m_centerLat * M_PI / 180.0) + 
                          1.0 / std::cos(m_centerLat * M_PI / 180.0)) / M_PI) / 2.0 * n;
    
    // Calculer combien de tuiles sont visibles
    int tilesX = (width() / 256) + 2;  // +2 pour les bords
    int tilesY = (height() / 256) + 2;
    
    // Dessiner les tuiles
    for (int dx = -tilesX/2; dx <= tilesX/2; dx++) {
        for (int dy = -tilesY/2; dy <= tilesY/2; dy++) {
            int tileX = static_cast<int>(centerTileX) + dx;
            int tileY = static_cast<int>(centerTileY) + dy;
            
            // Vérifier les limites
            if (tileX < 0 || tileX >= n || tileY < 0 || tileY >= n) {
                continue;
            }
            
            // Obtenir la tuile (télécharge si nécessaire)
            QPixmap tile = m_tileManager->getTile(zoom, tileX, tileY);
            
            if (!tile.isNull()) {
                // Calculer la position d'affichage
                double screenX = width() / 2.0 + (tileX - centerTileX) * 256.0;
                double screenY = height() / 2.0 + (tileY - centerTileY) * 256.0;
                
                painter.drawPixmap(static_cast<int>(screenX), 
                                 static_cast<int>(screenY), 
                                 tile);
            } else {
                // Tuile en cours de téléchargement - afficher un placeholder
                double screenX = width() / 2.0 + (tileX - centerTileX) * 256.0;
                double screenY = height() / 2.0 + (tileY - centerTileY) * 256.0;
                
                painter.fillRect(static_cast<int>(screenX), 
                               static_cast<int>(screenY), 
                               256, 256, 
                               QColor(230, 230, 230));
                
                painter.setPen(QColor(150, 150, 150));
                painter.drawText(static_cast<int>(screenX) + 100, 
                               static_cast<int>(screenY) + 128, 
                               "Loading...");
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
        update();  // Force un dernier update avec antialiasing
    }
}

void MapView::wheelEvent(QWheelEvent* event) {
    // Sauvegarder la position de la souris en lat/lon
    QPointF mousePos = event->position();
    auto [oldLat, oldLon] = screenToLatLon(mousePos);
    
    // Changer le zoom
    int delta = event->angleDelta().y();
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
    
    update();
}

void MapView::onSimulationUpdate() {
    update(); // Trigger repaint
}

} // namespace visualization
} // namespace v2v
