#include "llmservice.h"
#include "services/logger.h"

LLMService::LLMService(const QString& modelPath, QObject *parent)
    : QObject(parent)
    , m_modelPath(modelPath)
    , m_isCancelled(false)
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

void LLMService::emitError(const QString& error)
{
    qWarning() << "LLM Service error:" << error;
    emit errorOccurred(error);
}
