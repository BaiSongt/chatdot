#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>
#include "viewmodels/settingsviewmodel.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(SettingsViewModel* viewModel, QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void onModelTypeChanged(int index);
    void onApiProviderChanged(int index);
    void onRefreshStateChanged(bool isRefreshing);
    void onSaveClicked();
    void onCancelClicked();
    void onBrowseLocalModelClicked();
    void updateOllamaModelList();
    void updateApiModelList(const QString& provider);

private:
    void setupUI();
    void setupConnections();
    void updateUIFromViewModel();

    SettingsViewModel* m_viewModel;

    // UI组件
    QComboBox* m_modelTypeSelector;
    QStackedWidget* m_modelSettingsStack;

    // API设置
    QWidget* m_apiSettingsWidget;
    QComboBox* m_apiProviderSelector;
    QLineEdit* m_apiKeyInput;
    QWidget* m_apiUrlWidget;
    QLineEdit* m_apiUrlInput;
    QComboBox* m_apiModelSelector;

    // Ollama设置
    QWidget* m_ollamaSettingsWidget;
    QLineEdit* m_ollamaHostInput;
    QSpinBox* m_ollamaPortInput;
    QComboBox* m_ollamaModelSelector;
    QPushButton* m_refreshOllamaModelsBtn;

    // 本地模型设置
    QWidget* m_localModelSettingsWidget;
    QLineEdit* m_localModelPathInput;
    QPushButton* m_browseLocalModelBtn;

    // 按钮
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

#endif // SETTINGSDIALOG_H
