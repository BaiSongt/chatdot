#ifndef LLMSERVICE_H
#define LLMSERVICE_H

#include <QObject>
#include <QFuture>
#include <QString>

class LLMService : public QObject
{
    Q_OBJECT

public:
    explicit LLMService(const QString& modelPath, QObject *parent = nullptr);
    virtual ~LLMService();

    virtual QFuture<QString> generateResponse(const QString& prompt) = 0;
    virtual bool isAvailable() const = 0;
    virtual QString getModelName() const = 0;
    virtual void cancelGeneration();

signals:
    void responseGenerated(const QString& response);
    void streamResponseReceived(const QString& partialResponse);
    void errorOccurred(const QString& error);

protected:
    QString m_modelPath;
    bool m_isCancelled;
};

#endif // LLMSERVICE_H
