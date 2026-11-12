#include "ParamPanel.h"

#include "Icons.h"
#include "Theme.h"
#include "Widgets.h"

#include <QDateTime>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegExpValidator>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

struct SizePreset {
    const char *label;
    int width;
    int height;
};

const SizePreset kPresets[] = {
    {"1024 x 1024  (1:1)", 1024, 1024},
    {"1152 x 896  (4:3)", 1152, 896},
    {"896 x 1152  (3:4)", 896, 1152},
    {"1344 x 768  (16:9)", 1344, 768},
    {"768 x 1344  (9:16)", 768, 1344},
    {"1328 x 1328  (square)", 1328, 1328},
    {"Custom", 0, 0},
};

const int kPresetCount = int(sizeof(kPresets) / sizeof(kPresets[0]));

QString placeholderFor(PageMode mode)
{
    switch (mode) {
    case PageMode::Generate:
        return "Describe the image you want to create...";
    case PageMode::Edit:
        return "Describe the change you want to make...";
    case PageMode::FaceSwap:
        return "Describe the scene and the identity you want in it...";
    case PageMode::MaskInfill:
        return "Describe what should appear inside the painted area...";
    }
    return QString("Describe the image...");
}

QString actionFor(PageMode mode)
{
    switch (mode) {
    case PageMode::Generate: return "Generate";
    case PageMode::Edit: return "Apply edit";
    case PageMode::FaceSwap: return "Swap face";
    case PageMode::MaskInfill: return "Infill mask";
    }
    return "Generate";
}

} // namespace

