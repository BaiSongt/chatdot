#ifndef SETTINGSMODEL_H
#define SETTINGSMODEL_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QStringList>

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

    QString apiKey() const { return m_apiKey; }
    void setApiKey(const QString &apiKey);

    QString modelPath() const { return m_modelPath; }
    void setModelPath(const QString &modelPath);

    QString apiUrl() const { return m_apiUrl; }
    void setApiUrl(const QString &apiUrl);

    ModelType modelType() const { return m_modelType; }
    void setModelType(ModelType type);

    QString currentModelName() const { return m_currentModelName; }
    void setCurrentModelName(const QString &name);

    QStringList ollamaModels() const { return m_ollamaModels; }
    void setOllamaModels(const QStringList &models);
    void refreshOllamaModels();

    // 配置文件操作
    void loadSettings();
    void saveSettings();
    QString getSettingsPath() const;

signals:
    void apiKeyChanged();
    void modelPathChanged();
    void modelTypeChanged();
    void apiUrlChanged();
    void currentModelNameChanged(const QString& modelName);
    void ollamaModelsChanged();

private:
    explicit SettingsModel(QObject *parent = nullptr);
    ~SettingsModel() = default;
    SettingsModel(const SettingsModel&) = delete;
    SettingsModel& operator=(const SettingsModel&) = delete;

    QString m_apiKey;
    QString m_modelPath;
    QString m_apiUrl;
    ModelType m_modelType = ModelType::API;
    QString m_currentModelName;
    QStringList m_ollamaModels;
};

#endif // SETTINGSMODEL_H
