#include "mainwindow.h"
#include <QApplication>
#include <QStyle>
#include <QDateTime>
#include <QScrollBar>
#include "models/chatmodel.h"
#include "models/imagemodel.h"
#include "models/settingsmodel.h"
#include "viewmodels/chatviewmodel.h"
#include "viewmodels/settingsviewmodel.h"
#include "services/logger.h"
#include "views/settingsdialog.h"
#include <QDir>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    LOG_INFO("开始初始化主窗口...");    // 初始化模型
    m_chatModel = new ChatModel(this);
    m_imageModel = new ImageModel(this);
    m_settingsModel = &SettingsModel::instance(); // 使用单例

    LOG_INFO("模型初始化完成");

    // 初始化视图模型
    m_chatViewModel = new ChatViewModel(m_chatModel, this);
    m_settingsViewModel = new SettingsViewModel(m_settingsModel, this);

    LOG_INFO("视图模型初始化完成");

    // 设置UI
    setupUI();
    LOG_INFO("UI设置完成");

    // 设置菜单
    setupMenu();
    LOG_INFO("菜单设置完成");

    // 设置状态栏
    setupStatusBar();
    LOG_INFO("状态栏设置完成");

    // 设置信号连接
    setupConnections();
    LOG_INFO("信号连接设置完成");

    // 加载设置
    loadSettings();
    LOG_INFO("设置加载完成");

    // 连接日志信号
    connect(&Logger::instance(), &Logger::logMessage,
            this, &MainWindow::onLogMessage);

    // 设置窗口标题
    setWindowTitle(this->tr("ChatDot - AI聊天助手"));

    LOG_INFO("主窗口初始化完成");
}

MainWindow::~MainWindow()
{
    LOG_INFO("正在保存设置...");
    saveSettings();
}

void MainWindow::setupUI()
{
    // 创建中央部件
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // 创建主布局
    m_mainLayout = new QVBoxLayout(m_centralWidget);

    // 添加模型标签
    QHBoxLayout* modelLabelLayout = new QHBoxLayout();
    m_modelLabel = new QLabel(this);
    m_modelLabel->setStyleSheet("QLabel { color: #666666; font-size: 12px; }");
    modelLabelLayout->addWidget(m_modelLabel);
    modelLabelLayout->addStretch();
    m_mainLayout->addLayout(modelLabelLayout);

    // 创建状态栏显示区域
    QHBoxLayout* statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(this);
    statusLayout->addWidget(m_statusLabel);
    m_mainLayout->addLayout(statusLayout);

    // 聊天显示区域
    m_chatDisplay = new QTextEdit(this);
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setMinimumHeight(400);
    m_mainLayout->addWidget(m_chatDisplay);

    // 输入区域
    QHBoxLayout* inputLayout = new QHBoxLayout();

    m_messageInput = new QLineEdit(this);
    m_messageInput->setPlaceholderText(this->tr("输入消息..."));
    inputLayout->addWidget(m_messageInput);

    m_sendButton = new QPushButton(this->tr("发送"), this);
    m_sendButton->setIcon(style()->standardIcon(QStyle::SP_CommandLink));
    inputLayout->addWidget(m_sendButton);

    m_clearButton = new QPushButton(this->tr("清除"), this);
    m_clearButton->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    inputLayout->addWidget(m_clearButton);

    m_imageButton = new QPushButton(this->tr("图片"), this);
    m_imageButton->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    inputLayout->addWidget(m_imageButton);

    m_mainLayout->addLayout(inputLayout);

    // 设置窗口大小
    resize(800, 600);

    // 更新当前模型标签
    updateModelLabel(SettingsModel::instance().currentModelName());
}

void MainWindow::setupMenu()
{
    // 创建菜单栏
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // 文件菜单
    QMenu* fileMenu = menuBar->addMenu(this->tr("文件"));
    m_saveChatAction = fileMenu->addAction(this->tr("保存对话"));
    m_loadChatAction = fileMenu->addAction(this->tr("加载对话"));
    fileMenu->addSeparator();
    fileMenu->addAction(this->tr("退出"), this, &QWidget::close);

    // 设置菜单
    QMenu* settingsMenu = menuBar->addMenu(this->tr("设置"));
    m_settingsAction = settingsMenu->addAction(this->tr("首选项"));

    // 帮助菜单
    QMenu* helpMenu = menuBar->addMenu(this->tr("帮助"));
    m_aboutAction = helpMenu->addAction(this->tr("关于"));
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel);
    updateStatusBar();
}

void MainWindow::setupConnections()
{
    // 连接按钮信号
    connect(m_sendButton, &QPushButton::clicked,
            this, &MainWindow::onSendMessage);
    connect(m_clearButton, &QPushButton::clicked,
            this, &MainWindow::onClearChat);
    connect(m_imageButton, &QPushButton::clicked,
            this, &MainWindow::selectImage);

    // 连接输入框回车信号
    connect(m_messageInput, &QLineEdit::returnPressed,
            this, &MainWindow::onSendMessage);

    // 连接菜单动作
    connect(m_settingsAction, &QAction::triggered,
            this, &MainWindow::onOpenSettings);
    connect(m_saveChatAction, &QAction::triggered,
            this, &MainWindow::onSaveChat);
    connect(m_loadChatAction, &QAction::triggered,
            this, &MainWindow::onLoadChat);
    connect(m_aboutAction, &QAction::triggered,
            this, &MainWindow::onAbout);

    // 连接视图模型信号
    connect(m_chatViewModel, &ChatViewModel::responseReceived,
            this, [this](const QString& response) {
                m_chatDisplay->append(this->tr("<p><b>AI:</b> %1</p>").arg(response));
                updateStatusBar();
            });
    connect(m_chatViewModel, &ChatViewModel::errorOccurred,
            this, &MainWindow::onError);

    // 连接设置模型的信号
    connect(&SettingsModel::instance(), &SettingsModel::currentModelNameChanged,
            this, &MainWindow::updateModelLabel);

    // 连接设置改变信号，更新LLMService
    connect(m_settingsViewModel, &SettingsViewModel::settingsChanged,
            this, [this]() {
                LLMService* service = m_settingsViewModel->createLLMService();
                m_chatViewModel->setLLMService(service);
            });
}

