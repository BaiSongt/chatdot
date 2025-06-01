#include "settingsdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include "services/logger.h"

SettingsDialog::SettingsDialog(SettingsModel* model, QWidget *parent)
    : QDialog(parent)
    , m_model(model)
{
    setupUI();
    setupConnections();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    setWindowTitle(tr("设置"));
    resize(600, 400);

    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 通用设置组
    m_generalGroup = new QGroupBox(tr("通用设置"), this);
    QVBoxLayout* generalLayout = new QVBoxLayout(m_generalGroup);

    // 模型类型选择
    QHBoxLayout* modelTypeLayout = new QHBoxLayout();
    modelTypeLayout->addWidget(new QLabel(tr("模型类型:")));
    m_modelTypeSelector = new QComboBox();
    m_modelTypeSelector->addItem(tr("API"), static_cast<int>(SettingsModel::ModelType::API));
    m_modelTypeSelector->addItem(tr("Ollama"), static_cast<int>(SettingsModel::ModelType::Ollama));
    m_modelTypeSelector->addItem(tr("本地模型"), static_cast<int>(SettingsModel::ModelType::Local));
    modelTypeLayout->addWidget(m_modelTypeSelector);
    generalLayout->addLayout(modelTypeLayout);

    // 自动保存设置
    m_autoSaveCheck = new QCheckBox(tr("自动保存对话历史"), m_generalGroup);
    generalLayout->addWidget(m_autoSaveCheck);

    // 最大历史记录数
    QHBoxLayout* maxHistoryLayout = new QHBoxLayout();
    maxHistoryLayout->addWidget(new QLabel(tr("最大历史记录数:")));
    m_maxHistorySpin = new QSpinBox();
    m_maxHistorySpin->setRange(10, 1000);
    m_maxHistorySpin->setValue(100);
    maxHistoryLayout->addWidget(m_maxHistorySpin);
    generalLayout->addLayout(maxHistoryLayout);

    // 超时设置
    QHBoxLayout* timeoutLayout = new QHBoxLayout();
    timeoutLayout->addWidget(new QLabel(tr("请求超时(秒):")));
    m_timeoutSpin = new QSpinBox();
    m_timeoutSpin->setRange(5, 300);
    m_timeoutSpin->setValue(30);
    timeoutLayout->addWidget(m_timeoutSpin);
    generalLayout->addLayout(timeoutLayout);

    mainLayout->addWidget(m_generalGroup);

    // API设置组
    m_apiGroup = new QGroupBox(tr("API设置"), this);
    QVBoxLayout* apiLayout = new QVBoxLayout(m_apiGroup);

    // API提供商选择
    QHBoxLayout* apiProviderLayout = new QHBoxLayout();
    apiProviderLayout->addWidget(new QLabel(tr("API提供商:")));
    m_apiProviderSelector = new QComboBox();
    m_apiProviderSelector->addItem("OpenAI", "https://api.openai.com/v1/chat/completions");
    m_apiProviderSelector->addItem("Deepseek", "https://api.deepseek.com/v1/chat/completions");
    m_apiProviderSelector->addItem(tr("自定义"), "custom");
    apiProviderLayout->addWidget(m_apiProviderSelector);
    apiLayout->addLayout(apiProviderLayout);

    // API URL输入（使用QWidget容器）
    m_apiUrlWidget = new QWidget();
    QHBoxLayout* apiUrlLayout = new QHBoxLayout(m_apiUrlWidget);
    apiUrlLayout->setContentsMargins(0, 0, 0, 0);
    apiUrlLayout->addWidget(new QLabel(tr("API地址:")));
    m_apiUrlInput = new QLineEdit();
    m_apiUrlInput->setPlaceholderText("https://your-api-endpoint/v1/chat/completions");
    apiUrlLayout->addWidget(m_apiUrlInput);
    apiLayout->addWidget(m_apiUrlWidget);

    // API密钥
    QHBoxLayout* apiKeyLayout = new QHBoxLayout();
    apiKeyLayout->addWidget(new QLabel(tr("API密钥:")));
    m_apiKeyInput = new QLineEdit();
    m_apiKeyInput->setEchoMode(QLineEdit::Password);
    apiKeyLayout->addWidget(m_apiKeyInput);
    apiLayout->addLayout(apiKeyLayout);

    // API模型选择
    QHBoxLayout* apiModelLayout = new QHBoxLayout();
    apiModelLayout->addWidget(new QLabel(tr("模型:")));
    m_apiModelSelector = new QComboBox();

    // OpenAI 模型
    m_openaiModels = QStringList({
        "gpt-4-1106-preview",
        "gpt-4",
        "gpt-3.5-turbo-16k",
        "gpt-3.5-turbo"
    });

    // Deepseek 模型
    m_deepseekModels = QStringList({
        "deepseek-chat",
        "deepseek-coder",
        "deepseek-math"
    });

    m_apiModelSelector->addItems(m_openaiModels);
    apiModelLayout->addWidget(m_apiModelSelector);
    apiLayout->addLayout(apiModelLayout);

    mainLayout->addWidget(m_apiGroup);

    // Ollama设置组
    m_ollamaGroup = new QGroupBox(tr("Ollama设置"), this);
    QVBoxLayout* ollamaLayout = new QVBoxLayout(m_ollamaGroup);

    // Ollama模型选择
    QHBoxLayout* ollamaModelLayout = new QHBoxLayout();
    ollamaModelLayout->addWidget(new QLabel(tr("模型:")));
    m_ollamaModelSelect = new QComboBox();
    ollamaModelLayout->addWidget(m_ollamaModelSelect);
    m_refreshOllamaModelsBtn = new QPushButton(tr("刷新"));
    ollamaModelLayout->addWidget(m_refreshOllamaModelsBtn);
    ollamaLayout->addLayout(ollamaModelLayout);

    // Ollama服务器设置
    QHBoxLayout* ollamaHostLayout = new QHBoxLayout();
    ollamaHostLayout->addWidget(new QLabel(tr("主机:")));
    m_ollamaHostInput = new QLineEdit();
    ollamaHostLayout->addWidget(m_ollamaHostInput);
    ollamaLayout->addLayout(ollamaHostLayout);

    QHBoxLayout* ollamaPortLayout = new QHBoxLayout();
    ollamaPortLayout->addWidget(new QLabel(tr("端口:")));
    m_ollamaPortInput = new QSpinBox();
    m_ollamaPortInput->setRange(1, 65535);
    ollamaPortLayout->addWidget(m_ollamaPortInput);
    ollamaLayout->addLayout(ollamaPortLayout);

    mainLayout->addWidget(m_ollamaGroup);

    // 本地模型设置组
    m_localGroup = new QGroupBox(tr("本地模型设置"), this);
    QVBoxLayout* localLayout = new QVBoxLayout(m_localGroup);

    // 模型路径
    QHBoxLayout* modelPathLayout = new QHBoxLayout();
    modelPathLayout->addWidget(new QLabel(tr("模型路径:")));
    m_localModelPathInput = new QLineEdit();
    modelPathLayout->addWidget(m_localModelPathInput);
    m_browseButton = new QPushButton(tr("浏览..."));
    modelPathLayout->addWidget(m_browseButton);
    localLayout->addLayout(modelPathLayout);

    mainLayout->addWidget(m_localGroup);

    // 按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_saveButton = new QPushButton(tr("保存"));
    m_cancelButton = new QPushButton(tr("取消"));
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);

    // 根据当前模型类型显示/隐藏相关设置组
    onModelTypeChanged(m_modelTypeSelector->currentIndex());
    onApiProviderChanged(m_apiProviderSelector->currentIndex());
}

