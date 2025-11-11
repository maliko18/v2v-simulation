#pragma once

#include <QPointF>
#include <cmath>

namespace v2v {
namespace data {

/**
 * @brief Utilitaires géométriques et conversions
 */
class GeometryUtils {
public:
    /**
     * @brief Constantes
     */
    static constexpr double EARTH_RADIUS_M = 6371000.0;
    static constexpr double PI = 3.14159265358979323846;
    
    /**
     * @brief Conversion lat/lon -> Web Mercator (EPSG:3857)
     */
    static QPointF latLonToMercator(double lat, double lon);
    static std::pair<double, double> mercatorToLatLon(const QPointF& mercator);
    
    /**
     * @brief Conversion lat/lon -> coordonnées tuile OSM
     */
    static std::pair<int, int> latLonToTile(double lat, double lon, int zoom);
    static std::pair<double, double> tileToLatLon(int x, int y, int zoom);
    
    /**
     * @brief Distance Haversine entre deux points GPS
     * @return Distance en mètres
     */
    static double haversineDistance(double lat1, double lon1, double lat2, double lon2);
    
    /**
     * @brief Distance euclidienne 2D
     */
    static double euclideanDistance(const QPointF& p1, const QPointF& p2);
    
    /**
     * @brief Bearing (direction) entre deux points GPS
     * @return Angle en radians (0 = Nord, PI/2 = Est)
     */
    static double bearing(double lat1, double lon1, double lat2, double lon2);
    
    /**
     * @brief Point à distance et bearing donnés
     */
    static std::pair<double, double> destinationPoint(
        double lat, double lon, double distance, double bearing);
    
    /**
     * @brief Interpolation linéaire entre deux points
     */
    static QPointF lerp(const QPointF& a, const QPointF& b, double t);
    
    /**
     * @brief Clamp valeur
     */
    static double clamp(double value, double min, double max);
    
    /**
     * @brief Deg <-> Rad
     */
    static double degToRad(double deg) { return deg * PI / 180.0; }
    static double radToDeg(double rad) { return rad * 180.0 / PI; }
};

} // namespace data
} // namespace v2v
