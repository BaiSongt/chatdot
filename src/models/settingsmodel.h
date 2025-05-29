#ifndef SETTINGSMODEL_H
#define SETTINGSMODEL_H

#include <QObject>
#include <QString>
#include <QJsonObject>

class SettingsModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(ModelType modelType READ modelType WRITE setModelType NOTIFY modelTypeChanged)
    Q_PROPERTY(QString apiUrl READ apiUrl WRITE setApiUrl NOTIFY apiUrlChanged)

public:
    enum class ModelType {
        API,
        Ollama,
        Local
    };
    Q_ENUM(ModelType)

    explicit SettingsModel(QObject *parent = nullptr);

    QString apiKey() const { return m_apiKey; }
    void setApiKey(const QString &apiKey);

    QString modelPath() const { return m_modelPath; }
    void setModelPath(const QString &modelPath);

    QString apiUrl() const { return m_apiUrl; }
    void setApiUrl(const QString &apiUrl);

    ModelType modelType() const { return m_modelType; }
    void setModelType(ModelType type);

    // JSON 文件操作
    bool saveToFile(const QString &filename);
    bool loadFromFile(const QString &filename);
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

signals:
    void apiKeyChanged();
    void modelPathChanged();
    void modelTypeChanged();
    void apiUrlChanged();

private:
    QString m_apiKey;
    QString m_modelPath;
    QString m_apiUrl;
    ModelType m_modelType = ModelType::API;
};

#endif // SETTINGSMODEL_H 