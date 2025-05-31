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
    LLMService* service = nullptr;

    try {
        QString modelName = m_model->currentModelName();
        LOG_INFO(QString("创建LLM服务，类型: %1, 模型: %2").arg(
            static_cast<int>(m_model->modelType())).arg(modelName));

        switch (m_model->modelType()) {
            case SettingsModel::ModelType::API: {
                QString apiKey = m_model->apiKey();
                QString apiUrl = m_model->apiUrl();
                
                LOG_INFO(QString("API配置 - Key长度: %1, URL: %2").arg(apiKey.length()).arg(apiUrl));
                
                if (apiKey.isEmpty()) {
                    LOG_ERROR("API密钥未配置");
                    emit errorOccurred("API密钥未配置");
                    return nullptr;
                }
                if (apiUrl.isEmpty()) {
                    LOG_ERROR("API地址未配置");
                    emit errorOccurred("API地址未配置");
                    return nullptr;
                }
                
                service = new APIService(apiKey, apiUrl, modelName, this);
                if (!service) {
                    LOG_ERROR("创建API服务失败");
                    emit errorOccurred("创建API服务失败");
                    return nullptr;
                }
                break;
            }
            case SettingsModel::ModelType::Ollama: {
                // 从Ollama: 前缀中提取真实的模型名称
                if (modelName.startsWith("Ollama: ")) {
                    modelName = modelName.mid(8);
                }
                LOG_INFO(QString("创建Ollama服务，模型: %1").arg(modelName));
                service = new OllamaService(modelName, this);
                if (!service) {
                    LOG_ERROR("创建Ollama服务失败");
                    emit errorOccurred("创建Ollama服务失败");
                    return nullptr;
                }
                break;
            }
            case SettingsModel::ModelType::Local: {
                QString modelPath = m_model->modelPath();
                LOG_INFO(QString("创建本地模型服务，路径: %1").arg(modelPath));
                
                if (modelPath.isEmpty()) {
                    LOG_ERROR("本地模型路径未配置");
                    emit errorOccurred("本地模型路径未配置");
                    return nullptr;
                }
                
                service = new LocalModelService(modelPath, this);
                if (!service) {
                    LOG_ERROR("创建本地模型服务失败");
                    emit errorOccurred("创建本地模型服务失败");
                    return nullptr;
                }
                break;
            }
            default: {
                LOG_ERROR(QString("未知的模型类型: %1").arg(static_cast<int>(m_model->modelType())));
                emit errorOccurred("未知的模型类型");
                return nullptr;
            }
        }

        if (!service->isAvailable()) {
            QString error = QString("模型服务不可用: %1").arg(service->getModelName());
            LOG_ERROR(error);
            delete service;
            emit errorOccurred(error);
            return nullptr;
        }

        LOG_INFO(QString("成功创建服务: %1").arg(service->getModelName()));
        return service;
    } catch (const std::exception& e) {
        LOG_ERROR(QString("创建模型服务失败: %1").arg(e.what()));
        delete service;
        emit errorOccurred(QString("创建模型服务失败: %1").arg(e.what()));
        return nullptr;
    }
}
