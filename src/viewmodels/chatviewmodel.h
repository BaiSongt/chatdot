#ifndef CHATVIEWMODEL_H
#define CHATVIEWMODEL_H

#include <QObject>
#include <QFuture>
#include <QString>
#include "models/chatmodel.h"
#include "services/llmservice.h"

class ChatViewModel : public QObject
{
    Q_OBJECT

public:
    explicit ChatViewModel(ChatModel* model, QObject *parent = nullptr);
    ~ChatViewModel();

    Q_INVOKABLE void sendMessage(const QString& message);
    Q_INVOKABLE void clearChat();

signals:
    void responseReceived(const QString& response);
    void errorOccurred(const QString& error);

public slots:
    void setLLMService(LLMService* service);

private slots:
    void handleResponse(const QString& response);
    void handleError(const QString& error);


private:
    ChatModel* m_model;
    LLMService* m_llmService;
};

#endif // CHATVIEWMODEL_H
