#include "chatviewmodel.h"
#include "services/apiservice.h"
#include "services/ollamaservice.h"
#include "services/localmodelservice.h"
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
        emit errorOccurred("AI模型不可用");
        return;
    }

    QFuture<QString> future = m_llmService->generateResponse(message);
    future.then([this](const QString& response) {
        handleResponse(response);
    }).onFailed([this](const std::exception& e) {
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
} 