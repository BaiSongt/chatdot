#include "settingsdialog.h"
#include "services/logger.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QFormLayout>

SettingsDialog::SettingsDialog(SettingsViewModel* viewModel, QWidget *parent)
    : QDialog(parent)
    , m_viewModel(viewModel)
{
    setupUI();
    setupConnections();
    updateUIFromViewModel();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    // 设置对话框属性
    setWindowTitle(tr("设置"));
    resize(600, 400);

    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 模型类型选择
    QHBoxLayout* modelTypeLayout = new QHBoxLayout();
    modelTypeLayout->addWidget(new QLabel(tr("模型类型:")));
    m_modelTypeSelector = new QComboBox();
    m_modelTypeSelector->addItem(tr("API"), static_cast<int>(SettingsModel::ModelType::API));
    m_modelTypeSelector->addItem(tr("Ollama"), static_cast<int>(SettingsModel::ModelType::Ollama));
    m_modelTypeSelector->addItem(tr("本地模型"), static_cast<int>(SettingsModel::ModelType::Local));
    modelTypeLayout->addWidget(m_modelTypeSelector);
    mainLayout->addLayout(modelTypeLayout);

    // 创建堆叠式小部件用于不同模型类型的设置
    m_modelSettingsStack = new QStackedWidget();
    mainLayout->addWidget(m_modelSettingsStack);
    
    // 1. API设置页面
    m_apiSettingsWidget = new QWidget();
    QVBoxLayout* apiLayout = new QVBoxLayout(m_apiSettingsWidget);

    // 使用表单布局来组织API设置
    QFormLayout* apiFormLayout = new QFormLayout();
    
    // API提供商选择
    m_apiProviderSelector = new QComboBox();
    m_apiProviderSelector->addItem("OpenAI", "https://api.openai.com/v1/chat/completions");
    m_apiProviderSelector->addItem("Deepseek", "https://api.deepseek.com/v1/chat/completions");
    m_apiProviderSelector->addItem(tr("自定义"), "custom");
    apiFormLayout->addRow(tr("API提供商:"), m_apiProviderSelector);

    // API URL设置（仅当选择自定义时显示）
    m_apiUrlWidget = new QWidget();
    QHBoxLayout* apiUrlLayout = new QHBoxLayout(m_apiUrlWidget);
    apiUrlLayout->setContentsMargins(0, 0, 0, 0);
    m_apiUrlInput = new QLineEdit();
    apiUrlLayout->addWidget(m_apiUrlInput);
    apiFormLayout->addRow(tr("API URL:"), m_apiUrlWidget);
    m_apiUrlWidget->setVisible(false); // 默认隐藏

    // API Key设置
    m_apiKeyInput = new QLineEdit();
    m_apiKeyInput->setEchoMode(QLineEdit::Password); // 密码模式显示
    apiFormLayout->addRow(tr("API Key:"), m_apiKeyInput);

    // API模型选择和获取按钮
    QHBoxLayout* apiModelLayout = new QHBoxLayout();
    m_apiModelSelector = new QComboBox();
    apiModelLayout->addWidget(m_apiModelSelector);
    m_fetchApiModelsBtn = new QPushButton(tr("获取模型列表"));
    apiModelLayout->addWidget(m_fetchApiModelsBtn);
    apiFormLayout->addRow(tr("API模型:"), apiModelLayout);
    
    apiLayout->addLayout(apiFormLayout);
    m_modelSettingsStack->addWidget(m_apiSettingsWidget);
    
    // 2. Ollama设置页面
    m_ollamaSettingsWidget = new QWidget();
    QVBoxLayout* ollamaLayout = new QVBoxLayout(m_ollamaSettingsWidget);
    
    QFormLayout* ollamaFormLayout = new QFormLayout();
    
    // Ollama主机设置
    m_ollamaHostInput = new QLineEdit();
    m_ollamaHostInput->setText("localhost"); // 默认值
    ollamaFormLayout->addRow(tr("主机:"), m_ollamaHostInput);

    // Ollama端口设置
    m_ollamaPortInput = new QSpinBox();
    m_ollamaPortInput->setRange(1, 65535);
    m_ollamaPortInput->setValue(11434); // 默认端口
    ollamaFormLayout->addRow(tr("端口:"), m_ollamaPortInput);

    // Ollama模型选择和刷新按钮
    QHBoxLayout* ollamaModelLayout = new QHBoxLayout();
    m_ollamaModelSelector = new QComboBox();
    ollamaModelLayout->addWidget(m_ollamaModelSelector);
    m_refreshOllamaModelsBtn = new QPushButton(tr("刷新模型列表"));
    ollamaModelLayout->addWidget(m_refreshOllamaModelsBtn);
    ollamaFormLayout->addRow(tr("模型:"), ollamaModelLayout);
    
    ollamaLayout->addLayout(ollamaFormLayout);
    m_modelSettingsStack->addWidget(m_ollamaSettingsWidget);

    // 3. 本地模型设置页面
    m_localModelSettingsWidget = new QWidget();
    QVBoxLayout* localLayout = new QVBoxLayout(m_localModelSettingsWidget);
    
    QFormLayout* localFormLayout = new QFormLayout();
    
    // 本地模型路径设置
    QHBoxLayout* localPathLayout = new QHBoxLayout();
    m_localModelPathInput = new QLineEdit();
    localPathLayout->addWidget(m_localModelPathInput);
    m_browseLocalModelBtn = new QPushButton(tr("浏览..."));
    localPathLayout->addWidget(m_browseLocalModelBtn);
    localFormLayout->addRow(tr("模型路径:"), localPathLayout);
    
    localLayout->addLayout(localFormLayout);
    m_modelSettingsStack->addWidget(m_localModelSettingsWidget);
    
    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_cancelButton = new QPushButton(tr("取消"));
    m_saveButton = new QPushButton(tr("保存"));
    m_saveButton->setDefault(true);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_saveButton);
    mainLayout->addLayout(buttonLayout);

    // 根据当前模型类型显示/隐藏相关设置组
    onModelTypeChanged(m_modelTypeSelector->currentIndex());
    onApiProviderChanged(m_apiProviderSelector->currentIndex());
}

