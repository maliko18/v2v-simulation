#pragma once

#include <QOpenGLFunctions>
#include <vector>
#include <memory>

namespace v2v {

// Forward declaration
namespace core { class Vehicle; }

namespace visualization {

/**
 * @brief Renderer optimisé pour afficher les véhicules
 * 
 * Optimisations:
 * - Instanced rendering (tous véhicules en 1 draw call)
 * - Level of Detail (LOD): points si zoom loin, polygones si proche
 * - Frustum culling: ne dessiner que véhicules visibles
 * - Batching par état
 */
class VehicleRenderer : protected QOpenGLFunctions {
public:
    VehicleRenderer();
    ~VehicleRenderer();
    
    void initialize();
    void render(const std::vector<std::shared_ptr<core::Vehicle>>& vehicles,
                int zoomLevel, const QRectF& viewportBounds);
    
    void cleanup();
    
    // Options
    void setLODEnabled(bool enabled);
    void setCullingEnabled(bool enabled);

private:
    void renderAsPoints(const std::vector<std::shared_ptr<core::Vehicle>>& vehicles);
    void renderAsPolygons(const std::vector<std::shared_ptr<core::Vehicle>>& vehicles);
    bool isInViewport(const core::Vehicle& vehicle, const QRectF& viewport) const;
    
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace visualization
} // namespace v2v
