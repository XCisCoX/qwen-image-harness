#include "CreatePage.h"

#include "Icons.h"
#include "ImageSlots.h"
#include "Log.h"
#include "MaskCanvas.h"
#include "OutputPane.h"
#include "ParamPanel.h"
#include "Settings.h"
#include "Theme.h"
#include "Widgets.h"

#include <QButtonGroup>
#include <cmath>
#include <QCheckBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

QString modeTitle(PageMode mode)
{
    switch (mode) {
    case PageMode::Generate: return "Generate";
    case PageMode::Edit: return "Edit";
    case PageMode::FaceSwap: return "Face swap";
    case PageMode::MaskInfill: return "Mask infill";
    }
    return "Generate";
}

QString modeSubtitle(PageMode mode)
{
    switch (mode) {
    case PageMode::Generate:
        return "Prompt, parameters and optional references become a new image.";
    case PageMode::Edit:
        return "Keep a primary image, describe the change, let references guide it.";
    case PageMode::FaceSwap:
        return "The primary image holds the scene. Your references supply the identities.";
    case PageMode::MaskInfill:
        return "Paint the region to regenerate. The mask stays apart from your 16 images.";
    }
    return QString();
}

QString primaryLabel(PageMode mode)
{
    switch (mode) {
    case PageMode::Edit: return "Image to edit";
    case PageMode::FaceSwap: return "Target scene";
    default: return "Primary image";
    }
}

QString primaryHint(PageMode mode)
{
    switch (mode) {
    case PageMode::Edit: return "This is what gets reworked";
    case PageMode::FaceSwap: return "Keeps its composition";
    default: return "Optional subject or style anchor";
    }
}

QToolButton *toolIcon(QWidget *parent, Icons::Id id, const QString &tip, bool checkable = false)
{
    QToolButton *b = new QToolButton(parent);
    b->setIcon(QIcon(Icons::pixmap(id, 17, Theme::TextDim)));
    b->setIconSize(QSize(17, 17));
    b->setFixedSize(32, 32);
    b->setCheckable(checkable);
    b->setCursor(Qt::PointingHandCursor);
    b->setToolTip(tip);
    return b;
}

} // namespace

CreatePage::CreatePage(PageMode mode, QWidget *parent)
    : QWidget(parent)
    , m_mode(mode)
    , m_images(nullptr)
    , m_canvas(nullptr)
    , m_params(nullptr)
    , m_output(nullptr)
    , m_countBadge(nullptr)
    , m_scaleLabel(nullptr)
    , m_scaleTarget()
    , m_refsBadge(nullptr)
    , m_brushSlider(nullptr)
    , m_brushLabel(nullptr)
    , m_undoButton(nullptr)
    , m_redoButton(nullptr)
    , m_clearButton(nullptr)
    , m_invertButton(nullptr)
    , m_loadMaskButton(nullptr)
    , m_saveMaskButton(nullptr)
    , m_backendReady(false)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 14, 18, 12);
    root->setSpacing(14);

    root->addWidget(buildHeader());

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(8);

    QWidget *inputColumn = (m_mode == PageMode::MaskInfill) ? buildMaskColumn() : buildInputColumn();
    inputColumn->setMinimumWidth(m_mode == PageMode::MaskInfill ? 360 : 340);
    splitter->addWidget(inputColumn);

    QWidget *centerColumn = buildCenterColumn();
    centerColumn->setMinimumWidth(360);
    splitter->addWidget(centerColumn);

    m_output = new OutputPane(splitter);
    m_output->setMinimumWidth(320);
    splitter->addWidget(m_output);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);

    if (m_mode == PageMode::MaskInfill)
        splitter->setSizes(QList<int>() << 405 << 385 << 355);
    else
        splitter->setSizes(QList<int>() << 365 << 425 << 380);

    root->addWidget(splitter, 1);

    connect(m_params, &ParamPanel::submitRequested, this, &CreatePage::onSubmit);

    connect(m_output, &OutputPane::discardRequested, this, &CreatePage::discardRequested);

    connect(m_output, &OutputPane::viewRequested, this, &CreatePage::viewRequested);
    connect(m_output, &OutputPane::revealRequested, this, &CreatePage::revealRequested);
    connect(m_output, &OutputPane::galleryRequested, this, &CreatePage::galleryRequested);
    connect(m_output, &OutputPane::cancelRequested, this, &CreatePage::cancelRequested);

    updateCounts();
}

