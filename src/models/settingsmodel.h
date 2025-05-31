#ifndef SETTINGSMODEL_H
#define SETTINGSMODEL_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
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

    QString ollamaUrl() const { return m_ollamaUrl; }
    void setOllamaUrl(const QString &url);

    double temperature() const { return m_temperature; }
    void setTemperature(double value);

    int maxTokens() const { return m_maxTokens; }
    void setMaxTokens(int value);

    int contextWindow() const { return m_contextWindow; }
    void setContextWindow(int value);

    QString theme() const { return m_theme; }
    void setTheme(const QString &theme);

    int fontSize() const { return m_fontSize; }
    void setFontSize(int size);

    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString &family);

    QString language() const { return m_language; }
    void setLanguage(const QString &lang);

    bool autoSave() const { return m_autoSave; }
    void setAutoSave(bool enabled);

    int saveInterval() const { return m_saveInterval; }
    void setSaveInterval(int seconds);

    bool proxyEnabled() const { return m_proxyEnabled; }
    void setProxyEnabled(bool enabled);

    QString proxyHost() const { return m_proxyHost; }
    void setProxyHost(const QString &host);

    int proxyPort() const { return m_proxyPort; }
    void setProxyPort(int port);

    QString proxyType() const { return m_proxyType; }
    void setProxyType(const QString &type);

    QString proxyUsername() const { return m_proxyUsername; }
    void setProxyUsername(const QString &username);

    QString proxyPassword() const { return m_proxyPassword; }
    void setProxyPassword(const QString &password);

    // 获取提供商配置
    QJsonObject getProviderConfig(const QString& type, const QString& provider) const;
    
    // 获取提供商的 API Key
    QString getProviderApiKey(const QString& type, const QString& provider) const;
    
    // 设置提供商的 API Key
    void setProviderApiKey(const QString& type, const QString& provider, const QString& apiKey);
    
    // 获取已配置的模型列表
    QStringList getConfiguredModels(const QString& type) const;
    
    // 添加已配置的模型
    void addConfiguredModel(const QString& type, const QString& modelName);
    
    // 移除已配置的模型
    void removeConfiguredModel(const QString& type, const QString& modelName);

    // 获取当前模型的提供商
    QString getCurrentProvider() const;

    // 获取当前模型的 API Key
    QString getCurrentApiKey() const;

    // 获取当前模型的 URL
    QString getCurrentApiUrl() const;

    // 获取模型类型字符串
    QString getModelTypeString() const;
    QString getModelTypeString(ModelType type) const;

    // 深度思考模式
    bool isDeepThinkingMode() const { return m_isDeepThinking; }
    void setDeepThinkingMode(bool enabled);

    // 检查模型配置是否完整
    bool isModelConfigComplete(const QString& type, const QString& modelName) const;
    
    // 检查提供商配置是否完整
    bool isProviderConfigComplete(const QString& provider, const QJsonObject& config) const;
    
    // 获取模型配置缺失项
    QStringList getMissingConfigItems(const QString& type, const QString& modelName) const;

    // 获取提供商配置缺失项
    QStringList getMissingProviderConfigItems(const QString& provider, const QJsonObject& config) const;

    // 获取模型对应的提供商
    QString getProviderForModel(const QString& modelName) const;

signals:
    void apiKeyChanged();
    void modelPathChanged();
    void modelTypeChanged();
    void apiUrlChanged();
    void currentModelNameChanged(const QString& modelName);
    void ollamaModelsChanged();
    void appStateChanged();
    void modelConfigChanged();
    void ollamaUrlChanged();
    void temperatureChanged();
    void maxTokensChanged();
    void contextWindowChanged();
    void themeChanged();
    void fontSizeChanged();
    void fontFamilyChanged();
    void languageChanged();
    void autoSaveChanged();
    void saveIntervalChanged();
    void proxySettingsChanged();

private:
    SettingsModel(const SettingsModel&) = delete;
    SettingsModel& operator=(const SettingsModel&) = delete;

    QString m_apiKey;
    QString m_modelPath;
    QString m_apiUrl;
    ModelType m_modelType = ModelType::API;
    QString m_currentModelName;
    QStringList m_ollamaModels;
    QString m_ollamaUrl;
    QTimer* m_saveTimer;
    QJsonObject m_appState;
    QJsonObject m_models_config;
    QJsonObject m_configuredModels;
    QString m_currentProvider;
    bool m_isDeepThinking = false;  // 添加深度思考模式标志

    double m_temperature;
    int m_maxTokens;
    int m_contextWindow;
    QString m_theme;
    int m_fontSize;
    QString m_fontFamily;
    QString m_language;
    bool m_autoSave;
    int m_saveInterval;

    bool m_proxyEnabled;
    QString m_proxyHost;
    int m_proxyPort;
    QString m_proxyType;
    QString m_proxyUsername;
    QString m_proxyPassword;

    void initializeDefaultModels();
    void migrateConfig();
    void updateConfiguredModels();

    // 验证模型配置
    bool validateModelConfig(const QString& type, const QString& modelName) const;
    
    // 验证提供商配置
    bool validateProviderConfig(const QString& type, const QString& provider) const;

    // 更新已配置的模型列表相关方法
    void updateApiConfiguredModels(QJsonArray& apiModels);
    void updateOllamaConfiguredModels(QJsonArray& ollamaModels);
    void updateLocalConfiguredModels(QJsonArray& localModels);
    void updateConfiguredModelsForType(const QString& type, QJsonArray& models);
    void addModelToConfiguredList(const QString& type, const QString& modelName, const QString& provider = QString());
    void logConfiguredModelsSummary();

    // 设置加载相关函数
    bool loadConfigFile(QJsonObject& root);
    void loadModelConfig(const QJsonObject& root);
    void loadConfiguredModels(const QJsonObject& root);
    void loadBasicSettings(const QJsonObject& root);
    void loadModelSpecificSettings();
    void loadAPIModelSettings();
    void loadOllamaModelSettings();
    void loadLocalModelSettings();
};

#endif // SETTINGSMODEL_H
