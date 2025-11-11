#include "utils/Logger.hpp"
#include <QMutex>
#include <QMutexLocker>

namespace v2v {
namespace utils {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_level(Level::Info)
    , m_consoleEnabled(true)
{
}

Logger::~Logger() {
    if (m_stream) {
        m_stream->flush();
    }
}

void Logger::setLogLevel(Level level) {
    m_level = level;
}

void Logger::setLogFile(const QString& filename) {
    m_logFile = std::make_unique<QFile>(filename);
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream = std::make_unique<QTextStream>(m_logFile.get());
    } else {
        qWarning() << "Failed to open log file:" << filename;
        m_logFile.reset();
    }
}

void Logger::enableConsole(bool enable) {
    m_consoleEnabled = enable;
}

void Logger::debug(const QString& message) {
    log(Level::Debug, message);
}

void Logger::info(const QString& message) {
    log(Level::Info, message);
}

void Logger::warning(const QString& message) {
    log(Level::Warning, message);
}

void Logger::error(const QString& message) {
    log(Level::Error, message);
}

void Logger::critical(const QString& message) {
    log(Level::Critical, message);
}

void Logger::log(Level level, const QString& message) {
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    
    if (level < m_level) {
        return;
    }
    
    write(level, message);
}

void Logger::write(Level level, const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString fullMessage = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);
    
    // Console output
    if (m_consoleEnabled) {
        if (level >= Level::Error) {
            qCritical().noquote() << fullMessage;
        } else if (level == Level::Warning) {
            qWarning().noquote() << fullMessage;
        } else {
            qDebug().noquote() << fullMessage;
        }
    }
    
    // File output
    if (m_stream) {
        *m_stream << fullMessage << Qt::endl;
        m_stream->flush();
    }
}

QString Logger::levelToString(Level level) const {
    switch (level) {
        case Level::Debug: return "DEBUG";
        case Level::Info: return "INFO ";
        case Level::Warning: return "WARN ";
        case Level::Error: return "ERROR";
        case Level::Critical: return "CRIT ";
        default: return "?????";
    }
}

} // namespace utils
} // namespace v2v
