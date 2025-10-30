#include "visualization/MainWindow.hpp"
#include "visualization/MapView.hpp"
#include "core/SimulationEngine.hpp"
#include "network/RoadGraph.hpp"
#include "data/OSMParser.hpp"
#include "utils/Logger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QMenuBar>
#include <QMenu>
#include <QLabel>
#include <QFrame>

namespace v2v {
namespace visualization {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_mapView(new MapView(this))
    , m_engine(new core::SimulationEngine(this))
    , m_isSimulationRunning(false)
{
    LOG_INFO("MainWindow constructing...");
    
    createUI();
    createToolbar();
    createStatusBar();
    createMenuBar();
    connectSignals();
    loadSettings();
    
    // Link engine to view
    m_mapView->setSimulationEngine(m_engine);
    
    LOG_INFO("MainWindow constructed");
}

MainWindow::~MainWindow() {
    saveSettings();
}

void MainWindow::createUI() {
    // Créer le widget central avec layout horizontal
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Créer le panneau de contrôle à gauche
    QWidget* leftPanel = new QWidget(this);
    leftPanel->setMinimumWidth(250);
    leftPanel->setMaximumWidth(300);
    leftPanel->setStyleSheet("QWidget { background-color: #2b2b2b; color: white; }");
    
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(15);
    leftLayout->setContentsMargins(10, 10, 10, 10);
    
    // Titre du panneau
    QLabel* titleLabel = new QLabel("SIMULATION CONTROLS", leftPanel);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #4CAF50;");
    titleLabel->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(titleLabel);
    
    // Séparateur
    QFrame* line1 = new QFrame(leftPanel);
    line1->setFrameShape(QFrame::HLine);
    line1->setStyleSheet("background-color: #555;");
    leftLayout->addWidget(line1);
    
    // Boutons de contrôle
    m_btnStart = new QPushButton("▶ Start Simulation", leftPanel);
    m_btnStart->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 10px; font-size: 13px; border-radius: 5px; }"
                              "QPushButton:hover { background-color: #45a049; }");
    leftLayout->addWidget(m_btnStart);
    
    m_btnPause = new QPushButton("⏸ Pause", leftPanel);
    m_btnPause->setEnabled(false);
    m_btnPause->setStyleSheet("QPushButton { background-color: #FFC107; color: black; padding: 8px; border-radius: 5px; }"
                              "QPushButton:hover { background-color: #FFB300; }"
                              "QPushButton:disabled { background-color: #555; color: #888; }");
    leftLayout->addWidget(m_btnPause);
    
