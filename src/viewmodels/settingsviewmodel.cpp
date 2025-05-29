#include "settingsviewmodel.h"
#include "services/apiservice.h"
#include "services/ollamaservice.h"
#include "services/localmodelservice.h"
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
                if (apiKey.isEmpty()) {
                    emit errorOccurred("未设置API密钥");
                    return nullptr;
                }
                service = new APIService(apiKey, this);
                break;
            }
            case SettingsModel::ModelType::Ollama: {
                QString modelName = m_model->currentModelName();
                if (modelName.isEmpty()) {
                    emit errorOccurred("未选择Ollama模型");
                    return nullptr;
                }
                service = new OllamaService(modelName, this);
                break;
            }
            case SettingsModel::ModelType::Local: {
                QString modelPath = m_model->modelPath();
                if (modelPath.isEmpty()) {
                    emit errorOccurred("未选择本地模型文件");
                    return nullptr;
                }
                service = new LocalModelService(modelPath, this);
                break;
            }
            default:
                emit errorOccurred("未知的模型类型");
                return nullptr;
        }

        if (service && !service->isAvailable()) {
            emit errorOccurred("模型服务不可用");
            delete service;
            return nullptr;
        }

        return service;
    } catch (const std::exception& e) {
        if (service) {
            delete service;
        }
        emit errorOccurred(QString("创建模型服务失败: %1").arg(e.what()));
        return nullptr;
    }
}
