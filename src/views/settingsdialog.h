#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include "models/settingsmodel.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(SettingsModel* model, QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void saveSettings();
    void loadSettings();
    void onModelTypeChanged(int index);
    void onBrowseModelPath();
    void refreshOllamaModels();
    void updateOllamaModelList();


private:
    void setupUI();
    void setupConnections();


    SettingsModel* m_model;

    // API设置组
    QGroupBox* m_apiGroup;
    QLineEdit* m_apiKeyInput;
    QComboBox* m_apiModelSelector;    // Ollama设置组
    QGroupBox* m_ollamaGroup;
    QComboBox* m_ollamaModelSelect;  // 改用ComboBox来选择模型
    QPushButton* m_refreshOllamaModelsBtn;  // 刷新模型列表按钮
    QLineEdit* m_ollamaHostInput;
    QSpinBox* m_ollamaPortInput;

    // 本地模型设置组
    QGroupBox* m_localGroup;
    QLineEdit* m_localModelPathInput;
    QPushButton* m_browseButton;

    // 通用设置组
    QGroupBox* m_generalGroup;
    QComboBox* m_modelTypeSelector;
    QCheckBox* m_autoSaveCheck;
    QSpinBox* m_maxHistorySpin;
    QSpinBox* m_timeoutSpin;

    // 按钮
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

#endif // SETTINGSDIALOG_H