    m_btnStop = new QPushButton("⏹ Stop", leftPanel);
    m_btnStop->setEnabled(false);
    m_btnStop->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 8px; border-radius: 5px; }"
                            "QPushButton:hover { background-color: #da190b; }"
                            "QPushButton:disabled { background-color: #555; color: #888; }");
    leftLayout->addWidget(m_btnStop);
    
    m_btnReset = new QPushButton("↻ Reset", leftPanel);
    m_btnReset->setStyleSheet("QPushButton { background-color: #607D8B; color: white; padding: 8px; border-radius: 5px; }"
                             "QPushButton:hover { background-color: #546E7A; }");
    leftLayout->addWidget(m_btnReset);
    
    // Bouton pour afficher/masquer les routes
    QPushButton* btnToggleRoads = new QPushButton("🛣️ Afficher Routes", leftPanel);
    btnToggleRoads->setCheckable(true);
    btnToggleRoads->setStyleSheet("QPushButton { background-color: #9C27B0; color: white; padding: 8px; border-radius: 5px; }"
                                 "QPushButton:hover { background-color: #7B1FA2; }"
                                 "QPushButton:checked { background-color: #4CAF50; }");
    connect(btnToggleRoads, &QPushButton::toggled, [this](bool checked) {
        m_mapView->setShowRoadGraph(checked);
    });
    leftLayout->addWidget(btnToggleRoads);
    
    // Séparateur
    QFrame* line2 = new QFrame(leftPanel);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("background-color: #555;");
    leftLayout->addWidget(line2);
    
    // Speed control
    QLabel* speedLabel = new QLabel("SIMULATION SPEED", leftPanel);
    speedLabel->setStyleSheet("font-weight: bold; color: #4CAF50;");
    leftLayout->addWidget(speedLabel);
    
    m_timeScaleSlider = new QSlider(Qt::Horizontal, leftPanel);
    m_timeScaleSlider->setMinimum(1);
    m_timeScaleSlider->setMaximum(100);
    m_timeScaleSlider->setValue(10);
    m_timeScaleSlider->setStyleSheet("QSlider::groove:horizontal { background: #555; height: 6px; border-radius: 3px; }"
                                    "QSlider::handle:horizontal { background: #4CAF50; width: 16px; margin: -5px 0; border-radius: 8px; }");
    leftLayout->addWidget(m_timeScaleSlider);
    
    m_timeScaleLabel = new QLabel("1.0x", leftPanel);
    m_timeScaleLabel->setAlignment(Qt::AlignCenter);
    m_timeScaleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #4CAF50;");
    leftLayout->addWidget(m_timeScaleLabel);
    
    // Séparateur
    QFrame* line3 = new QFrame(leftPanel);
    line3->setFrameShape(QFrame::HLine);
    line3->setStyleSheet("background-color: #555;");
    leftLayout->addWidget(line3);
    
    // Vehicle count
    QLabel* vehicleLabel = new QLabel("NUMBER OF VEHICLES", leftPanel);
    vehicleLabel->setStyleSheet("font-weight: bold; color: #4CAF50;");
    leftLayout->addWidget(vehicleLabel);
    
    m_vehicleCountSpinBox = new QSpinBox(leftPanel);
    m_vehicleCountSpinBox->setMinimum(10);
    m_vehicleCountSpinBox->setMaximum(500);  // Réduit de 5000 à 500 pour meilleures performances
    m_vehicleCountSpinBox->setValue(50);  // Réduit de 2000 à 50 pour démarrage rapide
    m_vehicleCountSpinBox->setStyleSheet("QSpinBox { background-color: #3b3b3b; color: white; padding: 8px; border: 1px solid #555; border-radius: 5px; font-size: 14px; }"
                                        "QSpinBox::up-button, QSpinBox::down-button { background-color: #4CAF50; }");
    leftLayout->addWidget(m_vehicleCountSpinBox);
    
    // Séparateur
    QFrame* line4 = new QFrame(leftPanel);
    line4->setFrameShape(QFrame::HLine);
    line4->setStyleSheet("background-color: #555;");
    leftLayout->addWidget(line4);
    
    // Transmission radius
    QLabel* radiusLabel = new QLabel("TRANSMISSION RADIUS", leftPanel);
    radiusLabel->setStyleSheet("font-weight: bold; color: #4CAF50;");
    leftLayout->addWidget(radiusLabel);
    
    m_transmissionRadiusSpinBox = new QSpinBox(leftPanel);
    m_transmissionRadiusSpinBox->setMinimum(100);
    m_transmissionRadiusSpinBox->setMaximum(500);
    m_transmissionRadiusSpinBox->setValue(300);
    m_transmissionRadiusSpinBox->setSuffix(" m");
    m_transmissionRadiusSpinBox->setStyleSheet("QSpinBox { background-color: #3b3b3b; color: white; padding: 8px; border: 1px solid #555; border-radius: 5px; font-size: 14px; }"
                                              "QSpinBox::up-button, QSpinBox::down-button { background-color: #4CAF50; }");
    leftLayout->addWidget(m_transmissionRadiusSpinBox);
    
    // Spacer pour pousser tout vers le haut
    leftLayout->addStretch();
    
    // Ajouter le panneau gauche et la carte au layout principal
    mainLayout->addWidget(leftPanel);
    mainLayout->addWidget(m_mapView, 1);  // stretch factor = 1 pour prendre tout l'espace
    
    setCentralWidget(centralWidget);
    setWindowTitle("V2V Simulator - Mulhouse");
}

void MainWindow::createToolbar() {
    // Ne plus créer de toolbar - tout est dans le panneau gauche maintenant
    // Cette fonction reste vide mais on la garde pour ne pas casser le code
}

void MainWindow::createStatusBar() {
    m_statusFPS = new QLabel("FPS: 0", this);
    m_statusVehicles = new QLabel("Vehicles: 0", this);
    m_statusConnections = new QLabel("Connections: 0", this);
    m_statusSimTime = new QLabel("Time: 0.0s", this);
    
    statusBar()->addWidget(m_statusFPS);
    statusBar()->addWidget(new QLabel(" | ", this));
    statusBar()->addWidget(m_statusVehicles);
    statusBar()->addWidget(new QLabel(" | ", this));
    statusBar()->addWidget(m_statusConnections);
    statusBar()->addWidget(new QLabel(" | ", this));
    statusBar()->addWidget(m_statusSimTime);
}

void MainWindow::createMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Load OSM...", this, &MainWindow::onLoadOSMFile);
    fileMenu->addSeparator();
    fileMenu->addAction("&Save Config", this, &MainWindow::onSaveConfiguration);
    fileMenu->addAction("&Load Config", this, &MainWindow::onLoadConfiguration);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close);
}

