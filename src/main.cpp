#include "views/mainwindow.h"
#include "services/logger.h"
#include "models/settingsmodel.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <iostream>
#include <windows.h>

int main(int argc, char *argv[])
{
    // 设置控制台输出
    QFile logFile("debug.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&logFile);
        out << "程序开始执行: " << QDateTime::currentDateTime().toString() << "\n";
    }

    try {
        std::cout << "程序开始执行" << std::endl;
        if (logFile.isOpen()) {
            QTextStream(&logFile) << "准备创建 QApplication\n";
        }

        QApplication app(argc, argv);
        if (logFile.isOpen()) {
            QTextStream(&logFile) << "QApplication 创建成功\n";
        }

        // 设置应用程序信息
        app.setApplicationName("ChatDot");
        app.setApplicationVersion("1.0.0");
        app.setOrganizationName("ChatDot");
        app.setOrganizationDomain("chatdot.org");

        // 初始化日志系统
        if (logFile.isOpen()) {
            QTextStream(&logFile) << "准备初始化日志系统\n";
        }
        Logger::instance().init();
        if (logFile.isOpen()) {
            QTextStream(&logFile) << "日志系统初始化完成\n";
        }

        // 加载设置
        if (logFile.isOpen()) {
            QTextStream(&logFile) << "准备加载设置\n";
        }
        SettingsModel::instance().loadSettings();
        if (logFile.isOpen()) {
            QTextStream(&logFile) << "设置加载完成\n";
        }

        // 刷新模型列表
        if (logFile.isOpen()) {
            QTextStream(&logFile) << "准备刷新模型列表\n";
        }
        SettingsModel::instance().refreshOllamaModels();
        if (logFile.isOpen()) {
            QTextStream(&logFile) << "模型列表刷新完成\n";
        }

        // 创建并显示主窗口
        if (logFile.isOpen()) {
            QTextStream(&logFile) << "准备创建主窗口\n";
            QTextStream(&logFile) << "Qt版本: " << QT_VERSION_STR << "\n";
            QTextStream(&logFile) << "应用程序路径: " << QCoreApplication::applicationDirPath() << "\n";
            QTextStream(&logFile) << "当前工作目录: " << QDir::currentPath() << "\n";
        }

        try {
            MainWindow w;
            if (logFile.isOpen()) {
                QTextStream(&logFile) << "主窗口创建成功\n";
            }
            w.show();
            if (logFile.isOpen()) {
                QTextStream(&logFile) << "主窗口显示成功\n";
            }
            return app.exec();
        } catch (const std::exception& e) {
            QString errorMsg = QString("创建主窗口时发生异常: %1").arg(e.what());
            if (logFile.isOpen()) {
                QTextStream(&logFile) << errorMsg << "\n";
            }
            QMessageBox::critical(nullptr, "错误", errorMsg);
            return 1;
        }
    } catch (const std::exception& e) {
        QString errorMsg = QString("程序发生异常: %1").arg(e.what());
        if (logFile.isOpen()) {
            QTextStream(&logFile) << errorMsg << "\n";
        }
        QMessageBox::critical(nullptr, "错误", errorMsg);
        return 1;
    } catch (...) {
        QString errorMsg = "程序发生未知异常";
        if (logFile.isOpen()) {
            QTextStream(&logFile) << errorMsg << "\n";
        }
        QMessageBox::critical(nullptr, "错误", errorMsg);
        return 1;
    }
}
