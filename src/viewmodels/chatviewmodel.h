#ifndef CHATVIEWMODEL_H
#define CHATVIEWMODEL_H

#include <QObject>
#include <QFuture>
#include <QString>
#include "models/chatmodel.h"
#include "services/llmservice.h"
#include "services/apiservice.h"
#include "services/ollamaservice.h"
#include "services/localmodelservice.h"

class ChatViewModel : public QObject
{
    Q_OBJECT

public:
    explicit ChatViewModel(ChatModel* model, QObject *parent = nullptr);
    ~ChatViewModel();

    Q_INVOKABLE void sendMessage(const QString& message);
    Q_INVOKABLE void clearChat();
    Q_INVOKABLE void cancelGeneration();

    // 设置和获取 LLMService
    void setLLMService(LLMService* service);
    bool hasLLMService() const { return m_llmService != nullptr; }
    QString getServiceStatus() const;

signals:
    void responseReceived(const QString& response);
    void errorOccurred(const QString& error);
    void generationStarted();
    void generationFinished();
    void streamResponse(const QString& partialResponse);

public slots:
    void handleResponse(const QString& response);
    void handleError(const QString& error);
    void handleStreamResponse(const QString& partialResponse);

private:
    ChatModel* m_model;
    LLMService* m_llmService;
    bool m_isCancelled;
    QString m_currentResponse;
    bool m_isGenerating;
};

#endif // CHATVIEWMODEL_H
