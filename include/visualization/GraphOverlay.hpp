#pragma once

#include <QOpenGLFunctions>
#include <vector>
#include <utility>

namespace v2v {

// Forward declarations
namespace network { class InterferenceGraph; }

namespace visualization {

/**
 * @brief Overlay pour afficher les connexions V2V
 * 
 * Affiche:
 * - Lignes entre véhicules connectés
 * - Zones de transmission (cercles)
 * - Statistiques de connexion
 */
class GraphOverlay : protected QOpenGLFunctions {
public:
    GraphOverlay();
    ~GraphOverlay();
    
    void initialize();
    void render(const network::InterferenceGraph* graph,
                int zoomLevel);
    
    void cleanup();
    
    // Options d'affichage
    void setShowConnections(bool show);
    void setShowTransmissionZones(bool show);
    void setConnectionColor(float r, float g, float b, float a);
    void setMaxConnectionsToShow(int max); // Limite pour performance

private:
    void renderConnections(const std::vector<std::pair<int, int>>& connections);
    void renderTransmissionZones();
    
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace visualization
} // namespace v2v
