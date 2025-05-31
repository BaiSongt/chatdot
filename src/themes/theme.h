#ifndef THEME_H
#define THEME_H

#include <QObject>
#include <QString>
#include <QSettings>
#include "services/logger.h"

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Theme currentTheme READ currentTheme WRITE setTheme NOTIFY themeChanged)

public:
    enum class Theme {
        System,
        Light,
        Dark
    };
    Q_ENUM(Theme)

    static ThemeManager& instance();

    Theme currentTheme() const { return m_currentTheme; }
    void setTheme(Theme theme);
    
    // 获取当前主题的样式表
    QString currentStyleSheet() const;
    
    // 应用主题到应用程序
    void applyTheme();

signals:
    void themeChanged(Theme theme);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager();

    void loadTheme(const QString& themeFile);
    void saveTheme();
    void loadSavedTheme();
    bool isSystemDarkMode() const;

    Theme m_currentTheme;
    QString m_currentStyleSheet;
    QSettings m_settings;
};

#endif // THEME_H 