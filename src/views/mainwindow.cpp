#include "mainwindow.h"
#include <QApplication>
#include <QStyle>
#include <QDateTime>
#include <QScrollBar>
#include <QActionGroup>
#include "../utils/markdownparser.h"
#include "models/chatmodel.h"
#include "models/imagemodel.h"
#include "models/settingsmodel.h"
#include "viewmodels/chatviewmodel.h"
#include "viewmodels/settingsviewmodel.h"
#include "services/logger.h"
#include "views/settingsdialog.h"
#include <QDir>
#include <QFileInfo>
#include "themes/theme.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chatModel(nullptr)
    , m_imageModel(nullptr)
    , m_settingsModel(nullptr)
    , m_chatViewModel(nullptr)
    , m_settingsViewModel(nullptr)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_chatDisplay(nullptr)
    , m_messageInput(nullptr)
    , m_sendButton(nullptr)
    , m_clearButton(nullptr)
    , m_imageButton(nullptr)
    , m_modelSelector(nullptr)
    , m_statusLabel(nullptr)
    , m_isGenerating(false)
    , m_isUpdating(false)
    , m_settingsAction(nullptr)
    , m_saveChatAction(nullptr)
    , m_loadChatAction(nullptr)
    , m_aboutAction(nullptr)
    , m_deepThinkingButton(nullptr)
    , m_isDeepThinking(false)
    , m_themeMenu(nullptr)
    , m_lightThemeAction(nullptr)
    , m_darkThemeAction(nullptr)
    , m_systemThemeAction(nullptr)
{
    try {
        LOG_INFO("开始初始化主窗口...");

        // 初始化模型
        try {
            m_chatModel = new ChatModel(this);
            if (!m_chatModel) {
                throw std::runtime_error("无法创建 ChatModel");
            }
            m_imageModel = new ImageModel(this);
            if (!m_imageModel) {
                throw std::runtime_error("无法创建 ImageModel");
            }
            m_settingsModel = &SettingsModel::instance();
            if (!m_settingsModel) {
                throw std::runtime_error("无法获取 SettingsModel 实例");
            }
            LOG_INFO("模型初始化完成");
        } catch (const std::exception& e) {
            LOG_ERROR("模型初始化失败: " + QString(e.what()));
            throw;
        }

        // 初始化视图模型
        try {
            m_chatViewModel = new ChatViewModel(m_chatModel, this);
            if (!m_chatViewModel) {
                throw std::runtime_error("无法创建 ChatViewModel");
            }
            m_settingsViewModel = new SettingsViewModel(m_settingsModel, this);
            if (!m_settingsViewModel) {
                throw std::runtime_error("无法创建 SettingsViewModel");
            }
            LOG_INFO("视图模型初始化完成");
        } catch (const std::exception& e) {
            LOG_ERROR("视图模型初始化失败: " + QString(e.what()));
            throw;
        }

        // 设置UI
        try {
            setupUI();
            if (!m_centralWidget || !m_mainLayout || !m_chatDisplay || 
                !m_messageInput || !m_sendButton || !m_clearButton || 
                !m_imageButton || !m_modelSelector || !m_deepThinkingButton) {
                throw std::runtime_error("UI组件创建失败");
            }
            LOG_INFO("UI设置完成");
        } catch (const std::exception& e) {
            LOG_ERROR("UI设置失败: " + QString(e.what()));
            throw;
        }

        // 设置菜单
        try {
            setupMenu();
            if (!m_settingsAction || !m_saveChatAction || 
                !m_loadChatAction || !m_aboutAction) {
                throw std::runtime_error("菜单项创建失败");
            }
            LOG_INFO("菜单设置完成");
        } catch (const std::exception& e) {
            LOG_ERROR("菜单设置失败: " + QString(e.what()));
            throw;
        }

        // 设置状态栏
        try {
            setupStatusBar();
            LOG_INFO("状态栏设置完成");
        } catch (const std::exception& e) {
            LOG_ERROR("状态栏设置失败: " + QString(e.what()));
            throw;
        }

        // 设置信号连接
        try {
            setupConnections();
            LOG_INFO("信号连接设置完成");
        } catch (const std::exception& e) {
            LOG_ERROR("信号连接设置失败: " + QString(e.what()));
            throw;
        }

        // 加载设置
        try {
            loadSettings();
            LOG_INFO("设置和模型选择器初始化完成");
        } catch (const std::exception& e) {
            LOG_ERROR("设置加载失败: " + QString(e.what()));
            throw;
        }

        // 连接日志信号
        connect(&Logger::instance(), &Logger::logMessage,
                this, &MainWindow::onLogMessage);

        // 设置窗口标题
        setWindowTitle(this->tr("ChatDot - AI聊天助手"));

        // 创建主题菜单
        createThemeMenu();

        // 连接主题管理器的信号
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
                this, &MainWindow::onThemeChanged);

        LOG_INFO("主窗口初始化完成");
    } catch (const std::exception& e) {
        LOG_ERROR("主窗口初始化过程中发生异常: " + QString(e.what()));
        QMessageBox::critical(this, tr("错误"), 
            tr("主窗口初始化失败: %1").arg(e.what()));
        throw;
    } catch (...) {
        LOG_ERROR("主窗口初始化过程中发生未知异常");
        QMessageBox::critical(this, tr("错误"), 
            tr("主窗口初始化过程中发生未知异常"));
        throw;
    }
}

