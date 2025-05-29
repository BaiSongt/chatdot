#include "ollamaservice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

OllamaService::OllamaService(const QString& modelPath, QObject *parent)
    : LLMService(parent)
    , m_modelPath(modelPath)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

OllamaService::~OllamaService()
{
}

QFuture<QString> OllamaService::generateResponse(const QString& prompt)
{
    QFutureInterface<QString> future;
    m_currentFuture = future;

    QUrl url("http://localhost:11434/api/generate"); // Ollama API endpoint
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["model"] = m_modelPath;
    json["prompt"] = prompt;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleResponse(reply);
        reply->deleteLater();
    });

    return future.future();
}

bool OllamaService::isAvailable() const
{
    // TODO: Implement a proper check for Ollama service availability
    // This might involve sending a small request or checking a status endpoint
    return !m_modelPath.isEmpty(); // Basic check: model path is set
}

QString OllamaService::getModelName() const
{
    return m_modelPath.isEmpty() ? "Ollama Model" : m_modelPath;
}

void OllamaService::handleResponse(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        // Ollama can return responses in chunks (newline-delimited JSON)
        // For simplicity, we'll just read the whole reply for now.
        // A more robust implementation would process line by line.
        QString response;
        QByteArray replyData = reply->readAll();
        // Assuming the reply is a single JSON object for now
        QJsonDocument doc = QJsonDocument::fromJson(replyData);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            response = obj["response"].toString();
        } else {
             // Handle potential line-delimited JSON (need to parse each line)
             QString dataString = QString::fromUtf8(replyData);
             QStringList lines = dataString.split('\n', Qt::SkipEmptyParts);
             for (const QString& line : lines) {
                 QJsonDocument lineDoc = QJsonDocument::fromJson(line.toUtf8());
                 if (lineDoc.isObject()) {
                     QJsonObject lineObj = lineDoc.object();
                     response += lineObj["response"].toString();
                 }
             }
        }

        m_currentFuture.reportFinished(&response);
    } else {
        QString error = reply->errorString();
        std::exception_ptr e = std::make_exception_ptr(std::runtime_error(error.toStdString()));
        m_currentFuture.reportException(e);
        emit errorOccurred(error);
    }
} 