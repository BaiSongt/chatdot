#ifndef OLLAMASERVICE_H
#define OLLAMASERVICE_H

#include "llmservice.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFutureInterface>
#include <QString>

class OllamaService : public LLMService
{
    Q_OBJECT

public:
    explicit OllamaService(const QString& modelPath, QObject *parent = nullptr);
    ~OllamaService() override;

    QFuture<QString> generateResponse(const QString& prompt) override;
    bool isAvailable() const override;
    QString getModelName() const override;

private slots:
    void handleResponse(QNetworkReply* reply);

private:
    QString m_modelPath;
    QNetworkAccessManager* m_networkManager;
    QFutureInterface<QString> m_currentFuture;
};

#endif // OLLAMASERVICE_H 