#include "settingsmodel.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include "services/logger.h"

SettingsModel::SettingsModel(QObject *parent)
    : QObject(parent)
{
    LOG_INFO("初始化 SettingsModel");
    // 设置默认值
    m_modelType = ModelType::API;
    m_apiUrl = "http://localhost:11434";
}

void SettingsModel::setApiKey(const QString &apiKey)
{
    if (m_apiKey != apiKey) {
        m_apiKey = apiKey;
        LOG_INFO("API密钥已更新");
        emit apiKeyChanged();
    }
}

void SettingsModel::setModelPath(const QString &modelPath)
{
    if (m_modelPath != modelPath) {
        m_modelPath = modelPath;
        LOG_INFO(QString("模型路径已更新: %1").arg(modelPath));
        emit modelPathChanged();
    }
}

void SettingsModel::setApiUrl(const QString &apiUrl)
{
    if (m_apiUrl != apiUrl) {
        m_apiUrl = apiUrl;
        LOG_INFO(QString("API地址已更新: %1").arg(apiUrl));
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
                break;
            case ModelType::Local:
                typeStr = "Local";
                break;
        }
        LOG_INFO(QString("模型类型已更新: %1").arg(typeStr));
        emit modelTypeChanged();
    }
}

bool SettingsModel::saveToFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("无法打开设置文件进行写入: %1").arg(filename));
        return false;
    }

    QJsonDocument doc(toJson());
    if (file.write(doc.toJson()) == -1) {
        LOG_ERROR("写入设置文件失败");
        return false;
    }
    
    LOG_INFO("设置已成功保存到文件");
    return true;
}

bool SettingsModel::loadFromFile(const QString &filename)
{
    QFile file(filename);
    if (!file.exists()) {
        LOG_INFO("设置文件不存在，将使用默认设置");
        return false;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("无法打开设置文件进行读取: %1").arg(filename));
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        LOG_ERROR("设置文件格式无效");
        return false;
    }

    fromJson(doc.object());
    LOG_INFO("设置已成功从文件加载");
    return true;
}

QJsonObject SettingsModel::toJson() const
{
    QJsonObject obj;
    obj["apiKey"] = m_apiKey;
    obj["modelPath"] = m_modelPath;
    obj["apiUrl"] = m_apiUrl;
    obj["modelType"] = static_cast<int>(m_modelType);
    return obj;
}

void SettingsModel::fromJson(const QJsonObject &json)
{
    if (json.contains("apiKey")) {
        setApiKey(json["apiKey"].toString());
    }
    if (json.contains("modelPath")) {
        setModelPath(json["modelPath"].toString());
    }
    if (json.contains("apiUrl")) {
        setApiUrl(json["apiUrl"].toString());
    }
    if (json.contains("modelType")) {
        int type = json["modelType"].toInt();
        if (type >= 0 && type <= 2) {  // 确保类型值有效
            setModelType(static_cast<ModelType>(type));
        }
    }
} 