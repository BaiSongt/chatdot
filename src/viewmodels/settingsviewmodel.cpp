#include "settingsviewmodel.h"
#include "services/apiservice.h"
#include "services/ollamaservice.h"
#include "services/localmodelservice.h"
#include "services/logger.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

SettingsViewModel::SettingsViewModel(SettingsModel* model, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_isRefreshingModels(false)
{
    // 监听设置变化
    connect(m_model, &SettingsModel::modelTypeChanged, this, &SettingsViewModel::modelTypeChanged);
    connect(m_model, &SettingsModel::modelTypeChanged, this, &SettingsViewModel::settingsChanged);
    
    connect(m_model, &SettingsModel::currentModelNameChanged, this, &SettingsViewModel::currentModelNameChanged);
    connect(m_model, &SettingsModel::currentModelNameChanged, this, &SettingsViewModel::settingsChanged);
    
    connect(m_model, &SettingsModel::apiKeyChanged, this, &SettingsViewModel::apiKeyChanged);
    connect(m_model, &SettingsModel::apiKeyChanged, this, &SettingsViewModel::settingsChanged);
    
    connect(m_model, &SettingsModel::modelPathChanged, this, &SettingsViewModel::settingsChanged);
    
    connect(m_model, &SettingsModel::apiUrlChanged, this, &SettingsViewModel::apiUrlChanged);
    connect(m_model, &SettingsModel::apiUrlChanged, this, &SettingsViewModel::settingsChanged);
    
    connect(m_model, &SettingsModel::ollamaModelsChanged, this, &SettingsViewModel::ollamaModelsChanged);
    connect(m_model, &SettingsModel::ollamaModelsChanged, this, [this]() {
        m_isRefreshingModels = false;
        emit refreshStateChanged(false);
    });
}

SettingsViewModel::~SettingsViewModel()
{
    saveSettings();
}

// 属性访问器
int SettingsViewModel::modelType() const
{
    return static_cast<int>(m_model->modelType());
}

void SettingsViewModel::setModelType(int type)
{
    m_model->setModelType(static_cast<SettingsModel::ModelType>(type));
}

QString SettingsViewModel::apiProvider() const
{
    // 从SettingsModel获取当前API提供商
    // 由于SettingsModel没有直接提供此方法，我们需要自行实现
    return m_model->appStateValue("apiProvider").toString();
}

void SettingsViewModel::setApiProvider(const QString& provider)
{
    // 设置当前API提供商
    m_model->setAppStateValue("apiProvider", provider);
    emit apiProviderChanged();
    emit apiModelsChanged();
}

QString SettingsViewModel::apiKey() const
{
    return m_model->apiKey();
}

void SettingsViewModel::setApiKey(const QString& key)
{
    m_model->setApiKey(key);
}

QString SettingsViewModel::apiUrl() const
{
    return m_model->apiUrl();
}

void SettingsViewModel::setApiUrl(const QString& url)
{
    m_model->setApiUrl(url);
}

QStringList SettingsViewModel::ollamaModels() const
{
    return m_model->ollamaModels();
}

QString SettingsViewModel::currentModelName() const
{
    return m_model->currentModelName();
}

void SettingsViewModel::setCurrentModelName(const QString& name)
{
    m_model->setCurrentModelName(name);
}

bool SettingsViewModel::isRefreshingModels() const
{
    return m_isRefreshingModels;
}

// 命令
void SettingsViewModel::saveSettings()
{
    m_model->saveSettings();
    emit settingsChanged();
}

void SettingsViewModel::loadSettings()
{
    m_model->loadSettings();
}

void SettingsViewModel::refreshOllamaModels()
{
    if (m_isRefreshingModels) {
        return;
    }
    
    m_isRefreshingModels = true;
    emit refreshStateChanged(true);
    m_model->refreshOllamaModels();
}

void SettingsViewModel::fetchApiModelsFromProvider(const QString& provider)
{
    if (m_isRefreshingModels) {
        return;
    }
    
    // 获取API密钥
    QString apiKey = getProviderApiKey(provider);
    if (apiKey.isEmpty()) {
        emit errorOccurred(tr("API密钥未设置，请先设置API密钥"));
        return;
    }
    
    // 获取API URL
    QString baseUrl;
    if (provider == "OpenAI") {
        baseUrl = "https://api.openai.com/v1/models";
    } else if (provider == "Deepseek") {
        baseUrl = "https://api.deepseek.com/v1/models";
    } else {
        // 对于自定义提供商，使用默认URL的基础部分
        QJsonObject providerConfig = m_model->getProviderConfig("api", provider);
        if (providerConfig.contains("default_url")) {
            QString defaultUrl = providerConfig["default_url"].toString();
            // 从完整URL中提取基础URL
            QUrl url(defaultUrl);
            baseUrl = url.scheme() + "://" + url.host();
            if (url.port() != -1) {
                baseUrl += ":" + QString::number(url.port());
            }
            baseUrl += "/v1/models"; // 假设大多数API都遵循这种模式
        } else {
            emit errorOccurred(tr("无法确定API URL，请检查提供商配置"));
            return;
        }
    }
    
    LOG_INFO(QString("正在从提供商 %1 获取模型列表，URL: %2").arg(provider).arg(baseUrl));
    
    // 设置刷新状态
    m_isRefreshingModels = true;
    emit refreshStateChanged(true);
    
    // 创建网络请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QUrl url(baseUrl);
    QNetworkRequest request;
    request.setUrl(url);
    
    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    
    // 发送GET请求
    QNetworkReply* reply = manager->get(request);
    
    // 处理请求完成信号
    connect(reply, &QNetworkReply::finished, this, [this, reply, provider, manager]() {
        m_isRefreshingModels = false;
        emit refreshStateChanged(false);
        
        if (reply->error() != QNetworkReply::NoError) {
            LOG_ERROR(QString("获取模型列表失败: %1").arg(reply->errorString()));
            emit errorOccurred(tr("获取模型列表失败: %1").arg(reply->errorString()));
            reply->deleteLater();
            manager->deleteLater();
            return;
        }
        
        // 解析响应数据
        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        
        if (doc.isNull() || !doc.isObject()) {
            LOG_ERROR("解析模型列表响应失败: 无效的JSON数据");
            emit errorOccurred(tr("解析模型列表响应失败: 无效的JSON数据"));
            reply->deleteLater();
            manager->deleteLater();
            return;
        }
        
        QJsonObject root = doc.object();
        QStringList modelNames;
        
        // 解析模型列表
        if (root.contains("data") && root["data"].isArray()) {
            QJsonArray models = root["data"].toArray();
            
            for (const QJsonValue& model : models) {
                if (model.isObject() && model.toObject().contains("id")) {
                    QString modelId = model.toObject()["id"].toString();
                    modelNames.append(modelId);
                    LOG_INFO(QString("发现模型: %1").arg(modelId));
                }
            }
            
            LOG_INFO(QString("从提供商 %1 获取到 %2 个模型").arg(provider).arg(modelNames.size()));
            
            // 更新模型配置
            QJsonObject providerConfig = m_model->getProviderConfig("api", provider);
            QJsonObject modelsConfig;
            
            // 保留现有的模型配置
            if (providerConfig.contains("models")) {
                modelsConfig = providerConfig["models"].toObject();
            }
            
            // 添加新的模型
            for (const QString& modelId : modelNames) {
                if (!modelsConfig.contains(modelId)) {
                    QJsonObject modelConfig;
                    modelConfig["name"] = modelId;
                    modelConfig["enabled"] = true;
                    
                    // 使用提供商的默认URL
                    if (providerConfig.contains("default_url")) {
                        modelConfig["url"] = providerConfig["default_url"].toString();
                    }
                    
                    modelsConfig[modelId] = modelConfig;
                }
            }
            
            // 更新提供商配置
            providerConfig["models"] = modelsConfig;
            
            // 保存到设置模型
            m_model->setProviderConfig("api", provider, providerConfig);
            
            // 通知模型列表已更新
            emit apiModelsChanged();
        } else {
            LOG_ERROR("解析模型列表响应失败: 未找到模型数据");
            emit errorOccurred(tr("解析模型列表响应失败: 未找到模型数据"));
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

QStringList SettingsViewModel::getApiModelsForProvider(const QString& provider)
{
    QStringList result;
    QJsonObject providerConfig = m_model->getProviderConfig("api", provider);
    if (providerConfig.contains("models")) {
        QJsonObject models = providerConfig["models"].toObject();
        for (auto it = models.begin(); it != models.end(); ++it) {
            result.append(it.key());
        }
    }
    return result;
}

QStringList SettingsViewModel::getApiModels(const QString& provider)
{
    // 这是为了兼容SettingsDialog中的调用
    return getApiModelsForProvider(provider);
}

QString SettingsViewModel::getProviderApiKey(const QString& provider)
{
    return m_model->getProviderApiKey("api", provider);
}

void SettingsViewModel::setProviderApiKey(const QString& provider, const QString& key)
{
    m_model->setProviderApiKey("api", provider, key);
    emit apiKeyChanged();
}

// Ollama设置
QString SettingsViewModel::ollamaUrl() const
{
    return m_model->ollamaUrl();
}

void SettingsViewModel::setOllamaUrl(const QString& url)
{
    m_model->setOllamaUrl(url);
}

// 本地模型设置
QString SettingsViewModel::getLocalModelPath() const
{
    return m_model->modelPath();
}

void SettingsViewModel::setLocalModelPath(const QString& path)
{
    m_model->setModelPath(path);
}

LLMService* SettingsViewModel::createLLMService()
{
    if (!m_model) {
        LOG_ERROR("SettingsModel 为空");
        return nullptr;
    }

    QString modelName = m_model->currentModelName();
    if (modelName.isEmpty()) {
        LOG_ERROR("未选择模型");
        return nullptr;
    }

    QString typeStr = m_model->getModelTypeString();
    LOG_INFO(QString("创建服务 - 类型: %1, 模型: %2").arg(typeStr).arg(modelName));

    // 检查模型配置是否完整
    if (!m_model->isModelConfigComplete(typeStr, modelName)) {
        QStringList missingItems = m_model->getMissingConfigItems(typeStr, modelName);
        LOG_ERROR(QString("模型配置不完整，缺少: %1").arg(missingItems.join(", ")));
        return nullptr;
    }

    LLMService* service = nullptr;
    try {
        switch (m_model->modelType()) {
            case SettingsModel::ModelType::API: {
                QString provider = m_model->getProviderForModel(modelName);
                if (provider.isEmpty()) {
                    LOG_ERROR("未找到模型对应的提供商");
                    return nullptr;
                }
                QString apiKey = m_model->getProviderApiKey("api", provider);
                QString apiUrl = m_model->getModelConfig("api", modelName)["url"].toString();
                if (apiKey.isEmpty() || apiUrl.isEmpty()) {
                    LOG_ERROR("API配置不完整");
                    return nullptr;
                }
                service = new APIService(apiKey, apiUrl, modelName);
                break;
            }
            case SettingsModel::ModelType::Ollama: {
                if (modelName.isEmpty()) {
                    LOG_ERROR("Ollama模型名称为空");
                    return nullptr;
                }
                service = new OllamaService(modelName);
                break;
            }
            case SettingsModel::ModelType::Local: {
                QString modelPath = m_model->modelPath();
                if (modelPath.isEmpty()) {
                    LOG_ERROR("本地模型路径为空");
                    return nullptr;
                }
                service = new LocalModelService(modelPath);
                break;
            }
            default:
                LOG_ERROR(QString("未知的模型类型: %1").arg(static_cast<int>(m_model->modelType())));
                return nullptr;
        }

        if (service) {
            // 设置深度思考模式
            service->setDeepThinkingMode(m_model->isDeepThinkingMode());
            LOG_INFO(QString("成功创建服务: %1").arg(modelName));
        } else {
            LOG_ERROR("服务创建失败");
        }
    } catch (const std::exception& e) {
        LOG_ERROR(QString("创建服务时发生异常: %1").arg(e.what()));
        delete service;
        service = nullptr;
    }

    return service;
}