ParamPanel::ParamPanel(PageMode mode, QWidget *parent)
    : QWidget(parent)
    , m_mode(mode)
    , m_backendReady(false)
    , m_busy(false)
    , m_updatingSize(false)
    , m_prompt(nullptr)
    , m_negative(nullptr)
    , m_sizeCombo(nullptr)
    , m_width(nullptr)
    , m_height(nullptr)
    , m_stepsSlider(nullptr)
    , m_steps(nullptr)
    , m_cfg(nullptr)
    , m_seed(nullptr)
    , m_seedLock(nullptr)
    , m_dice(nullptr)
    , m_batch(nullptr)
    , m_sampler(nullptr)
    , m_submit(nullptr)
    , m_count(nullptr)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(9);

    root->addWidget(Widgets::sectionLabel("PROMPT", this));

    m_prompt = new QPlainTextEdit(this);
    m_prompt->setObjectName("Prompt");
    m_prompt->setFixedHeight(104);
    m_prompt->setPlaceholderText(placeholderFor(mode));
    m_prompt->setTabChangesFocus(true);
    root->addWidget(m_prompt);

    m_negative = new QLineEdit(this);
    m_negative->setPlaceholderText("Negative prompt - what to avoid");
    m_negative->setFixedHeight(34);
    root->addWidget(m_negative);

    root->addWidget(Widgets::separator(this));
    root->addWidget(Widgets::sectionLabel("PARAMETERS", this));

    QGridLayout *grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(9);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    const auto field = [this](const QString &label, QWidget *content) {
        QWidget *box = new QWidget(this);
        QVBoxLayout *v = new QVBoxLayout(box);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(4);
        QLabel *l = new QLabel(label, box);
        l->setObjectName("Hint");
        v->addWidget(l);
        content->setParent(box);
        v->addWidget(content);
        return box;
    };

    // Size row
    QWidget *sizeRow = new QWidget(this);
    QHBoxLayout *sizeLayout = new QHBoxLayout(sizeRow);
    sizeLayout->setContentsMargins(0, 0, 0, 0);
    sizeLayout->setSpacing(6);

    m_sizeCombo = new ThemeCombo(sizeRow);
    for (int i = 0; i < kPresetCount; ++i)
        m_sizeCombo->addItem(QString::fromUtf8(kPresets[i].label));
    sizeLayout->addWidget(m_sizeCombo, 1);

    m_width = new QSpinBox(sizeRow);
    m_width->setRange(256, 2048);
    m_width->setSingleStep(64);
    m_width->setFixedWidth(72);
    m_width->setSuffix("");
    sizeLayout->addWidget(m_width);

    QLabel *times = new QLabel(QString::fromUtf8("\xC3\x97"), sizeRow);
    times->setStyleSheet(QString("color: %1;").arg(Theme::TextFaint.name()));
    sizeLayout->addWidget(times);

    m_height = new QSpinBox(sizeRow);
    m_height->setRange(256, 2048);
    m_height->setSingleStep(64);
    m_height->setFixedWidth(72);
    sizeLayout->addWidget(m_height);

    QWidget *sizeField = field("Size", sizeRow);
    grid->addWidget(sizeField, 0, 0, 1, 2);

    // Steps
    QWidget *stepsRow = new QWidget(this);
    QHBoxLayout *stepsLayout = new QHBoxLayout(stepsRow);
    stepsLayout->setContentsMargins(0, 0, 0, 0);
    stepsLayout->setSpacing(8);

    m_stepsSlider = new QSlider(Qt::Horizontal, stepsRow);
    m_stepsSlider->setRange(4, 80);
    stepsLayout->addWidget(m_stepsSlider, 1);

    m_steps = new QSpinBox(stepsRow);
    m_steps->setRange(4, 80);
    m_steps->setFixedWidth(56);
    stepsLayout->addWidget(m_steps);
    grid->addWidget(field("Steps", stepsRow), 1, 0);

    // CFG
    m_cfg = new QDoubleSpinBox(this);
    m_cfg->setRange(0.0, 25.0);
    m_cfg->setSingleStep(0.5);
    m_cfg->setDecimals(1);
    grid->addWidget(field("Guidance (CFG)", m_cfg), 1, 1);

    // Seed
    QWidget *seedRow = new QWidget(this);
    QHBoxLayout *seedLayout = new QHBoxLayout(seedRow);
    seedLayout->setContentsMargins(0, 0, 0, 0);
    seedLayout->setSpacing(5);

    m_seed = new QLineEdit(seedRow);
    m_seed->setPlaceholderText("Random");
    m_seed->setValidator(new QRegExpValidator(QRegExp("\\d{1,18}"), m_seed));
    seedLayout->addWidget(m_seed, 1);

    m_dice = new QToolButton(seedRow);
    m_dice->setIcon(QIcon(Icons::pixmap(Icons::Dice, 16, Theme::TextDim)));
    m_dice->setIconSize(QSize(16, 16));
    m_dice->setFixedSize(32, 32);
    m_dice->setCursor(Qt::PointingHandCursor);
    m_dice->setToolTip("Random seed");
    seedLayout->addWidget(m_dice);

    m_seedLock = new QToolButton(seedRow);
    m_seedLock->setCheckable(true);
    m_seedLock->setIconSize(QSize(16, 16));
    m_seedLock->setFixedSize(32, 32);
    m_seedLock->setCursor(Qt::PointingHandCursor);
    m_seedLock->setToolTip("Lock the seed so results are reproducible");
    seedLayout->addWidget(m_seedLock);

    grid->addWidget(field("Seed", seedRow), 2, 0);

    // Batch
    m_batch = new QSpinBox(this);
    m_batch->setRange(1, 8);
    m_batch->setSuffix("");
    grid->addWidget(field("Batch", m_batch), 2, 1);

    // Sampler
    m_sampler = new ThemeCombo(this);
    m_sampler->addItems(QStringList() << "Euler" << "Euler a" << "DPM++ 2M"
                                      << "DPM++ SDE" << "UniPC" << "DDIM");
    grid->addWidget(field("Sampler", m_sampler), 3, 0);

    QPushButton *reset = new QPushButton("Restore defaults", this);
    reset->setObjectName("Ghost");
    reset->setCursor(Qt::PointingHandCursor);
    QWidget *resetBox = new QWidget(this);
    QVBoxLayout *resetLayout = new QVBoxLayout(resetBox);
    resetLayout->setContentsMargins(0, 0, 0, 0);
    resetLayout->setSpacing(4);
    QLabel *resetLabel = new QLabel(" ", resetBox);
    resetLabel->setObjectName("Hint");
    resetLayout->addWidget(resetLabel);
    resetLayout->addWidget(reset);
    grid->addWidget(resetBox, 3, 1);

    root->addLayout(grid);

    root->addStretch(1);

    m_submit = new QPushButton(actionFor(mode), this);
    m_submit->setObjectName("Primary");
    m_submit->setCursor(Qt::PointingHandCursor);
    m_submit->setFixedHeight(42);
    m_submit->setIcon(QIcon(Icons::pixmap(Icons::Sparkles, 16, QColor(255, 255, 255))));
    m_submit->setIconSize(QSize(16, 16));
    root->addWidget(m_submit);

    m_count = new QLabel("Ctrl + Enter to run", this);
    m_count->setObjectName("Hint");
    m_count->setAlignment(Qt::AlignRight);
    root->addWidget(m_count);

    connect(m_submit, &QPushButton::clicked, this, &ParamPanel::submitRequested);
    connect(m_sizeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ParamPanel::onSizePresetChanged);
    connect(m_width, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int v) {
        Q_UNUSED(v)
        if (!m_updatingSize) {
            m_updatingSize = true;
            m_sizeCombo->setCurrentIndex(kPresetCount - 1);
            m_updatingSize = false;
        }
    });
    connect(m_height, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int v) {
        Q_UNUSED(v)
        if (!m_updatingSize) {
            m_updatingSize = true;
            m_sizeCombo->setCurrentIndex(kPresetCount - 1);
            m_updatingSize = false;
        }
    });
    connect(m_stepsSlider, &QSlider::valueChanged, this, &ParamPanel::onStepsSliderChanged);
    connect(m_steps, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ParamPanel::onStepsSpinChanged);
    connect(m_seedLock, &QToolButton::toggled, this, &ParamPanel::onSeedLockToggled);
    connect(m_dice, &QToolButton::clicked, this, &ParamPanel::onRandomSeed);
    connect(reset, &QPushButton::clicked, this, &ParamPanel::restoreDefaults);
    connect(m_prompt, &QPlainTextEdit::textChanged, this, &ParamPanel::updateSubmitState);

    QShortcut *run = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return), m_prompt);
    run->setContext(Qt::WindowShortcut);
    connect(run, &QShortcut::activated, this, [this]() {
        if (m_submit->isEnabled())
            emit submitRequested();
    });

    setDefaults(AppSettings::instance().defaults());
    updateSubmitState();
}

