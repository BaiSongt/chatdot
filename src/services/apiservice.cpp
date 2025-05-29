#include "apiservice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include "services/logger.h"

APIService::APIService(const QString& apiKey, const QString& apiUrl, const QString& modelName, QObject *parent)
    : LLMService(parent)
    , m_apiKey(apiKey)
    , m_apiUrl(apiUrl)
    , m_currentModelName(modelName)
    , m_networkManager(new QNetworkAccessManager(this))
{
    m_provider = getProviderFromUrl(apiUrl);
    LOG_INFO(QString("初始化 API 服务: %1, 模型: %2").arg(m_provider, m_currentModelName));
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

    // 设置生成参数
    json["temperature"] = 0.7;
    json["max_tokens"] = 2000;

    if (m_provider == "Deepseek") {
        // Deepseek 特定的参数
        json["do_sample"] = true;
        json["top_p"] = 0.8;
    }

    return QJsonDocument(json).toJson();
}

QFuture<QString> APIService::generateResponse(const QString& prompt)
{
    QFutureInterface<QString> future;
    m_currentFuture = future;

    QUrl url(m_apiUrl);
    QNetworkRequest request(url);

    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QString authHeader = QString("Bearer %1").arg(m_apiKey);
    request.setRawHeader("Authorization", authHeader.toUtf8());

    // 发送请求
    QNetworkReply* reply = m_networkManager->post(request, prepareRequestData(prompt));

    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() {
                handleResponse(reply);
                reply->deleteLater();
            });

    return future.future();
}

void APIService::handleResponse(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QString response;
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject json = doc.object();

        if (m_provider == "Deepseek") {
            // Deepseek API 响应格式处理
            if (json.contains("choices") && json["choices"].isArray()) {
                QJsonArray choices = json["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject choice = choices.first().toObject();
                    if (choice.contains("message") && choice["message"].isObject()) {
                        response = choice["message"].toObject()["content"].toString();
                    }
                }
            }
        } else {
            // OpenAI 和其他 API 响应格式处理
            if (json.contains("choices") && json["choices"].isArray()) {
                QJsonArray choices = json["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject choice = choices.first().toObject();
                    if (choice.contains("message") && choice["message"].isObject()) {
                        response = choice["message"].toObject()["content"].toString();
                    }
                }
            }
        }

        if (!response.isEmpty()) {
            m_currentFuture.reportFinished(&response);
        } else {
            QString error = tr("API响应格式错误");
            m_currentFuture.reportException(std::make_exception_ptr(std::runtime_error(error.toStdString())));
        }
    } else {
        QString error = reply->errorString();
        LOG_ERROR(QString("API请求失败: %1").arg(error));
        m_currentFuture.reportException(std::make_exception_ptr(std::runtime_error(error.toStdString())));
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