void SettingsDialog::setupConnections()
{
    connect(m_modelTypeSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onModelTypeChanged);
    connect(m_browseButton, &QPushButton::clicked,
            this, &SettingsDialog::onBrowseModelPath);
    connect(m_saveButton, &QPushButton::clicked,
            this, &SettingsDialog::saveSettings);
    connect(m_cancelButton, &QPushButton::clicked,
            this, &QDialog::reject);

    // 连接API提供商变更信号
    connect(m_apiProviderSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onApiProviderChanged);

    // Ollama相关信号
    connect(m_refreshOllamaModelsBtn, &QPushButton::clicked,
            this, &SettingsDialog::refreshOllamaModels);

    connect(m_model, &SettingsModel::ollamaModelsChanged,
            this, &SettingsDialog::updateOllamaModelList);
}

void SettingsDialog::refreshOllamaModels()
{
    LOG_INFO("开始刷新Ollama模型列表");
    m_refreshOllamaModelsBtn->setEnabled(false);
    m_refreshOllamaModelsBtn->setText(tr("正在刷新..."));
    
    // 调用模型刷新函数
    m_model->refreshOllamaModels();
    
    // 添加一个延时器，确保在刷新完成后更新UI
    QTimer::singleShot(1000, this, [this]() {
        // 在延时后强制更新模型列表
        updateOllamaModelList();
    });
}

