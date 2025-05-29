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
    m_settings.sync();
}

void SettingsViewModel::loadSettings()
{
    m_model->setApiKey(m_settings.value("apiKey").toString());
    m_model->setModelPath(m_settings.value("modelPath").toString());
    m_model->setModelType(static_cast<SettingsModel::ModelType>(
        m_settings.value("modelType", 0).toInt()));
}

LLMService* SettingsViewModel::createLLMService()
{
    switch (m_model->modelType()) {
        case SettingsModel::ModelType::API:
            return new APIService(m_model->apiKey(), this);
        case SettingsModel::ModelType::Ollama:
            return new OllamaService(m_model->modelPath(), this);
        case SettingsModel::ModelType::Local:
            return new LocalModelService(m_model->modelPath(), this);
        default:
            emit errorOccurred("未知的模型类型");
            return nullptr;
    }
} 