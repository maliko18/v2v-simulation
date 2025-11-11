#include <QApplication>
#include "visualization/MainWindow.hpp"
#include "utils/Logger.hpp"
#include "utils/Config.hpp"

int main(int argc, char *argv[]) {
    // Initialisation Qt
    QApplication app(argc, argv);
    app.setOrganizationName("V2V");
    app.setApplicationName("V2V Simulator");
    app.setApplicationVersion("1.0.0");
    
    // Configuration logging
    v2v::utils::Logger::instance().setLogLevel(v2v::utils::Logger::Level::Info);
    v2v::utils::Logger::instance().enableConsole(true);
    v2v::utils::Logger::instance().setLogFile("v2v_simulator.log");
    
    LOG_INFO("========================================");
    LOG_INFO("V2V Simulator - Starting");
    LOG_INFO("========================================");
    
    // Charger configuration
    v2v::utils::Config& config = v2v::utils::Config::instance();
    if (config.load("../config/mulhouse.json")) {
        LOG_INFO("Configuration loaded successfully");
    } else {
        LOG_WARNING("Could not load config, using defaults");
    }
    
    // Créer fenêtre principale
    v2v::visualization::MainWindow mainWindow;
    mainWindow.setWindowTitle("V2V Simulator - Mulhouse");
    mainWindow.resize(1920, 1080);
    mainWindow.show();
    
    LOG_INFO("Main window created and shown");
    LOG_INFO("Application ready");
    
    // Boucle événements Qt
    int ret = app.exec();
    
    LOG_INFO("Application exiting");
    return ret;
}
