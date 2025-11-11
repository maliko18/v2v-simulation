#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <memory>

// Forward declaration
class QPainter;

namespace v2v {

// Forward declarations
namespace core { class SimulationEngine; }
namespace data { class TileManager; }

namespace visualization {

/**
 * @brief Widget pour affichage carte + véhicules + connexions
 * 
 * Fonctionnalités:
 * - Rendu avec QPainter (2D natif Qt)
 * - Zoom avec molette souris
 * - Pan avec drag souris
 * - Affichage tuiles OSM
 * - Rendu véhicules avec LOD
 * - Affichage connexions V2V
 * - Frustum culling pour performance
 */
class MapView : public QWidget {
    Q_OBJECT

public:
    explicit MapView(QWidget* parent = nullptr);
    ~MapView() override;
    
    void setSimulationEngine(core::SimulationEngine* engine);
    
    // Contrôles de vue
    void setCenter(double latitude, double longitude);
    void setZoomLevel(int level);
    int getZoomLevel() const { return m_zoomLevel; }
    
    // Options d'affichage
    void setShowVehicles(bool show);
    void setShowConnections(bool show);
    void setShowRoadGraph(bool show);
    
    // Performance
    void setVSync(bool enabled);
    void setAntialiasing(bool enabled);

protected:
    // Qt overrides
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    
    // Mouse events
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onSimulationUpdate();

private:
    void drawOSMTiles(QPainter& painter);
    void renderConnections();
    void renderUI();
    
    // Transformation coordonnées
    QPointF latLonToScreen(double lat, double lon) const;
    std::pair<double, double> screenToLatLon(const QPointF& screen) const;
    double metersToPixels(double meters, double latitude) const;
    
    core::SimulationEngine* m_engine;
    
    // OSM Tiles
    std::unique_ptr<data::TileManager> m_tileManager;
    
    // Vue
    double m_centerLat;
    double m_centerLon;
    int m_zoomLevel;
    QPointF m_offset;
    double m_scale;
    
    // Interaction
    bool m_isDragging;
    QPoint m_lastMousePos;
    
    // Options
    bool m_showVehicles;
    bool m_showConnections;
    bool m_showRoadGraph;
    bool m_vsyncEnabled;
    bool m_antialiasingEnabled;
    
    // Performance
    int m_frameCount;
    qint64 m_lastFPSTime;
};

} // namespace visualization
} // namespace v2v