void SettingsDialog::setupConnections()
{
    // ViewModel -> View 信号连接
    connect(m_viewModel, &SettingsViewModel::modelTypeChanged, this, [this]() {
        // 更新模型类型选择器和堆叠式小部件
        int type = static_cast<int>(m_viewModel->modelType());
        m_modelTypeSelector->setCurrentIndex(m_modelTypeSelector->findData(type));
        m_modelSettingsStack->setCurrentIndex(type);
    });
    
    // 使用lambda函数处理信号参数不匹配问题
    connect(m_viewModel, &SettingsViewModel::apiModelsChanged, this, [this]() {
        // 当API模型列表变化时，使用当前提供商更新列表
        updateApiModelList(m_viewModel->apiProvider());
    });
    
    connect(m_viewModel, &SettingsViewModel::ollamaModelsChanged, this, &SettingsDialog::updateOllamaModelList);
    connect(m_viewModel, &SettingsViewModel::refreshStateChanged, this, &SettingsDialog::onRefreshStateChanged);
    
    // View -> ViewModel 信号连接
    connect(m_modelTypeSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &SettingsDialog::onModelTypeChanged);
    
    connect(m_apiProviderSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onApiProviderChanged);
    
    connect(m_apiKeyInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        // 当API Key变化时更新ViewModel
        QString provider = m_apiProviderSelector->currentText();
        m_viewModel->setProviderApiKey(provider, text);
    });
    
    connect(m_apiUrlInput, &QLineEdit::textChanged, m_viewModel, &SettingsViewModel::setApiUrl);
    connect(m_apiModelSelector, &QComboBox::currentTextChanged, m_viewModel, &SettingsViewModel::setCurrentModelName);
    
    // 将Ollama主机和端口的变化转换为URL格式并设置到ViewModel
    connect(m_ollamaHostInput, &QLineEdit::textChanged, this, [this](const QString& host) {
        QString url = QString("http://%1:%2").arg(host).arg(m_ollamaPortInput->value());
        m_viewModel->setOllamaUrl(url);
    });
    
    connect(m_ollamaPortInput, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, [this](int port) {
        QString url = QString("http://%1:%2").arg(m_ollamaHostInput->text()).arg(port);
        m_viewModel->setOllamaUrl(url);
    });
    connect(m_ollamaModelSelector, &QComboBox::currentTextChanged, 
            m_viewModel, &SettingsViewModel::setCurrentModelName);
    
    connect(m_refreshOllamaModelsBtn, &QPushButton::clicked, 
            m_viewModel, &SettingsViewModel::refreshOllamaModels);
    
    connect(m_fetchApiModelsBtn, &QPushButton::clicked, this, [this]() {
        QString provider = m_apiProviderSelector->currentText();
        if (!provider.isEmpty()) {
            m_viewModel->fetchApiModelsFromProvider(provider);
        }
    });
    
    connect(m_localModelPathInput, &QLineEdit::textChanged, 
            m_viewModel, &SettingsViewModel::setLocalModelPath);
    
    connect(m_browseLocalModelBtn, &QPushButton::clicked, 
            this, &SettingsDialog::onBrowseLocalModelClicked);
    
    // 错误处理连接
    connect(m_viewModel, &SettingsViewModel::errorOccurred, this, &SettingsDialog::onErrorOccurred);
    
    // 按钮连接
    connect(m_saveButton, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsDialog::onCancelClicked);
}