MainWindow::~MainWindow()
{
    LOG_INFO("正在关闭主窗口...");
    
    // 保存设置
    LOG_INFO("正在保存设置...");
    saveSettings();
    
    // 清理服务
    if (m_chatViewModel) {
        LOG_INFO("正在清理聊天服务...");
        m_chatViewModel->setLLMService(nullptr);
    }
    
    // 记录当前模型信息
    if (m_settingsModel) {
        LOG_INFO(QString("关闭时的模型信息 - 类型: %1, 名称: %2")
            .arg(static_cast<int>(m_settingsModel->modelType()))
            .arg(m_settingsModel->currentModelName()));
    }
    
    LOG_INFO("主窗口已关闭");
}

void MainWindow::setupUI()
{
    // 创建中央部件
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // 创建主布局
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(10);  // 设置组件之间的间距
    m_mainLayout->setContentsMargins(10, 10, 10, 10);  // 设置边距
    
    // CSS动画和样式将在创建聊天显示区域时添加

    // 顶部工具栏布局
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setSpacing(5);

    // 添加模型选择器
    QLabel* modelLabel = new QLabel(tr("当前模型:"));
    modelLabel->setFixedWidth(80);  // 固定标签宽度
    topLayout->addWidget(modelLabel);
    
    m_modelSelector = new QComboBox();
    m_modelSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_modelSelector->setMinimumWidth(100);
    topLayout->addWidget(m_modelSelector);
    topLayout->addStretch();

    m_mainLayout->addLayout(topLayout);

    // 聊天显示区域 - 使用现代化样式
    m_chatDisplay = new QTextEdit(this);
    m_chatDisplay->setObjectName("chatDisplay"); // 设置对象名称以匹配主题样式
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setMinimumHeight(500);  // 增加聊天区域高度
    
    // 从主题文件中加载聊天气泡样式
    QString chatBubblesStyle;
    QString appDir = QCoreApplication::applicationDirPath();
    QString bubblePath = QDir(appDir).absoluteFilePath("themes/chat_bubbles.css");
    
    QFile bubbleFile(bubblePath);
    if (bubbleFile.open(QFile::ReadOnly | QFile::Text)) {
        chatBubblesStyle = QString::fromUtf8(bubbleFile.readAll());
        bubbleFile.close();
        LOG_INFO(QString("成功加载聊天气泡样式: %1").arg(bubblePath));
    } else {
        LOG_WARNING(QString("无法打开聊天气泡样式文件: %1, 错误: %2")
            .arg(bubblePath)
            .arg(bubbleFile.errorString()));
        
        // 使用默认样式
        chatBubblesStyle = 
            "@keyframes fadeIn {"
            "  from { opacity: 0; transform: translateY(10px); }"
            "  to { opacity: 1; transform: translateY(0); }"
            "}"
            ".message-container {"
            "  display: flex;"
            "  align-items: flex-start;"
            "  margin-bottom: 12px;"
            "  animation: fadeIn 0.3s ease-in-out;"
            "  clear: both;"
            "}"
            ".avatar {"
            "  width: 36px;"
            "  height: 36px;"
            "  margin-right: 10px;"
            "  flex-shrink: 0;"
            "  border-radius: 18px;"
            "  text-align: center;"
            "  line-height: 36px;"
            "  font-weight: bold;"
            "  color: white;"
            "}"
            ".user-avatar { background-color: #2979FF; }"
            ".ai-avatar { background-color: #00BFA5; }"
            ".message-content {"
            "  flex-grow: 1;"
            "  max-width: calc(100% - 46px);"
            "}"
            ".sender-info {"
            "  font-weight: bold;"
            "  margin-bottom: 2px;"
            "}"
            ".timestamp {"
            "  color: #888;"
            "  font-weight: normal;"
            "  font-size: 0.8em;"
            "}"
            ".user-bubble-light {"
            "  background-color: #e1f3fb;"
            "  color: #000000;"
            "  border-radius: 10px;"
            "  padding: 10px;"
            "  margin: 5px 0;"
            "  display: inline-block;"
            "  max-width: 100%;"
            "  word-wrap: break-word;"
            "  box-shadow: 0 1px 2px rgba(0,0,0,0.1);"
            "}"
            ".ai-bubble-light {"
            "  background-color: #f0f0f0;"
            "  color: #000000;"
            "  border-radius: 10px;"
            "  padding: 10px;"
            "  margin: 5px 0;"
            "  display: inline-block;"
            "  max-width: 100%;"
            "  word-wrap: break-word;"
            "  box-shadow: 0 1px 2px rgba(0,0,0,0.1);"
            "}"
            ".user-bubble-dark {"
            "  background-color: #2C4F70;"
            "  color: #FFFFFF;"
            "  border-radius: 10px;"
            "  padding: 10px;"
            "  margin: 5px 0;"
            "  display: inline-block;"
            "  max-width: 100%;"
            "  word-wrap: break-word;"
            "  box-shadow: 0 1px 2px rgba(0,0,0,0.3);"
            "}"
            ".ai-bubble-dark {"
            "  background-color: #383838;"
            "  color: #FFFFFF;"
            "  border-radius: 10px;"
            "  padding: 10px;"
            "  margin: 5px 0;"
            "  display: inline-block;"
            "  max-width: 100%;"
            "  word-wrap: break-word;"
            "  box-shadow: 0 1px 2px rgba(0,0,0,0.3);"
            "}"
            ".markdown-content h1 {"
            "  font-size: 1.5em;"
            "  margin: 0.5em 0;"
            "  border-bottom: 1px solid #ddd;"
            "  padding-bottom: 0.3em;"
            "}"
            ".markdown-content h2 {"
            "  font-size: 1.3em;"
            "  margin: 0.5em 0;"
            "}"
            ".markdown-content h3 {"
            "  font-size: 1.1em;"
            "  margin: 0.5em 0;"
            "}"
            ".markdown-content ul, .markdown-content ol {"
            "  padding-left: 1.5em;"
            "  margin: 0.5em 0;"
            "}"
            ".markdown-content li {"
            "  margin: 0.2em 0;"
            "}"
            ".markdown-content code {"
            "  background-color: rgba(0, 0, 0, 0.1);"
            "  padding: 0.2em 0.4em;"
            "  border-radius: 3px;"
            "  font-family: monospace;"
            "}"
            ".markdown-content pre {"
            "  background-color: rgba(0, 0, 0, 0.1);"
            "  padding: 0.5em;"
            "  border-radius: 5px;"
            "  overflow-x: auto;"
            "  margin: 0.5em 0;"
            "}"
            ".markdown-content pre code {"
            "  background-color: transparent;"
            "  padding: 0;"
            "}"
            ".markdown-content blockquote {"
            "  border-left: 4px solid #ccc;"
            "  padding-left: 1em;"
            "  margin: 0.5em 0;"
            "  color: #777;"
            "}"
            ".markdown-content p {"
            "  margin: 0.5em 0;"
            "}";
    }
    
    // 设置样式表
    m_chatDisplay->document()->setDefaultStyleSheet(chatBubblesStyle);
    
    // 设置文档背景和间距
    m_chatDisplay->document()->setDocumentMargin(10);
    m_mainLayout->addWidget(m_chatDisplay);

    // 功能按钮区域 - 简洁紧凑样式
    QHBoxLayout* functionLayout = new QHBoxLayout();
    functionLayout->setSpacing(5); // 减少间距
    functionLayout->setContentsMargins(0, 0, 0, 0); // 移除边距

    // 创建简洁的按钮样式
    auto createCompactButton = [this](QPushButton* &button, const QString &text, QStyle::StandardPixmap icon, int width) {
        button = new QPushButton(text, this);
        button->setIcon(style()->standardIcon(icon));
        button->setFixedWidth(width);
        button->setFixedHeight(32); // 减少高度
        button->setObjectName("compactButton"); // 统一样式名
        return button;
    };

    m_clearButton = createCompactButton(m_clearButton, this->tr("清除"), QStyle::SP_DialogResetButton, 70);
    functionLayout->addWidget(m_clearButton);

    m_imageButton = createCompactButton(m_imageButton, this->tr("图片"), QStyle::SP_FileIcon, 70);
    functionLayout->addWidget(m_imageButton);

    m_deepThinkingButton = createCompactButton(m_deepThinkingButton, this->tr("思考"), QStyle::SP_FileDialogDetailedView, 70);
    m_deepThinkingButton->setCheckable(true);  // 设置为可切换状态
    functionLayout->addWidget(m_deepThinkingButton);

    functionLayout->addStretch();
    m_mainLayout->addLayout(functionLayout);

    // 输入区域 - 简洁紧凑样式
    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(5);  // 减少间距
    inputLayout->setContentsMargins(0, 5, 0, 0); // 只保留上边距

    m_messageInput = new QLineEdit(this);
    m_messageInput->setObjectName("messageInput"); // 设置对象名称以匹配主题样式
    m_messageInput->setPlaceholderText(this->tr("输入消息..."));
    m_messageInput->setFixedHeight(36); // 调整高度
    inputLayout->addWidget(m_messageInput);

    m_sendButton = new QPushButton(this->tr("发送"), this);
    m_sendButton->setObjectName("sendButton"); // 设置对象名称以匹配主题样式
    m_sendButton->setIcon(style()->standardIcon(QStyle::SP_CommandLink));
    m_sendButton->setFixedWidth(70);
    m_sendButton->setFixedHeight(36); // 与输入框保持一致高度
    inputLayout->addWidget(m_sendButton);

    m_mainLayout->addLayout(inputLayout);

    // 设置窗口大小为9:16比例
    int width = 400;  // 基础宽度
    int height = width * 16 / 9;  // 按9:16比例计算高度
    resize(width, height);
    
    // 设置最小窗口大小
    setMinimumSize(width, height);  // 保持9:16比例的最小尺寸
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

    // 主题菜单
    m_themeMenu = menuBar->addMenu(tr("主题"));
    m_lightThemeAction = new QAction(tr("浅色主题"), this);
    m_darkThemeAction = new QAction(tr("深色主题"), this);
    m_systemThemeAction = new QAction(tr("跟随系统"), this);

    // 设置动作为可检查
    m_lightThemeAction->setCheckable(true);
    m_darkThemeAction->setCheckable(true);
    m_systemThemeAction->setCheckable(true);

    // 创建动作组，确保单选
    QActionGroup* themeGroup = new QActionGroup(this);
    themeGroup->addAction(m_lightThemeAction);
    themeGroup->addAction(m_darkThemeAction);
    themeGroup->addAction(m_systemThemeAction);
    themeGroup->setExclusive(true);

    // 添加动作到菜单
    m_themeMenu->addAction(m_lightThemeAction);
    m_themeMenu->addAction(m_darkThemeAction);
    m_themeMenu->addAction(m_systemThemeAction);

    // 连接动作的信号
    connect(m_lightThemeAction, &QAction::triggered, this, &MainWindow::setLightTheme);
    connect(m_darkThemeAction, &QAction::triggered, this, &MainWindow::setDarkTheme);
    connect(m_systemThemeAction, &QAction::triggered, this, &MainWindow::setSystemTheme);

    // 帮助菜单
    QMenu* helpMenu = menuBar->addMenu(this->tr("帮助"));
    m_aboutAction = helpMenu->addAction(this->tr("关于"));

    // 设置当前主题的选中状态
    updateThemeActions();
}