void MainWindow::loadSettings()
{
    // 确保设置已加载
    m_settingsModel->loadSettings();

    // 更新模型标签
    updateModelLabel(m_settingsModel->currentModelName());

    // 创建并设置LLMService
    LLMService* service = m_settingsViewModel->createLLMService();
    m_chatViewModel->setLLMService(service);

    LOG_INFO("设置加载完成");
}

void MainWindow::saveSettings()
{
    // 保存设置
    m_settingsModel->saveSettings();
    LOG_INFO("设置保存完成");
}

void MainWindow::updateModelLabel(const QString& modelName)
{
    QString modelType;
    switch (SettingsModel::instance().modelType()) {
        case SettingsModel::ModelType::API:
            modelType = tr("API");
            break;
        case SettingsModel::ModelType::Ollama:
            modelType = tr("Ollama");
            break;
        case SettingsModel::ModelType::Local:
            modelType = tr("本地模型");
            break;
    }

    m_modelLabel->setText(tr("当前模型: %1 (%2)").arg(modelName, modelType));
}

void MainWindow::updateStatusBar()
{
    QString modelType;
    switch (m_settingsModel->modelType()) {
        case SettingsModel::ModelType::API:
            modelType = this->tr("API模式");
            break;
        case SettingsModel::ModelType::Ollama:
            modelType = this->tr("Ollama模式");
            break;
        case SettingsModel::ModelType::Local:
            modelType = this->tr("本地模式");
            break;
    }

    QString status = tr("就绪 | 当前模型: %1").arg(SettingsModel::instance().currentModelName());
    m_statusLabel->setText(status);
}

void MainWindow::onSendMessage()
{
    QString message = m_messageInput->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // 显示用户消息
    m_chatDisplay->append(this->tr("<p><b>用户:</b> %1</p>").arg(message));
    m_messageInput->clear();

    // 发送消息
    m_chatViewModel->sendMessage(message);
    updateStatusBar();
}

void MainWindow::onClearChat()
{
    m_chatDisplay->clear();
    m_chatViewModel->clearChat();
    updateStatusBar();
}

void MainWindow::onOpenSettings()
{
    SettingsDialog dialog(&SettingsModel::instance(), this);
    if (dialog.exec() == QDialog::Accepted) {
        updateModelLabel(m_settingsModel->currentModelName());
        LOG_INFO("设置已更新");
    }
}

void MainWindow::onSaveChat()
{
    QString fileName = QFileDialog::getSaveFileName(this, this->tr("保存聊天记录"), QString(), this->tr("文本文件 (*.txt)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        showError(this->tr("保存失败"), this->tr("无法保存文件"));
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << m_chatDisplay->toPlainText();
}

void MainWindow::onLoadChat()
{
    QString fileName = QFileDialog::getOpenFileName(this, this->tr("加载聊天记录"), QString(), this->tr("文本文件 (*.txt)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        showError(this->tr("加载失败"), this->tr("无法打开文件"));
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    m_chatDisplay->setPlainText(in.readAll());
}

void MainWindow::onAbout()
{
    QMessageBox::about(this,
        this->tr("关于 ChatDot"),
        this->tr("ChatDot - AI聊天助手\n\n"
           "版本: %1\n"
           "基于Qt6开发\n\n"
           "支持多种AI模型:\n"
           "- OpenAI API\n"
           "- Ollama\n"
           "- 本地模型")
        .arg(QApplication::applicationVersion()));
}

void MainWindow::onLogMessage(Logger::Level level, const QString& message)
{
    switch (level) {
        case Logger::Level::Debug:
            m_statusLabel->setText(this->tr("调试: %1").arg(message));
            break;
        case Logger::Level::Info:
            m_statusLabel->setText(this->tr("信息: %1").arg(message));
            break;
        case Logger::Level::Warning:
            m_statusLabel->setText(this->tr("警告: %1").arg(message));
            break;
        case Logger::Level::Error:
            m_statusLabel->setText(this->tr("错误: %1").arg(message));
            break;
    }
}

void MainWindow::onError(const QString& error)
{
    showError(this->tr("错误"), error);
    LOG_ERROR(error);
}

void MainWindow::showError(const QString& title, const QString& message)
{
    QMessageBox::critical(this, title, message);
}

void MainWindow::selectImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        this->tr("选择图片"),
        QString(),
        this->tr("图片文件 (*.png *.jpg *.jpeg *.bmp);;所有文件 (*.*)"));

    if (fileName.isEmpty()) {
        return;
    }

    // TODO: 处理图片上传
    LOG_INFO(this->tr("已选择图片: %1").arg(fileName));
}
