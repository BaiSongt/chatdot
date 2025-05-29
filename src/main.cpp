#include "views/mainwindow.h"
#include "services/logger.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("ChatDot");
    QApplication::setApplicationVersion("1.0.0");

    // 初始化日志系统
    Logger::instance().init();

    // 设置翻译
    QTranslator qtTranslator;
    if (!qtTranslator.load("qt_" + QLocale::system().name(),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        qWarning() << "无法加载 Qt 翻译文件";
    }
    app.installTranslator(&qtTranslator);

    QTranslator appTranslator;
    if (!appTranslator.load("chatdot_" + QLocale::system().name())) {
        qWarning() << "无法加载应用程序翻译文件";
    }
    app.installTranslator(&appTranslator);

    // 解析命令行参数
    QCommandLineParser parser;
    parser.setApplicationDescription("ChatDot - 一个简单的聊天应用");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
} 