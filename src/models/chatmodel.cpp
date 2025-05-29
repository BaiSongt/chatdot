#include "chatmodel.h"

ChatModel::ChatModel(QObject *parent)
    : QObject(parent)
{
}

void ChatModel::addMessage(const QString& role, const QString& content)
{
    Message msg;
    msg.role = role;
    msg.content = content;
    msg.timestamp = QDateTime::currentDateTime();
    
    m_messages.append(msg);
    emit messagesChanged();
}

void ChatModel::clearMessages()
{
    m_messages.clear();
    emit messagesChanged();
} 