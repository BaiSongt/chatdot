#include "settingsmodel.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include "services/logger.h"

// 单例实现
SettingsModel& SettingsModel::instance()
{
    static SettingsModel instance;
    return instance;
}

SettingsModel::SettingsModel(QObject *parent)
    : QObject(parent)
    , m_saveTimer(new QTimer(this))
    , m_modelType(ModelType::API)
    , m_temperature(0.7)
    , m_maxTokens(2048)
    , m_contextWindow(2048)
    , m_fontSize(12)
    , m_autoSave(true)
    , m_saveInterval(60)
    , m_proxyEnabled(false)
    , m_proxyPort(80)
{
    LOG_INFO("初始化 SettingsModel");
    
    // 设置延迟保存定时器
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(1000); // 1秒延迟
    connect(m_saveTimer, &QTimer::timeout, this, &SettingsModel::saveSettings);
    
    loadSettings();
}

SettingsModel::~SettingsModel()
{
    // 确保在析构时保存设置
    if (m_saveTimer->isActive()) {
        m_saveTimer->stop();
        saveSettings();
    }
}

QString SettingsModel::getSettingsPath() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (path.isEmpty()) {
        LOG_ERROR("无法获取应用程序数据目录");
        return QString();
    }
    
    // 确保目录存在
    QDir dir(path);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            LOG_ERROR("无法创建设置目录");
            return QString();
        }
    }
    
    return path + "/settings.json";
}

void SettingsModel::initializeDefaultModels()
{
    // 初始化模型配置
    QJsonObject models;
    
    // API 模型配置
    QJsonObject apiConfig;
    
    // OpenAI 配置
    QJsonObject openaiConfig;
    openaiConfig["api_key"] = "";
    openaiConfig["default_url"] = "https://api.openai.com/v1/chat/completions";
    
    QJsonObject openaiModels;
    QJsonObject gpt35;
    gpt35["name"] = "gpt-3.5-turbo";
    gpt35["url"] = "https://api.openai.com/v1/chat/completions";
    gpt35["enabled"] = true;
    openaiModels["gpt-3.5-turbo"] = gpt35;

    QJsonObject gpt4;
    gpt4["name"] = "gpt-4";
    gpt4["url"] = "https://api.openai.com/v1/chat/completions";
    gpt4["enabled"] = true;
    openaiModels["gpt-4"] = gpt4;
    
    openaiConfig["models"] = openaiModels;
    apiConfig["OpenAI"] = openaiConfig;

    // Deepseek 配置
    QJsonObject deepseekConfig;
    deepseekConfig["api_key"] = "";
    deepseekConfig["default_url"] = "https://api.deepseek.com/v1/chat/completions";
    
    QJsonObject deepseekModels;
    QJsonObject deepseek;
    deepseek["name"] = "deepseek-chat";
    deepseek["url"] = "https://api.deepseek.com/v1/chat/completions";
    deepseek["enabled"] = true;
    deepseekModels["deepseek-chat"] = deepseek;
    
    deepseekConfig["models"] = deepseekModels;
    apiConfig["Deepseek"] = deepseekConfig;
    
    models["api"] = apiConfig;

    // Ollama 模型配置
    QJsonObject ollamaConfig;
    ollamaConfig["default_url"] = "http://localhost:11434";
    ollamaConfig["models"] = QJsonObject();
    models["ollama"] = ollamaConfig;

    // 本地模型配置
    QJsonObject localConfig;
    localConfig["models"] = QJsonObject();
    models["local"] = localConfig;

    m_models_config = models;
    
    // 初始化已配置的模型列表
    updateConfiguredModels();
}

void SettingsModel::migrateConfig()
{
    LOG_INFO("开始迁移配置到新结构");
    
    // 检查是否需要迁移
    if (m_models_config.contains("api") && m_models_config["api"].isObject()) {
        QJsonObject apiConfig = m_models_config["api"].toObject();
        if (apiConfig.contains("has_API")) {
            // 需要迁移
            QJsonObject newApiConfig;
            
            // 迁移 OpenAI 配置
            if (apiConfig.contains("OpenAI")) {
                QJsonObject openaiConfig;
                openaiConfig["api_key"] = ""; // 需要用户重新配置
                openaiConfig["default_url"] = "https://api.openai.com/v1/chat/completions";
                openaiConfig["models"] = apiConfig["OpenAI"].toObject();
                newApiConfig["OpenAI"] = openaiConfig;
            }
            
            // 迁移 Deepseek 配置
            if (apiConfig.contains("Deepseek")) {
                QJsonObject deepseekConfig;
                deepseekConfig["api_key"] = ""; // 需要用户重新配置
                deepseekConfig["default_url"] = "https://api.deepseek.com/v1/chat/completions";
                deepseekConfig["models"] = apiConfig["Deepseek"].toObject();
                newApiConfig["Deepseek"] = deepseekConfig;
            }
            
            m_models_config["api"] = newApiConfig;
            LOG_INFO("API 配置迁移完成");
        }
    }
    
    // 更新已配置的模型列表
    updateConfiguredModels();
    
    // 保存迁移后的配置
    saveSettings();
    LOG_INFO("配置迁移完成");
}

