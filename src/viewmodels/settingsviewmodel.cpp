#include "settingsviewmodel.h"
#include "services/apiservice.h"
#include "services/ollamaservice.h"
#include "services/localmodelservice.h"
#include "services/logger.h"
#include <QDebug>

SettingsViewModel::SettingsViewModel(SettingsModel* model, QObject *parent)
    : QObject(parent)
    , m_model(model)
{
    // 监听设置变化
    connect(m_model, &SettingsModel::modelTypeChanged, this, &SettingsViewModel::settingsChanged);
    connect(m_model, &SettingsModel::currentModelNameChanged, this, &SettingsViewModel::settingsChanged);
    connect(m_model, &SettingsModel::apiKeyChanged, this, &SettingsViewModel::settingsChanged);
    connect(m_model, &SettingsModel::modelPathChanged, this, &SettingsViewModel::settingsChanged);
    connect(m_model, &SettingsModel::apiUrlChanged, this, &SettingsViewModel::settingsChanged);
}

SettingsViewModel::~SettingsViewModel()
{
    saveSettings();
}

void SettingsViewModel::saveSettings()
{
    m_model->saveSettings();
    emit settingsChanged();
}

void SettingsViewModel::loadSettings()
{
    m_model->loadSettings();
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