void MainWindow::setupStatusBar()
{
    // 移除状态栏
    statusBar()->hide();
}

void MainWindow::setupConnections()
{
    // 连接按钮信号
    connect(m_sendButton, &QPushButton::clicked,
            this, [this]() {
                if (m_isGenerating) {
                    m_chatViewModel->cancelGeneration();
                } else {
                    onSendMessage();
                }
            });
    connect(m_clearButton, &QPushButton::clicked,
            this, &MainWindow::onClearChat);
    connect(m_imageButton, &QPushButton::clicked,
            this, &MainWindow::selectImage);
    connect(m_deepThinkingButton, &QPushButton::toggled,
            this, &MainWindow::onDeepThinkingToggled);

    // 连接生成状态信号
    connect(m_chatViewModel, &ChatViewModel::generationStarted,
            this, &MainWindow::onGenerationStarted);
    connect(m_chatViewModel, &ChatViewModel::generationFinished,
            this, &MainWindow::onGenerationFinished);
    connect(m_chatViewModel, &ChatViewModel::streamResponse,
            this, &MainWindow::onStreamResponse);

    // 连接输入框回车信号
    connect(m_messageInput, &QLineEdit::returnPressed,
            this, [this]() {
                if (!m_isGenerating) {
                    onSendMessage();
                }
            });

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
                m_chatDisplay->moveCursor(QTextCursor::End);
                m_chatDisplay->insertPlainText("\n"); // 新段落
            });
    connect(m_chatViewModel, &ChatViewModel::errorOccurred,
            this, &MainWindow::onError);

    // 连接模型选择器信号
    connect(m_modelSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModelSelectionChanged);

    // 连接设置模型的信号
    connect(m_settingsModel, &SettingsModel::currentModelNameChanged,
            this, &MainWindow::updateModelList);
    connect(m_settingsModel, &SettingsModel::ollamaModelsChanged,
            this, &MainWindow::updateModelList);
}

