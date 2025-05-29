#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <QString>
#include <QByteArray>
#include <QCryptographicHash>
#include <QRandomGenerator>

class Encryption {
public:
    static QString encrypt(const QString& input) {
        if (input.isEmpty()) return QString();

        QByteArray data = input.toUtf8();
        QByteArray key = QCryptographicHash::hash("ChatDotSecretKey", QCryptographicHash::Sha256);

        // 添加随机IV
        QByteArray iv;
        iv.resize(16);
        for (int i = 0; i < 16; ++i) {
            iv[i] = QRandomGenerator::global()->bounded(256);
        }

        // 使用key和iv进行简单加密
        QByteArray encrypted = iv;
        for (int i = 0; i < data.size(); ++i) {
            encrypted.append(data[i] ^ key[i % key.size()] ^ iv[i % iv.size()]);
        }

        return QString::fromLatin1(encrypted.toBase64());
    }

    static QString decrypt(const QString& input) {
        if (input.isEmpty()) return QString();

        QByteArray data = QByteArray::fromBase64(input.toLatin1());
        if (data.size() <= 16) return QString();

        QByteArray key = QCryptographicHash::hash("ChatDotSecretKey", QCryptographicHash::Sha256);
        QByteArray iv = data.left(16);
        QByteArray encrypted = data.mid(16);

        QByteArray decrypted;
        for (int i = 0; i < encrypted.size(); ++i) {
            decrypted.append(encrypted[i] ^ key[i % key.size()] ^ iv[i % iv.size()]);
        }

        return QString::fromUtf8(decrypted);
    }
};

#endif // ENCRYPTION_H
