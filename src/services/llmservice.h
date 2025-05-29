#ifndef LLMSERVICE_H
#define LLMSERVICE_H

#include <QObject>
#include <QFuture>
#include <QString>

class LLMService : public QObject
{
    Q_OBJECT

public:
    explicit LLMService(QObject *parent = nullptr);
    virtual ~LLMService();

    virtual QFuture<QString> generateResponse(const QString& prompt) = 0;
    virtual bool isAvailable() const = 0;
    virtual QString getModelName() const = 0;

signals:
    void errorOccurred(const QString& error);
    void responseGenerated(const QString& response);

protected:
    void emitError(const QString& error);
};

#endif // LLMSERVICE_H 