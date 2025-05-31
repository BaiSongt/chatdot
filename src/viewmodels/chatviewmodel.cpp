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
    , m_isDeepThinking(false)
{
}

ChatViewModel::~ChatViewModel()
{
    delete m_llmService;
}

void ChatViewModel::sendMessage(const QString& message)
{
    if (message.isEmpty()) {
        LOG_WARNING("尝试发送空消息");
        return;
    }

    // 检查是否选择了AI模型
    if (!m_llmService) {
        QString errorMsg = tr("未选择AI模型，请先在设置中选择一个模型");
        LOG_ERROR(errorMsg);
        emit errorOccurred(errorMsg);
        return;
    }

    // 检查服务是否可用
    if (!m_llmService->isAvailable()) {
        QString errorMsg = tr("当前选择的模型服务不可用，请检查配置");
        LOG_ERROR(errorMsg);
        emit errorOccurred(errorMsg);
        return;
    }

    // 添加用户消息到聊天记录
    m_model->addMessage("user", message);
    LOG_INFO(QString("发送用户消息: %1").arg(message));

    // 开始生成回复
    emit generationStarted();
    m_isGenerating = true;
    m_isCancelled = false;
    m_currentResponse.clear();

    // 发送消息到AI服务
    QFuture<QString> future = m_llmService->generateResponse(message);
    LOG_INFO("已发送消息到AI服务，等待响应...");

    // 使用QFuture的异步回调处理响应和错误
    future.then([this](const QString& response) {
        if (!m_isCancelled) {
            LOG_INFO(QString("收到完整响应: %1字符").arg(response.length()));
            handleResponse(response);
        } else {
            LOG_INFO("生成已被取消");
        }
        m_isGenerating = false;
        emit generationFinished();
    }).onFailed([this](const std::exception& e) {
        QString errorMsg = QString("处理消息时发生错误: %1").arg(e.what());
        LOG_ERROR(errorMsg);
        handleError(errorMsg);
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
        m_currentResponse = partialResponse;
        emit streamResponse(partialResponse);
        // LOG_INFO(QString("收到流式响应: %1").arg(partialResponse));
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
                Qt::QueuedConnection);  // 使用 QueuedConnection 确保在主线程中处理

        connect(m_llmService, &LLMService::responseGenerated,
                this, &ChatViewModel::handleResponse,
                Qt::QueuedConnection);

        connect(m_llmService, &LLMService::errorOccurred,
                this, &ChatViewModel::handleError,
                Qt::QueuedConnection);

        LOG_INFO(QString("已切换到模型: %1").arg(m_llmService->getModelName()));
    }
}

void ChatViewModel::clearChat()
{
    // 清空聊天模型中的消息
    m_model->clearMessages();
    LOG_INFO("聊天记录已清除");
}

QString ChatViewModel::getServiceStatus() const
{
    if (!m_llmService) {
        return "未设置服务";
    }

    QString status;
    if (dynamic_cast<APIService*>(m_llmService)) {
        status = "API服务";
    } else if (dynamic_cast<OllamaService*>(m_llmService)) {
        status = "Ollama服务";
    } else if (dynamic_cast<LocalModelService*>(m_llmService)) {
        status = "本地服务";
    } else {
        status = "未知服务类型";
    }

    return status;
}

void ChatViewModel::setDeepThinkingMode(bool enabled)
{
    if (m_isDeepThinking != enabled) {
        m_isDeepThinking = enabled;
        emit deepThinkingModeChanged(enabled);
        LOG_INFO(QString("深度思考模式: %1").arg(enabled ? "开启" : "关闭"));
    }
}
