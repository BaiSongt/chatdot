#include "theme.h"
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QStyleFactory>

ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
    , m_currentTheme(Theme::System)
    , m_settings("ChatDot", "Theme")
{
    loadSavedTheme();
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

QString ThemeManager::currentStyleSheet() const
{
    return m_currentStyleSheet;
}

void ThemeManager::applyTheme()
{
    Theme effectiveTheme = m_currentTheme;
    if (effectiveTheme == Theme::System) {
        effectiveTheme = isSystemDarkMode() ? Theme::Dark : Theme::Light;
    }

    QString themeFile;
    switch (effectiveTheme) {
        case Theme::Light:
            themeFile = ":/themes/light.qss";
            break;
        case Theme::Dark:
            themeFile = ":/themes/dark.qss";
            break;
        default:
            themeFile = ":/themes/light.qss";
            break;
    }

    loadTheme(themeFile);
    qApp->setStyleSheet(m_currentStyleSheet);
}

void ThemeManager::loadTheme(const QString& themeFile)
{
    QFile file(themeFile);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        m_currentStyleSheet = QString::fromUtf8(file.readAll());
        file.close();
    } else {
        LOG_ERROR(QString("无法加载主题文件: %1").arg(themeFile));
    }
}

void ThemeManager::saveTheme()
{
    m_settings.setValue("theme", static_cast<int>(m_currentTheme));
}

void ThemeManager::loadSavedTheme()
{
    int savedTheme = m_settings.value("theme", static_cast<int>(Theme::System)).toInt();
    m_currentTheme = static_cast<Theme>(savedTheme);
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