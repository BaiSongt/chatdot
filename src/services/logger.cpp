#include "logger.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QMutex>

bool Logger::m_initialized = false;

Logger::Logger(QObject *parent)
    : QObject(parent)
    , m_level(Level::Debug)
{
}

Logger::~Logger()
{
    if (m_logFile.isOpen()) {
        m_logStream.flush();
        m_logFile.close();
    }
    m_initialized = false;
}

void Logger::init()
{
    if (m_initialized) {
        return;
    }

    // 创建日志目录
    QString logDir = QCoreApplication::applicationDirPath() + "/logs";
    QDir().mkpath(logDir);

    // 设置日志文件
    QString logPath = logDir + "/chatdot_" + 
                     QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log";
    m_logFile.setFileName(logPath);
    
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "无法打开日志文件:" << logPath;
        return;
    }

    m_logStream.setDevice(&m_logFile);
    m_logStream.setEncoding(QStringConverter::Utf8);
    
    m_initialized = true;
    LOG_INFO("日志系统初始化完成");
}

void Logger::writeLog(Level level, const QString& message)
{
    if (!m_initialized || level < m_level) {
        return;
    }

    QString levelStr = levelToString(level);
    QString formattedMessage = QString("[%1] [%2] %3")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
        .arg(levelStr)
        .arg(message);

    // 输出到控制台
    switch (level) {
        case Level::Debug:
            qDebug() << formattedMessage;
            break;
        case Level::Info:
            qInfo() << formattedMessage;
            break;
        case Level::Warning:
            qWarning() << formattedMessage;
            break;
        case Level::Error:
            qCritical() << formattedMessage;
            break;
    }

    // 写入文件
    if (m_logFile.isOpen()) {
        m_logStream << formattedMessage << "\n";
        m_logStream.flush();
    }

    // 发送信号
    emit logMessage(level, message);
}

QString Logger::levelToString(Level level) const
{
    switch (level) {
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO";
        case Level::Warning:
            return "WARNING";
        case Level::Error:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

void Logger::debug(const QString& message)
{
    writeLog(Level::Debug, message);
}

void Logger::info(const QString& message)
{
    writeLog(Level::Info, message);
}

void Logger::warning(const QString& message)
{
    writeLog(Level::Warning, message);
}

void Logger::error(const QString& message)
{
    writeLog(Level::Error, message);
} 