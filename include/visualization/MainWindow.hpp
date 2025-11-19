#pragma once

#include <QMainWindow>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <memory>

namespace v2v {

// Forward declarations
namespace core { class SimulationEngine; }
namespace visualization { class MapView; }

namespace visualization {

/**
 * @brief Fenêtre principale de l'application
 * 
 * Interface professionnelle avec:
 * - Toolbar avec contrôles de simulation
 * - Vue carte centrale (MapView)
 * - Status bar avec métriques (FPS, véhicules, connexions)
 * - Panneau de configuration
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Contrôles de simulation
    void onStartSimulation();
    void onPauseSimulation();
    void onResetSimulation();
    
    // Paramètres
    void onTimeScaleChanged(int value);
    void onVehicleCountChanged(int value);
    void onTransmissionRadiusChanged(int value);
    
    // Updates
    void updateControls();
    
    // Fichiers
    void onLoadOSMFile();

private:
    void createUI();
    void createToolbar();
    void createStatusBar();
    void createMenuBar();
    void connectSignals();
    void loadSettings();
    void saveSettings();
    
    // Widgets principaux
    MapView* m_mapView;
    core::SimulationEngine* m_engine;
    
    // Toolbar widgets
    QPushButton* m_btnStart;
    QPushButton* m_btnPause;
    QPushButton* m_btnReset;
    QSlider* m_timeScaleSlider;
    QLabel* m_timeScaleLabel;
    QSpinBox* m_vehicleCountSpinBox;
    QSpinBox* m_transmissionRadiusSpinBox;
    
    // Status bar widgets
    QLabel* m_statusFPS;
    QLabel* m_statusVehicles;
    QLabel* m_statusConnections;
    QLabel* m_statusSimTime;
    
    // State
    bool m_isSimulationRunning;
};

} // namespace visualization
} // namespace v2v