QString CreatePage::pageId() const
{
    return pageModeId(m_mode);
}

QWidget *CreatePage::buildHeader()
{
    QWidget *header = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QVBoxLayout *titles = new QVBoxLayout();
    titles->setContentsMargins(0, 0, 0, 0);
    titles->setSpacing(2);

    QLabel *title = new QLabel(modeTitle(m_mode), header);
    title->setObjectName("PageTitle");
    titles->addWidget(title);

    QLabel *subtitle = new QLabel(modeSubtitle(m_mode), header);
    subtitle->setObjectName("PageSub");
    titles->addWidget(subtitle);

    layout->addLayout(titles);
    layout->addStretch(1);

    m_countBadge = new QLabel("0 / 16", header);
    m_countBadge->setObjectName("Badge");
    m_countBadge->setToolTip(m_mode == PageMode::MaskInfill
                                 ? "Images in this job: the masked image plus references"
                                 : "Primary image plus references, 16 in total");
    layout->addWidget(m_countBadge, 0, Qt::AlignVCenter);

    return header;
}

QWidget *CreatePage::buildInputColumn()
{
    QWidget *column = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    Card *card = new Card("Images", column);

    QPushButton *clear = new QPushButton("Clear", card);
    clear->setObjectName("Ghost");
    clear->setCursor(Qt::PointingHandCursor);
    connect(clear, &QPushButton::clicked, this, [this]() {
        m_images->clear();
        updateCounts();
    });
    card->addHeaderWidget(clear);

    m_images = new ImageSlots(card);
    m_images->setPrimaryLabel(primaryLabel(m_mode), primaryHint(m_mode));
    card->body()->addWidget(m_images);

    connect(m_images, &ImageSlots::changed, this, &CreatePage::updateCounts);
    connect(m_images, &ImageSlots::viewRequested, this, [this](const QString &path) {
        emit viewRequested(QStringList() << path, 0);
    });

    layout->addWidget(card);

    QLabel *hint = new QLabel("Drag images anywhere on the grid, or click a tile to browse.\n"
                              "The first image is the primary subject - the remaining slots are references.",
                              column);
    hint->setObjectName("Hint");
    hint->setWordWrap(true);
    layout->addWidget(hint);

    if (m_mode != PageMode::Generate) {
        QWidget *scaleRow = new QWidget(column);
        QHBoxLayout *scaleLayout = new QHBoxLayout(scaleRow);
        scaleLayout->setContentsMargins(0, 0, 0, 0);
        scaleLayout->setSpacing(8);

        m_scaleLabel = new QLabel("Output size: source", scaleRow);
        m_scaleLabel->setObjectName("Hint");

        QPushButton *scaleButton = new QPushButton("Scale...", scaleRow);
        scaleButton->setObjectName("Ghost");
        scaleButton->setCursor(Qt::PointingHandCursor);
        scaleButton->setToolTip("Resize the primary image before the job: smaller saves VRAM, "
                                "larger gives a bigger result.");
        connect(scaleButton, &QPushButton::clicked, this, &CreatePage::promptScale);

        scaleLayout->addWidget(m_scaleLabel);
        scaleLayout->addStretch(1);
        scaleLayout->addWidget(scaleButton);
        layout->addWidget(scaleRow);
    }

    layout->addStretch(1);
    return column;
}

