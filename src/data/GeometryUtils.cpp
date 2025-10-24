#include "data/GeometryUtils.hpp"

namespace v2v {
namespace data {

QPointF GeometryUtils::latLonToMercator(double lat, double lon) {
    double x = lon * 20037508.34 / 180.0;
    double y = std::log(std::tan((90.0 + lat) * PI / 360.0)) / (PI / 180.0);
    y = y * 20037508.34 / 180.0;
    return QPointF(x, y);
}

std::pair<double, double> GeometryUtils::mercatorToLatLon(const QPointF& mercator) {
    double lon = (mercator.x() / 20037508.34) * 180.0;
    double lat = (mercator.y() / 20037508.34) * 180.0;
    lat = 180.0 / PI * (2.0 * std::atan(std::exp(lat * PI / 180.0)) - PI / 2.0);
    return {lat, lon};
}

std::pair<int, int> GeometryUtils::latLonToTile(double lat, double lon, int zoom) {
    int n = 1 << zoom;
    int x = static_cast<int>((lon + 180.0) / 360.0 * n);
    double latRad = degToRad(lat);
    int y = static_cast<int>((1.0 - std::asinh(std::tan(latRad)) / PI) / 2.0 * n);
    return {x, y};
}

std::pair<double, double> GeometryUtils::tileToLatLon(int x, int y, int zoom) {
    int n = 1 << zoom;
    double lon = x / static_cast<double>(n) * 360.0 - 180.0;
    double latRad = std::atan(std::sinh(PI * (1.0 - 2.0 * y / static_cast<double>(n))));
    double lat = radToDeg(latRad);
    return {lat, lon};
}

double GeometryUtils::haversineDistance(double lat1, double lon1, double lat2, double lon2) {
    double dLat = degToRad(lat2 - lat1);
    double dLon = degToRad(lon2 - lon1);
    
    lat1 = degToRad(lat1);
    lat2 = degToRad(lat2);
    
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0) * std::cos(lat1) * std::cos(lat2);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    
    return EARTH_RADIUS_M * c;
}

double GeometryUtils::euclideanDistance(const QPointF& p1, const QPointF& p2) {
    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    return std::sqrt(dx * dx + dy * dy);
}

double GeometryUtils::bearing(double lat1, double lon1, double lat2, double lon2) {
    double dLon = degToRad(lon2 - lon1);
    lat1 = degToRad(lat1);
    lat2 = degToRad(lat2);
    
    double y = std::sin(dLon) * std::cos(lat2);
    double x = std::cos(lat1) * std::sin(lat2) - 
               std::sin(lat1) * std::cos(lat2) * std::cos(dLon);
    
    return std::atan2(y, x);
}

std::pair<double, double> GeometryUtils::destinationPoint(
    double lat, double lon, double distance, double bearing) {
    
    double latRad = degToRad(lat);
    double lonRad = degToRad(lon);
    double angularDistance = distance / EARTH_RADIUS_M;
    
    double lat2 = std::asin(std::sin(latRad) * std::cos(angularDistance) +
                            std::cos(latRad) * std::sin(angularDistance) * std::cos(bearing));
    
    double lon2 = lonRad + std::atan2(std::sin(bearing) * std::sin(angularDistance) * std::cos(latRad),
                                       std::cos(angularDistance) - std::sin(latRad) * std::sin(lat2));
    
    return {radToDeg(lat2), radToDeg(lon2)};
}

QPointF GeometryUtils::lerp(const QPointF& a, const QPointF& b, double t) {
    return QPointF(
        a.x() + t * (b.x() - a.x()),
        a.y() + t * (b.y() - a.y())
    );
}

double GeometryUtils::clamp(double value, double min, double max) {
    return std::max(min, std::min(max, value));
}

} // namespace data
} // namespace v2v