void MainWindow::loadSettings()
{
    // 加载设置
    m_settingsModel->loadSettings();
    LOG_INFO("设置加载完成");
    
    // 立即创建服务
    QString currentModel = m_settingsModel->currentModelName();
    if (!currentModel.isEmpty()) {
        LOG_INFO(QString("准备创建初始服务 - 类型: %1, 模型: %2")
            .arg(static_cast<int>(m_settingsModel->modelType()))
            .arg(currentModel));
        
        // 创建新的LLMService
        LLMService* service = m_settingsViewModel->createLLMService();
        if (service) {
            m_chatViewModel->setLLMService(service);
            LOG_INFO(QString("成功创建并设置初始服务: %1").arg(currentModel));
        } else {
            LOG_ERROR(QString("创建初始服务失败: %1").arg(currentModel));
        }
    }
}

void MainWindow::updateModelList()
{
    LOG_INFO("开始更新模型列表");
    
    // 阻止信号触发，避免在更新时触发 onModelSelectionChanged
    m_modelSelector->blockSignals(true);
    
    // 保存当前选择的模型
    QString currentModel = m_settingsModel->currentModelName();
    m_modelSelector->clear();

    // 获取当前模型类型
    QString modelType = getModelTypeString();
    QStringList availableModels;
    
    // 根据不同类型获取可用模型列表
    if (modelType == "api") {
        // 对于API类型，获取所有已配置的提供商的所有模型
        QStringList configuredProviders = m_settingsModel->getConfiguredProviders();
        LOG_INFO(QString("获取到已配置的API提供商列表，共 %1 个提供商").arg(configuredProviders.size()));
        
        // 遍历所有已配置的提供商，获取它们的所有模型
        for (const QString& provider : configuredProviders) {
            if (!provider.isEmpty()) {
                QStringList providerModels = m_settingsViewModel->getApiModelsForProvider(provider);
                LOG_INFO(QString("获取到API提供商 %1 的所有模型，共 %2 个模型")
                    .arg(provider)
                    .arg(providerModels.size()));
                
                // 将当前提供商的所有模型添加到可用模型列表
                updateApiModelsForProvider(provider, providerModels);
            }
        }
    } else if (modelType == "ollama") {
        // 对于Ollama类型，获取已配置的模型
        availableModels = m_settingsModel->getConfiguredModels(modelType);
        LOG_INFO(QString("获取到 %1 类型已配置的模型列表，共 %2 个模型")
            .arg(modelType)
            .arg(availableModels.size()));
        updateOllamaModels(availableModels);
    } else if (modelType == "local") {
        // 对于本地类型，获取已配置的模型
        availableModels = m_settingsModel->getConfiguredModels(modelType);
        LOG_INFO(QString("获取到 %1 类型已配置的模型列表，共 %2 个模型")
            .arg(modelType)
            .arg(availableModels.size()));
        updateLocalModels(availableModels);
    }

    // 如果没有可用的模型，添加提示信息
    if (availableModels.isEmpty()) {
        m_modelSelector->addItem(tr("请配置模型"), "");
        LOG_INFO("没有可用的模型，显示配置提示");
    }

    // 恢复之前选择的模型
    restoreModelSelection(currentModel);

    // 恢复信号连接
    m_modelSelector->blockSignals(false);

    // 自动触发一次模型切换，确保服务被设置
    if (m_modelSelector->count() > 0) {
        int idx = m_modelSelector->currentIndex();
        if (idx >= 0) {
            QString selectedModel = m_modelSelector->currentData().toString();
            if (!selectedModel.isEmpty()) {
                // 先清理当前的服务
                if (m_chatViewModel) {
                    m_chatViewModel->setLLMService(nullptr);
                }
                
                // 创建新的LLMService
                LLMService* service = m_settingsViewModel->createLLMService();
                if (service) {
                    m_chatViewModel->setLLMService(service);
                    LOG_INFO(QString("成功创建并设置模型服务: %1").arg(selectedModel));
                } else {
                    LOG_ERROR(QString("创建模型服务失败: %1").arg(selectedModel));
                }
            }
        }
    }

    LOG_INFO(QString("模型列表更新完成，当前选择: %1，可用模型数量: %2")
        .arg(m_modelSelector->currentText())
        .arg(availableModels.size()));
}