void MainWindow::connectSignals() {
    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::onStartSimulation);
    connect(m_btnPause, &QPushButton::clicked, this, &MainWindow::onPauseSimulation);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::onStopSimulation);
    connect(m_btnReset, &QPushButton::clicked, this, &MainWindow::onResetSimulation);
    
    connect(m_timeScaleSlider, &QSlider::valueChanged, this, &MainWindow::onTimeScaleChanged);
    connect(m_vehicleCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onVehicleCountChanged);
    connect(m_transmissionRadiusSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onTransmissionRadiusChanged);
    
    // Engine signals
    connect(m_engine, &core::SimulationEngine::fpsChanged,
            [this](int fps) { m_statusFPS->setText(QString("FPS: %1").arg(fps)); });
    // Repaint map view on each simulation tick so vehicles update smoothly
    connect(m_engine, &core::SimulationEngine::tick, m_mapView, qOverload<>(&QWidget::update));
}

void MainWindow::onStartSimulation() {
    LOG_INFO("Starting simulation");
    
    // Create vehicles if not already created
    if (m_engine->getVehicles().empty()) {
        m_engine->setVehicleCount(m_vehicleCountSpinBox->value());
    }
    
    m_engine->start();
    m_isSimulationRunning = true;
    updateControls();
}

void MainWindow::onPauseSimulation() {
    LOG_INFO("Pausing simulation");
    m_engine->pause();
    m_isSimulationRunning = false;
    updateControls();
}

void MainWindow::onStopSimulation() {
    LOG_INFO("Stopping simulation");
    m_engine->stop();
    m_isSimulationRunning = false;
    updateControls();
}

void MainWindow::onResetSimulation() {
    LOG_INFO("Resetting simulation");
    m_engine->reset();
    m_isSimulationRunning = false;
    updateControls();
}

void MainWindow::onTimeScaleChanged(int value) {
    double scale = value / 10.0; // 1-100 -> 0.1-10.0
    m_engine->setTimeScale(scale);
    m_timeScaleLabel->setText(QString("%1x").arg(scale, 0, 'f', 1));
}

void MainWindow::onVehicleCountChanged(int value) {
    if (!m_isSimulationRunning) {
        // Only allow changing when stopped
        LOG_INFO(QString("Vehicle count set to: %1").arg(value));
    }
}

void MainWindow::onTransmissionRadiusChanged(int value) {
    LOG_INFO(QString("Transmission radius set to: %1m").arg(value));
}

void MainWindow::updateStatusBar() {
    // TODO: Update with real data
}

void MainWindow::updateControls() {
    m_btnStart->setEnabled(!m_isSimulationRunning);
    m_btnPause->setEnabled(m_isSimulationRunning);
    m_btnStop->setEnabled(m_isSimulationRunning);
}

void MainWindow::onLoadOSMFile() {
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Load OSM File",
        "../data/osm",
        "OSM Files (*.osm *.osm.pbf);;All Files (*)"
    );
    
    if (!filename.isEmpty()) {
        LOG_INFO(QString("Loading OSM file: %1").arg(filename));
        
        // Utiliser OSMParser pour charger le fichier
        v2v::data::OSMParser parser;
        
        // Filtrer par bounding box de Mulhouse (optionnel)
        // parser.setBoundingBox(47.7, 7.2, 47.8, 7.4);
        
        auto* roadGraph = m_engine->getRoadGraph();
        if (parser.loadFile(filename.toStdString(), roadGraph)) {
            LOG_INFO(QString("OSM file loaded successfully: %1 nodes, %2 edges")
                     .arg(roadGraph->getNodeCount())
                     .arg(roadGraph->getEdgeCount()));
            
            // Recréer les véhicules pour qu'ils utilisent le nouveau graphe routier
            int currentVehicleCount = m_vehicleCountSpinBox->value();
            if (currentVehicleCount > 0) {
                m_engine->setVehicleCount(currentVehicleCount);
                LOG_INFO(QString("Recreated %1 vehicles on road network").arg(currentVehicleCount));
            }
            
            QMessageBox::information(
                this,
                "OSM Loaded",
                QString("Road graph loaded successfully!\n\nNodes: %1\nEdges: %2\nVehicles: %3")
                    .arg(roadGraph->getNodeCount())
                    .arg(roadGraph->getEdgeCount())
                    .arg(currentVehicleCount)
            );
            
            // Rafraîchir l'affichage
            m_mapView->update();
        } else {
            LOG_ERROR("Failed to load OSM file");
            QMessageBox::warning(
                this,
                "Error",
                "Failed to load OSM file. Check the log for details."
            );
        }
    }
}

void MainWindow::onSaveConfiguration() {
    LOG_INFO("Saving configuration");
    // TODO: Save to JSON
}

void MainWindow::onLoadConfiguration() {
    LOG_INFO("Loading configuration");
    // TODO: Load from JSON
}

void MainWindow::loadSettings() {
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::saveSettings() {
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_isSimulationRunning) {
        m_engine->stop();
    }
    saveSettings();
    QMainWindow::closeEvent(event);
}

} // namespace visualization
} // namespace v2v
