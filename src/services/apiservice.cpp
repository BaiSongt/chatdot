#include "apiservice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include "services/logger.h"
#include <QTimer>

APIService::APIService(const QString& apiKey, const QString& apiUrl, const QString& modelName, QObject *parent)
    : LLMService(parent)
    , m_apiKey(apiKey)
    , m_apiUrl(apiUrl)
    , m_currentModelName(modelName)
    , m_networkManager(new QNetworkAccessManager(this))
{
    if (apiKey.isEmpty()) {
        LOG_ERROR("API Key 为空");
    }
    if (apiUrl.isEmpty()) {
        LOG_ERROR("API URL 为空");
    }
    if (modelName.isEmpty()) {
        LOG_ERROR("模型名称为空");
    }

    m_provider = getProviderFromUrl(apiUrl);
    LOG_INFO(QString("初始化 API 服务: %1, 模型: %2, URL: %3")
        .arg(m_provider)
        .arg(m_currentModelName)
        .arg(m_apiUrl));
}

APIService::~APIService()
{
}

QString APIService::getProviderFromUrl(const QString& url) const
{
    if (url.contains("api.openai.com")) {
        return "OpenAI";
    } else if (url.contains("api.deepseek.com")) {
        return "Deepseek";
    }
    return "Custom";
}

QByteArray APIService::prepareRequestData(const QString& prompt) const
{
    QJsonObject json;

    // 基本请求结构
    json["model"] = getModelName();

    // 根据不同提供商准备消息格式
    QJsonArray messages;
    QJsonObject message;
    message["role"] = "user";
    message["content"] = prompt;
    messages.append(message);
    json["messages"] = messages;

    // 根据深度思考模式设置生成参数
    if (m_isDeepThinking) {
        json["temperature"] = 0.3;  // 降低温度以获得更确定的回答
        json["max_tokens"] = 4000;  // 增加最大token数
        json["top_p"] = 0.7;        // 降低采样范围
        json["frequency_penalty"] = 0.5; // 增加频率惩罚
        json["presence_penalty"] = 0.5;  // 增加存在惩罚
    } else {
        json["temperature"] = 0.7;
        json["max_tokens"] = 2000;
        json["top_p"] = 0.9;
        json["frequency_penalty"] = 0.0;
        json["presence_penalty"] = 0.0;
    }

    if (m_provider == "Deepseek") {
        // Deepseek 特定的参数
        json["do_sample"] = true;
        if (m_isDeepThinking) {
            json["top_p"] = 0.7;
            json["top_k"] = 40;
        } else {
            json["top_p"] = 0.8;
            json["top_k"] = 50;
        }
    }

    return QJsonDocument(json).toJson();
}

