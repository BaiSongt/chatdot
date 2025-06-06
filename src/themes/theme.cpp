#include "theme.h"
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>

ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
    , m_currentTheme(Theme::System)
{
    loadTheme();
}

ThemeManager::~ThemeManager()
{
    saveTheme();
}

void ThemeManager::setTheme(Theme theme)
{
    if (m_currentTheme != theme) {
        m_currentTheme = theme;
        applyTheme();
        saveTheme();
        emit themeChanged(theme);
        LOG_INFO(QString("主题已切换为: %1").arg(static_cast<int>(theme)));
    }
}

ThemeManager::Theme ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

QString ThemeManager::currentStyleSheet() const
{
    return m_currentStyleSheet;
}

QString ThemeManager::getUserBubbleStyle() const
{
    // 根据当前主题返回用户气泡样式类名
    bool isDark = (m_currentTheme == Theme::Dark) || 
                  (m_currentTheme == Theme::System && isSystemDarkMode());
    return isDark ? "user-bubble-dark" : "user-bubble-light";
}

QString ThemeManager::getAIBubbleStyle() const
{
    // 根据当前主题返回AI气泡样式类名
    bool isDark = (m_currentTheme == Theme::Dark) || 
                  (m_currentTheme == Theme::System && isSystemDarkMode());
    return isDark ? "ai-bubble-dark" : "ai-bubble-light";
}

void ThemeManager::applyTheme()
{
    QString themeFile;
    switch (m_currentTheme) {
        case Theme::Light:
            themeFile = "themes/light.qss";
            break;
        case Theme::Dark:
            themeFile = "themes/dark.qss";
            break;
        case Theme::System:
            themeFile = isSystemDarkMode() ? "themes/dark.qss" : "themes/light.qss";
            break;
    }
    
    // 同时加载聊天气泡样式文件
    QString chatBubblesFile = "themes/chat_bubbles.css";

    // 获取应用程序目录
    QString appDir = QCoreApplication::applicationDirPath();
    QString fullPath = QDir(appDir).absoluteFilePath(themeFile);
    QString bubblePath = QDir(appDir).absoluteFilePath(chatBubblesFile);
    LOG_INFO(QString("正在加载主题文件: %1").arg(fullPath));
    LOG_INFO(QString("正在加载聊天气泡样式: %1").arg(bubblePath));

    // 读取主题文件
    QString styleSheet;
    QFile file(fullPath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        styleSheet = QString::fromUtf8(file.readAll());
        file.close();

        // 检查样式表内容是否为空
        if (styleSheet.isEmpty()) {
            LOG_ERROR(QString("主题文件为空: %1").arg(fullPath));
            return;
        }
    } else {
        LOG_ERROR(QString("无法打开主题文件: %1, 错误: %2")
            .arg(fullPath)
            .arg(file.errorString()));
        return;
    }
    
    // 读取聊天气泡样式文件并合并
    QFile bubbleFile(bubblePath);
    if (bubbleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString bubbleStyleSheet = QString::fromUtf8(bubbleFile.readAll());
        bubbleFile.close();
        
        if (!bubbleStyleSheet.isEmpty()) {
            styleSheet += "\n" + bubbleStyleSheet;
            LOG_INFO("成功加载聊天气泡样式");
        }
    } else {
        LOG_WARNING(QString("无法打开聊天气泡样式文件: %1, 错误: %2")
            .arg(bubblePath)
            .arg(bubbleFile.errorString()));
    }

    // 尝试应用样式表
    try {
        qApp->setStyleSheet(styleSheet);
        m_currentStyleSheet = styleSheet;
        LOG_INFO(QString("成功加载主题: %1").arg(themeFile));
    } catch (const std::exception& e) {
        LOG_ERROR(QString("应用主题时发生错误: %1").arg(e.what()));
    }
}

void ThemeManager::saveTheme()
{
    QSettings settings;
    settings.setValue("theme", static_cast<int>(m_currentTheme));
}

void ThemeManager::loadTheme()
{
    QSettings settings;
    m_currentTheme = static_cast<Theme>(settings.value("theme", static_cast<int>(Theme::System)).toInt());
    applyTheme();
}

bool ThemeManager::isSystemDarkMode() const
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
    return settings.value("AppsUseLightTheme", 1).toInt() == 0;
#elif defined(Q_OS_MAC)
    // macOS 系统主题检测
    QProcess process;
    process.start("defaults", QStringList() << "read" << "-g" << "AppleInterfaceStyle");
    process.waitForFinished();
    QString output = process.readAllStandardOutput().trimmed();
    return output == "Dark";
#else
    // Linux 系统主题检测
    QProcess process;
    process.start("gsettings", QStringList() << "get" << "org.gnome.desktop.interface" << "gtk-theme");
    process.waitForFinished();
    QString output = process.readAllStandardOutput().trimmed();
    return output.contains("dark", Qt::CaseInsensitive);
#endif
} 