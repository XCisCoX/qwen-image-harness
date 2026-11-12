#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>

class Card;
class QButtonGroup;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class ThemeCombo;
class ToggleSwitch;

/*! Directory configuration, data controls, server options and defaults. */
class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

    void reload();

signals:
    void saved();
    void outputCleared();

private slots:
    void save();
    void revert();
    void markDirty();
    void clearOutput();
    void resetAll();

private:
    void loadFromSettings();
    void applyToSettings();

    QWidget *buildDirectoriesCard();
    QWidget *buildDataCard();
    QWidget *buildServerCard();
    QWidget *buildDefaultsCard();
    QWidget *buildAppearanceCard();
    QWidget *buildAboutCard();

    QWidget *dirRow(const QString &label, QLineEdit *edit, const QString &hint);
    QWidget *switchRow(const QString &title, const QString &desc, ToggleSwitch *toggle,
                       QWidget *extra = nullptr);

    QLineEdit *m_outputDir;
    QLineEdit *m_logDir;
    QLineEdit *m_maskDir;

    ToggleSwitch *m_autoSave;
    ToggleSwitch *m_sidecar;
    ToggleSwitch *m_retention;
    QSpinBox *m_retentionDays;

    QLineEdit *m_command;
    QLineEdit *m_arguments;
    QLineEdit *m_workdir;
    QSpinBox *m_port;
    ToggleSwitch *m_autoStart;
    ToggleSwitch *m_stopOnExit;

    QSpinBox *m_width;
    QSpinBox *m_height;
    QSpinBox *m_steps;
    QDoubleSpinBox *m_cfg;
    QSpinBox *m_batch;
    ThemeCombo *m_sampler;

    QButtonGroup *m_accents;

    QLabel *m_dirtyLabel;
    QPushButton *m_saveButton;
    QPushButton *m_revertButton;
    bool m_loading;
};

#endif // SETTINGSPAGE_H