void ParamPanel::setMode(PageMode mode)
{
    m_mode = mode;
    m_prompt->setPlaceholderText(placeholderFor(mode));
    restyleSubmit();
    updateSubmitState();
}

void ParamPanel::restyleSubmit()
{
    m_submit->setText(actionFor(m_mode));
}

void ParamPanel::applySize(const QSize &size)
{
    m_updatingSize = true;
    m_width->setValue(qBound(256, size.width(), 2048));
    m_height->setValue(qBound(256, size.height(), 2048));

    int preset = kPresetCount - 1;
    for (int i = 0; i < kPresetCount - 1; ++i) {
        if (kPresets[i].width == m_width->value() && kPresets[i].height == m_height->value()) {
            preset = i;
            break;
        }
    }
    m_sizeCombo->setCurrentIndex(preset);
    m_updatingSize = false;
}

void ParamPanel::setDefaults(const GenerationDefaults &defaults)
{
    applySize(defaults.size);
    m_steps->setValue(qBound(4, defaults.steps, 80));
    m_stepsSlider->setValue(qBound(4, defaults.steps, 80));
    m_cfg->setValue(qBound(0.0, defaults.cfg, 25.0));
    m_batch->setValue(qBound(1, defaults.batch, 8));
    const int samplerIndex = m_sampler->findText(defaults.sampler);
    if (samplerIndex >= 0)
        m_sampler->setCurrentIndex(samplerIndex);
    m_seedLock->setChecked(defaults.fixedSeed);
    m_seed->setText(QString::number(defaults.seed > 0 ? defaults.seed : 0));
    m_seed->setEnabled(defaults.fixedSeed);
    updateSeedIcon();
}