void MainWindow::updateApiModelsForProvider(const QString& provider, const QStringList& availableModels)
{
    if (provider.isEmpty()) {
        LOG_WARNING("提供商名称为空，无法显示模型列表");
        return;
    }
    
    LOG_INFO(QString("正在为提供商 %1 更新API模型列表").arg(provider));
    
    // 获取提供商的配置
    QJsonObject providerConfig = m_settingsModel->getProviderConfig("api", provider);
    
    // 检查提供商配置是否包含models字段
    if (!providerConfig.contains("models")) {
        LOG_WARNING(QString("提供商 %1 配置中不包含models字段").arg(provider));
        return;
    }
    
    // 检查提供商是否配置了API Key
    if (!providerConfig.contains("api_key") || providerConfig["api_key"].toString().isEmpty()) {
        LOG_WARNING(QString("提供商 %1 未配置API Key").arg(provider));
        return;
    }
    
    // 获取提供商的所有模型配置
    QJsonObject modelsConfig = providerConfig["models"].toObject();
    
    // 遍历传入的所有可用模型
    for (const QString& modelName : availableModels) {
        // 检查模型是否在配置中
        if (modelsConfig.contains(modelName)) {
            // 检查模型配置是否完整
            bool isComplete = m_settingsModel->isModelConfigComplete("api", modelName);
            QStringList missingItems;
            if (!isComplete) {
                missingItems = m_settingsModel->getMissingConfigItems("api", modelName);
            }
            
            // 获取模型显示名称并添加到选择器
            QString displayName = getModelDisplayName("api", modelName, provider);
            addModelToSelector(displayName, modelName, isComplete, missingItems);
            
            LOG_INFO(QString("添加API模型: %1 (提供商: %2, 显示名称: %3)")
                .arg(modelName)
                .arg(provider)
                .arg(displayName));
        } else {
            LOG_WARNING(QString("模型 %1 在提供商 %2 的配置中不存在").arg(modelName, provider));
        }
    }
}

void MainWindow::updateApiModels(const QStringList& availableModels)
{
    // 此方法保留为兼容性考虑，实际上已经不再使用
    // 获取当前选择的API提供商
    QString currentProvider = m_settingsModel->getCurrentProvider();
    if (currentProvider.isEmpty()) {
        LOG_WARNING("当前未选择API提供商，无法显示模型列表");
        return;
    }
    
    // 调用新的方法处理当前提供商的模型
    updateApiModelsForProvider(currentProvider, availableModels);
}

void MainWindow::updateOllamaModels(const QStringList& availableModels)
{
    for (const QString& modelName : availableModels) {
        QJsonObject config = m_settingsModel->getModelConfig("ollama", modelName);
        if (config["enabled"].toBool(true)) {
            bool isComplete = m_settingsModel->isModelConfigComplete("ollama", modelName);
            QStringList missingItems;
            if (!isComplete) {
                missingItems = m_settingsModel->getMissingConfigItems("ollama", modelName);
            }
            QString displayName = getModelDisplayName("ollama", modelName);
            addModelToSelector(displayName, modelName, isComplete, missingItems);
        }
    }
}

void MainWindow::updateLocalModels(const QStringList& availableModels)
{
    for (const QString& modelName : availableModels) {
        QJsonObject config = m_settingsModel->getModelConfig("local", modelName);
        if (config["enabled"].toBool(true)) {
            bool isComplete = m_settingsModel->isModelConfigComplete("local", modelName);
            QStringList missingItems;
            if (!isComplete) {
                missingItems = m_settingsModel->getMissingConfigItems("local", modelName);
            }
            QString displayName = getModelDisplayName("local", modelName);
            addModelToSelector(displayName, modelName, isComplete, missingItems);
        }
    }
}

void MainWindow::addModelToSelector(const QString& displayName, const QString& modelName, bool isComplete, const QStringList& missingItems)
{
    if (isComplete) {
        m_modelSelector->addItem(displayName, modelName);
        LOG_INFO(QString("添加模型: %1").arg(displayName));
    } else {
        QString incompleteDisplayName = QString("%1 (配置不完整: %2)")
            .arg(displayName)
            .arg(missingItems.join(", "));
        m_modelSelector->addItem(incompleteDisplayName, modelName);
        LOG_WARNING(QString("添加未完整配置的模型: %1").arg(incompleteDisplayName));
    }
}

void MainWindow::restoreModelSelection(const QString& currentModel)
{
    int index = -1;
    if (!currentModel.isEmpty()) {
        // 先尝试通过 modelName 匹配
        index = m_modelSelector->findData(currentModel);
        if (index < 0) {
            // 如果找不到，尝试通过显示文本匹配
            for (int i = 0; i < m_modelSelector->count(); i++) {
                QString itemText = m_modelSelector->itemText(i);
                if (itemText.contains(currentModel, Qt::CaseInsensitive)) {
                    index = i;
                    break;
                }
            }
        }
    }

    // 设置当前选择的模型
    if (index >= 0) {
        m_modelSelector->setCurrentIndex(index);
        LOG_INFO(QString("恢复选择模型: %1").arg(m_modelSelector->currentText()));
    } else if (m_modelSelector->count() > 0) {
        m_modelSelector->setCurrentIndex(0);
        LOG_INFO(QString("选择第一个模型: %1").arg(m_modelSelector->currentText()));
    }
}

