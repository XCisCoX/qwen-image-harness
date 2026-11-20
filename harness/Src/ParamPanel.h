#ifndef PARAMPANEL_H
#define PARAMPANEL_H

#include "Backend.h"
#include "Settings.h"

#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSlider;
class QSpinBox;
class QToolButton;
class ThemeCombo;

/*! Prompt, negative prompt, sampler parameters and the submit button. */
class QCheckBox;
class QComboBox;

class ParamPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ParamPanel(PageMode mode, QWidget *parent = nullptr);

    void setMode(PageMode mode);
    void setDefaults(const GenerationDefaults &defaults);
    void setBackendReady(bool ready);
    void setBusy(bool busy);

    GenerationRequest request() const;
    QString prompt() const;
    void focusPrompt();

signals:
    void submitRequested();
    void queueRequested();

private slots:
    void onSizePresetChanged(int index);
    void onStepsSliderChanged(int value);
    void onStepsSpinChanged(int value);
    void onSeedLockToggled(bool on);
    void onRandomSeed();
    void updateSubmitState();
    void restoreDefaults();

private:
    void applySize(const QSize &size);
    void restyleSubmit();
    void updateSeedIcon();
    PageMode m_mode;
    bool m_backendReady;
    bool m_busy;
    bool m_updatingSize;

    QPlainTextEdit *m_prompt;
    QLineEdit *m_negative;
    ThemeCombo *m_sizeCombo;
    QSpinBox *m_width;
    QSpinBox *m_height;
    QSlider *m_stepsSlider;
    QSpinBox *m_steps;
    QDoubleSpinBox *m_cfg;
    QLineEdit *m_seed;
    QToolButton *m_seedLock;
    QToolButton *m_dice;
    QSpinBox *m_batch;
    ThemeCombo *m_sampler;
    QPushButton *m_submit;
    QPushButton *m_queue;
    QCheckBox *m_eraseBox;
    QComboBox *m_contextBox;
    QLabel *m_count;
};

#endif // PARAMPANEL_H
