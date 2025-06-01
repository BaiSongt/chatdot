#ifndef SETTINGSVIEWMODEL_H
#define SETTINGSVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include "models/settingsmodel.h"
#include "services/llmservice.h"
// #include "utils/encryption.h"

class SettingsViewModel : public QObject
{
    Q_OBJECT
    
    // 属性定义
    Q_PROPERTY(int modelType READ modelType WRITE setModelType NOTIFY modelTypeChanged)
    Q_PROPERTY(QString apiProvider READ apiProvider WRITE setApiProvider NOTIFY apiProviderChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString apiUrl READ apiUrl WRITE setApiUrl NOTIFY apiUrlChanged)
    Q_PROPERTY(QStringList ollamaModels READ ollamaModels NOTIFY ollamaModelsChanged)
    Q_PROPERTY(QString currentModelName READ currentModelName WRITE setCurrentModelName NOTIFY currentModelNameChanged)
    Q_PROPERTY(bool isRefreshingModels READ isRefreshingModels NOTIFY refreshStateChanged)
    
public:
    explicit SettingsViewModel(SettingsModel* model, QObject *parent = nullptr);
    ~SettingsViewModel();
    
    // 属性访问器
    int modelType() const;
    void setModelType(int type);
    
    QString apiProvider() const;
    void setApiProvider(const QString& provider);
    
    QString apiKey() const;
    void setApiKey(const QString& key);
    
    QString apiUrl() const;
    void setApiUrl(const QString& url);
    
    QStringList ollamaModels() const;
    
    QString currentModelName() const;
    void setCurrentModelName(const QString& name);
    
    bool isRefreshingModels() const;
    
    // 命令
    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void refreshOllamaModels();
    Q_INVOKABLE QStringList getApiModelsForProvider(const QString& provider);
    Q_INVOKABLE QStringList getApiModels(const QString& provider);
    Q_INVOKABLE QString getProviderApiKey(const QString& provider);
    Q_INVOKABLE void setProviderApiKey(const QString& provider, const QString& key);
    Q_INVOKABLE LLMService* createLLMService();
    
    // Ollama设置
    Q_INVOKABLE QString ollamaUrl() const;
    Q_INVOKABLE void setOllamaUrl(const QString& url);
    
    // 本地模型设置
    Q_INVOKABLE QString getLocalModelPath() const;
    Q_INVOKABLE void setLocalModelPath(const QString& path);
    
signals:
    void modelTypeChanged();
    void apiProviderChanged();
    void apiKeyChanged();
    void apiUrlChanged();
    void ollamaModelsChanged();
    void apiModelsChanged();
    void currentModelNameChanged();
    void settingsChanged();
    void errorOccurred(const QString& error);
    void refreshStateChanged(bool isRefreshing);
    
private:
    SettingsModel* m_model;
    bool m_isRefreshingModels;
};

#endif // SETTINGSVIEWMODEL_H
