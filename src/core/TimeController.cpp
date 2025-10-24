#include "core/TimeController.hpp"
#include <QDateTime>

namespace v2v {
namespace core {

TimeController::TimeController(QObject* parent)
    : QObject(parent)
    , m_timeScale(1.0)
    , m_isPaused(false)
    , m_lastUpdateTime(QDateTime::currentMSecsSinceEpoch())
{
}

void TimeController::setTimeScale(double scale) {
    m_timeScale = std::clamp(scale, 0.1, 10.0);
    emit timeScaleChanged(m_timeScale);
}

void TimeController::pause() {
    if (!m_isPaused) {
        m_isPaused = true;
        emit paused();
    }
}

void TimeController::resume() {
    if (m_isPaused) {
        m_isPaused = false;
        m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
        emit resumed();
    }
}

double TimeController::getDeltaTime() const {
    if (m_isPaused) {
        return 0.0;
    }
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    double delta = (currentTime - m_lastUpdateTime) / 1000.0 * m_timeScale;
    return delta;
}

} // namespace core
} // namespace v2v
