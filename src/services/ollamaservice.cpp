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

OllamaService::OllamaService(const QString& modelName, QObject *parent)
    : LLMService(parent)
    , m_modelName(modelName)
    , m_networkManager(new QNetworkAccessManager(this))
{
    LOG_INFO(QString("创建 Ollama 服务，模型: %1").arg(modelName));

    // 设置网络请求超时
    m_networkManager->setTransferTimeout(120000);  // 120秒超时

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
    m_currentResponse.clear();  // 清空当前响应

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
    request.setHeader(QNetworkRequest::UserAgentHeader, "ChatDot/1.0");

    QJsonObject json;
    json["model"] = m_modelName;
    // 添加默认中文回复提示
    QString fullPrompt = QString("请用中文回复以下问题：\n%1").arg(prompt);
    json["prompt"] = fullPrompt;
    json["stream"] = true;  // 启用流式输出
    
    // 默认关闭深度思考模式
    QJsonObject options;
    options["temperature"] = 0.7;
    options["top_p"] = 0.9;
    options["num_predict"] = 1000;
    options["repeat_penalty"] = 1.1;
    options["top_k"] = 50;
    json["options"] = options;

    QByteArray jsonData = QJsonDocument(json).toJson();
    LOG_INFO(QString("Ollama 请求数据: %1").arg(QString(jsonData)));

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
        QString errorMsg;
        switch (error) {
            case QNetworkReply::OperationCanceledError:
                errorMsg = "请求超时被取消";
                break;
            case QNetworkReply::ConnectionRefusedError:
                errorMsg = "连接被拒绝，请确保 Ollama 服务正在运行";
                break;
            case QNetworkReply::HostNotFoundError:
                errorMsg = "无法连接到 Ollama 服务";
                break;
            default:
                errorMsg = QString("网络请求错误: %1").arg(error);
        }
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
            
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &parseError);
            
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject json = doc.object();
                if (json.contains("response")) {
                    QString chunk = json["response"].toString();
                    m_currentResponse = chunk;
                    LOG_INFO(QString("收到响应片段: %1").arg(chunk));
                    
                    // 发送流式响应信号
                    emit streamResponseReceived(m_currentResponse);
                    
                    // 报告当前累积的响应
                    if (m_currentFuture.isRunning()) {
                        // 使用 Qt::QueuedConnection 确保在主线程中更新 UI
                        QMetaObject::invokeMethod(this, [this]() {
                            m_currentFuture.reportResult(m_currentResponse);
                        }, Qt::QueuedConnection);
                    }
                }
                
                // 检查是否是最后一个响应
                if (json.contains("done") && json["done"].toBool()) {
                    LOG_INFO(QString("流式输出完成，总长度: %1 字符").arg(m_currentResponse.length()));
                    if (m_currentFuture.isRunning()) {
                        // 使用 Qt::QueuedConnection 确保在主线程中更新 UI
                        QMetaObject::invokeMethod(this, [this]() {
                            m_currentFuture.reportResult(m_currentResponse);
                            m_currentFuture.reportFinished();
                        }, Qt::QueuedConnection);
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
    bool modelFound = output.contains(m_modelName, Qt::CaseInsensitive);

    if (!modelFound) {
        LOG_ERROR(QString("未找到模型: %1\n可用模型列表:\n%2")
                 .arg(m_modelName)
                 .arg(output));
        return false;
    }

    LOG_INFO(QString("Ollama 服务可用，已确认模型 %1 存在").arg(m_modelName));
    return true;
}

QString OllamaService::getModelName() const
{
    return m_modelName;
}

void OllamaService::handleResponse(QNetworkReply* reply)
{
    // 流式输出模式下不需要处理完整响应
    if (reply->error() != QNetworkReply::NoError) {
        QString error = QString("网络请求错误: %1").arg(reply->errorString());
        LOG_ERROR(error);
        if (m_currentFuture.isRunning()) {
            m_currentFuture.reportException(std::make_exception_ptr(std::runtime_error(error.toStdString())));
            m_currentFuture.reportFinished();
        }
    }
}