QWidget *CreatePage::buildMaskColumn()
{
    QWidget *column = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    Card *maskCard = new Card("Mask editor", column);

    QPushButton *pick = new QPushButton("Choose image", maskCard);
    pick->setObjectName("Ghost");
    pick->setCursor(Qt::PointingHandCursor);
    pick->setIcon(QIcon(Icons::pixmap(Icons::Folder, 14, Theme::TextDim)));
    connect(pick, &QPushButton::clicked, this, &CreatePage::chooseBaseImage);
    maskCard->addHeaderWidget(pick);

    m_canvas = new MaskCanvas(maskCard);
    m_canvas->setMinimumHeight(160);
    maskCard->body()->addWidget(m_canvas, 1);
    maskCard->body()->addWidget(buildMaskToolbar(maskCard));
    maskCard->setMinimumHeight(330);

    connect(m_canvas, &MaskCanvas::browseRequested, this, &CreatePage::chooseBaseImage);
    connect(m_canvas, &MaskCanvas::baseDropped, this, [this](const QString &path) {
        QString error;
        if (!m_canvas->loadBase(path, &error)) {
            Logging::error(error);
            return;
        }
        Logging::success(QString("Mask image set: %1").arg(QFileInfo(path).fileName()));
        updateCounts();
        updateMaskButtons();
    });
    connect(m_canvas, &MaskCanvas::baseChanged, this, &CreatePage::updateCounts);
    connect(m_canvas, &MaskCanvas::maskChanged, this, &CreatePage::updateMaskButtons);
    connect(m_canvas, &MaskCanvas::historyChanged, this, &CreatePage::updateMaskButtons);

    QShortcut *undoShortcut = new QShortcut(QKeySequence::Undo, m_canvas);
    undoShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(undoShortcut, &QShortcut::activated, this, [this]() { m_canvas->undo(); });

    QShortcut *redoShortcut = new QShortcut(QKeySequence::Redo, m_canvas);
    redoShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(redoShortcut, &QShortcut::activated, this, [this]() { m_canvas->redo(); });

    Card *refsCard = new Card("References", column);
    refsCard->setMinimumHeight(120);
    m_refsBadge = new QLabel("0 / 15", refsCard);
    m_refsBadge->setObjectName("Badge");
    refsCard->addHeaderWidget(m_refsBadge);

    m_images = new ImageSlots(refsCard);
    m_images->setShowPrimary(false);

    QScrollArea *refsScroll = new QScrollArea(refsCard);
    refsScroll->setWidgetResizable(true);
    refsScroll->setFrameShape(QFrame::NoFrame);
    refsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    refsScroll->setWidget(m_images);
    refsCard->body()->addWidget(refsScroll, 1);

    connect(m_images, &ImageSlots::changed, this, &CreatePage::updateCounts);
    connect(m_images, &ImageSlots::viewRequested, this, [this](const QString &path) {
        emit viewRequested(QStringList() << path, 0);
    });

    QSplitter *leftSplit = new QSplitter(Qt::Vertical, column);
    leftSplit->setChildrenCollapsible(false);
    leftSplit->setHandleWidth(8);
    leftSplit->addWidget(maskCard);
    leftSplit->addWidget(refsCard);
    leftSplit->setStretchFactor(0, 1);
    leftSplit->setStretchFactor(1, 0);
    leftSplit->setSizes(QList<int>() << 460 << 180);

    layout->addWidget(leftSplit, 1);

    return column;
}

