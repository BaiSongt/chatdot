#ifndef SETTINGSVIEWMODEL_H
#define SETTINGSVIEWMODEL_H

#include <QObject>
#include <QString>
#include "models/settingsmodel.h"
#include "services/llmservice.h"
// #include "utils/encryption.h"

class SettingsViewModel : public QObject
{
    Q_OBJECT

public:
    explicit SettingsViewModel(SettingsModel* model, QObject *parent = nullptr);
    ~SettingsViewModel();

    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE LLMService* createLLMService();

signals:
    void settingsChanged();
    void errorOccurred(const QString& error);

private:
    SettingsModel* m_model;
};

#endif // SETTINGSVIEWMODEL_H
