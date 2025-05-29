#ifndef IMAGEMODEL_H
#define IMAGEMODEL_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QTimer>

class ImageModel : public QObject
{
    Q_OBJECT

public:
    explicit ImageModel(QObject *parent = nullptr);
    ~ImageModel();

    Q_INVOKABLE void processImage(const QImage& image);
    Q_INVOKABLE void uploadImage(const QString& filePath);

signals:
    void imageProcessed(const QString& result);
    void errorOccurred(const QString& error);

private:
    // TODO: Add image processing related members (e.g., image data, processing engine)
};

#endif // IMAGEMODEL_H 