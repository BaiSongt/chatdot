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
    // 初始化 API 模型配置
    QJsonObject apiModels;
    
    // OpenAI 模型
    QJsonObject gpt35;
    gpt35["name"] = "gpt-3.5-turbo";
    gpt35["provider"] = "OpenAI";
    gpt35["url"] = "https://api.openai.com/v1/chat/completions";
    gpt35["enabled"] = true;
    apiModels["gpt-3.5-turbo"] = gpt35;

    QJsonObject gpt4;
    gpt4["name"] = "gpt-4";
    gpt4["provider"] = "OpenAI";
    gpt4["url"] = "https://api.openai.com/v1/chat/completions";
    gpt4["enabled"] = true;
    apiModels["gpt-4"] = gpt4;

    // Deepseek 模型
    QJsonObject deepseek;
    deepseek["name"] = "deepseek-chat";
    deepseek["provider"] = "Deepseek";
    deepseek["url"] = "https://api.deepseek.com/v1/chat/completions";
    deepseek["enabled"] = true;
    apiModels["deepseek-chat"] = deepseek;

    m_models["api"] = apiModels;

    // 初始化 Ollama 模型配置
    QJsonObject ollamaModels;
    m_models["ollama"] = ollamaModels;

    // 初始化本地模型配置
    QJsonObject localModels;
    m_models["local"] = localModels;
}

void SettingsModel::loadSettings()
{
    QString settingsPath = getSettingsPath();
    if (settingsPath.isEmpty()) {
        LOG_ERROR("无法获取设置文件路径");
        setDefaultSettings();
        return;
    }

    QFile file(settingsPath);
    if (!file.exists()) {
        LOG_INFO("设置文件不存在，使用默认值");
        setDefaultSettings();
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR("无法打开配置文件");
        setDefaultSettings();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        LOG_ERROR("配置文件格式无效");
        setDefaultSettings();
        return;
    }

    QJsonObject obj = doc.object();

    // 加载 API 密钥
    if (obj.contains("apiKey")) {
        m_apiKey = obj["apiKey"].toString();
    }

    // 加载应用状态
    if (obj.contains("appState") && obj["appState"].isObject()) {
        m_appState = obj["appState"].toObject();
    } else {
        m_appState = QJsonObject();
        m_appState["isFirstRun"] = true;
    }

    // 加载模型配置
    if (obj.contains("models") && obj["models"].isObject()) {
        m_models = obj["models"].toObject();
    } else {
        initializeDefaultModels();
    }

    // 从应用状态中恢复上次的模型选择
    if (m_appState.contains("lastModelType")) {
        m_modelType = static_cast<ModelType>(m_appState["lastModelType"].toInt(0));
    }
    if (m_appState.contains("lastSelectedModel")) {
        m_currentModelName = m_appState["lastSelectedModel"].toString();
    }

    // 根据不同的模型类型设置默认值
    switch (m_modelType) {
        case ModelType::API: {
            if (m_currentModelName.isEmpty()) {
                m_currentModelName = "gpt-3.5-turbo";
            }
            // 从模型配置中获取 API URL
            QJsonObject config = getModelConfig("api", m_currentModelName);
            if (!config.isEmpty()) {
                m_apiUrl = config["url"].toString();
                LOG_INFO(QString("从配置加载 API URL: %1").arg(m_apiUrl));
            } else {
                LOG_WARNING(QString("未找到模型 %1 的配置").arg(m_currentModelName));
            }
            break;
        }
        case ModelType::Ollama: {
            if (m_currentModelName.isEmpty()) {
                m_currentModelName = m_appState["ollamaModel"].toString();
            }
            refreshOllamaModels();
            break;
        }
        case ModelType::Local: {
            if (!m_modelPath.isEmpty() && m_currentModelName.isEmpty()) {
                m_currentModelName = QFileInfo(m_modelPath).fileName();
            }
            // 从模型配置中获取模型路径
            QJsonObject config = getModelConfig("local", m_currentModelName);
            if (!config.isEmpty()) {
                m_modelPath = config["path"].toString();
                LOG_INFO(QString("从配置加载模型路径: %1").arg(m_modelPath));
            } else {
                LOG_WARNING(QString("未找到模型 %1 的配置").arg(m_currentModelName));
            }
            break;
        }
    }

    LOG_INFO(QString("设置加载完成，当前模型类型: %1，模型名称: %2，API URL: %3，模型路径: %4")
             .arg(static_cast<int>(m_modelType))
             .arg(m_currentModelName)
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

    // 保存 API 密钥
    if (!m_apiKey.isEmpty()) {
        obj["apiKey"] = m_apiKey;
    }

    // 保存应用状态
    m_appState["lastModelType"] = static_cast<int>(m_modelType);
    m_appState["lastSelectedModel"] = m_currentModelName;
    obj["appState"] = m_appState;

    // 保存模型配置
    obj["models"] = m_models;

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
    m_appState = QJsonObject();
    m_appState["isFirstRun"] = true;
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
    if (m_models.contains(type) && m_models[type].isObject()) {
        QJsonObject typeObj = m_models[type].toObject();
        if (typeObj.contains(name) && typeObj[name].isObject()) {
            return typeObj[name].toObject();
        }
    }
    return QJsonObject();
}

void SettingsModel::setModelConfig(const QString& type, const QString& name, const QJsonObject& config)
{
    if (!m_models.contains(type)) {
        m_models[type] = QJsonObject();
    }
    QJsonObject typeObj = m_models[type].toObject();
    typeObj[name] = config;
    m_models[type] = typeObj;
    scheduleSave();
    emit modelConfigChanged();
}

QStringList SettingsModel::getAvailableModels(const QString& type) const
{
    QStringList models;
    if (m_models.contains(type) && m_models[type].isObject()) {
        QJsonObject typeObj = m_models[type].toObject();
        for (auto it = typeObj.begin(); it != typeObj.end(); ++it) {
            if (it.value().isObject()) {
                QJsonObject modelObj = it.value().toObject();
                if (modelObj["enabled"].toBool(true)) {
                    models.append(it.key());
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
