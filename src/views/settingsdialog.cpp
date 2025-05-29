#include "settingsdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

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
    m_apiModelSelector->addItems({"gpt-3.5-turbo", "gpt-4", "claude-3-opus", "claude-3-sonnet"});
    apiModelLayout->addWidget(m_apiModelSelector);
    apiLayout->addLayout(apiModelLayout);

    mainLayout->addWidget(m_apiGroup);

    // Ollama设置组
    m_ollamaGroup = new QGroupBox(tr("Ollama设置"), this);
    QVBoxLayout* ollamaLayout = new QVBoxLayout(m_ollamaGroup);
    
    // Ollama模型
    QHBoxLayout* ollamaModelLayout = new QHBoxLayout();
    ollamaModelLayout->addWidget(new QLabel(tr("模型:")));
    m_ollamaModelInput = new QLineEdit();
    ollamaModelLayout->addWidget(m_ollamaModelInput);
    ollamaLayout->addLayout(ollamaModelLayout);

    // Ollama主机
    QHBoxLayout* ollamaHostLayout = new QHBoxLayout();
    ollamaHostLayout->addWidget(new QLabel(tr("主机:")));
    m_ollamaHostInput = new QLineEdit();
    m_ollamaHostInput->setText("localhost");
    ollamaHostLayout->addWidget(m_ollamaHostInput);
    ollamaLayout->addLayout(ollamaHostLayout);

    // Ollama端口
    QHBoxLayout* ollamaPortLayout = new QHBoxLayout();
    ollamaPortLayout->addWidget(new QLabel(tr("端口:")));
    m_ollamaPortInput = new QSpinBox();
    m_ollamaPortInput->setRange(1, 65535);
    m_ollamaPortInput->setValue(11434);
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
}

void SettingsDialog::loadSettings()
{
    QSettings settings;
    
    // 加载通用设置
    m_modelTypeSelector->setCurrentIndex(settings.value("modelType", 0).toInt());
    m_autoSaveCheck->setChecked(settings.value("autoSave", true).toBool());
    m_maxHistorySpin->setValue(settings.value("maxHistory", 100).toInt());
    m_timeoutSpin->setValue(settings.value("timeout", 30).toInt());

    // 加载API设置
    m_apiKeyInput->setText(settings.value("apiKey").toString());
    m_apiModelSelector->setCurrentText(settings.value("apiModel", "gpt-3.5-turbo").toString());

    // 加载Ollama设置
    m_ollamaModelInput->setText(settings.value("ollamaModel", "llama2").toString());
    m_ollamaHostInput->setText(settings.value("ollamaHost", "localhost").toString());
    m_ollamaPortInput->setValue(settings.value("ollamaPort", 11434).toInt());

    // 加载本地模型设置
    m_localModelPathInput->setText(settings.value("localModelPath").toString());
}

void SettingsDialog::saveSettings()
{
    QSettings settings;
    
    // 保存通用设置
    settings.setValue("modelType", m_modelTypeSelector->currentIndex());
    settings.setValue("autoSave", m_autoSaveCheck->isChecked());
    settings.setValue("maxHistory", m_maxHistorySpin->value());
    settings.setValue("timeout", m_timeoutSpin->value());

    // 保存API设置
    settings.setValue("apiKey", m_apiKeyInput->text());
    settings.setValue("apiModel", m_apiModelSelector->currentText());

    // 保存Ollama设置
    settings.setValue("ollamaModel", m_ollamaModelInput->text());
    settings.setValue("ollamaHost", m_ollamaHostInput->text());
    settings.setValue("ollamaPort", m_ollamaPortInput->value());

    // 保存本地模型设置
    settings.setValue("localModelPath", m_localModelPathInput->text());

    // 更新模型
    if (m_model) {
        m_model->setModelType(static_cast<SettingsModel::ModelType>(
            m_modelTypeSelector->currentData().toInt()));
        m_model->setApiKey(m_apiKeyInput->text());
        m_model->setModelPath(m_localModelPathInput->text());
    }

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