QString MainWindow::getModelTypeString() const
{
    switch (m_settingsModel->modelType()) {
        case SettingsModel::ModelType::API:
            return "api";
        case SettingsModel::ModelType::Ollama:
            return "ollama";
        case SettingsModel::ModelType::Local:
            return "local";
        default:
            LOG_WARNING("未知的模型类型");
            return "api";
    }
}

QString MainWindow::getModelDisplayName(const QString& type, const QString& modelName, const QString& provider) const
{
    if (type == "api") {
        QJsonObject config = m_settingsModel->getModelConfig(type, modelName);
        return QString("%1: %2")
            .arg(provider)
            .arg(config["name"].toString(modelName));
    } else if (type == "ollama") {
        return QString("Ollama: %1").arg(modelName);
    } else if (type == "local") {
        QJsonObject config = m_settingsModel->getModelConfig(type, modelName);
        return QString("本地: %1")
            .arg(config["name"].toString(modelName));
    }
    return modelName;
}

void MainWindow::onModelSelectionChanged(int index)
{
    if (index < 0 || m_isUpdating) {
        return;
    }

    QString displayName = m_modelSelector->currentText();
    QString modelName = m_modelSelector->currentData().toString();

    // 如果选择了"无可用模型"，直接返回
    if (displayName == tr("无可用模型") || modelName.isEmpty()) {
        LOG_WARNING("未选择有效的模型");
        if (m_chatViewModel) {
            m_chatViewModel->setLLMService(nullptr);
        }
        return;
    }

    LOG_INFO(QString("选择模型: %1 (显示名称: %2)").arg(modelName).arg(displayName));

    // 先清理当前的服务
    if (m_chatViewModel) {
        m_chatViewModel->setLLMService(nullptr);
    }

    // 根据显示名称判断模型类型
    SettingsModel::ModelType modelType;
    if (displayName.startsWith("Ollama:")) {
        modelType = SettingsModel::ModelType::Ollama;
        // 移除 "Ollama: " 前缀
        modelName = displayName.mid(8).trimmed();
    } else if (displayName.startsWith("本地:")) {
        modelType = SettingsModel::ModelType::Local;
        // 移除 "本地: " 前缀
        modelName = displayName.mid(4).trimmed();
    } else {
        modelType = SettingsModel::ModelType::API;
    }

    // 检查模型配置是否完整
    if (!m_settingsModel->isModelConfigComplete(m_settingsModel->getModelTypeString(modelType), modelName)) {
        QStringList missingItems = m_settingsModel->getMissingConfigItems(
            m_settingsModel->getModelTypeString(modelType), modelName);
        QString errorMsg = tr("模型配置不完整，缺少: %1").arg(missingItems.join(", "));
        LOG_ERROR(errorMsg);
        showError(tr("配置错误"), errorMsg);
        // 恢复到之前的选择
        m_isUpdating = true;
        updateModelList();
        m_isUpdating = false;
        return;
    }

    // 更新当前模型
    m_settingsModel->setModelType(modelType);
    m_settingsModel->setCurrentModelName(modelName);
    LOG_INFO(QString("切换到模型: %1 (类型: %2)").arg(modelName).arg(static_cast<int>(modelType)));

    // 创建新的LLMService
    LLMService* service = m_settingsViewModel->createLLMService();
    if (!service) {
        QString errorMsg = tr("创建模型服务失败: %1").arg(modelName);
        LOG_ERROR(errorMsg);
        showError(tr("服务错误"), errorMsg);
        // 恢复到之前的选择
        m_isUpdating = true;
        updateModelList();
        m_isUpdating = false;
        return;
    }

    // 设置新的服务
    m_chatViewModel->setLLMService(service);
    LOG_INFO(QString("已切换到模型服务: %1").arg(modelName));
}

void MainWindow::saveSettings()
{
    // 保存设置
    m_settingsModel->saveSettings();
    LOG_INFO("设置保存完成");
}

void MainWindow::applyMessageAnimation()
{
    // 获取最后一个消息元素并应用淡入动画
    QTextCursor cursor = m_chatDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_chatDisplay->setTextCursor(cursor);
    
    // 确保滚动到底部以显示新消息
    QScrollBar* scrollBar = m_chatDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
    
    // 给消息容器应用动画效果
    // 注意：动画已在CSS中定义，这里只需要确保滚动到最新消息
}