void SettingsModel::updateConfiguredModels()
{
    LOG_INFO("开始更新已配置的模型列表");
    m_configuredModels = QJsonObject();

    // 更新各类型的已配置模型
    QJsonArray apiModels;
    QJsonArray ollamaModels;
    QJsonArray localModels;

    updateApiConfiguredModels(apiModels);
    updateOllamaConfiguredModels(ollamaModels);
    updateLocalConfiguredModels(localModels);

    // 保存更新后的模型列表
    m_configuredModels["api"] = apiModels;
    m_configuredModels["ollama"] = ollamaModels;
    m_configuredModels["local"] = localModels;

    // 如果当前没有选择模型，但有可用模型，则选择第一个
    if (m_currentModelName.isEmpty()) {
        if (m_modelType == ModelType::API && !apiModels.isEmpty()) {
            m_currentModelName = apiModels.first().toString();
            emit currentModelNameChanged(m_currentModelName);
        } else if (m_modelType == ModelType::Ollama && !ollamaModels.isEmpty()) {
            m_currentModelName = ollamaModels.first().toString();
            emit currentModelNameChanged(m_currentModelName);
        } else if (m_modelType == ModelType::Local && !localModels.isEmpty()) {
            m_currentModelName = localModels.first().toString();
            emit currentModelNameChanged(m_currentModelName);
        }
    }

    logConfiguredModelsSummary();
}

void SettingsModel::updateApiConfiguredModels(QJsonArray& apiModels)
{
    if (!m_models_config.contains("api")) {
        return;
    }

    QJsonObject apiConfig = m_models_config["api"].toObject();
    for (auto providerIt = apiConfig.begin(); providerIt != apiConfig.end(); ++providerIt) {
        if (providerIt.key() != "has_API" && providerIt.key() != "default_url" && 
            providerIt.value().isObject()) {
            QString provider = providerIt.key();
            QJsonObject providerConfig = providerIt.value().toObject();
            
            if (isProviderConfigComplete(provider, providerConfig)) {
                if (providerConfig.contains("models")) {
                    QJsonObject models = providerConfig["models"].toObject();
                    for (auto modelIt = models.begin(); modelIt != models.end(); ++modelIt) {
                        QString modelName = modelIt.key();
                        if (isModelConfigComplete("api", modelName)) {
                            addModelToConfiguredList("api", modelName, provider);
                            apiModels.append(modelName);
                        } else {
                            QStringList missingItems = getMissingConfigItems("api", modelName);
                            LOG_WARNING(QString("API模型 %1 配置不完整，缺少: %2")
                                .arg(modelName, missingItems.join(", ")));
                        }
                    }
                }
            } else {
                QStringList missingItems = getMissingProviderConfigItems(provider, providerConfig);
                LOG_WARNING(QString("API提供商 %1 配置不完整，缺少: %2")
                    .arg(provider, missingItems.join(", ")));
            }
        }
    }
}

void SettingsModel::updateOllamaConfiguredModels(QJsonArray& ollamaModels)
{
    if (!m_models_config.contains("ollama")) {
        return;
    }

    QJsonObject ollamaConfig = m_models_config["ollama"].toObject();
    if (ollamaConfig.contains("models")) {
        QJsonObject models = ollamaConfig["models"].toObject();
        for (auto it = models.begin(); it != models.end(); ++it) {
            QString modelName = it.key();
            if (isModelConfigComplete("ollama", modelName)) {
                addModelToConfiguredList("ollama", modelName);
                ollamaModels.append(modelName);
            } else {
                QStringList missingItems = getMissingConfigItems("ollama", modelName);
                LOG_WARNING(QString("Ollama模型 %1 配置不完整，缺少: %2")
                    .arg(modelName, missingItems.join(", ")));
            }
        }
    }
}

void SettingsModel::updateLocalConfiguredModels(QJsonArray& localModels)
{
    if (!m_models_config.contains("local")) {
        return;
    }

    QJsonObject localConfig = m_models_config["local"].toObject();
    if (localConfig.contains("models")) {
        QJsonObject models = localConfig["models"].toObject();
        for (auto it = models.begin(); it != models.end(); ++it) {
            QString modelName = it.key();
            if (isModelConfigComplete("local", modelName)) {
                addModelToConfiguredList("local", modelName);
                localModels.append(modelName);
            } else {
                QStringList missingItems = getMissingConfigItems("local", modelName);
                LOG_WARNING(QString("本地模型 %1 配置不完整，缺少: %2")
                    .arg(modelName, missingItems.join(", ")));
            }
        }
    }
}

void SettingsModel::addModelToConfiguredList(const QString& type, const QString& modelName, const QString& provider)
{
    QString logMessage;
    if (type == "api") {
        logMessage = QString("添加已配置的API模型: %1 (提供商: %2)").arg(modelName, provider);
    } else if (type == "ollama") {
        logMessage = QString("添加已配置的Ollama模型: %1").arg(modelName);
    } else if (type == "local") {
        logMessage = QString("添加已配置的本地模型: %1").arg(modelName);
    }
    LOG_INFO(logMessage);
}

