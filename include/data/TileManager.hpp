#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QPixmap>
#include <QString>
#include <memory>
#include <unordered_map>

namespace v2v {
namespace data {

/**
 * @brief Gestionnaire de tuiles OSM avec cache
 * 
 * Architecture:
 * - Cache L1: Mémoire (LRU, ~100 MB)
 * - Cache L2: SQLite disque (~500 MB)
 * - Download: Serveurs OSM
 */
class TileManager : public QObject {
    Q_OBJECT

public:
    struct TileCoord {
        int zoom;
        int x;
        int y;
        
        bool operator==(const TileCoord& other) const {
            return zoom == other.zoom && x == other.x && y == other.y;
        }
    };
    
    struct TileCoordHash {
        std::size_t operator()(const TileCoord& coord) const {
            return std::hash<int>()(coord.zoom) ^
                   (std::hash<int>()(coord.x) << 1) ^
                   (std::hash<int>()(coord.y) << 2);
        }
    };

    explicit TileManager(const QString& cacheDir, QObject* parent = nullptr);
    ~TileManager() override;
    
    /**
     * @brief Obtenir tuile (cache uniquement - ne bloque jamais)
     * @return QPixmap valide si en cache, QPixmap null sinon (déclenche download async)
     */
    QPixmap getTile(int zoom, int x, int y);
    
    /**
     * @brief Précharger tuiles
     */
    void preloadArea(double centerLat, double centerLon, int zoom, int radius);
    
    /**
     * @brief Configuration
     */
    void setMaxCacheSize(size_t sizeBytes);
    
    /**
     * @brief Clear
     */
    void clearMemoryCache();
    void clearAll();

signals:
    /**
     * @brief Émis quand une tuile est téléchargée
     */
    void tileDownloaded(int zoom, int x, int y);

private slots:
    void onDownloadFinished(QNetworkReply* reply);

private:
    void downloadTileAsync(const TileCoord& coord);
    bool loadFromDisk(const TileCoord& coord, QPixmap& tile);
    void saveToDisk(const TileCoord& coord, const QPixmap& tile);
    QString getTilePath(const TileCoord& coord) const;
    
    // Memory cache
    std::unordered_map<TileCoord, QPixmap, TileCoordHash> m_memoryCache;
    
    // Downloading tiles (to avoid duplicate requests)
    std::unordered_map<TileCoord, QNetworkReply*, TileCoordHash> m_pendingDownloads;
    
    // Disk cache
    QString m_cacheDir;
    size_t m_maxCacheSize;
    
    // Network
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
};

} // namespace data
} // namespace v2v
