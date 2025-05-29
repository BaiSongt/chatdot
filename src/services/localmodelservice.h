#ifndef LOCALMODELSERVICE_H
#define LOCALMODELSERVICE_H

#include "llmservice.h"
#include <QFutureInterface>
#include <QString>

class LocalModelService : public LLMService
{
    Q_OBJECT

public:
    explicit LocalModelService(const QString& modelPath, QObject *parent = nullptr);
    ~LocalModelService() override;

    QFuture<QString> generateResponse(const QString& prompt) override;
    bool isAvailable() const override;
    QString getModelName() const override;

private:
    QString m_modelPath;
    // TODO: Add local model related members (e.g., ONNX Runtime session, model object)
};

#endif // LOCALMODELSERVICE_H 