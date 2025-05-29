#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QComboBox>
#include "models/chatmodel.h"
#include "models/imagemodel.h"
#include "models/settingsmodel.h"
#include "viewmodels/chatviewmodel.h"
#include "viewmodels/settingsviewmodel.h"
#include "services/logger.h"
#include "views/settingsdialog.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSendMessage();
    void onClearChat();
    void onOpenSettings();
    void onSaveChat();
    void onLoadChat();
    void onAbout();
    void onLogMessage(Logger::Level level, const QString& message);
    void updateStatusBar();
    void onError(const QString& error);
    void selectImage();

private:
    void setupUI();
    void setupMenu();
    void setupConnections();
    void setupStatusBar();
    void loadSettings();
    void saveSettings();
    void showError(const QString& title, const QString& message);

    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    QTextEdit* m_chatDisplay;
    QLineEdit* m_messageInput;
    QPushButton* m_sendButton;
    QPushButton* m_clearButton;
    QPushButton* m_imageButton;
    QComboBox* m_modelTypeComboBox;
    QLineEdit* m_apiKeyLineEdit;
    QLineEdit* m_apiUrlLineEdit;
    QPushButton* m_settingsButton;
    QLabel* m_statusLabel;

    ChatModel* m_chatModel;
    SettingsModel* m_settingsModel;
    ChatViewModel* m_chatViewModel;
    SettingsViewModel* m_settingsViewModel;
    ImageModel* m_imageModel;

    QAction* m_settingsAction;
    QAction* m_saveChatAction;
    QAction* m_loadChatAction;
    QAction* m_aboutAction;
};

#endif // MAINWINDOW_H 