void SettingsDialog::updateOllamaModelList()
{
    LOG_INFO("正在更新Ollama模型列表UI");
    QString currentModel = m_ollamaModelSelect->currentText();
    m_ollamaModelSelect->clear();

    QStringList models = m_model->ollamaModels();
    LOG_INFO(QString("获取到%1个Ollama模型").arg(models.size()));

    if (models.isEmpty()) {
        m_ollamaModelSelect->addItem(tr("无可用模型"));
        m_ollamaModelSelect->setEnabled(false);
    } else {
        m_ollamaModelSelect->setEnabled(true);
        m_ollamaModelSelect->addItems(models);

        // 恢复之前选择的模型
        int index = m_ollamaModelSelect->findText(currentModel);
        if (index >= 0) {
            m_ollamaModelSelect->setCurrentIndex(index);
            LOG_INFO(QString("恢复选择模型: %1").arg(currentModel));
        } else if (!models.isEmpty()) {
            // 如果找不到之前的模型，但列表不为空，选择第一个
            m_ollamaModelSelect->setCurrentIndex(0);
            LOG_INFO(QString("选择新模型: %1").arg(m_ollamaModelSelect->currentText()));
        }
    }

    // 确保刷新按钮可用
    m_refreshOllamaModelsBtn->setEnabled(true);
    m_refreshOllamaModelsBtn->setText(tr("刷新"));
    
    // 移除这行以避免循环调用
    // emit m_model->ollamaModelsChanged();
}

void SettingsDialog::loadSettings()
{
    LOG_INFO("开始加载设置");
    QSettings settings;

    // 加载通用设置
    m_modelTypeSelector->setCurrentIndex(settings.value("modelType", 0).toInt());
    m_autoSaveCheck->setChecked(settings.value("autoSave", true).toBool());
    m_maxHistorySpin->setValue(settings.value("maxHistory", 100).toInt());
    m_timeoutSpin->setValue(settings.value("timeout", 30).toInt());

    // 加载API设置
    QString apiUrl = settings.value("apiUrl").toString();
    int providerIndex = m_apiProviderSelector->findData(apiUrl);
    if (providerIndex >= 0) {
        m_apiProviderSelector->setCurrentIndex(providerIndex);
    } else {
        // 如果是自定义API地址
        m_apiProviderSelector->setCurrentText("自定义");
        m_apiUrlInput->setText(apiUrl);
    }

    // 获取当前选择的提供商
    QString provider = m_apiProviderSelector->currentText();
    
    // 从SettingsModel获取当前提供商的API Key，而不是从全局设置中获取
    if (m_model) {
        QString apiKey = m_model->getProviderApiKey("api", provider);
        m_apiKeyInput->setText(apiKey);
    } else {
        m_apiKeyInput->setText("");
    }
    
    m_apiModelSelector->setCurrentText(settings.value("apiModel", "gpt-3.5-turbo").toString());

    // 加载Ollama设置
    m_ollamaHostInput->setText(settings.value("ollamaHost", "localhost").toString());
    m_ollamaPortInput->setValue(settings.value("ollamaPort", 11434).toInt());

    // 如果当前选择的是Ollama类型，先刷新模型列表
    if (m_modelTypeSelector->currentData().toInt() == static_cast<int>(SettingsModel::ModelType::Ollama)) {
        refreshOllamaModels();
    }

    // 加载本地模型设置
    m_localModelPathInput->setText(settings.value("localModelPath").toString());

    LOG_INFO("设置加载完成");
}

