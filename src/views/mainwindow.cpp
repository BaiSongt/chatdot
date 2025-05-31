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
                !m_imageButton || !m_modelSelector) {
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

    // 顶部工具栏布局
    QHBoxLayout* topLayout = new QHBoxLayout();

    // 添加模型选择器
    topLayout->addWidget(new QLabel(tr("当前模型:")));
    m_modelSelector = new QComboBox();
    m_modelSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_modelSelector->setMinimumWidth(200);
    topLayout->addWidget(m_modelSelector);
    topLayout->addStretch();

    m_mainLayout->addLayout(topLayout);

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

    // 获取当前类型下已配置的模型列表
    QString modelType = getModelTypeString();
    QStringList availableModels = m_settingsModel->getConfiguredModels(modelType);
    LOG_INFO(QString("获取到 %1 类型已配置的模型列表，共 %2 个模型").arg(modelType).arg(availableModels.size()));

    // 根据不同类型处理模型列表
    if (modelType == "api") {
        updateApiModels(availableModels);
    } else if (modelType == "ollama") {
        updateOllamaModels(availableModels);
    } else if (modelType == "local") {
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

    LOG_INFO(QString("模型列表更新完成，当前选择: %1，可用模型数量: %2")
        .arg(m_modelSelector->currentText())
        .arg(availableModels.size()));
}

void MainWindow::updateApiModels(const QStringList& availableModels)
{
    QJsonObject apiConfig = m_settingsModel->getModelConfig("models_config", "");
    for (const QString& modelName : availableModels) {
        // 遍历所有提供商查找模型
        for (auto providerIt = apiConfig.begin(); providerIt != apiConfig.end(); ++providerIt) {
            if (providerIt.key() != "has_API" && providerIt.key() != "default_url" && 
                providerIt.value().isObject()) {
                QString provider = providerIt.key();
                QJsonObject providerConfig = providerIt.value().toObject();
                if (providerConfig.contains("models")) {
                    QJsonObject models = providerConfig["models"].toObject();
                    if (models.contains(modelName)) {
                        bool isComplete = m_settingsModel->isModelConfigComplete("models_config", modelName);
                        QStringList missingItems;
                        if (!isComplete) {
                            missingItems = m_settingsModel->getMissingConfigItems("models_config", modelName);
                        }
                        QString displayName = getModelDisplayName("models_config", modelName, provider);
                        addModelToSelector(displayName, modelName, isComplete, missingItems);
                        break;
                    }
                }
            }
        }
    }
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
    if (displayName.startsWith("Ollama:")) {
        m_settingsModel->setModelType(SettingsModel::ModelType::Ollama);
        // 移除 "Ollama: " 前缀
        modelName = displayName.mid(8).trimmed();
    } else if (displayName.startsWith("本地:")) {
        m_settingsModel->setModelType(SettingsModel::ModelType::Local);
        // 移除 "本地: " 前缀
        modelName = displayName.mid(4).trimmed();
    } else {
        m_settingsModel->setModelType(SettingsModel::ModelType::API);
    }

    // 更新当前模型
    m_settingsModel->setCurrentModelName(modelName);
    LOG_INFO(QString("切换到模型: %1 (类型: %2)").arg(modelName).arg(static_cast<int>(m_settingsModel->modelType())));

    // 创建新的LLMService
    LLMService* service = m_settingsViewModel->createLLMService();
    if (!service) {
        // 恢复到之前的选择
        m_isUpdating = true;
        updateModelList();  // 只更新列表，不刷新服务
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

void MainWindow::onSendMessage()
{
    QString message = m_messageInput->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // 显示用户消息
    m_chatDisplay->append(this->tr("<p><b>用户:</b> %1</p>").arg(message));
    m_messageInput->clear();

    // 添加 AI 的初始响应
    m_chatDisplay->append(this->tr("<p><b>AI:</b> "));

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
    // 完成当前响应
    m_chatDisplay->append("</p>");
}

void MainWindow::onStreamResponse(const QString& partialResponse)
{
    // 在当前行追加部分响应
    m_chatDisplay->moveCursor(QTextCursor::End);
    m_chatDisplay->insertPlainText(partialResponse);
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
    SettingsDialog dialog(m_settingsModel, this);
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
           "一个简单而强大的AI聊天应用程序。\n\n"
           "支持多种AI模型，包括：\n"
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
