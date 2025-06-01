#include "settingsviewmodel.h"
#include "services/apiservice.h"
#include "services/ollamaservice.h"
#include "services/localmodelservice.h"
#include "services/logger.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>

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
    m_isRefreshingModels = true;
    emit refreshStateChanged(true);
    
    LOG_INFO("开始刷新Ollama模型列表");
    m_model->refreshOllamaModels();
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