void SettingsDialog::saveSettings()
{
    LOG_INFO("开始保存设置");
    QSettings settings;

    // 保存通用设置
    settings.setValue("modelType", m_modelTypeSelector->currentIndex());
    settings.setValue("autoSave", m_autoSaveCheck->isChecked());
    settings.setValue("maxHistory", m_maxHistorySpin->value());
    settings.setValue("timeout", m_timeoutSpin->value());

    // 保存API设置
    QString apiUrl = m_apiUrlInput->text();
    QString apiKey = m_apiKeyInput->text();
    QString provider = m_apiProviderSelector->currentText();
    QString modelName = m_apiModelSelector->currentText();

    settings.setValue("apiUrl", apiUrl);
    settings.setValue("apiKey", apiKey);
    settings.setValue("apiModel", modelName);

    // 保存Ollama设置
    settings.setValue("ollamaHost", m_ollamaHostInput->text());
    settings.setValue("ollamaPort", m_ollamaPortInput->value());
    settings.setValue("ollamaModel", m_ollamaModelSelect->currentText());

    // 保存本地模型设置
    settings.setValue("localModelPath", m_localModelPathInput->text());

    // 更新SettingsModel
    if (m_model) {
        SettingsModel::ModelType type = static_cast<SettingsModel::ModelType>(
            m_modelTypeSelector->currentData().toInt());

        m_model->setModelType(type);
        m_model->setApiUrl(apiUrl);

        // 根据不同的模型类型设置相应的配置
        switch (type) {
            case SettingsModel::ModelType::API: {
                // 设置 API Key
                m_model->setProviderApiKey("api", provider, apiKey);
                
                // 设置模型配置
                QJsonObject modelConfig;
                modelConfig["name"] = modelName;
                modelConfig["url"] = apiUrl;
                modelConfig["enabled"] = true;
                m_model->setModelConfig("api", modelName, modelConfig);
                
                // 设置当前模型
                m_model->setCurrentModelName(modelName);
                LOG_INFO(QString("设置 API 模型: %1, 提供商: %2").arg(modelName, provider));
                break;
            }

            case SettingsModel::ModelType::Ollama:
                if (!m_ollamaModelSelect->currentText().isEmpty()) {
                    m_model->setCurrentModelName(m_ollamaModelSelect->currentText());
                    LOG_INFO(QString("设置Ollama模型: %1").arg(m_ollamaModelSelect->currentText()));
                } else {
                    LOG_WARNING("Ollama模型未选择");
                }
                break;

            case SettingsModel::ModelType::Local:
                m_model->setModelPath(m_localModelPathInput->text());
                m_model->setCurrentModelName(QFileInfo(m_localModelPathInput->text()).fileName());
                break;
        }
    }

    LOG_INFO("设置保存完成");
    accept();
}

void SettingsDialog::onModelTypeChanged(int index)
{
    SettingsModel::ModelType type = static_cast<SettingsModel::ModelType>(
        m_modelTypeSelector->itemData(index).toInt());

    m_apiGroup->setVisible(type == SettingsModel::ModelType::API);
    m_ollamaGroup->setVisible(type == SettingsModel::ModelType::Ollama);
    m_localGroup->setVisible(type == SettingsModel::ModelType::Local);
}

void SettingsDialog::onApiProviderChanged(int index)
{
    QString provider = m_apiProviderSelector->currentText();
    QString apiUrl = m_apiProviderSelector->currentData().toString();

    // 更新API模型列表
    updateApiModelList(provider);

    // 显示/隐藏自定义API地址输入
    bool isCustom = (apiUrl == "custom");
    m_apiUrlWidget->setVisible(isCustom);

    // 如果不是自定义,设置默认API地址
    if (!isCustom) {
        m_apiUrlInput->setText(apiUrl);
    }
    
    // 从SettingsModel获取当前提供商的API Key
    if (m_model) {
        QString apiKey = m_model->getProviderApiKey("api", provider);
        m_apiKeyInput->setText(apiKey);
    }

    LOG_INFO(QString("切换API提供商到: %1").arg(provider));
}

void SettingsDialog::onBrowseModelPath()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("选择模型文件"),
        m_localModelPathInput->text(),
        tr("模型文件 (*.onnx *.bin);;所有文件 (*.*)"));

    if (!path.isEmpty()) {
        m_localModelPathInput->setText(path);
    }
}

void SettingsDialog::updateApiModelList(const QString& provider)
{
    LOG_INFO(QString("更新API模型列表，当前选择的提供商: %1").arg(provider));

    m_apiModelSelector->clear();

    if (provider == "OpenAI") {
        m_apiModelSelector->addItems(m_openaiModels);
    } else if (provider == "Deepseek") {
        m_apiModelSelector->addItems(m_deepseekModels);
    } else {
        m_apiModelSelector->addItem(tr("未知提供商或未配置模型"));
    }

    // 设置当前选择的模型
    QString currentModel = m_model->currentModelName();
    int index = m_apiModelSelector->findText(currentModel);
    if (index >= 0) {
        m_apiModelSelector->setCurrentIndex(index);
    } else if (m_apiModelSelector->count() > 0) {
        m_apiModelSelector->setCurrentIndex(0);
    }
}
