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
    , m_isGenerating(false)
{
}

ChatViewModel::~ChatViewModel()
{
    delete m_llmService;
}

void ChatViewModel::sendMessage(const QString& message)
{
    if (message.isEmpty()) {
        return;
    }

    // 检查是否选择了AI模型
    if (!m_llmService) {
        emit errorOccurred(tr("未选择AI模型，请先在设置中选择一个模型"));
        return;
    }

    // 添加用户消息到聊天记录
    m_model->addMessage("user", message);

    // 开始生成回复
    emit generationStarted();
    m_isGenerating = true;
    m_isCancelled = false;
    m_currentResponse.clear();

    // 发送消息到AI服务
    QFuture<QString> future = m_llmService->generateResponse(message);

    // 使用QFuture的异步回调处理响应和错误
    future.then([this](const QString& response) {
        if (!m_isCancelled) {
            LOG_INFO(QString("收到完整响应: %1字符").arg(response.length()));
            handleResponse(response);
        }
        m_isGenerating = false;
        emit generationFinished();
    }).onFailed([this](const std::exception& e) {
        LOG_ERROR(QString("处理消息时发生错误: %1").arg(e.what()));
        handleError(QString::fromStdString(e.what()));
        m_isGenerating = false;
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
    // 如果当前有服务，先清理它
    if (m_llmService) {
        // 断开所有信号连接
        disconnect(m_llmService, nullptr, this, nullptr);
        // 删除旧服务
        delete m_llmService;
        m_llmService = nullptr;
    }

    // 设置新服务
    m_llmService = service;
    if (m_llmService) {
        // 连接新服务的信号
        connect(m_llmService, &LLMService::streamResponseReceived,
                this, &ChatViewModel::handleStreamResponse,
                Qt::UniqueConnection);

        LOG_INFO(QString("已切换到模型: %1").arg(m_llmService->getModelName()));
    }
}

void ChatViewModel::clearChat()
{
    // 清空聊天模型中的消息
    m_model->clearMessages();
    LOG_INFO("聊天记录已清除");
}