QWidget *CreatePage::buildMaskToolbar(QWidget *parent)
{
    QWidget *bar = new QWidget(parent);
    QVBoxLayout *outer = new QVBoxLayout(bar);
    outer->setContentsMargins(0, 2, 0, 0);
    outer->setSpacing(6);

    QHBoxLayout *row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(6);

    QToolButton *brush = toolIcon(bar, Icons::Brush, "Brush (B)", true);
    brush->setChecked(true);
    QToolButton *eraser = toolIcon(bar, Icons::Eraser, "Eraser (E)", true);

    QButtonGroup *tools = new QButtonGroup(bar);
    tools->setExclusive(true);
    tools->addButton(brush);
    tools->addButton(eraser);
    row->addWidget(brush);
    row->addWidget(eraser);

    connect(brush, &QToolButton::clicked, this, [this]() { m_canvas->setTool(MaskCanvas::Brush); });
    connect(eraser, &QToolButton::clicked, this, [this]() { m_canvas->setTool(MaskCanvas::Eraser); });

    row->addWidget(Widgets::verticalSeparator(bar));

    QLabel *sizeLabel = new QLabel("Size", bar);
    sizeLabel->setObjectName("Hint");
    row->addWidget(sizeLabel);

    m_brushSlider = new QSlider(Qt::Horizontal, bar);
    m_brushSlider->setRange(4, 160);
    m_brushSlider->setValue(36);
    m_brushSlider->setFixedWidth(76);
    m_brushSlider->setToolTip("Brush size");
    row->addWidget(m_brushSlider);

    m_brushLabel = new QLabel("36", bar);
    m_brushLabel->setObjectName("Hint");
    m_brushLabel->setFixedWidth(22);
    row->addWidget(m_brushLabel);

    connect(m_brushSlider, &QSlider::valueChanged, this, [this](int value) {
        m_canvas->setBrushSize(value);
        m_brushLabel->setText(QString::number(value));
    });

    row->addStretch(1);

    QToolButton *fit = toolIcon(bar, Icons::Fit, "Fit to view");
    connect(fit, &QToolButton::clicked, this, [this]() { m_canvas->fitToView(); });
    row->addWidget(fit);

    outer->addLayout(row);

    QHBoxLayout *second = new QHBoxLayout();
    second->setContentsMargins(0, 0, 0, 0);
    second->setSpacing(6);

    m_undoButton = toolIcon(bar, Icons::Undo, "Undo (Ctrl+Z)");
    m_redoButton = toolIcon(bar, Icons::Redo, "Redo (Ctrl+Y)");
    m_clearButton = toolIcon(bar, Icons::Trash, "Clear mask");
    m_invertButton = toolIcon(bar, Icons::Refresh, "Invert mask");
    second->addWidget(m_undoButton);
    second->addWidget(m_redoButton);
    second->addWidget(m_clearButton);
    second->addWidget(m_invertButton);

    connect(m_undoButton, &QToolButton::clicked, this, [this]() { m_canvas->undo(); });
    connect(m_redoButton, &QToolButton::clicked, this, [this]() { m_canvas->redo(); });
    connect(m_clearButton, &QToolButton::clicked, this, [this]() { m_canvas->clearMask(); });
    connect(m_invertButton, &QToolButton::clicked, this, [this]() { m_canvas->invertMask(); });

    second->addWidget(Widgets::verticalSeparator(bar));

    m_loadMaskButton = toolIcon(bar, Icons::Folder, "Load a mask image");
    connect(m_loadMaskButton, &QToolButton::clicked, this, &CreatePage::chooseMaskFile);
    second->addWidget(m_loadMaskButton);

    m_saveMaskButton = toolIcon(bar, Icons::Save, "Save the current mask");
    connect(m_saveMaskButton, &QToolButton::clicked, this, &CreatePage::saveMaskFile);
    second->addWidget(m_saveMaskButton);

    second->addStretch(1);

    outer->addLayout(second);

    QLabel *hint = new QLabel("White is regenerated. The mask stays separate from the 16 images.", bar);
    hint->setObjectName("Hint");
    hint->setWordWrap(true);
    outer->addWidget(hint);

    updateMaskButtons();
    return bar;
}

QWidget *CreatePage::buildCenterColumn()
{
    QWidget *column = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    Card *card = new Card("Compose", column);

    m_params = new ParamPanel(m_mode, card);
    m_params->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    card->body()->addWidget(m_params, 1);

    layout->addWidget(card, 1);

    m_params->setBackendReady(m_backendReady);
    return column;
}

