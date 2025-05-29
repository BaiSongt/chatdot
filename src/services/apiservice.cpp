#include "apiservice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

APIService::APIService(const QString& apiKey, QObject *parent)
    : LLMService(parent)
    , m_apiKey(apiKey)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

APIService::~APIService()
{
}

QFuture<QString> APIService::generateResponse(const QString& prompt)
{
    QFutureInterface<QString> future;
    m_currentFuture = future;

    QNetworkRequest request(QUrl("https://api.openai.com/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    QJsonObject json;
    json["model"] = "gpt-3.5-turbo";
    json["messages"] = QJsonArray{QJsonObject{
        {"role", "user"},
        {"content", prompt}
    }};

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleResponse(reply);
        reply->deleteLater();
    });

    return future.future();
}

bool APIService::isAvailable() const
{
    return !m_apiKey.isEmpty();
}

QString APIService::getModelName() const
{
    return "GPT-3.5-Turbo";
}

void APIService::handleResponse(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        QString response = obj["choices"].toArray().first().toObject()["message"].toObject()["content"].toString();
        m_currentFuture.reportFinished(&response);
    } else {
        QString error = reply->errorString();
        std::exception_ptr e = std::make_exception_ptr(std::runtime_error(error.toStdString()));
        m_currentFuture.reportException(e);
        emit errorOccurred(error);
    }
} 