void SettingsModel::logConfiguredModelsSummary()
{
    LOG_INFO(QString("已配置的模型列表更新完成，API: %1, Ollama: %2, 本地: %3")
        .arg(m_configuredModels["api"].toArray().size())
        .arg(m_configuredModels["ollama"].toArray().size())
        .arg(m_configuredModels["local"].toArray().size()));
}

bool SettingsModel::isProviderConfigComplete(const QString& provider, const QJsonObject& config) const
{
    // 检查必需的提供商配置项
    if (!config.contains("api_key") || config["api_key"].toString().isEmpty()) {
        return false;
    }
    if (!config.contains("default_url") || config["default_url"].toString().isEmpty()) {
        return false;
    }
    if (!config.contains("models") || !config["models"].isObject()) {
        return false;
    }
    return true;
}

QStringList SettingsModel::getMissingProviderConfigItems(const QString& provider, const QJsonObject& config) const
{
    QStringList missingItems;
    
    if (!config.contains("api_key") || config["api_key"].toString().isEmpty()) {
        missingItems << "api_key";
    }
    if (!config.contains("default_url") || config["default_url"].toString().isEmpty()) {
        missingItems << "default_url";
    }
    if (!config.contains("models") || !config["models"].isObject()) {
        missingItems << "models";
    }
    
    return missingItems;
}

bool SettingsModel::isModelConfigComplete(const QString& type, const QString& modelName) const
{
    QJsonObject config = getModelConfig(type, modelName);
    
    // 检查模型是否启用
    if (!config["enabled"].toBool(true)) {
        return false;
    }

    // 根据不同类型检查必需的配置项
    if (type == "api") {
        // 检查模型名称
        if (!config.contains("name") || config["name"].toString().isEmpty()) {
            return false;
        }
        // 检查API URL
        if (!config.contains("url") || config["url"].toString().isEmpty()) {
            return false;
        }
        // 检查提供商配置
        QString provider = getProviderForModel(modelName);
        if (provider.isEmpty()) {
            return false;
        }
        QJsonObject providerConfig = m_models_config["api"].toObject()[provider].toObject();
        if (!isProviderConfigComplete(provider, providerConfig)) {
            return false;
        }
    }
    else if (type == "ollama") {
        // 检查模型名称
        if (!config.contains("name") || config["name"].toString().isEmpty()) {
            return false;
        }
        // 检查主机和端口
        if (!config.contains("host") || config["host"].toString().isEmpty()) {
            return false;
        }
        if (!config.contains("port") || config["port"].toInt() <= 0) {
            return false;
        }
    }
    else if (type == "local") {
        // 检查模型名称
        if (!config.contains("name") || config["name"].toString().isEmpty()) {
            return false;
        }
        // 检查模型路径
        if (!config.contains("path") || config["path"].toString().isEmpty()) {
            return false;
        }
        // 检查模型文件是否存在
        QString path = config["path"].toString();
        if (!QFile::exists(path)) {
            return false;
        }
    }

    return true;
}

QStringList SettingsModel::getMissingConfigItems(const QString& type, const QString& modelName) const
{
    QStringList missingItems;
    QJsonObject config = getModelConfig(type, modelName);
    
    // 检查模型是否启用
    if (!config["enabled"].toBool(true)) {
        missingItems << "enabled";
    }

    // 根据不同类型检查必需的配置项
    if (type == "api") {
        // 检查模型名称
        if (!config.contains("name") || config["name"].toString().isEmpty()) {
            missingItems << "name";
        }
        // 检查API URL
        if (!config.contains("url") || config["url"].toString().isEmpty()) {
            missingItems << "url";
        }
        // 检查提供商配置
        QString provider = getProviderForModel(modelName);
        if (provider.isEmpty()) {
            missingItems << "provider";
        } else {
            QJsonObject providerConfig = m_models_config["api"].toObject()[provider].toObject();
            missingItems << getMissingProviderConfigItems(provider, providerConfig);
        }
    }
    else if (type == "ollama") {
        // 检查模型名称
        if (!config.contains("name") || config["name"].toString().isEmpty()) {
            missingItems << "name";
        }
        // 检查主机和端口
        if (!config.contains("host") || config["host"].toString().isEmpty()) {
            missingItems << "host";
        }
        if (!config.contains("port") || config["port"].toInt() <= 0) {
            missingItems << "port";
        }
    }
    else if (type == "local") {
        // 检查模型名称
        if (!config.contains("name") || config["name"].toString().isEmpty()) {
            missingItems << "name";
        }
        // 检查模型路径
        if (!config.contains("path") || config["path"].toString().isEmpty()) {
            missingItems << "path";
        }
        // 检查模型文件是否存在
        QString path = config["path"].toString();
        if (!QFile::exists(path)) {
            missingItems << "path (文件不存在)";
        }
    }

    return missingItems;
}

