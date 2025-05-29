#include "settingsviewmodel.h"
#include "services/apiservice.h"
#include "services/ollamaservice.h"
#include "services/localmodelservice.h"
#include "services/logger.h"
#include <QDebug>

SettingsViewModel::SettingsViewModel(SettingsModel* model, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_settings("ChatDot", "Settings")
{
    loadSettings();
    // 监听设置变化
    connect(m_model, &SettingsModel::modelTypeChanged, this, &SettingsViewModel::settingsChanged);
    connect(m_model, &SettingsModel::currentModelNameChanged, this, &SettingsViewModel::settingsChanged);
    connect(m_model, &SettingsModel::apiKeyChanged, this, &SettingsViewModel::settingsChanged);
    connect(m_model, &SettingsModel::modelPathChanged, this, &SettingsViewModel::settingsChanged);
}

SettingsViewModel::~SettingsViewModel()
{
    saveSettings();
}

void SettingsViewModel::saveSettings()
{
    m_settings.setValue("apiKey", m_model->apiKey());
    m_settings.setValue("modelPath", m_model->modelPath());
    m_settings.setValue("modelType", static_cast<int>(m_model->modelType()));
    m_settings.setValue("currentModelName", m_model->currentModelName());
    m_settings.sync();
    emit settingsChanged();
}

void SettingsViewModel::loadSettings()
{
    m_model->setApiKey(m_settings.value("apiKey").toString());
    m_model->setModelPath(m_settings.value("modelPath").toString());
    m_model->setModelType(static_cast<SettingsModel::ModelType>(
        m_settings.value("modelType", 0).toInt()));
    m_model->setCurrentModelName(m_settings.value("currentModelName").toString());
}

LLMService* SettingsViewModel::createLLMService()
{
    LLMService* service = nullptr;

    try {
        switch (m_model->modelType()) {
            case SettingsModel::ModelType::API: {
                QString apiKey = m_model->apiKey();
                QString apiUrl = m_model->apiUrl();
                QString modelName = m_model->currentModelName();
                service = new APIService(apiKey, apiUrl, modelName, this);
                break;
            }
            case SettingsModel::ModelType::Ollama: {
                service = new OllamaService(m_model->currentModelName(), this);
                break;
            }
            case SettingsModel::ModelType::Local: {
                service = new LocalModelService(m_model->modelPath(), this);
                break;
            }
            default: {
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

        return service;
    } catch (const std::exception& e) {
        LOG_ERROR(QString("创建模型服务失败: %1").arg(e.what()));
        delete service;
        emit errorOccurred(QString("创建模型服务失败: %1").arg(e.what()));
        return nullptr;
    }
}