void SettingsDialog::updateUIFromViewModel()
{
    // 从 ViewModel 更新 UI 状态
    LOG_INFO("从 ViewModel 更新 UI");
    
    // 更新模型类型
    int modelType = m_viewModel->modelType();
    int typeIndex = m_modelTypeSelector->findData(modelType);
    if (typeIndex >= 0) {
        m_modelTypeSelector->setCurrentIndex(typeIndex);
        m_modelSettingsStack->setCurrentIndex(typeIndex);
    }
    
    // 更新API设置
    QString apiProvider = m_viewModel->apiProvider();
    int providerIndex = m_apiProviderSelector->findText(apiProvider);
    if (providerIndex >= 0) {
        m_apiProviderSelector->setCurrentIndex(providerIndex);
    }
    
    m_apiUrlInput->setText(m_viewModel->apiUrl());
    m_apiKeyInput->setText(m_viewModel->getProviderApiKey(apiProvider));
    
    // 更新API模型列表
    updateApiModelList(apiProvider);
    m_apiModelSelector->setCurrentText(m_viewModel->currentModelName());
    
    // 更新Ollama设置
    QString ollamaUrl = m_viewModel->ollamaUrl();
    // 分解URL为主机和端口
    QUrl url(ollamaUrl);
    m_ollamaHostInput->setText(url.host());
    m_ollamaPortInput->setValue(url.port(9000)); // 默认端口9000
    updateOllamaModelList();
    
    // 更新本地模型设置
    m_localModelPathInput->setText(m_viewModel->getLocalModelPath());
}

void SettingsDialog::onRefreshStateChanged(bool isRefreshing)
{
    // 处理模型刷新状态变化
    // 根据当前显示的设置页面决定更新哪个按钮
    int currentIndex = m_modelSettingsStack->currentIndex();
    
    if (currentIndex == static_cast<int>(SettingsModel::ModelType::API)) {
        // 更新API模型获取按钮状态
        m_fetchApiModelsBtn->setEnabled(!isRefreshing);
        m_fetchApiModelsBtn->setText(isRefreshing ? tr("正在获取...") : tr("获取模型列表"));
    } else if (currentIndex == static_cast<int>(SettingsModel::ModelType::Ollama)) {
        // 更新Ollama模型刷新按钮状态
        m_refreshOllamaModelsBtn->setEnabled(!isRefreshing);
        m_refreshOllamaModelsBtn->setText(isRefreshing ? tr("正在刷新...") : tr("刷新模型列表"));
    }
}

