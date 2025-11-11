#pragma once

#include <QElapsedTimer>
#include <QString>
#include <QHash>
#include <vector>

namespace v2v {
namespace utils {

/**
 * @brief Profiler de performance
 * 
 * Utilisation:
 * PROFILE_SCOPE("function_name");
 */
class Profiler {
public:
    struct Entry {
        QString name;
        qint64 totalTime; // microseconds
        int callCount;
        qint64 minTime;
        qint64 maxTime;
        
        double getAverageMs() const {
            return callCount > 0 ? (totalTime / callCount) / 1000.0 : 0.0;
        }
    };
    
    static Profiler& instance();
    
    void begin(const QString& name);
    void end(const QString& name);
    
    const QHash<QString, Entry>& getEntries() const;
    void reset();
    void printReport() const;
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

private:
    Profiler() : m_enabled(true) {}
    
    QHash<QString, Entry> m_entries;
    QHash<QString, QElapsedTimer> m_activeTimers;
    bool m_enabled;
};

/**
 * @brief RAII scope profiler
 */
class ProfileScope {
public:
    explicit ProfileScope(const QString& name) : m_name(name) {
        if (Profiler::instance().isEnabled()) {
            Profiler::instance().begin(m_name);
        }
    }
    
    ~ProfileScope() {
        if (Profiler::instance().isEnabled()) {
            Profiler::instance().end(m_name);
        }
    }

private:
    QString m_name;
};

// Macro pour profiling facile
#define PROFILE_SCOPE(name) v2v::utils::ProfileScope _profiler_scope(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)

} // namespace utils
} // namespace v2v
