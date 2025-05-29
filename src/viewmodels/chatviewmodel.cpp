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
    , m_isCancelled(false)
    , m_currentResponse("")
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
        LOG_INFO(QString("模型 %1 不可用").arg(modelName));
        emit errorOccurred(QString("AI模型 %1 不可用，请检查服务是否正常运行").arg(modelName));
        return;
    }

    m_isCancelled = false;
    m_currentResponse.clear();
    emit generationStarted();

    // 添加用户消息到模型
    m_model->addMessage("user", message);

    LOG_INFO(QString("发送消息到模型 %1").arg(m_llmService->getModelName()));

    // 连接流式输出信号
    connect(m_llmService, &LLMService::streamResponseReceived,
            this, &ChatViewModel::handleStreamResponse,
            Qt::UniqueConnection);

    QFuture<QString> future = m_llmService->generateResponse(message);

    // 使用QFuture的异步回调处理响应和错误
    future.then([this](const QString& response) {
        if (!m_isCancelled) {
            LOG_INFO(QString("收到完整响应: %1字符").arg(response.length()));
            handleResponse(response);
        }
        emit generationFinished();
    }).onFailed([this](const std::exception& e) {
        LOG_ERROR(QString("处理消息时发生错误: %1").arg(e.what()));
        handleError(QString::fromStdString(e.what()));
        emit generationFinished();
    });
}

void ChatViewModel::cancelGeneration()
{
    if (m_llmService) {
        m_isCancelled = true;
        m_llmService->cancelGeneration();
        LOG_INFO("已取消生成");

        // 如果有部分响应，添加到模型中
        if (!m_currentResponse.isEmpty()) {
            m_model->addMessage("assistant", m_currentResponse);
        }

        emit generationFinished();
    }
}

void ChatViewModel::handleStreamResponse(const QString& partialResponse)
{
    if (!m_isCancelled) {
        m_currentResponse += partialResponse;
        emit streamResponse(partialResponse);
    }
}

void ChatViewModel::handleResponse(const QString& response)
{
    if (!m_isCancelled) {
        m_model->addMessage("assistant", response);
        emit responseReceived(response);
    }
}

void ChatViewModel::handleError(const QString& error)
{
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
