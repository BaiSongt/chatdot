#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
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
    void onApiProviderChanged(int index);
    void onBrowseModelPath();
    void refreshOllamaModels();
    void updateOllamaModelList();


private:
    void setupUI();
    void setupConnections();
    void updateApiModelList(const QString& provider);


    SettingsModel* m_model;

    // 通用设置组件
    QGroupBox* m_generalGroup;
    QComboBox* m_modelTypeSelector;
    QCheckBox* m_autoSaveCheck;
    QSpinBox* m_maxHistorySpin;
    QSpinBox* m_timeoutSpin;

    // API设置组件
    QGroupBox* m_apiGroup;
    QComboBox* m_apiProviderSelector;
    QWidget* m_apiUrlWidget;  // 容器用于控制可见性
    QLineEdit* m_apiUrlInput;
    QLineEdit* m_apiKeyInput;
    QComboBox* m_apiModelSelector;
    QStringList m_openaiModels;
    QStringList m_deepseekModels;

    // Ollama设置组件
    QGroupBox* m_ollamaGroup;
    QComboBox* m_ollamaModelSelect;
    QPushButton* m_refreshOllamaModelsBtn;
    QLineEdit* m_ollamaHostInput;
    QSpinBox* m_ollamaPortInput;

    // 本地模型设置组件
    QGroupBox* m_localGroup;
    QLineEdit* m_localModelPathInput;
    QPushButton* m_browseButton;

    // 对话框按钮
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

#endif // SETTINGSDIALOG_H
