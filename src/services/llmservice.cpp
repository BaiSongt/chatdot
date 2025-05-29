#include "llmservice.h"
#include <QDebug>

LLMService::LLMService(QObject *parent)
    : QObject(parent)
{
}

LLMService::~LLMService()
{
}

void LLMService::emitError(const QString& error)
{
    qWarning() << "LLM Service error:" << error;
    emit errorOccurred(error);
} 