QFuture<QString> APIService::generateResponse(const QString& prompt)
{
    QFutureInterface<QString> future;
    m_currentFuture = future;
    m_currentResponse.clear();  // 清空当前响应

    QUrl url(m_apiUrl);
    QNetworkRequest request(url);

    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString authHeader = QString("Bearer %1").arg(m_apiKey);
    request.setRawHeader("Authorization", authHeader.toUtf8());

    // 准备请求数据
    QJsonObject json;
    json["model"] = getModelName();
    json["stream"] = true;  // 启用流式输出

    // 根据不同提供商准备消息格式
    QJsonArray messages;
    QJsonObject message;
    message["role"] = "user";
    message["content"] = prompt;
    messages.append(message);
    json["messages"] = messages;

    // 根据深度思考模式设置生成参数
    if (m_isDeepThinking) {
        json["thinking"] = true;
        json["temperature"] = 0.3;
        json["max_tokens"] = 4000;
        json["top_p"] = 0.7;
        json["frequency_penalty"] = 0.5;
        json["presence_penalty"] = 0.5;
    } else {
        json["thinking"] = false;
        json["temperature"] = 0.7;
        json["max_tokens"] = 2000;
        json["top_p"] = 0.9;
        json["frequency_penalty"] = 0.0;
        json["presence_penalty"] = 0.0;
    }

    QByteArray jsonData = QJsonDocument(json).toJson();
    LOG_INFO(QString("API 请求数据: %1").arg(QString(jsonData)));

    // 发送请求
    QNetworkReply* reply = m_networkManager->post(request, jsonData);

    // 设置单个请求的超时
    QTimer::singleShot(120000, reply, [reply]() {  // 120秒
        if (reply->isRunning()) {
            LOG_WARNING("请求超时，正在取消...");
            reply->abort();
        }
    });

    // 连接错误信号
    connect(reply, &QNetworkReply::errorOccurred,
            this, [this, reply](QNetworkReply::NetworkError error) {
        QString errorMsg = QString("网络错误: %1").arg(reply->errorString());
        LOG_ERROR(errorMsg);
        if (m_currentFuture.isRunning()) {
            m_currentFuture.reportException(std::make_exception_ptr(std::runtime_error(errorMsg.toStdString())));
            m_currentFuture.reportFinished();
        }
    });

    // 连接数据接收信号
    connect(reply, &QNetworkReply::readyRead,
            this, [this, reply]() {
        QByteArray data = reply->readAll();
        QStringList lines = QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts);
        
        for (const QString& line : lines) {
            if (line.trimmed().isEmpty()) continue;
            
            // 移除 "data: " 前缀
            QString jsonStr = line.trimmed();
            if (jsonStr.startsWith("data: ")) {
                jsonStr = jsonStr.mid(6);
            }
            
            // 检查是否是结束标记
            if (jsonStr == "[DONE]") {
                LOG_INFO(QString("流式输出完成，总长度: %1 字符").arg(m_currentResponse.length()));
                if (m_currentFuture.isRunning()) {
                    QMetaObject::invokeMethod(this, [this]() {
                        m_currentFuture.reportResult(m_currentResponse);
                        m_currentFuture.reportFinished();
                    }, Qt::QueuedConnection);
                }
                continue;
            }
            
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
            
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject json = doc.object();
                if (json.contains("choices") && json["choices"].isArray()) {
                    QJsonArray choices = json["choices"].toArray();
                    if (!choices.isEmpty()) {
                        QJsonObject choice = choices.first().toObject();
                        if (choice.contains("delta") && choice["delta"].isObject()) {
                            QJsonObject delta = choice["delta"].toObject();
                            if (delta.contains("content")) {
                                QString chunk = delta["content"].toString();
                                m_currentResponse = chunk;
                                LOG_INFO(QString("收到响应片段: %1").arg(chunk));
                                
                                // 发送流式响应信号
                                emit streamResponseReceived(m_currentResponse);
                                
                                // 报告当前累积的响应
                                if (m_currentFuture.isRunning()) {
                                    QMetaObject::invokeMethod(this, [this]() {
                                        m_currentFuture.reportResult(m_currentResponse);
                                    }, Qt::QueuedConnection);
                                }
                            }
                        }
                    }
                }
            } else {
                LOG_WARNING(QString("解析响应失败: %1").arg(parseError.errorString()));
            }
        }
    });

    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() {
        reply->deleteLater();
    });

    future.reportStarted();
    return future.future();
}

void APIService::handleResponse(QNetworkReply* reply)
{
    // 这个方法现在只用于处理非流式响应
    if (reply->error() == QNetworkReply::NoError) {
        QString response;
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject json = doc.object();

        if (json.contains("choices") && json["choices"].isArray()) {
            QJsonArray choices = json["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject choice = choices.first().toObject();
                if (choice.contains("message") && choice["message"].isObject()) {
                    response = choice["message"].toObject()["content"].toString();
                }
            }
        }

        if (!response.isEmpty()) {
            m_currentFuture.reportFinished(&response);
        } else {
            QString error = tr("API响应格式错误");
            LOG_ERROR(error);
            m_currentFuture.reportFinished(&error);
        }
    } else {
        QString error = QString("API请求失败: %1").arg(reply->errorString());
        LOG_ERROR(error);
        m_currentFuture.reportFinished(&error);
    }
}

bool APIService::isAvailable() const
{
    return !m_apiKey.isEmpty() && !m_apiUrl.isEmpty();
}

QString APIService::getModelName() const
{
    return m_currentModelName;
}
