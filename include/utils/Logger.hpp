#pragma once

#include <QString>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <memory>

namespace v2v {
namespace utils {

/**
 * @brief Logger système avec niveaux
 */
class Logger {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };
    
    static Logger& instance();
    
    void setLogLevel(Level level);
    void setLogFile(const QString& filename);
    void enableConsole(bool enable);
    
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);
    void critical(const QString& message);
    
    // Macro-friendly
    void log(Level level, const QString& message);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void write(Level level, const QString& message);
    QString levelToString(Level level) const;
    
    Level m_level;
    bool m_consoleEnabled;
    std::unique_ptr<QFile> m_logFile;
    std::unique_ptr<QTextStream> m_stream;
};

// Macros pour faciliter l'utilisation
#define LOG_DEBUG(msg) v2v::utils::Logger::instance().debug(msg)
#define LOG_INFO(msg) v2v::utils::Logger::instance().info(msg)
#define LOG_WARNING(msg) v2v::utils::Logger::instance().warning(msg)
#define LOG_ERROR(msg) v2v::utils::Logger::instance().error(msg)
#define LOG_CRITICAL(msg) v2v::utils::Logger::instance().critical(msg)

} // namespace utils
} // namespace v2v
