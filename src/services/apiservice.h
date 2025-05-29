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
    explicit APIService(const QString& apiKey, QObject *parent = nullptr);
    ~APIService() override;

    QFuture<QString> generateResponse(const QString& prompt) override;
    bool isAvailable() const override;
    QString getModelName() const override;

private slots:
    void handleResponse(QNetworkReply* reply);

private:
    QString m_apiKey;
    QNetworkAccessManager* m_networkManager;
    QFutureInterface<QString> m_currentFuture;
};

#endif // APISERVICE_H 