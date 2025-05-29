#include "settingsmodel.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QSettings>
#include "services/logger.h"

// 单例实现
SettingsModel& SettingsModel::instance()
{
    static SettingsModel instance;
    return instance;
}

SettingsModel::SettingsModel(QObject *parent)
    : QObject(parent)
{
    LOG_INFO("初始化 SettingsModel");
    loadSettings();
}

QString SettingsModel::getSettingsPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + "/settings.json";
}

void SettingsModel::loadSettings()
{
    QSettings settings;

    // 加载模型类型
    m_modelType = static_cast<ModelType>(settings.value("modelType", 0).toInt());

    // 加载API设置
    m_apiKey = settings.value("apiKey").toString();
    m_apiUrl = settings.value("apiUrl", "https://api.openai.com/v1").toString();

    // 根据不同的模型类型加载相应的设置
    switch (m_modelType) {
        case ModelType::API:
            m_currentModelName = settings.value("apiModel", "gpt-3.5-turbo").toString();
            break;

        case ModelType::Ollama:
            // 先刷新模型列表
            refreshOllamaModels();
            m_currentModelName = settings.value("ollamaModel").toString();
            // 如果没有设置模型名称且有可用模型，使用第一个
            if (m_currentModelName.isEmpty() && !m_ollamaModels.isEmpty()) {
                m_currentModelName = m_ollamaModels.first();
            }
            break;

        case ModelType::Local:
            m_modelPath = settings.value("localModelPath").toString();
            if (!m_modelPath.isEmpty()) {
                m_currentModelName = QFileInfo(m_modelPath).fileName();
            }
            break;
    }

    LOG_INFO(QString("设置加载完成，当前模型类型: %1，模型名称: %2")
             .arg(static_cast<int>(m_modelType))
             .arg(m_currentModelName));

    emit currentModelNameChanged(m_currentModelName);
}

void SettingsModel::saveSettings()
{
    QSettings settings;

    settings.setValue("modelType", static_cast<int>(m_modelType));
    settings.setValue("apiKey", m_apiKey);
    settings.setValue("currentModel", m_currentModelName);
    settings.setValue("apiUrl", m_apiUrl);
    settings.setValue("modelPath", m_modelPath);

    settings.sync();
    LOG_INFO("设置保存完成");
}

void SettingsModel::setApiKey(const QString &apiKey)
{
    if (m_apiKey != apiKey) {
        m_apiKey = apiKey;
        LOG_INFO("API密钥已更新");
        saveSettings();
        emit apiKeyChanged();
    }
}

void SettingsModel::setModelPath(const QString &modelPath)
{
    if (m_modelPath != modelPath) {
        m_modelPath = modelPath;
        LOG_INFO(QString("模型路径已更新: %1").arg(modelPath));
        saveSettings();
        emit modelPathChanged();
    }
}

void SettingsModel::setApiUrl(const QString &apiUrl)
{
    if (m_apiUrl != apiUrl) {
        m_apiUrl = apiUrl;
        LOG_INFO(QString("API地址已更新: %1").arg(apiUrl));
        saveSettings();
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
        saveSettings();
        emit modelTypeChanged();
    }
}

void SettingsModel::setCurrentModelName(const QString &name)
{
    if (m_currentModelName != name) {
        m_currentModelName = name;
        LOG_INFO(QString("当前模型已更新: %1").arg(name));
        saveSettings();
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
