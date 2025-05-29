#include "chatviewmodel.h"
#include "services/apiservice.h"
#include "services/ollamaservice.h"
#include "services/localmodelservice.h"
#include "services/logger.h"
#include <QDebug>

ChatViewModel::ChatViewModel(ChatModel* model, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_llmService(nullptr)
{
}

ChatViewModel::~ChatViewModel()
{
    delete m_llmService;
}

void ChatViewModel::sendMessage(const QString& message)
{
    if (!m_llmService) {
        emit errorOccurred("未选择AI模型");
        return;
    }

    if (!m_llmService->isAvailable()) {
        QString modelName = m_llmService->getModelName();
        LOG_ERROR(QString("模型 %1 不可用").arg(modelName));
        emit errorOccurred(QString("AI模型 %1 不可用，请检查服务是否正常运行").arg(modelName));
        return;
    }

    LOG_INFO(QString("发送消息到模型 %1").arg(m_llmService->getModelName()));
    QFuture<QString> future = m_llmService->generateResponse(message);

    // 使用QFuture的异步回调处理响应和错误
    future.then([this](const QString& response) {
        LOG_INFO(QString("收到模型响应: %1").arg(response.length()));
        handleResponse(response);
    }).onFailed([this](const std::exception& e) {
        LOG_ERROR(QString("处理消息时发生错误: %1").arg(e.what()));
        handleError(QString::fromStdString(e.what()));
    });
}

void ChatViewModel::clearChat()
{
    m_model->clearMessages();
}

void ChatViewModel::handleResponse(const QString& response)
{
    emit responseReceived(response);
}

void ChatViewModel::handleError(const QString& error)
{
    qWarning() << "Chat error:" << error;
    emit errorOccurred(error);
}

void ChatViewModel::setLLMService(LLMService* service)
{
    if (m_llmService) {
        delete m_llmService;
    }
    m_llmService = service;
    if (!m_llmService) {
        emit errorOccurred("无法创建AI模型服务");
    }
}
