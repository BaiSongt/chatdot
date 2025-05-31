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

void SettingsModel::loadSettings()
{
    QString settingsPath = getSettingsPath();
    if (settingsPath.isEmpty()) {
        LOG_ERROR("无法获取设置文件路径");
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
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        LOG_ERROR("配置文件格式无效");
        return;
    }

    QJsonObject obj = doc.object();

    // 加载并解密 API 密钥
    if (obj.contains("apiKey")) {
        QString encryptedKey = obj["apiKey"].toString();
        // m_apiKey = Encryption::decrypt(encryptedKey);
        m_apiKey = encryptedKey;
        if (m_apiKey.isEmpty()) {
            LOG_WARNING("API密钥解密失败");
        }
    }

    // 加载其他设置
    m_modelType = static_cast<ModelType>(obj.value("modelType").toInt(0));
    m_apiUrl = obj.value("apiUrl").toString("https://api.openai.com/v1/chat/completions");
    m_currentModelName = obj.value("currentModel").toString();
    m_modelPath = obj.value("modelPath").toString();

    // 发送 API URL 变更信号
    emit apiUrlChanged();

    // 根据不同的模型类型设置默认值
    switch (m_modelType) {
        case ModelType::API:
            if (m_currentModelName.isEmpty()) {
                m_currentModelName = obj.value("apiModel").toString("gpt-3.5-turbo");
            }
            break;

        case ModelType::Ollama:
            if (m_currentModelName.isEmpty()) {
                m_currentModelName = obj.value("ollamaModel").toString();
            }
            refreshOllamaModels();
            break;

        case ModelType::Local:
            if (!m_modelPath.isEmpty() && m_currentModelName.isEmpty()) {
                m_currentModelName = QFileInfo(m_modelPath).fileName();
            }
            break;
    }

    LOG_INFO(QString("设置加载完成，当前模型类型: %1，模型名称: %2")
             .arg(static_cast<int>(m_modelType))
             .arg(m_currentModelName));

    emit currentModelNameChanged(m_currentModelName);
    emit modelTypeChanged();
}

void SettingsModel::saveSettings()
{
    QString settingsPath = getSettingsPath();
    if (settingsPath.isEmpty()) {
        LOG_ERROR("无法获取设置文件路径");
        return;
    }

    QJsonObject obj;

    // 加密并保存 API 密钥
    if (!m_apiKey.isEmpty()) {
        // QString encryptedKey = Encryption::encrypt(m_apiKey);
        QString encryptedKey = m_apiKey;
        if (!encryptedKey.isEmpty()) {
            obj["apiKey"] = encryptedKey;
        } else {
            LOG_WARNING("API密钥加密失败");
        }
    }

    // 保存其他设置
    obj["modelType"] = static_cast<int>(m_modelType);
    obj["currentModel"] = m_currentModelName;
    obj["apiUrl"] = m_apiUrl;
    obj["modelPath"] = m_modelPath;

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
    m_apiUrl = "https://api.openai.com/v1/chat/completions";
    m_currentModelName = "gpt-3.5-turbo";
    m_modelPath = "";
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