void SettingsDialog::updateOllamaModelList()
{
    LOG_INFO("正在更新Ollama模型列表UI");
    QString currentModel = m_ollamaModelSelector->currentText();
    m_ollamaModelSelector->clear();

    QStringList models = m_viewModel->ollamaModels();
    LOG_INFO(QString("获取到%1个Ollama模型").arg(models.size()));

    if (models.isEmpty()) {
        m_ollamaModelSelector->addItem(tr("无可用模型"));
        m_ollamaModelSelector->setEnabled(false);
    } else {
        m_ollamaModelSelector->setEnabled(true);
        m_ollamaModelSelector->addItems(models);

        // 恢复之前选择的模型
        int index = m_ollamaModelSelector->findText(currentModel);
        if (index >= 0) {
            m_ollamaModelSelector->setCurrentIndex(index);
            LOG_INFO(QString("恢复选择模型: %1").arg(currentModel));
        } else if (!models.isEmpty()) {
            // 如果找不到之前的模型，但列表不为空，选择第一个
            m_ollamaModelSelector->setCurrentIndex(0);
            LOG_INFO(QString("选择新模型: %1").arg(m_ollamaModelSelector->currentText()));
        }
    }
}

void SettingsDialog::updateApiModelList(const QString& provider)
{
    LOG_INFO(QString("更新API模型列表，提供商: %1").arg(provider));
    
    QString currentModel = m_apiModelSelector->currentText();
    m_apiModelSelector->clear();
    
    QStringList models = m_viewModel->getApiModels(provider);
    if (!models.isEmpty()) {
        m_apiModelSelector->addItems(models);
        
        // 尝试恢复之前的选择
        int index = m_apiModelSelector->findText(currentModel);
        if (index >= 0) {
            m_apiModelSelector->setCurrentIndex(index);
        } else if (!models.isEmpty()) {
            m_apiModelSelector->setCurrentIndex(0);
        }
    }
}

void SettingsDialog::onModelTypeChanged(int index)
{
    LOG_INFO(QString("模型类型已更改为: %1").arg(index));
    
    // 获取模型类型数据
    int type = m_modelTypeSelector->itemData(index).toInt();
    
    // 更新ViewModel
    m_viewModel->setModelType(type);
    
    // 切换到相应的设置页面
    m_modelSettingsStack->setCurrentIndex(index);
}

void SettingsDialog::onApiProviderChanged(int index)
{
    LOG_INFO(QString("API提供商已更改为索引: %1").arg(index));
    
    QString provider = m_apiProviderSelector->currentText();
    QString url = m_apiProviderSelector->currentData().toString();
    
    // 更新ViewModel
    m_viewModel->setApiProvider(provider);
    
    // 如果是自定义提供商，显示URL输入框
    bool isCustom = (provider == tr("自定义"));
    m_apiUrlWidget->setVisible(isCustom);
    
    if (!isCustom) {
        m_apiUrlInput->setText(url);
        m_viewModel->setApiUrl(url);
    }
    
    // 更新API Key显示
    m_apiKeyInput->setText(m_viewModel->getProviderApiKey(provider));
    
    // 更新模型列表
    updateApiModelList(provider);
}

void SettingsDialog::onSaveClicked()
{
    LOG_INFO("保存设置");
    
    // 调用ViewModel的保存方法
    m_viewModel->saveSettings();
    
    // 关闭对话框
    accept();
}

void SettingsDialog::onCancelClicked()
{
    LOG_INFO("取消设置");
    reject();
}

void SettingsDialog::onBrowseLocalModelClicked()
{
    LOG_INFO("浏览本地模型");
    
    // 打开文件选择对话框
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择模型文件"), "", tr("GGUF模型文件 (*.gguf);;所有文件 (*)"));
    if (!filePath.isEmpty()) {
        m_localModelPathInput->setText(filePath);
    }
}

void SettingsDialog::onErrorOccurred(const QString& error)
{
    // 显示错误消息对话框
    QMessageBox::warning(this, tr("操作失败"), error);
    
    // 如果是获取API模型列表失败，重置按钮状态
    m_fetchApiModelsBtn->setEnabled(true);
    m_fetchApiModelsBtn->setText(tr("获取模型列表"));
}

// 这些方法已经被移除，因为它们已经被新的MVVM架构替代
