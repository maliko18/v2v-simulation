#pragma once

#include <QObject>

namespace v2v {
namespace core {

/**
 * @brief Contrôleur du temps de simulation
 * 
 * Gère l'accélération, la pause, et le ralentissement du temps
 */
class TimeController : public QObject {
    Q_OBJECT

public:
    explicit TimeController(QObject* parent = nullptr);
    
    void setTimeScale(double scale);
    double getTimeScale() const { return m_timeScale; }
    
    void pause();
    void resume();
    bool isPaused() const { return m_isPaused; }
    
    double getDeltaTime() const;
    
signals:
    void timeScaleChanged(double scale);
    void paused();
    void resumed();

private:
    double m_timeScale;
    bool m_isPaused;
    qint64 m_lastUpdateTime;
};

} // namespace core
} // namespace v2v
