#ifndef THEME_H
#define THEME_H

#include <QObject>
#include <QString>
#include "services/logger.h"

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum class Theme {
        Light,
        Dark,
        System
    };
    Q_ENUM(Theme)

    static ThemeManager& instance();

    Theme currentTheme() const;
    QString currentStyleSheet() const;

public slots:
    void setTheme(Theme theme);

signals:
    void themeChanged(Theme theme);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager();

    void applyTheme();
    void saveTheme();
    void loadTheme();
    bool isSystemDarkMode() const;

    Theme m_currentTheme;
    QString m_currentStyleSheet;
};

#endif // THEME_H 