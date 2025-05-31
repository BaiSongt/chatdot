#include "ollamaservice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QProcess>
#include <QTimer>
#include <QtConcurrent>
#include <exception>
#include <stdexcept>
#include "services/logger.h"

OllamaService::OllamaService(const QString& modelPath, QObject *parent)
    : LLMService(parent)
    , m_modelPath(modelPath)
    , m_networkManager(new QNetworkAccessManager(this))
{
    LOG_INFO(QString("创建 Ollama 服务，模型: %1").arg(modelPath));

    // 设置网络请求超时
    m_networkManager->setTransferTimeout(30000);  // 30秒超时

    // 连接网络错误信号
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, [this](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) {
            LOG_ERROR(QString("网络请求错误: %1").arg(reply->errorString()));
        }
    });
}

OllamaService::~OllamaService()
{
}

QFuture<QString> OllamaService::generateResponse(const QString& prompt)
{
    LOG_INFO("发送 Ollama 请求");
    QFutureInterface<QString> future;
    m_currentFuture = future;

    // 再次检查服务可用性
    if (!isAvailable()) {
        QString error = "Ollama 服务不可用，请确保服务正在运行且模型已下载";
        LOG_ERROR(error);
        future.reportStarted();
        future.reportException(std::make_exception_ptr(std::runtime_error(error.toStdString())));
        future.reportFinished();
        return future.future();
    }

    QUrl url("http://localhost:11434/api/generate");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["model"] = m_modelPath;
    json["prompt"] = prompt;
    json["stream"] = false;
    
    // 根据深度思考模式调整参数
    QJsonObject options;
    if (m_isDeepThinking) {
        options["temperature"] = 0.3;  // 降低温度以获得更确定的回答
        options["top_p"] = 0.7;       // 降低采样范围
        options["num_predict"] = 2000; // 增加生成的最大token数
        options["repeat_penalty"] = 1.2; // 增加重复惩罚
        options["top_k"] = 40;        // 降低top_k值
    } else {
        options["temperature"] = 0.7;
        options["top_p"] = 0.9;
        options["num_predict"] = 1000;
        options["repeat_penalty"] = 1.1;
        options["top_k"] = 50;
    }
    json["options"] = options;

    QByteArray jsonData = QJsonDocument(json).toJson();
    LOG_INFO(QString("Ollama 请求数据: %1").arg(QString(jsonData)));

    QNetworkReply* reply = m_networkManager->post(request, jsonData);

    // 设置单个请求的超时
    QTimer::singleShot(30000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });

    // 连接错误信号
    connect(reply, &QNetworkReply::errorOccurred,
            this, [this](QNetworkReply::NetworkError error) {
        LOG_ERROR(QString("网络请求错误: %1").arg(error));
    });

    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() {
        handleResponse(reply);
        reply->deleteLater();
    });

    future.reportStarted();
    return future.future();
}

bool OllamaService::isAvailable() const
{
    // 检查 Ollama 服务是否运行
    QProcess process;
    process.start("ollama", QStringList() << "list");

    bool started = process.waitForStarted(3000);  // 等待最多3秒
    if (!started) {
        LOG_ERROR("Ollama 服务未运行");
        return false;
    }

    bool finished = process.waitForFinished(3000);  // 等待最多3秒
    if (!finished || process.exitCode() != 0) {
        LOG_ERROR(QString("Ollama 服务检查失败: %1").arg(
            QString::fromUtf8(process.readAllStandardError())));
        return false;
    }

    // 检查模型是否在列表中
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    bool modelFound = output.contains(m_modelPath, Qt::CaseInsensitive);

    if (!modelFound) {
        LOG_ERROR(QString("未找到模型: %1\n可用模型列表:\n%2")
                 .arg(m_modelPath)
                 .arg(output));
        return false;
    }

    LOG_INFO(QString("Ollama 服务可用，已确认模型 %1 存在").arg(m_modelPath));
    return true;
}

QString OllamaService::getModelName() const
{
    return m_modelPath;
}

void OllamaService::handleResponse(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        LOG_INFO(QString("收到 Ollama 响应，长度: %1 字节").arg(data.length()));

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            QString error = QString("JSON解析错误: %1").arg(parseError.errorString());
            LOG_ERROR(error);            m_currentFuture.reportException(std::make_exception_ptr(std::runtime_error(error.toStdString())));
        }
        else if (!doc.isObject()) {
            LOG_ERROR("响应不是有效的JSON对象");
            m_currentFuture.reportException(std::make_exception_ptr(std::runtime_error("响应格式无效")));
        }
        else {
            QJsonObject json = doc.object();
            if (json.contains("response")) {
                QString response = json["response"].toString();
                LOG_INFO(QString("生成的响应长度: %1 字符").arg(response.length()));
                m_currentFuture.reportResult(response);
            }
            else if (json.contains("error")) {
                QString error = json["error"].toString();                LOG_ERROR(QString("Ollama 返回错误: %1").arg(error));
                m_currentFuture.reportException(std::make_exception_ptr(std::runtime_error(error.toStdString())));
            }
            else {
                LOG_ERROR("响应中没有找到 response 或 error 字段");
                m_currentFuture.reportException(std::make_exception_ptr(std::runtime_error("响应格式无效")));
            }
        }
    }    else {
        QString errorString = reply->errorString();
        LOG_ERROR(QString("网络请求失败: %1").arg(errorString));
        m_currentFuture.reportException(std::make_exception_ptr(std::runtime_error(errorString.toStdString())));
    }

    m_currentFuture.reportFinished();
}