void MainWindow::onSendMessage()
{
    QString message = m_messageInput->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // 获取当前时间
    QDateTime currentTime = QDateTime::currentDateTime();
    QString timeStr = currentTime.toString("yyyy-MM-dd hh:mm:ss");
    
    // 将用户消息转换为HTML（支持Markdown语法）
    QString htmlMessage = MarkdownParser::toHtml(message);
    
    // 获取气泡样式类名
    QString userBubbleClass = ThemeManager::instance().getUserBubbleStyle();
    
    // 添加用户头像和气泡样式的消息，使用CSS类
    m_chatDisplay->append(QString(
        "<div class='message-container'>\n"
        "  <div class='avatar user-avatar'>用</div>\n"
        "  <div class='message-content'>\n"
        "    <div class='sender-info'>用户 <span class='timestamp'>[%1]</span></div>\n"
        "    <div class='%2 markdown-content'>%3</div>\n"
        "  </div>\n"
        "</div>")
        .arg(timeStr)
        .arg(userBubbleClass)
        .arg(htmlMessage));
    
    // 应用消息动画
    applyMessageAnimation();

    m_messageInput->clear();

    // 获取AI名称
    QString aiName = m_settingsModel->aiName();
    if (aiName.isEmpty()) {
        aiName = "皮蛋"; // 默认名称
    }
    
    // 获取AI气泡样式类名
    QString aiBubbleClass = ThemeManager::instance().getAIBubbleStyle();
    QDateTime aiResponseTime = QDateTime::currentDateTime();
    QString aiTimeStr = aiResponseTime.toString("yyyy-MM-dd hh:mm:ss");
    
    // 添加AI头像和气泡样式的初始响应，使用CSS类
    m_chatDisplay->append(QString(
        "<div class='message-container'>\n"
        "  <div class='avatar ai-avatar'>皮</div>\n"
        "  <div class='message-content'>\n"
        "    <div class='sender-info'>%1 <span class='timestamp'>[%2]</span></div>\n"
        "    <div class='%3 markdown-content'>")
        .arg(aiName)
        .arg(aiTimeStr)
        .arg(aiBubbleClass));
    
    // 应用消息动画
    applyMessageAnimation();

    // 发送消息
    m_chatViewModel->sendMessage(message);
}

void MainWindow::onGenerationStarted()
{
    updateSendButton(true);
}

void MainWindow::onGenerationFinished()
{
    updateSendButton(false);
    
    // 完成当前响应，关闭HTML标签
    m_chatDisplay->moveCursor(QTextCursor::End);
    m_chatDisplay->insertHtml("</div>\n  </div>\n</div>");
    
    // 应用消息动画
    applyMessageAnimation();
    
    // 确保滚动到最新消息
    m_chatDisplay->verticalScrollBar()->setValue(
        m_chatDisplay->verticalScrollBar()->maximum()
    );
}

void MainWindow::onStreamResponse(const QString& partialResponse)
{
    // 对于流式响应，我们使用简化的Markdown解析
    QString processedResponse = partialResponse.toHtmlEscaped().replace("\n", "<br>");
    
    // 处理行内代码段 `code`
    QRegularExpression inlineCodeRegex("`([^`]*?)`");
    QRegularExpressionMatchIterator i = inlineCodeRegex.globalMatch(processedResponse);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString code = match.captured(1);
        QString htmlCode = QString("<code style=\"background-color: rgba(0,0,0,0.1); padding: 2px 4px; border-radius: 3px; font-family: monospace;\">%1</code>").arg(code);
        processedResponse.replace(match.captured(0), htmlCode);
    }
    
    // 处理粗体 **text**
    QRegularExpression boldRegex("\\*\\*(.*?)\\*\\*");
    i = boldRegex.globalMatch(processedResponse);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString text = match.captured(1);
        QString htmlText = QString("<strong>%1</strong>").arg(text);
        processedResponse.replace(match.captured(0), htmlText);
    }
    
    // 处理斜体 *text*
    QRegularExpression italicRegex("\\*([^\\*]*?)\\*");
    i = italicRegex.globalMatch(processedResponse);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString text = match.captured(1);
        QString htmlText = QString("<em>%1</em>").arg(text);
        processedResponse.replace(match.captured(0), htmlText);
    }
    
    m_chatDisplay->insertHtml(processedResponse);
    
    // 保持滚动到底部
    m_chatDisplay->verticalScrollBar()->setValue(
        m_chatDisplay->verticalScrollBar()->maximum()
    );
}

void MainWindow::updateSendButton(bool isGenerating)
{
    m_isGenerating = isGenerating;
    if (isGenerating) {
        m_sendButton->setText(tr("取消"));
        m_sendButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
    } else {
        m_sendButton->setText(tr("发送"));
        m_sendButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_CommandLink));
    }
    m_messageInput->setEnabled(!isGenerating);
}

void MainWindow::onClearChat()
{
    // 清空聊天显示区域
    m_chatDisplay->clear();
    LOG_INFO("聊天记录已清除");
}

void MainWindow::onOpenSettings()
{
    // 创建并显示设置对话框
    SettingsDialog dialog(m_settingsViewModel, this);
    if (dialog.exec() == QDialog::Accepted) {
        LOG_INFO("设置对话框已确认，开始更新设置");
        
        // 保存设置
        saveSettings();
        
        // 如果当前类型是Ollama，先刷新模型列表
        if (m_settingsModel->modelType() == SettingsModel::ModelType::Ollama) {
            m_settingsModel->refreshOllamaModels();
        }
        
        // 更新模型列表
        updateModelList();
        
        // 获取当前选择的模型
        QString currentModel = m_settingsModel->currentModelName();
        if (!currentModel.isEmpty()) {
            LOG_INFO(QString("准备创建模型服务 - 类型: %1, 模型: %2")
                .arg(static_cast<int>(m_settingsModel->modelType()))
                .arg(currentModel));
            
            // 先清理当前的服务
            if (m_chatViewModel) {
                m_chatViewModel->setLLMService(nullptr);
            }
            
            // 创建新的LLMService
            LLMService* service = m_settingsViewModel->createLLMService();
            if (service) {
                m_chatViewModel->setLLMService(service);
                LOG_INFO(QString("成功创建并设置模型服务: %1").arg(currentModel));
            } else {
                LOG_ERROR(QString("创建模型服务失败: %1").arg(currentModel));
            }
        } else {
            LOG_WARNING("当前没有选择模型，跳过服务创建");
        }
        
        LOG_INFO("设置更新完成");
    }
}

