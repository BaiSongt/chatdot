#include "llmservice.h"
#include "services/logger.h"

LLMService::LLMService(const QString& modelPath, QObject *parent)
    : QObject(parent)
    , m_modelPath(modelPath)
    , m_isCancelled(false)
    , m_isDeepThinking(false)
{
}

LLMService::LLMService(QObject *parent)
    : QObject(parent)
    , m_isCancelled(false)
    , m_isDeepThinking(false)
{
}

LLMService::~LLMService()
{
}

void LLMService::cancelGeneration()
{
    m_isCancelled = true;
    LOG_INFO("模型生成已取消");
}

void LLMService::setDeepThinkingMode(bool enabled)
{
    if (m_isDeepThinking != enabled) {
        m_isDeepThinking = enabled;
        emit deepThinkingModeChanged(enabled);
        LOG_INFO(QString("深度思考模式: %1").arg(enabled ? "开启" : "关闭"));
    }
}