void CreatePage::onSubmit()
{
    GenerationRequest request = m_params->request();
    QStringList images;

    if (m_mode == PageMode::MaskInfill) {
        if (!m_canvas->hasBase()) {
            Logging::error("Choose a primary image before infilling.");
            return;
        }
        images.append(m_canvas->basePath());
        images.append(m_images->refPaths());

        if (m_canvas->hasMask()) {
            QString dir = AppSettings::instance().maskDir();
            if (dir.isEmpty())
                dir = QDir::tempPath();
            QDir().mkpath(dir);
            const QString maskPath = dir + "/inpaint_mask.png";
            QString error;
            if (!m_canvas->saveMaskFile(maskPath, &error)) {
                Logging::error(error);
                return;
            }
            request.maskPath = maskPath;
        } else {
            Logging::warn("No mask painted - the whole image will be regenerated.");
        }
    } else {
        images = m_images->paths();
        if (images.isEmpty() && m_mode != PageMode::Generate) {
            Logging::error(QString("%1 needs a primary image.").arg(modeTitle(m_mode)));
            return;
        }
    }

    // Optional on-the-fly rescale of the primary image (and the mask that goes with it).
    if (m_scaleTarget.isValid() && m_mode != PageMode::Generate && !images.isEmpty()) {
        const QImage srcImage(images.first());
        if (!srcImage.isNull() && srcImage.size() != m_scaleTarget) {
            QString dir = AppSettings::instance().maskDir();
            if (dir.isEmpty())
                dir = QDir::tempPath();
            QDir().mkpath(dir);

            const QImage scaled = srcImage.convertToFormat(QImage::Format_RGB888)
                                      .scaled(m_scaleTarget, Qt::IgnoreAspectRatio,
                                              Qt::SmoothTransformation);
            const QString scaledPath = dir + "/scaled_primary.png";
            if (scaled.save(scaledPath, "PNG")) {
                images[0] = scaledPath;
                if (m_mode == PageMode::MaskInfill && !request.maskPath.isEmpty()) {
                    QImage maskImage(request.maskPath);
                    if (!maskImage.isNull()) {
                        maskImage.scaled(m_scaleTarget, Qt::IgnoreAspectRatio,
                                         Qt::SmoothTransformation)
                            .save(request.maskPath, "PNG");
                    }
                }
                Logging::info(QString("Resized primary %1x%2 -> %3x%4")
                                  .arg(srcImage.width()).arg(srcImage.height())
                                  .arg(m_scaleTarget.width()).arg(m_scaleTarget.height()));
            } else {
                Logging::warn("Could not write the resized image; using the original.");
            }
        }
    }

    request.images = images;
    emit submitRequested(request, pageId());
}


namespace {
int snap16(double v) { return qMax(256, int(std::lround(v / 16.0) * 16)); }
}