void MainWindow::onSaveChat()
{
    // 获取保存文件的路径
    QString filePath = QFileDialog::getSaveFileName(this,
        tr("保存聊天记录"),
        QDir::homePath(),
        tr("文本文件 (*.txt);;所有文件 (*.*)"));

    if (filePath.isEmpty()) {
        return;
    }

    // 打开文件并保存聊天内容
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << m_chatDisplay->toPlainText();
        file.close();
        LOG_INFO(QString("聊天记录已保存到: %1").arg(filePath));
    } else {
        showError(tr("保存失败"), tr("无法保存聊天记录到文件"));
    }
}

void MainWindow::onLoadChat()
{
    // 获取要加载的文件路径
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("加载聊天记录"),
        QDir::homePath(),
        tr("文本文件 (*.txt);;所有文件 (*.*)"));

    if (filePath.isEmpty()) {
        return;
    }

    // 打开文件并读取内容
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        m_chatDisplay->setPlainText(in.readAll());
        file.close();
        LOG_INFO(QString("已加载聊天记录: %1").arg(filePath));
    } else {
        showError(tr("加载失败"), tr("无法从文件加载聊天记录"));
    }
}

void MainWindow::onAbout()
{
    // 显示关于对话框
    QMessageBox::about(this,
        tr("关于 ChatDot"),
        tr("ChatDot - AI聊天助手\n\n"
           "版本: 1.0.0\n"
           "一个简单而强大的聊天应用程序。\n\n"
           "角色预设: 皮蛋 - 你的智能聊天助手\n\n"
           "支持多种模型，包括：\n"
           "- OpenAI API\n"
           "- Deepseek API\n"
           "- Ollama本地模型\n"
           "- 本地模型"));
}

void MainWindow::onError(const QString& error)
{
    showError(tr("错误"), error);
}

void MainWindow::onLogMessage(Logger::Level level, const QString& message)
{
    // 根据日志级别设置不同的颜色
    QString color;
    switch (level) {
        case Logger::Level::Debug:
            color = "gray";
            break;
        case Logger::Level::Info:
            color = "black";
            break;
        case Logger::Level::Warning:
            color = "orange";
            break;
        case Logger::Level::Error:
            color = "red";
            break;
        default:
            color = "black";
    }

    // 在状态栏显示日志消息
    if (m_statusLabel) {
        m_statusLabel->setText(QString("<span style='color: %1'>%2</span>")
            .arg(color)
            .arg(message));
    }
}

void MainWindow::updateStatusBar()
{
    // 更新状态栏信息
    QString status;
    if (m_isGenerating) {
        status = tr("正在生成回复...");
    } else {
        status = tr("就绪");
    }

    if (m_statusLabel) {
        m_statusLabel->setText(status);
    }
}

void MainWindow::selectImage()
{
    // 获取要上传的图片路径
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("选择图片"),
        QDir::homePath(),
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif);;所有文件 (*.*)"));

    if (filePath.isEmpty()) {
        return;
    }

    // 加载图片
    QImage image(filePath);
    if (image.isNull()) {
        showError(tr("错误"), tr("无法加载图片"));
        return;
    }

    // 将图片添加到聊天显示区域
    m_chatDisplay->append(tr("<p><b>用户:</b> [图片]</p>"));
    m_chatDisplay->append(tr("<p><img src='%1' width='300'/></p>").arg(filePath));

    // 处理图片（这里可以添加图片处理逻辑）
    LOG_INFO(QString("已选择图片: %1").arg(filePath));
}

void MainWindow::showError(const QString& title, const QString& message)
{
    QMessageBox::critical(this, title, message);
    LOG_ERROR(message);
}

void MainWindow::onDeepThinkingToggled(bool checked)
{
    m_isDeepThinking = checked;
    if (m_chatViewModel) {
        m_chatViewModel->setDeepThinkingMode(checked);
    }
    
    // 更新按钮状态显示
    if (checked) {
        m_deepThinkingButton->setText(tr("深度思考中"));
        m_deepThinkingButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    } else {
        m_deepThinkingButton->setText(tr("深度思考"));
        m_deepThinkingButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    }
    
    LOG_INFO(QString("深度思考模式: %1").arg(checked ? "开启" : "关闭"));
}

void MainWindow::createThemeMenu()
{
    // 这个方法不再需要，因为主题菜单已经在 setupMenu 中创建
}

void MainWindow::onThemeChanged(ThemeManager::Theme theme)
{
    updateThemeActions();
}

void MainWindow::setLightTheme()
{
    ThemeManager::instance().setTheme(ThemeManager::Theme::Light);
}

void MainWindow::setDarkTheme()
{
    ThemeManager::instance().setTheme(ThemeManager::Theme::Dark);
}

void MainWindow::setSystemTheme()
{
    ThemeManager::instance().setTheme(ThemeManager::Theme::System);
}

void MainWindow::updateThemeActions()
{
    ThemeManager::Theme currentTheme = ThemeManager::instance().currentTheme();
    
    // 断开所有动作的信号连接，防止触发不必要的主题切换
    m_lightThemeAction->blockSignals(true);
    m_darkThemeAction->blockSignals(true);
    m_systemThemeAction->blockSignals(true);
    
    // 更新选中状态
    m_lightThemeAction->setChecked(currentTheme == ThemeManager::Theme::Light);
    m_darkThemeAction->setChecked(currentTheme == ThemeManager::Theme::Dark);
    m_systemThemeAction->setChecked(currentTheme == ThemeManager::Theme::System);
    
    // 恢复信号连接
    m_lightThemeAction->blockSignals(false);
    m_darkThemeAction->blockSignals(false);
    m_systemThemeAction->blockSignals(false);
}
