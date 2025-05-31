#ifndef SETTINGSMODEL_H
#define SETTINGSMODEL_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStringList>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
// #include "utils/encryption.h"

class SettingsModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(ModelType modelType READ modelType WRITE setModelType NOTIFY modelTypeChanged)
    Q_PROPERTY(QString apiUrl READ apiUrl WRITE setApiUrl NOTIFY apiUrlChanged)
    Q_PROPERTY(QString currentModelName READ currentModelName WRITE setCurrentModelName NOTIFY currentModelNameChanged)
    Q_PROPERTY(QStringList ollamaModels READ ollamaModels WRITE setOllamaModels NOTIFY ollamaModelsChanged)

public:
    enum class ModelType {
        API,
        Ollama,
        Local
    };
    Q_ENUM(ModelType)

    static SettingsModel& instance();

    explicit SettingsModel(QObject *parent = nullptr);
    ~SettingsModel();

    QString apiKey() const { return m_apiKey; }
    QString modelPath() const { return m_modelPath; }
    QString apiUrl() const { return m_apiUrl; }
    ModelType modelType() const { return m_modelType; }
    QString currentModelName() const { return m_currentModelName; }
    QStringList ollamaModels() const { return m_ollamaModels; }

    void setApiKey(const QString &apiKey);
    void setModelPath(const QString &modelPath);
    void setApiUrl(const QString &apiUrl);
    void setModelType(ModelType type);
    void setCurrentModelName(const QString &name);
    void setOllamaModels(const QStringList &models);
    void refreshOllamaModels();

    void loadSettings();
    void saveSettings();
    QString getSettingsPath() const;

    void setDefaultSettings();
    void scheduleSave();

    void setAppStateValue(const QString& key, const QJsonValue& value);
    QJsonValue appStateValue(const QString& key) const;
    QJsonObject appState() const { return m_appState; }

    QJsonObject getModelConfig(const QString& type, const QString& name) const;
    void setModelConfig(const QString& type, const QString& name, const QJsonObject& config);
    QStringList getAvailableModels(const QString& type) const;
    bool isModelEnabled(const QString& type, const QString& name) const;
    void setModelEnabled(const QString& type, const QString& name, bool enabled);

signals:
    void apiKeyChanged();
    void modelPathChanged();
    void modelTypeChanged();
    void apiUrlChanged();
    void currentModelNameChanged(const QString& modelName);
    void ollamaModelsChanged();
    void appStateChanged();
    void modelConfigChanged();

private:
    SettingsModel(const SettingsModel&) = delete;
    SettingsModel& operator=(const SettingsModel&) = delete;

    QString m_apiKey;
    QString m_modelPath;
    QString m_apiUrl;
    ModelType m_modelType = ModelType::API;
    QString m_currentModelName;
    QStringList m_ollamaModels;
    QTimer* m_saveTimer;
    QJsonObject m_appState;
    QJsonObject m_models;

    void initializeDefaultModels();
};

#endif // SETTINGSMODEL_H
