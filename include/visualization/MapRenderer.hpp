#pragma once

#include <QObject>
#include <QImage>
#include <QPixmap>
#include <memory>

namespace v2v {
namespace visualization {

/**
 * @brief Gestionnaire de tuiles OSM
 * 
 * Fonctionnalités:
 * - Téléchargement tuiles depuis serveurs OSM
 * - Cache disque SQLite
 * - Cache mémoire LRU
 * - Chargement asynchrone
 */
class TileManager : public QObject {
    Q_OBJECT

public:
    explicit TileManager(QObject* parent = nullptr);
    ~TileManager() override;
    
    /**
     * @brief Obtenir une tuile (depuis cache ou télécharge)
     * @param zoom Niveau de zoom (0-19)
     * @param x Coordonnée X de la tuile
     * @param y Coordonnée Y de la tuile
     * @return QPixmap de la tuile, null si pas encore chargée
     */
    QPixmap getTile(int zoom, int x, int y);
    
    /**
     * @brief Précharger des tuiles autour d'une position
     */
    void preloadTiles(double lat, double lon, int zoom, int radius);
    
    /**
     * @brief Clear cache
     */
    void clearCache();
    
    /**
     * @brief Statistiques
     */
    size_t getCacheSize() const;
    double getCacheHitRate() const;

signals:
    void tileLoaded(int zoom, int x, int y);
    void tileLoadFailed(int zoom, int x, int y);

private:
    struct TileKey {
        int zoom, x, y;
        bool operator==(const TileKey& other) const;
    };
    
    struct TileKeyHash {
        std::size_t operator()(const TileKey& k) const;
    };
    
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Renderer de carte OSM
 */
class MapRenderer {
public:
    MapRenderer();
    ~MapRenderer();
    
    void render(double centerLat, double centerLon, int zoom, 
                int screenWidth, int screenHeight);
    
    void setTileManager(TileManager* manager);

private:
    TileManager* m_tileManager;
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace visualization
} // namespace v2v