void CreatePage::promptScale()
{
    QString path;
    if (m_mode == PageMode::MaskInfill)
        path = m_canvas ? m_canvas->basePath() : QString();
    else if (m_images)
        path = m_images->paths().value(0);

    if (path.isEmpty()) {
        Logging::warn("Choose a primary image first, then set its output size.");
        return;
    }

    const QImage source(path);
    if (source.isNull()) {
        Logging::error(QString("Cannot read %1").arg(QDir::toNativeSeparators(path)));
        return;
    }
    const QSize src = source.size();

    // Default: keep the aspect ratio and land on ~1.0 MP, which is the model's
    // native budget on this card.
    const double area = 1024.0 * 1024.0;
    const double k = std::sqrt(area / (double(src.width()) * double(src.height())));
    int fitW = snap16(src.width() * k);
    int fitH = snap16(src.height() * k);

    QDialog dialog(this);
    dialog.setWindowTitle("Scale primary image");
    dialog.setMinimumWidth(360);
    QVBoxLayout *v = new QVBoxLayout(&dialog);
    v->setSpacing(9);

    QLabel *info = new QLabel(QString("Source: %1 x %2  (%3 MP)")
                                  .arg(src.width()).arg(src.height())
                                  .arg(QString::number(double(src.width()) * src.height() / 1e6, 'f', 2)),
                              &dialog);
    info->setObjectName("Hint");
    v->addWidget(info);

    QHBoxLayout *row = new QHBoxLayout();
    QSpinBox *wBox = new QSpinBox(&dialog);
    QSpinBox *hBox = new QSpinBox(&dialog);
    for (QSpinBox *box : {wBox, hBox}) {
        box->setRange(256, 2048);
        box->setSingleStep(16);
        box->setSuffix(" px");
    }
    wBox->setValue(m_scaleTarget.isValid() ? m_scaleTarget.width() : fitW);
    hBox->setValue(m_scaleTarget.isValid() ? m_scaleTarget.height() : fitH);
    row->addWidget(new QLabel("Width", &dialog));
    row->addWidget(wBox, 1);
    row->addWidget(new QLabel("Height", &dialog));
    row->addWidget(hBox, 1);
    v->addLayout(row);

    QCheckBox *lock = new QCheckBox("Keep aspect ratio", &dialog);
    lock->setChecked(true);
    v->addWidget(lock);

    QLabel *note = new QLabel(&dialog);
    note->setObjectName("Hint");
    note->setWordWrap(true);
    v->addWidget(note);

    QHBoxLayout *presets = new QHBoxLayout();
    QPushButton *fitButton = new QPushButton(QString("Fit 1.0 MP (%1 x %2)").arg(fitW).arg(fitH), &dialog);
    QPushButton *srcButton = new QPushButton("Use source", &dialog);
    presets->addWidget(fitButton);
    presets->addWidget(srcButton);
    presets->addStretch(1);
    v->addLayout(presets);

    bool syncing = false;
    auto refreshNote = [&]() {
        const double mp = double(wBox->value()) * hBox->value() / 1e6;
        note->setText(mp > 2.4
                          ? QString("%1 MP - above ~2.3 MP this card may run out of VRAM.").arg(mp, 0, 'f', 2)
                          : QString("%1 MP").arg(mp, 0, 'f', 2));
    };
    auto linkFromWidth = [&]() {
        if (syncing || !lock->isChecked())
            return;
        syncing = true;
        hBox->setValue(snap16(double(wBox->value()) * src.height() / src.width()));
        syncing = false;
        refreshNote();
    };
    auto linkFromHeight = [&]() {
        if (syncing || !lock->isChecked())
            return;
        syncing = true;
        wBox->setValue(snap16(double(hBox->value()) * src.width() / src.height()));
        syncing = false;
        refreshNote();
    };

    connect(wBox, QOverload<int>::of(&QSpinBox::valueChanged), this, linkFromWidth);
    connect(hBox, QOverload<int>::of(&QSpinBox::valueChanged), this, linkFromHeight);
    connect(fitButton, &QPushButton::clicked, this, [&]() {
        wBox->setValue(fitW);
        hBox->setValue(fitH);
        refreshNote();
    });
    connect(srcButton, &QPushButton::clicked, this, [&]() {
        wBox->setValue(snap16(src.width()));
        hBox->setValue(snap16(src.height()));
        refreshNote();
    });

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    v->addWidget(buttons);

    refreshNote();
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QSize target(wBox->value(), hBox->value());
    m_scaleTarget = (target == src) ? QSize() : target;
    if (m_scaleLabel) {
        m_scaleLabel->setText(m_scaleTarget.isValid()
                                  ? QString("Output size: %1 x %2").arg(target.width()).arg(target.height())
                                  : QString("Output size: source (%1 x %2)").arg(src.width()).arg(src.height()));
    }
    Logging::info(m_scaleTarget.isValid()
                      ? QString("Primary image will be resized %1x%2 -> %3x%4 before the job.")
                            .arg(src.width()).arg(src.height())
                            .arg(target.width()).arg(target.height())
                      : QString("Primary image stays at %1x%2.").arg(src.width()).arg(src.height()));
}

