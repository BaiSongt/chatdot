#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QCoreApplication>

class Logger : public QObject
{
    Q_OBJECT

public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };
    Q_ENUM(Level)

    static Logger& instance()
    {
        static Logger instance;
        return instance;
    }

    void init();
    void setLogLevel(Level level) { m_level = level; }
    Level logLevel() const { return m_level; }

    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);

signals:
    void logMessage(Level level, const QString& message);

private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void writeLog(Level level, const QString& message);
    QString levelToString(Level level) const;

    Level m_level = Level::Debug;
    QFile m_logFile;
    QTextStream m_logStream;
    static bool m_initialized;
};

// 便捷宏
#define LOG_DEBUG(msg) Logger::instance().debug(msg)
#define LOG_INFO(msg) Logger::instance().info(msg)
#define LOG_WARNING(msg) Logger::instance().warning(msg)
#define LOG_ERROR(msg) Logger::instance().error(msg)

#endif // LOGGER_H 