QString SettingsModel::getProviderForModel(const QString& modelName) const
{
    if (!m_models_config.contains("api")) {
        return QString();
    }

    QJsonObject apiConfig = m_models_config["api"].toObject();
    for (auto it = apiConfig.begin(); it != apiConfig.end(); ++it) {
        if (it.key() != "has_API" && it.key() != "default_url" && 
            it.value().isObject()) {
            QString provider = it.key();
            QJsonObject providerConfig = it.value().toObject();
            if (providerConfig.contains("models")) {
                QJsonObject models = providerConfig["models"].toObject();
                if (models.contains(modelName)) {
                    return provider;
                }
            }
        }
    }
    return QString();
}

QJsonObject SettingsModel::getProviderConfig(const QString& type, const QString& provider) const
{
    if (m_models_config.contains(type) && m_models_config[type].isObject()) {
        QJsonObject typeConfig = m_models_config[type].toObject();
        if (typeConfig.contains(provider)) {
            return typeConfig[provider].toObject();
        }
    }
    return QJsonObject();
}

QString SettingsModel::getProviderApiKey(const QString& type, const QString& provider) const
{
    QJsonObject providerConfig = getProviderConfig(type, provider);
    return providerConfig["api_key"].toString();
}

void SettingsModel::setProviderApiKey(const QString& type, const QString& provider, const QString& apiKey)
{
    if (m_models_config.contains(type) && m_models_config[type].isObject()) {
        QJsonObject typeConfig = m_models_config[type].toObject();
        if (typeConfig.contains(provider)) {
            QJsonObject providerConfig = typeConfig[provider].toObject();
            providerConfig["api_key"] = apiKey;
            typeConfig[provider] = providerConfig;
            m_models_config[type] = typeConfig;
            
            // 如果当前正在使用这个提供商，更新 API Key
            if (m_modelType == ModelType::API && m_currentProvider == provider) {
                m_apiKey = apiKey;
            }
            
            // 更新已配置的模型列表
            updateConfiguredModels();
            
            scheduleSave();
            emit apiKeyChanged();
            LOG_INFO(QString("已更新提供商 %1 的 API Key").arg(provider));
        }
    }
}

QStringList SettingsModel::getConfiguredModels(const QString& type) const
{
    QStringList models;
    if (m_configuredModels.contains(type)) {
        QJsonArray modelArray = m_configuredModels[type].toArray();
        for (const QJsonValue& value : modelArray) {
            models.append(value.toString());
        }
    }
    return models;
}

void SettingsModel::addConfiguredModel(const QString& type, const QString& modelName)
{
    if (!m_configuredModels.contains(type)) {
        m_configuredModels[type] = QJsonArray();
    }
    
    QJsonArray modelArray = m_configuredModels[type].toArray();
    if (!modelArray.contains(modelName)) {
        modelArray.append(modelName);
        m_configuredModels[type] = modelArray;
        scheduleSave();
    }
}

void SettingsModel::removeConfiguredModel(const QString& type, const QString& modelName)
{
    if (m_configuredModels.contains(type)) {
        QJsonArray modelArray = m_configuredModels[type].toArray();
        QJsonArray newArray;
        
        for (const QJsonValue& value : modelArray) {
            if (value.toString() != modelName) {
                newArray.append(value);
            }
        }
        
        m_configuredModels[type] = newArray;
        scheduleSave();
    }
}

QString SettingsModel::getCurrentProvider() const
{
    return m_currentProvider;
}

QString SettingsModel::getCurrentApiKey() const
{
    if (m_modelType == ModelType::API && !m_currentProvider.isEmpty()) {
        return getProviderApiKey("api", m_currentProvider);
    }
    return QString();
}

QString SettingsModel::getCurrentApiUrl() const
{
    if (m_modelType == ModelType::API && !m_currentProvider.isEmpty()) {
        QJsonObject providerConfig = getProviderConfig("api", m_currentProvider);
        if (!providerConfig.isEmpty()) {
            return providerConfig["default_url"].toString();
        }
    }
    return QString();
}

