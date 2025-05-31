#include <QApplication>
#include <QDebug>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    qDebug() << "测试程序开始执行";
    QApplication app(argc, argv);
    qDebug() << "QApplication 创建成功";
    
    QMessageBox::information(nullptr, "测试", "这是一个测试消息");
    qDebug() << "消息框显示成功";
    
    return app.exec();
} 