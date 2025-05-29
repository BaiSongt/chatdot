#ifndef CHATMODEL_H
#define CHATMODEL_H

#include <QObject>
#include <QList>
#include <QDateTime>

class ChatModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<Message> messages READ messages NOTIFY messagesChanged)

public:
    explicit ChatModel(QObject *parent = nullptr);

    struct Message {
        QString role;
        QString content;
        QDateTime timestamp;
    };

    QList<Message> messages() const { return m_messages; }

public slots:
    void addMessage(const QString& role, const QString& content);
    void clearMessages();

signals:
    void messagesChanged();

private:
    QList<Message> m_messages;
};

#endif // CHATMODEL_H 