bool SettingsModel::loadConfigFile(QJsonObject& root)
{
    QString settingsPath = getSettingsPath();
    if (settingsPath.isEmpty()) {
        LOG_ERROR("无法获取设置文件路径");
        return false;
    }

    QFile file(settingsPath);
    if (!file.exists()) {
        LOG_INFO("设置文件不存在，使用默认值");
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("无法打开配置文件: %1").arg(settingsPath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        LOG_ERROR("配置文件格式错误");
        return false;
    }

    root = doc.object();
    return true;
}

void SettingsModel::loadModelConfig(const QJsonObject& root)
{
    if (root.contains("models_config")) {
        m_models_config = root["models_config"].toObject();
        LOG_INFO("已加载模型配置");
        
        // 检查 API 配置
        if (m_models_config.contains("api")) {
            QJsonObject apiConfig = m_models_config["api"].toObject();
            LOG_INFO(QString("API配置包含 %1 个提供商").arg(apiConfig.size()));
            
            // 检查每个提供商的配置
            for (auto it = apiConfig.begin(); it != apiConfig.end(); ++it) {
                if (it.value().isObject()) {
                    QJsonObject providerConfig = it.value().toObject();
                    QString provider = it.key();
                    LOG_INFO(QString("提供商 %1 配置:").arg(provider));
                    LOG_INFO(QString("  - API Key: %1").arg(providerConfig["api_key"].toString().isEmpty() ? "未设置" : "已设置"));
                    LOG_INFO(QString("  - 默认URL: %1").arg(providerConfig["default_url"].toString()));
                    
                    if (providerConfig.contains("models")) {
                        QJsonObject models = providerConfig["models"].toObject();
                        LOG_INFO(QString("  - 包含 %1 个模型").arg(models.size()));
                    }
                }
            }
        }
    } else {
        LOG_INFO("未找到模型配置，使用默认配置");
        initializeDefaultModels();
    }
}

void SettingsModel::loadConfiguredModels(const QJsonObject& root)
{
    if (root.contains("configured_models")) {
        m_configuredModels = root["configured_models"].toObject();
        LOG_INFO("已加载配置的模型列表");
        
        // 检查已配置的模型
        if (m_configuredModels.contains("api")) {
            QJsonArray apiModels = m_configuredModels["api"].toArray();
            LOG_INFO(QString("已配置的API模型: %1").arg(apiModels.size()));
            for (const QJsonValue& model : apiModels) {
                LOG_INFO(QString("  - %1").arg(model.toString()));
            }
        }
    } else {
        updateConfiguredModels();
    }
}

void SettingsModel::loadBasicSettings(const QJsonObject& root)
{
    if (root.contains("appState")) {
        QJsonObject appState = root["appState"].toObject();
        if (appState.contains("lastModelType")) {
            m_modelType = static_cast<ModelType>(appState["lastModelType"].toInt());
            LOG_INFO(QString("已加载上次使用的模型类型: %1").arg(static_cast<int>(m_modelType)));
        }
        if (appState.contains("lastSelectedModel")) {
            m_currentModelName = appState["lastSelectedModel"].toString();
            LOG_INFO(QString("已加载上次选择的模型: %1").arg(m_currentModelName));
        }
    }

    // 加载其他设置
    if (root.contains("temperature")) {
        m_temperature = root["temperature"].toDouble();
    }
    if (root.contains("maxTokens")) {
        m_maxTokens = root["maxTokens"].toInt();
    }
    if (root.contains("contextWindow")) {
        m_contextWindow = root["contextWindow"].toInt();
    }
    if (root.contains("theme")) {
        m_theme = root["theme"].toString();
    }
    if (root.contains("fontSize")) {
        m_fontSize = root["fontSize"].toInt();
    }
    if (root.contains("fontFamily")) {
        m_fontFamily = root["fontFamily"].toString();
    }
    if (root.contains("language")) {
        m_language = root["language"].toString();
    }
    if (root.contains("autoSave")) {
        m_autoSave = root["autoSave"].toBool();
    }
    if (root.contains("saveInterval")) {
        m_saveInterval = root["saveInterval"].toInt();
    }
    if (root.contains("proxyEnabled")) {
        m_proxyEnabled = root["proxyEnabled"].toBool();
    }
    if (!m_proxyHost.isEmpty()) {
        m_proxyHost = root["proxyHost"].toString();
    }
    if (m_proxyPort != 80) {
        m_proxyPort = root["proxyPort"].toInt();
    }
    if (root.contains("proxyType")) {
        m_proxyType = root["proxyType"].toString();
    }
    if (!m_proxyUsername.isEmpty()) {
        m_proxyUsername = root["proxyUsername"].toString();
    }
    if (!m_proxyPassword.isEmpty()) {
        m_proxyPassword = root["proxyPassword"].toString();
    }
}

void SettingsModel::loadModelSpecificSettings()
{
    switch (m_modelType) {
        case ModelType::API:
            loadAPIModelSettings();
            break;
        case ModelType::Ollama:
            loadOllamaModelSettings();
            break;
        case ModelType::Local:
            loadLocalModelSettings();
            break;
    }
}

void SettingsModel::loadAPIModelSettings()
{
    // 如果没有选择模型，尝试从配置的模型列表中选择第一个
    if (m_currentModelName.isEmpty() && m_configuredModels.contains("api")) {
        QJsonArray apiModels = m_configuredModels["api"].toArray();
        if (!apiModels.isEmpty()) {
            m_currentModelName = apiModels.first().toString();
            LOG_INFO(QString("自动选择第一个API模型: %1").arg(m_currentModelName));
        }
    }
    
    // 从模型配置中获取提供商和 API URL
    if (!m_currentModelName.isEmpty()) {
        QJsonObject apiConfig = m_models_config["api"].toObject();
        bool found = false;
        
        for (auto providerIt = apiConfig.begin(); providerIt != apiConfig.end(); ++providerIt) {
            if (providerIt.value().isObject()) {
                QJsonObject providerConfig = providerIt.value().toObject();
                if (providerConfig.contains("models")) {
                    QJsonObject models = providerConfig["models"].toObject();
                    if (models.contains(m_currentModelName)) {
                        m_currentProvider = providerIt.key();
                        m_apiKey = providerConfig["api_key"].toString();
                        m_apiUrl = providerConfig["default_url"].toString();
                        
                        // 如果模型配置中有特定的 URL，使用模型的 URL
                        QJsonObject modelConfig = models[m_currentModelName].toObject();
                        if (modelConfig.contains("url") && !modelConfig["url"].toString().isEmpty()) {
                            m_apiUrl = modelConfig["url"].toString();
                        }
                        
                        LOG_INFO(QString("已加载模型配置 - 提供商: %1, API Key: %2, API URL: %3")
                            .arg(m_currentProvider)
                            .arg(m_apiKey.isEmpty() ? "未设置" : "已设置")
                            .arg(m_apiUrl));
                        found = true;
                        break;
                    }
                }
            }
        }
        
        if (!found) {
            LOG_ERROR(QString("未找到模型 %1 的配置").arg(m_currentModelName));
        }
    }
}

void SettingsModel::loadOllamaModelSettings()
{
    if (m_currentModelName.isEmpty() && m_configuredModels.contains("ollama")) {
        QJsonArray ollamaModels = m_configuredModels["ollama"].toArray();
        if (!ollamaModels.isEmpty()) {
            m_currentModelName = ollamaModels.first().toString();
            LOG_INFO(QString("自动选择第一个Ollama模型: %1").arg(m_currentModelName));
        }
    }
    // 获取 Ollama 配置
    QJsonObject ollamaConfig = m_models_config["ollama"].toObject();
    m_ollamaUrl = ollamaConfig["default_url"].toString();
    LOG_INFO(QString("已加载Ollama配置 - URL: %1").arg(m_ollamaUrl));
}

void SettingsModel::loadLocalModelSettings()
{
    if (m_currentModelName.isEmpty() && m_configuredModels.contains("local")) {
        QJsonArray localModels = m_configuredModels["local"].toArray();
        if (!localModels.isEmpty()) {
            m_currentModelName = localModels.first().toString();
            LOG_INFO(QString("自动选择第一个本地模型: %1").arg(m_currentModelName));
        }
    }
    // 从模型配置中获取模型路径
    if (!m_currentModelName.isEmpty()) {
        QJsonObject config = getModelConfig("local", m_currentModelName);
        if (!config.isEmpty()) {
            m_modelPath = config["path"].toString();
            LOG_INFO(QString("已加载本地模型配置 - 路径: %1").arg(m_modelPath));
        } else {
            LOG_WARNING(QString("未找到模型 %1 的配置").arg(m_currentModelName));
        }
    }
}

void SettingsModel::loadSettings()
{
    QJsonObject root;
    if (!loadConfigFile(root)) {
        setDefaultSettings();
        return;
    }

    loadModelConfig(root);
    loadConfiguredModels(root);
    loadBasicSettings(root);
    loadModelSpecificSettings();

    LOG_INFO(QString("设置加载完成 - 当前配置:")
             .arg(static_cast<int>(m_modelType))
             .arg(m_currentModelName)
             .arg(m_currentProvider)
             .arg(m_apiUrl)
             .arg(m_modelPath));

    // 发送所有必要的信号
    emit apiKeyChanged();
    emit modelTypeChanged();
    emit currentModelNameChanged(m_currentModelName);
    emit apiUrlChanged();
    emit modelPathChanged();
}

void SettingsModel::saveSettings()
{
    QString settingsPath = getSettingsPath();
    if (settingsPath.isEmpty()) {
        LOG_ERROR("无法获取设置文件路径");
        return;
    }

    QJsonObject obj;

    // 保存应用状态
    QJsonObject appState;
    appState["lastModelType"] = static_cast<int>(m_modelType);
    appState["lastSelectedModel"] = m_currentModelName;
    obj["appState"] = appState;

    // 保存模型配置
    obj["models_config"] = m_models_config;

    // 保存已配置的模型列表
    obj["configured_models"] = m_configuredModels;

    // 保存其他设置
    if (m_temperature != 0.7) {
        obj["temperature"] = m_temperature;
    }
    if (m_maxTokens != 2048) {
        obj["maxTokens"] = m_maxTokens;
    }
    if (m_contextWindow != 2048) {
        obj["contextWindow"] = m_contextWindow;
    }
    if (!m_theme.isEmpty()) {
        obj["theme"] = m_theme;
    }
    if (m_fontSize != 12) {
        obj["fontSize"] = m_fontSize;
    }
    if (!m_fontFamily.isEmpty()) {
        obj["fontFamily"] = m_fontFamily;
    }
    if (!m_language.isEmpty()) {
        obj["language"] = m_language;
    }
    if (m_autoSave) {
        obj["autoSave"] = m_autoSave;
    }
    if (m_saveInterval != 60) {
        obj["saveInterval"] = m_saveInterval;
    }
    if (m_proxyEnabled) {
        obj["proxyEnabled"] = m_proxyEnabled;
    }
    if (!m_proxyHost.isEmpty()) {
        obj["proxyHost"] = m_proxyHost;
    }
    if (m_proxyPort != 80) {
        obj["proxyPort"] = m_proxyPort;
    }
    if (!m_proxyType.isEmpty()) {
        obj["proxyType"] = m_proxyType;
    }
    if (!m_proxyUsername.isEmpty()) {
        obj["proxyUsername"] = m_proxyUsername;
    }
    if (!m_proxyPassword.isEmpty()) {
        obj["proxyPassword"] = m_proxyPassword;
    }

    QJsonDocument doc(obj);

    // 保存到文件
    QFile file(settingsPath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR("无法保存配置文件");
        return;
    }

    file.write(doc.toJson());
    file.close();

    LOG_INFO("设置保存完成");
}

void SettingsModel::setDefaultSettings()
{
    m_modelType = ModelType::API;
    m_apiKey = "";
    m_currentModelName = "gpt-3.5-turbo";
    m_modelPath = "";
    
    // 初始化默认模型配置
    initializeDefaultModels();
    
    // 设置首次运行标记
    QJsonObject appState;
    appState["isFirstRun"] = true;
    m_appState = appState;
}

void SettingsModel::setAppStateValue(const QString& key, const QJsonValue& value)
{
    m_appState[key] = value;
    scheduleSave();
    emit appStateChanged();
}

QJsonValue SettingsModel::appStateValue(const QString& key) const
{
    return m_appState.value(key);
}

QJsonObject SettingsModel::getModelConfig(const QString& type, const QString& name) const
{
    if (m_models_config.contains(type) && m_models_config[type].isObject()) {
        QJsonObject typeConfig = m_models_config[type].toObject();
        
        // 如果 name 为空，返回整个类型的配置
        if (name.isEmpty()) {
            return typeConfig;
        }
        
        // 对于 API 类型
        if (type == "api") {
            // 遍历所有提供商
            for (auto providerIt = typeConfig.begin(); providerIt != typeConfig.end(); ++providerIt) {
                if (providerIt.key() != "has_API" && providerIt.key() != "default_url" && 
                    providerIt.value().isObject()) {
                    QJsonObject providerConfig = providerIt.value().toObject();
                    if (providerConfig.contains("models")) {
                        QJsonObject models = providerConfig["models"].toObject();
                        if (models.contains(name)) {
                            QJsonObject modelConfig = models[name].toObject();
                            modelConfig["provider"] = providerIt.key();
                            // 如果模型没有指定 url，使用提供商的 default_url
                            if (!modelConfig.contains("url") || modelConfig["url"].toString().isEmpty()) {
                                modelConfig["url"] = providerConfig["default_url"].toString();
                            }
                            return modelConfig;
                        }
                    }
                }
            }
        }
        // 对于其他类型（如 Ollama 和本地模型）
        else if (typeConfig.contains("models")) {
            QJsonObject modelsObj = typeConfig["models"].toObject();
            if (modelsObj.contains(name)) {
                return modelsObj[name].toObject();
            }
        }
    }
    return QJsonObject();
}

void SettingsModel::setModelConfig(const QString& type, const QString& name, const QJsonObject& config)
{
    if (!m_models_config.contains(type)) {
        m_models_config[type] = QJsonObject();
    }
    
    QJsonObject typeConfig = m_models_config[type].toObject();
    
    if (type == "api") {
        // 对于 API 类型，需要找到模型所属的提供商
        for (auto providerIt = typeConfig.begin(); providerIt != typeConfig.end(); ++providerIt) {
            if (providerIt.value().isObject()) {
                QJsonObject providerConfig = providerIt.value().toObject();
                if (providerConfig.contains("models")) {
                    QJsonObject models = providerConfig["models"].toObject();
                    if (models.contains(name)) {
                        models[name] = config;
                        providerConfig["models"] = models;
                        typeConfig[providerIt.key()] = providerConfig;
                        m_models_config[type] = typeConfig;
                        
                        // 更新已配置的模型列表
                        updateConfiguredModels();
                        
                        scheduleSave();
                        emit modelConfigChanged();
                        return;
                    }
                }
            }
        }
    }
    // 对于其他类型，直接更新配置
    if (!typeConfig.contains("models")) {
        typeConfig["models"] = QJsonObject();
    }
    QJsonObject models = typeConfig["models"].toObject();
    models[name] = config;
    typeConfig["models"] = models;
    m_models_config[type] = typeConfig;
    
    // 更新已配置的模型列表
    updateConfiguredModels();
    
    scheduleSave();
    emit modelConfigChanged();
}

QStringList SettingsModel::getAvailableModels(const QString& type) const
{
    QStringList models;
    if (m_models_config.contains(type) && m_models_config[type].isObject()) {
        QJsonObject typeConfig = m_models_config[type].toObject();
        if (typeConfig.contains("models")) {
            QJsonObject modelsObj = typeConfig["models"].toObject();
            for (auto it = modelsObj.begin(); it != modelsObj.end(); ++it) {
                if (it.value().isObject()) {
                    QJsonObject modelObj = it.value().toObject();
                    if (modelObj["enabled"].toBool(true)) {
                        models.append(it.key());
                    }
                }
            }
        } else {
            // 对于按提供商组织的模型（如 API 类型）
            for (auto providerIt = typeConfig.begin(); providerIt != typeConfig.end(); ++providerIt) {
                if (providerIt.key() != "has_API" && providerIt.key() != "default_url" && 
                    providerIt.value().isObject()) {
                    QJsonObject providerModels = providerIt.value().toObject();
                    for (auto modelIt = providerModels.begin(); modelIt != providerModels.end(); ++modelIt) {
                        if (modelIt.value().isObject()) {
                            QJsonObject modelObj = modelIt.value().toObject();
                            if (modelObj["enabled"].toBool(true)) {
                                models.append(modelIt.key());
                            }
                        }
                    }
                }
            }
        }
    }
    return models;
}

bool SettingsModel::isModelEnabled(const QString& type, const QString& name) const
{
    QJsonObject config = getModelConfig(type, name);
    return config["enabled"].toBool(true);
}

void SettingsModel::setModelEnabled(const QString& type, const QString& name, bool enabled)
{
    QJsonObject config = getModelConfig(type, name);
    config["enabled"] = enabled;
    setModelConfig(type, name, config);
}

void SettingsModel::scheduleSave()
{
    m_saveTimer->start();
}

void SettingsModel::setApiKey(const QString &apiKey)
{
    if (m_apiKey != apiKey) {
        m_apiKey = apiKey;
        LOG_INFO("API密钥已更新");
        scheduleSave();
        emit apiKeyChanged();
    }
}

void SettingsModel::setModelPath(const QString &modelPath)
{
    if (m_modelPath != modelPath) {
        m_modelPath = modelPath;
        LOG_INFO(QString("模型路径已更新: %1").arg(modelPath));
        scheduleSave();
        emit modelPathChanged();
    }
}

void SettingsModel::setApiUrl(const QString &apiUrl)
{
    if (m_apiUrl != apiUrl) {
        m_apiUrl = apiUrl;
        LOG_INFO(QString("API地址已更新: %1").arg(apiUrl));
        
        // 如果是 API 类型，更新当前模型的配置
        if (m_modelType == ModelType::API && !m_currentModelName.isEmpty()) {
            QJsonObject config = getModelConfig("api", m_currentModelName);
            if (!config.isEmpty()) {
                config["url"] = apiUrl;
                setModelConfig("api", m_currentModelName, config);
                LOG_INFO(QString("已更新模型 %1 的 API URL 配置").arg(m_currentModelName));
            }
        }
        
        scheduleSave();
        emit apiUrlChanged();
    }
}

void SettingsModel::setModelType(ModelType type)
{
    if (m_modelType != type) {
        m_modelType = type;
        QString typeStr;
        switch (type) {
            case ModelType::API:
                typeStr = "API";
                break;
            case ModelType::Ollama:
                typeStr = "Ollama";
                refreshOllamaModels();
                break;
            case ModelType::Local:
                typeStr = "Local";
                break;
        }
        LOG_INFO(QString("模型类型已更新: %1").arg(typeStr));
        scheduleSave();
        emit modelTypeChanged();
    }
}

void SettingsModel::setCurrentModelName(const QString &name)
{
    if (m_currentModelName != name) {
        m_currentModelName = name;
        LOG_INFO(QString("当前模型已更新: %1").arg(name));
        scheduleSave();
        emit currentModelNameChanged(name);
    }
}

void SettingsModel::refreshOllamaModels()
{
    // 使用QProcess异步获取Ollama模型列表
    QProcess *process = new QProcess(this);
    process->setProgram("ollama");
    process->setArguments(QStringList() << "list");

    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            QString output = QString::fromUtf8(process->readAllStandardOutput());
            QStringList models;

            // 解析输出获取模型列表
            for (const QString &line : output.split('\n')) {
                if (!line.trimmed().isEmpty()) {
                    QString modelName = line.split(' ').first();
                    models.append(modelName);
                }
            }

            setOllamaModels(models);
            LOG_INFO(QString("已刷新Ollama模型列表，共%1个模型").arg(models.size()));
        } else {
            QString error = QString::fromUtf8(process->readAllStandardError());
            LOG_ERROR(QString("获取Ollama模型列表失败: %1").arg(error));
        }

        process->deleteLater();
    });

    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        LOG_ERROR(QString("执行 ollama list 命令失败: %1").arg(error));
        process->deleteLater();
    });

    process->start();
    LOG_INFO("正在刷新Ollama模型列表...");
}

void SettingsModel::setOllamaModels(const QStringList &models)
{
    if (m_ollamaModels != models) {
        m_ollamaModels = models;
        LOG_INFO("Ollama模型列表已更新");
        emit ollamaModelsChanged();
    }
}
