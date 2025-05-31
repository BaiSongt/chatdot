#ifndef APISERVICE_H
#define APISERVICE_H

#include "llmservice.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFutureInterface>
#include <QString>

class APIService : public LLMService
{
    Q_OBJECT

public:
    explicit APIService(const QString& apiKey, const QString& apiUrl = "https://api.openai.com/v1/chat/completions", const QString& modelName = "gpt-3.5-turbo", QObject *parent = nullptr);
    ~APIService() override;

    QFuture<QString> generateResponse(const QString& prompt) override;
    bool isAvailable() const override;
    QString getModelName() const override;

private slots:
    void handleResponse(QNetworkReply* reply);

private:
    QString getProviderFromUrl(const QString& url) const;
    QByteArray prepareRequestData(const QString& prompt) const;

    QString m_apiKey;
    QString m_apiUrl;
    QString m_provider;
    QString m_currentModelName;
    QString m_currentResponse;  // 用于累积流式响应
    QNetworkAccessManager* m_networkManager;
    QFutureInterface<QString> m_currentFuture;
};

#endif // APISERVICE_H
