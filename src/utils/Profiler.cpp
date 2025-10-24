#include "utils/Profiler.hpp"
#include <QDebug>

namespace v2v {
namespace utils {

Profiler& Profiler::instance() {
    static Profiler instance;
    return instance;
}

void Profiler::begin(const QString& name) {
    if (!m_enabled) return;
    
    m_activeTimers[name].start();
}

void Profiler::end(const QString& name) {
    if (!m_enabled) return;
    
    auto it = m_activeTimers.find(name);
    if (it == m_activeTimers.end()) {
        return;
    }
    
    qint64 elapsed = it.value().nsecsElapsed() / 1000; // Convert to microseconds
    m_activeTimers.erase(it);
    
    // Update or create entry
    auto entryIt = m_entries.find(name);
    if (entryIt == m_entries.end()) {
        Entry entry;
        entry.name = name;
        entry.totalTime = elapsed;
        entry.callCount = 1;
        entry.minTime = elapsed;
        entry.maxTime = elapsed;
        m_entries[name] = entry;
    } else {
        Entry& entry = entryIt.value();
        entry.totalTime += elapsed;
        entry.callCount++;
        entry.minTime = std::min(entry.minTime, elapsed);
        entry.maxTime = std::max(entry.maxTime, elapsed);
    }
}

const QHash<QString, Profiler::Entry>& Profiler::getEntries() const {
    return m_entries;
}

void Profiler::reset() {
    m_entries.clear();
    m_activeTimers.clear();
}

void Profiler::printReport() const {
    qDebug() << "========================================";
    qDebug() << "Performance Profile Report";
    qDebug() << "========================================";
    qDebug() << QString("%1 | %2 | %3 | %4 | %5")
                .arg("Name", -30)
                .arg("Calls", 8)
                .arg("Avg(ms)", 10)
                .arg("Min(ms)", 10)
                .arg("Max(ms)", 10);
    qDebug() << "----------------------------------------";
    
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        const Entry& entry = it.value();
        qDebug() << QString("%1 | %2 | %3 | %4 | %5")
                    .arg(entry.name, -30)
                    .arg(entry.callCount, 8)
                    .arg(entry.getAverageMs(), 10, 'f', 3)
                    .arg(entry.minTime / 1000.0, 10, 'f', 3)
                    .arg(entry.maxTime / 1000.0, 10, 'f', 3);
    }
    
    qDebug() << "========================================";
}

} // namespace utils
} // namespace v2v
