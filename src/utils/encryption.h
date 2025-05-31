#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <QString>
#include <QByteArray>
#include <QRandomGenerator>
#include <QDebug>

class Encryption {
public:
    static QString encrypt(const QString& input) {
        // OpenSSL 相关功能已禁用，直接返回原文
        return input;
    }

    static QString decrypt(const QString& input) {
        // OpenSSL 相关功能已禁用，直接返回原文
        return input;
    }
};

#endif // ENCRYPTION_H
