#include "data/TileManager.hpp"
#include "utils/Logger.hpp"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QEventLoop>

namespace v2v {
namespace data {

TileManager::TileManager(const QString& cacheDir, QObject* parent)
    : QObject(parent)
    , m_cacheDir(cacheDir)
    , m_maxCacheSize(500 * 1024 * 1024) // 500 MB
{
    m_networkManager = std::make_unique<QNetworkAccessManager>(this);
    
    // Créer le répertoire de cache s'il n'existe pas
    QDir dir;
    if (!dir.exists(m_cacheDir)) {
        dir.mkpath(m_cacheDir);
        LOG_INFO(QString("Created tile cache directory: %1").arg(m_cacheDir));
    }
    
    LOG_INFO("TileManager initialized with async OSM tile download");
}

TileManager::~TileManager() = default;

QPixmap TileManager::getTile(int zoom, int x, int y) {
    TileCoord coord{zoom, x, y};
    
    // 1. Vérifier le cache mémoire
    auto it = m_memoryCache.find(coord);
    if (it != m_memoryCache.end()) {
        return it->second;
    }
    
    // 2. Vérifier le cache disque
    QPixmap tile;
    if (loadFromDisk(coord, tile)) {
        m_memoryCache[coord] = tile;
        return tile;
    }
    
    // 3. Déclencher téléchargement asynchrone (si pas déjà en cours)
    if (m_pendingDownloads.find(coord) == m_pendingDownloads.end()) {
        downloadTileAsync(coord);
    }
    
    // Retourner QPixmap null - le download se fera en arrière-plan
    return QPixmap();
}

void TileManager::downloadTileAsync(const TileCoord& coord) {
    // URL du serveur de tuiles OSM
    QString url = QString("https://tile.openstreetmap.org/%1/%2/%3.png")
                    .arg(coord.zoom)
                    .arg(coord.x)
                    .arg(coord.y);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "V2V-Simulator/1.0");
    
    // Stocker les coordonnées dans la requête pour les retrouver plus tard
    request.setAttribute(QNetworkRequest::User, coord.zoom);
    request.setAttribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1), coord.x);
    request.setAttribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 2), coord.y);
    
    // Download asynchrone
    QNetworkReply* reply = m_networkManager->get(request);
    m_pendingDownloads[coord] = reply;
    
    // Connecter le signal finished - utiliser QNetworkReply::finished directement
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onDownloadFinished(reply);
    });
}

void TileManager::onDownloadFinished(QNetworkReply* reply) {
    // Récupérer les coordonnées
    int zoom = reply->request().attribute(QNetworkRequest::User).toInt();
    int x = reply->request().attribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1)).toInt();
    int y = reply->request().attribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 2)).toInt();
    TileCoord coord{zoom, x, y};
    
    // Retirer des downloads en cours
    m_pendingDownloads.erase(coord);
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QPixmap tile;
        if (tile.loadFromData(data)) {
            // Sauvegarder en cache
            m_memoryCache[coord] = tile;
            saveToDisk(coord, tile);
            
            LOG_INFO(QString("Downloaded tile %1/%2/%3").arg(zoom).arg(x).arg(y));
            
            // Émettre signal pour redessiner
            emit tileDownloaded(zoom, x, y);
        }
    } else {
        LOG_ERROR(QString("Failed to download tile %1/%2/%3: %4")
                  .arg(zoom).arg(x).arg(y).arg(reply->errorString()));
    }
    
    reply->deleteLater();
}

bool TileManager::loadFromDisk(const TileCoord& coord, QPixmap& tile) {
    QString filename = getTilePath(coord);
    if (QFile::exists(filename)) {
        if (tile.load(filename)) {
            return true;
        }
    }
    return false;
}

void TileManager::saveToDisk(const TileCoord& coord, const QPixmap& tile) {
    QString filename = getTilePath(coord);
    
    // Créer les sous-répertoires
    QFileInfo fileInfo(filename);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    tile.save(filename, "PNG");
}

QString TileManager::getTilePath(const TileCoord& coord) const {
    return QString("%1/%2/%3/%4.png")
            .arg(m_cacheDir)
            .arg(coord.zoom)
            .arg(coord.x)
            .arg(coord.y);
}

void TileManager::preloadArea(double centerLat, double centerLon, int zoom, int radius) {
    Q_UNUSED(centerLat);
    Q_UNUSED(centerLon);
    Q_UNUSED(zoom);
    Q_UNUSED(radius);
    // TODO: Implémenter le préchargement
}

void TileManager::setMaxCacheSize(size_t sizeBytes) {
    m_maxCacheSize = sizeBytes;
}

void TileManager::clearMemoryCache() {
    m_memoryCache.clear();
    LOG_INFO("Memory cache cleared");
}

void TileManager::clearAll() {
    clearMemoryCache();
    // TODO: Supprimer les fichiers du cache disque
}

} // namespace data
} // namespace v2v