void ParamPanel::restoreDefaults()
{
    setDefaults(AppSettings::instance().defaults());
}

void ParamPanel::onSizePresetChanged(int index)
{
    if (m_updatingSize || index < 0 || index >= kPresetCount - 1)
        return;

    m_updatingSize = true;
    m_width->setValue(kPresets[index].width);
    m_height->setValue(kPresets[index].height);
    m_updatingSize = false;
}

void ParamPanel::onStepsSliderChanged(int value)
{
    if (m_steps->value() == value)
        return;
    const QSignalBlocker blocker(m_steps);
    m_steps->setValue(value);
}

void ParamPanel::onStepsSpinChanged(int value)
{
    if (m_stepsSlider->value() == value)
        return;
    const QSignalBlocker blocker(m_stepsSlider);
    m_stepsSlider->setValue(value);
}

void ParamPanel::onSeedLockToggled(bool on)
{
    m_seed->setEnabled(on);
    updateSeedIcon();
    if (on && m_seed->text().isEmpty())
        onRandomSeed();
}

void ParamPanel::updateSeedIcon()
{
    const bool locked = m_seedLock->isChecked();
    const QColor color = locked ? Theme::accent() : Theme::TextDim;
    m_seedLock->setIcon(QIcon(Icons::pixmap(Icons::Lock, 16, color)));
}

void ParamPanel::onRandomSeed()
{
    const qint64 value = (QDateTime::currentMSecsSinceEpoch() / 3) % 1000000000LL + (qrand() % 1000);
    m_seed->setText(QString::number(value));
    if (!m_seedLock->isChecked())
        m_seedLock->setChecked(true);
}

void ParamPanel::setBackendReady(bool ready)
{
    m_backendReady = ready;
    updateSubmitState();
}

void ParamPanel::setBusy(bool busy)
{
    m_busy = busy;
    updateSubmitState();
}

void ParamPanel::updateSubmitState()
{
    const bool hasPrompt = !m_prompt->toPlainText().trimmed().isEmpty();
    const bool enabled = m_backendReady && !m_busy && hasPrompt;

    m_submit->setEnabled(enabled);
    m_submit->setText(m_busy ? QString("Working...") : actionFor(m_mode));

    if (!m_backendReady)
        m_submit->setToolTip("Start the backend to generate");
    else if (!hasPrompt)
        m_submit->setToolTip("Write a prompt first");
    else
        m_submit->setToolTip("Ctrl + Enter");
}

GenerationRequest ParamPanel::request() const
{
    GenerationRequest req;
    req.mode = m_mode;
    req.prompt = m_prompt->toPlainText().trimmed();
    req.negativePrompt = m_negative->text().trimmed();
    req.size = QSize(m_width->value(), m_height->value());
    req.steps = m_steps->value();
    req.cfg = m_cfg->value();
    req.batch = m_batch->value();
    req.sampler = m_sampler->currentText();
    req.seed = m_seedLock->isChecked() && !m_seed->text().isEmpty()
                   ? m_seed->text().toLongLong()
                   : qint64(-1);
    return req;
}

QString ParamPanel::prompt() const
{
    return m_prompt->toPlainText().trimmed();
}

void ParamPanel::focusPrompt()
{
    m_prompt->setFocus();
}
