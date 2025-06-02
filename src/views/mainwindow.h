#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QMenuBar>
#include <QStatusBar>
#include "themes/theme.h"
#include "models/chatmodel.h"
#include "models/imagemodel.h"
#include "models/settingsmodel.h"
#include "viewmodels/chatviewmodel.h"
#include "viewmodels/settingsviewmodel.h"
#include "services/logger.h"
#include "views/settingsdialog.h"
#include "themes/theme.h"

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
    void onError(const QString& error);
    void onLogMessage(Logger::Level level, const QString& message);
    void onModelSelectionChanged(int index);
    void updateModelList();
    void onGenerationStarted();
    void onGenerationFinished();
    void onStreamResponse(const QString& partialResponse);
    void onDeepThinkingToggled(bool checked);
    void createThemeMenu();
    void onThemeChanged(ThemeManager::Theme theme);
    void setLightTheme();
    void setDarkTheme();
    void setSystemTheme();

private:
    void setupUI();
    void setupMenu();
    void setupStatusBar();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void updateStatusBar();
    void selectImage();
    void showError(const QString& title, const QString& message);
    void refreshModelList();
    void updateSendButton(bool isGenerating);
    
    // 消息动画相关方法
    void applyMessageAnimation();

    // 模型列表更新相关方法
    void updateApiModelsForProvider(const QString& provider, const QStringList& availableModels);
    void updateApiModels(const QStringList& availableModels); // 保留为兼容性考虑
    void updateOllamaModels(const QStringList& availableModels);
    void updateLocalModels(const QStringList& availableModels);
    void addModelToSelector(const QString& displayName, const QString& modelName, bool isComplete, const QStringList& missingItems = QStringList());
    void restoreModelSelection(const QString& currentModel);
    QString getModelTypeString() const;
    QString getModelDisplayName(const QString& type, const QString& modelName, const QString& provider = QString()) const;

    // Models
    ChatModel* m_chatModel;
    ImageModel* m_imageModel;
    SettingsModel* m_settingsModel;

    // ViewModels
    ChatViewModel* m_chatViewModel;
    SettingsViewModel* m_settingsViewModel;

    // UI Components
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    QTextEdit* m_chatDisplay;
    QLineEdit* m_messageInput;
    QPushButton* m_sendButton;
    QPushButton* m_clearButton;
    QPushButton* m_imageButton;
    QPushButton* m_deepThinkingButton;
    QComboBox* m_modelSelector;
    QLabel* m_statusLabel;
    bool m_isGenerating;
    bool m_isUpdating;  // 用于防止模型选择器的递归更新
    bool m_isDeepThinking;

    // Menu Items
    QAction* m_settingsAction;
    QAction* m_saveChatAction;
    QAction* m_loadChatAction;
    QAction* m_aboutAction;

    QMenu *m_themeMenu;
    QAction *m_lightThemeAction;
    QAction *m_darkThemeAction;
    QAction *m_systemThemeAction;
    void updateThemeActions();
};

#endif // MAINWINDOW_H
