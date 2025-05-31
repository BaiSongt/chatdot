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
    explicit LLMService(const QString& modelPath, QObject *parent = nullptr);
    virtual ~LLMService();

    virtual QFuture<QString> generateResponse(const QString& prompt) = 0;
    virtual bool isAvailable() const = 0;
    virtual QString getModelName() const = 0;
    virtual void cancelGeneration();

    // 深度思考模式相关方法
    void setDeepThinkingMode(bool enabled);
    bool isDeepThinkingMode() const { return m_isDeepThinking; }

signals:
    void responseGenerated(const QString& response);
    void streamResponseReceived(const QString& partialResponse);
    void errorOccurred(const QString& error);
    void deepThinkingModeChanged(bool enabled);

protected:
    QString m_modelPath;
    bool m_isCancelled;
    bool m_isDeepThinking;
};

#endif // LLMSERVICE_H
