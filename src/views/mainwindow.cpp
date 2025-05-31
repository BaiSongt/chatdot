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
            LOG_INFO("设置加载完成");
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
    // 确保设置已加载
    m_settingsModel->loadSettings();

    // 刷新模型列表
    refreshModelList();

    LOG_INFO("设置加载完成");
}

void MainWindow::refreshModelList()
{
    // 如果当前类型是Ollama，先刷新模型列表
    if (m_settingsModel->modelType() == SettingsModel::ModelType::Ollama) {
        m_settingsModel->refreshOllamaModels();
    }
    updateModelList();
}

void MainWindow::updateModelList()
{
    LOG_INFO("更新模型列表");
    QString currentModel = m_modelSelector->currentText();
    m_modelSelector->clear();

    // API 模型分类
    m_modelSelector->addItem(tr("--- OpenAI 模型 ---"), "");
    m_modelSelector->addItem("GPT-4 Turbo", "gpt-4-1106-preview");
    m_modelSelector->addItem("GPT-4", "gpt-4");
    m_modelSelector->addItem("GPT-3.5 Turbo 16K", "gpt-3.5-turbo-16k");
    m_modelSelector->addItem("GPT-3.5 Turbo", "gpt-3.5-turbo");

    // Deepseek 模型
    m_modelSelector->addItem(tr("--- Deepseek 模型 ---"), "");
    m_modelSelector->addItem("DeepSeek Chat", "deepseek-chat");
    m_modelSelector->addItem("DeepSeek Coder", "deepseek-coder");

    // Ollama 模型
    if (m_settingsModel->modelType() == SettingsModel::ModelType::Ollama) {
        QStringList ollamaModels = m_settingsModel->ollamaModels();
        if (!ollamaModels.isEmpty()) {
            m_modelSelector->addItem(tr("--- Ollama 模型 ---"), "");
            for (const QString& model : ollamaModels) {
                m_modelSelector->addItem(QString("Ollama: %1").arg(model), model);
            }
        }
    }

    // 本地模型
    if (!m_settingsModel->modelPath().isEmpty()) {
        m_modelSelector->addItem(tr("--- 本地模型 ---"), "");
        m_modelSelector->addItem(QString("本地: %1").arg(QFileInfo(m_settingsModel->modelPath()).fileName()));
    }

    // 恢复之前选择的模型
    int index = -1;

    // 先尝试通过modelName匹配
    if (!m_settingsModel->currentModelName().isEmpty()) {
        index = m_modelSelector->findData(m_settingsModel->currentModelName());
        if (index < 0) {
            // 如果找不到，尝试通过显示文本匹配
            QString modelName = m_settingsModel->currentModelName();
            for (int i = 0; i < m_modelSelector->count(); i++) {
                QString itemText = m_modelSelector->itemText(i);
                if (itemText.contains(modelName, Qt::CaseInsensitive)) {
                    index = i;
                    break;
                }
            }
        }
    }    // 阻止信号触发，避免在恢复选择时触发onModelSelectionChanged
    m_modelSelector->blockSignals(true);

    // 如果找到了匹配的选项，设置为当前选择
    if (index >= 0) {
        m_modelSelector->setCurrentIndex(index);
    }
    // 否则选择第一个有效的模型（跳过分类标题）
    else {
        for (int i = 0; i < m_modelSelector->count(); i++) {
            if (!m_modelSelector->itemData(i).toString().isEmpty() &&
                !m_modelSelector->itemText(i).startsWith("---")) {
                m_modelSelector->setCurrentIndex(i);
                break;
            }
        }
    }

    // 恢复信号连接
    m_modelSelector->blockSignals(false);

    LOG_INFO(QString("模型列表更新完成，当前选择: %1").arg(m_modelSelector->currentText()));
}

void MainWindow::onModelSelectionChanged(int index)
{
    // 如果正在更新列表，忽略选择变化
    static bool isUpdating = false;
    if (isUpdating) {
        return;
    }

    // 获取选择的模型数据
    QString modelName = m_modelSelector->currentData().toString();
    QString displayName = m_modelSelector->currentText();

    // 如果是分类标题（data为空），或者是分隔符，忽略此次选择
    if (modelName.isEmpty() || displayName.startsWith("---")) {
        LOG_INFO("忽略分类标题选择");

        // 标记开始更新
        isUpdating = true;
        // 恢复到之前的有效选择
        QString lastValidModel = m_settingsModel->currentModelName();
        int validIndex = -1;

        // 首先尝试通过modelName匹配
        if (!lastValidModel.isEmpty()) {
            validIndex = m_modelSelector->findData(lastValidModel);
        }

        // 如果没找到，尝试找到第一个有效的模型
        if (validIndex < 0) {
            for (int i = 0; i < m_modelSelector->count(); i++) {
                QString itemData = m_modelSelector->itemData(i).toString();
                QString itemText = m_modelSelector->itemText(i);
                if (!itemData.isEmpty() && !itemText.startsWith("---")) {
                    validIndex = i;
                    break;
                }
            }
        }

        // 如果找到了有效索引，设置它
        if (validIndex >= 0) {
            m_modelSelector->setCurrentIndex(validIndex);
        }

        // 标记更新结束
        isUpdating = false;
        return;
    }

    // 如果是有效选择，使用modelName
    if (modelName.isEmpty()) {
        modelName = displayName;
    }

    // 判断模型类型并设置
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
        showError(tr("错误"), tr("无法初始化选中的模型，请检查设置"));
        // 恢复到之前的选择
        isUpdating = true;
        refreshModelList();
        isUpdating = false;
        return;
    }

    // 设置新的服务
    m_chatViewModel->setLLMService(service);
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
        // 如果设置被修改，刷新模型列表
        refreshModelList();
        LOG_INFO("设置已更新");
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