void CreatePage::setBackendReady(bool ready)
{
    m_backendReady = ready;
    if (m_params)
        m_params->setBackendReady(ready);
}

void CreatePage::applyDefaults(const GenerationDefaults &defaults)
{
    if (m_params)
        m_params->setDefaults(defaults);
}

void CreatePage::setBusy(bool busy)
{
    if (m_params)
        m_params->setBusy(busy);
    if (m_output)
        m_output->setBusy(busy);
}

void CreatePage::showResult(const GenerationResult &result)
{
    if (m_output)
        m_output->showResult(result);
}

void CreatePage::setProgress(int percent, const QString &stage)
{
    if (m_output)
        m_output->setProgress(percent, stage);
}

void CreatePage::clearOutput()
{
    if (m_output)
        m_output->clear();
}

void CreatePage::removePath(const QString &path)
{
    if (m_output)
        m_output->removePath(path);
}

QStringList CreatePage::currentPaths() const
{
    return m_output ? m_output->currentPaths() : QStringList();
}

void CreatePage::focusPrompt()
{
    if (m_params)
        m_params->focusPrompt();
}

void CreatePage::updateCounts()
{
    int total = 0;
    if (m_mode == PageMode::MaskInfill) {
        total = (m_canvas && m_canvas->hasBase() ? 1 : 0) + (m_images ? m_images->refPaths().size() : 0);
        if (m_refsBadge && m_images)
            m_refsBadge->setText(QString("%1 / 15").arg(m_images->refPaths().size()));
    } else {
        total = m_images ? m_images->count() : 0;
    }

    if (m_countBadge)
        m_countBadge->setText(QString("%1 / 16").arg(total));
}

void CreatePage::updateMaskButtons()
{
    if (!m_canvas)
        return;

    m_undoButton->setEnabled(m_canvas->canUndo());
    m_redoButton->setEnabled(m_canvas->canRedo());
    m_clearButton->setEnabled(m_canvas->hasMask());
    m_invertButton->setEnabled(m_canvas->hasBase());
    m_saveMaskButton->setEnabled(m_canvas->hasMask());
    m_loadMaskButton->setEnabled(m_canvas->hasBase());
    updateCounts();
}

void CreatePage::chooseBaseImage()
{
    static QString lastDir;
    const QString path = QFileDialog::getOpenFileName(this, "Choose the image to mask", lastDir,
                                                      "Images (*.png *.jpg *.jpeg *.webp *.bmp)");
    if (path.isEmpty())
        return;

    lastDir = QFileInfo(path).absolutePath();
    QString error;
    if (!m_canvas->loadBase(path, &error)) {
        Logging::error(error);
        return;
    }
    Logging::success(QString("Mask image set: %1").arg(QFileInfo(path).fileName()));
    updateCounts();
    updateMaskButtons();
}

void CreatePage::chooseMaskFile()
{
    const QString dir = AppSettings::instance().maskDir();
    const QString path = QFileDialog::getOpenFileName(this, "Load mask", dir,
                                                      "Mask images (*.png *.jpg *.jpeg)");
    if (path.isEmpty())
        return;

    QString error;
    if (!m_canvas->loadMaskFile(path, &error)) {
        Logging::error(error);
        return;
    }
    Logging::success(QString("Mask loaded: %1").arg(QFileInfo(path).fileName()));
    updateMaskButtons();
}

void CreatePage::saveMaskFile()
{
    QString dir = AppSettings::instance().maskDir();
    if (dir.isEmpty())
        dir = QDir::homePath();
    QDir().mkpath(dir);

    const QString path = QFileDialog::getSaveFileName(this, "Save mask",
                                                      dir + "/mask_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".png",
                                                      "PNG image (*.png)");
    if (path.isEmpty())
        return;

    QString error;
    if (!m_canvas->saveMaskFile(path, &error)) {
        Logging::error(error);
        return;
    }
    Logging::success(QString("Mask saved: %1").arg(QFileInfo(path).fileName()));
}
