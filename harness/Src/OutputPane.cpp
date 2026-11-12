#include "OutputPane.h"

#include "Icons.h"
#include "Settings.h"
#include "Theme.h"
#include "Widgets.h"

#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

QImage readImage(const QString &path)
{
    QImageReader reader(path);
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    reader.setAutoTransform(true);
#endif
    return reader.read();
}

QPixmap thumbnail(const QString &path, int size)
{
    QImageReader reader(path);
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    reader.setAutoTransform(true);
#endif
    QImage img = reader.read();
    if (img.isNull())
        return QPixmap();

    img = img.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const int x = qMax(0, (img.width() - size) / 2);
    const int y = qMax(0, (img.height() - size) / 2);
    return QPixmap::fromImage(img.copy(x, y, size, size));
}

} // namespace

OutputPane::OutputPane(QWidget *parent)
    : QWidget(parent)
    , m_card(nullptr)
    , m_stack(nullptr)
    , m_emptyPage(nullptr)
    , m_busyPage(nullptr)
    , m_resultPage(nullptr)
    , m_preview(nullptr)
    , m_index(0)
    , m_hasResult(false)
    , m_thumbBar(nullptr)
    , m_thumbLayout(nullptr)
    , m_meta(nullptr)
    , m_saved(nullptr)
    , m_busyTitle(nullptr)
    , m_bar(nullptr)
    , m_percent(nullptr)
    , m_stage(nullptr)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_card = new Card("Output", this);

    QPushButton *gallery = new QPushButton("Gallery", m_card);
    gallery->setObjectName("Ghost");
    gallery->setCursor(Qt::PointingHandCursor);
    gallery->setIcon(QIcon(Icons::pixmap(Icons::Grid, 14, Theme::TextDim)));
    connect(gallery, &QPushButton::clicked, this, &OutputPane::galleryRequested);
    m_card->addHeaderWidget(gallery);

    QPushButton *folder = new QPushButton("Open folder", m_card);
    folder->setObjectName("Ghost");
    folder->setCursor(Qt::PointingHandCursor);
    folder->setIcon(QIcon(Icons::pixmap(Icons::Folder, 14, Theme::TextDim)));
    connect(folder, &QPushButton::clicked, this, [this]() {
        if (!m_paths.isEmpty())
            emit revealRequested(m_paths.at(m_index));
        else
            emit revealRequested(QString());
    });
    m_card->addHeaderWidget(folder);

    root->addWidget(m_card);

    m_stack = new QStackedWidget(m_card);
    m_card->body()->addWidget(m_stack, 1);

    // ---------------------------------------------------------------- empty
    m_emptyPage = new QWidget(m_stack);
    QVBoxLayout *emptyLayout = new QVBoxLayout(m_emptyPage);
    emptyLayout->setContentsMargins(12, 12, 12, 12);
    emptyLayout->setSpacing(6);
    emptyLayout->addStretch(1);

    QLabel *emptyIcon = new QLabel(m_emptyPage);
    emptyIcon->setPixmap(Icons::pixmap(Icons::Image, 38, QColor(0x3A, 0x41, 0x50)));
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyIcon);

    QLabel *emptyTitle = new QLabel("No results yet", m_emptyPage);
    emptyTitle->setFont(Theme::ui(13, QFont::DemiBold));
    emptyTitle->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyTitle);

    QLabel *emptyText = new QLabel("Run a job and the image lands here,\nalready saved to your output folder.",
                                   m_emptyPage);
    emptyText->setObjectName("Hint");
    emptyText->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyText);
    emptyLayout->addStretch(1);

    // ----------------------------------------------------------------- busy
    m_busyPage = new QWidget(m_stack);
    QVBoxLayout *busyLayout = new QVBoxLayout(m_busyPage);
    busyLayout->setContentsMargins(14, 14, 14, 14);
    busyLayout->setSpacing(10);
    busyLayout->addStretch(1);

    m_busyTitle = new QLabel("Generating", m_busyPage);
    m_busyTitle->setFont(Theme::ui(14, QFont::DemiBold));
    m_busyTitle->setAlignment(Qt::AlignCenter);
    busyLayout->addWidget(m_busyTitle);

    m_bar = new ProgressBar(m_busyPage);
    m_bar->setFixedHeight(6);
    busyLayout->addWidget(m_bar);

    m_percent = new QLabel("0%", m_busyPage);
    m_percent->setFont(Theme::mono(11));
    m_percent->setAlignment(Qt::AlignCenter);
    m_percent->setStyleSheet(QString("color: %1;").arg(Theme::TextDim.name()));
    busyLayout->addWidget(m_percent);

    m_stage = new QLabel("Queued", m_busyPage);
    m_stage->setObjectName("Hint");
    m_stage->setAlignment(Qt::AlignCenter);
    busyLayout->addWidget(m_stage);

    QPushButton *cancel = new QPushButton("Cancel job", m_busyPage);
    cancel->setObjectName("Ghost");
    cancel->setCursor(Qt::PointingHandCursor);
    connect(cancel, &QPushButton::clicked, this, &OutputPane::cancelRequested);
    busyLayout->addWidget(cancel, 0, Qt::AlignCenter);

    busyLayout->addStretch(1);

    // --------------------------------------------------------------- result
    m_resultPage = new QWidget(m_stack);
    QVBoxLayout *resultLayout = new QVBoxLayout(m_resultPage);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    resultLayout->setSpacing(8);

    m_preview = new QLabel(m_resultPage);
    m_preview->setObjectName("Preview");
    m_preview->setMinimumHeight(170);
    m_preview->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setCursor(Qt::PointingHandCursor);
    m_preview->setToolTip("Click to open the viewer");
    m_preview->installEventFilter(this);
    resultLayout->addWidget(m_preview, 1);

    m_thumbBar = new QWidget(m_resultPage);
    m_thumbLayout = new QHBoxLayout(m_thumbBar);
    m_thumbLayout->setContentsMargins(0, 0, 0, 0);
    m_thumbLayout->setSpacing(6);
    m_thumbLayout->addStretch(1);
    resultLayout->addWidget(m_thumbBar);

    m_meta = new ElidedLabel(m_resultPage);
    m_meta->setTextColor(Theme::TextFaint);
    resultLayout->addWidget(m_meta);

    m_saved = new ElidedLabel(m_resultPage);
    m_saved->setTextColor(Theme::TextFaint);
    resultLayout->addWidget(m_saved);

    QHBoxLayout *actions = new QHBoxLayout();
    actions->setSpacing(8);

    QPushButton *discard = new QPushButton("Discard", m_resultPage);
    discard->setObjectName("Danger");
    discard->setCursor(Qt::PointingHandCursor);
    discard->setToolTip("Permanently delete this image from the output folder");
    discard->setIcon(QIcon(Icons::pixmap(Icons::Trash, 14, Theme::Err)));
    connect(discard, &QPushButton::clicked, this, [this]() {
        if (m_index >= 0 && m_index < m_paths.size())
            emit discardRequested(m_paths.at(m_index));
    });
    actions->addWidget(discard);

    QPushButton *reveal = new QPushButton("Show in folder", m_resultPage);
    reveal->setObjectName("Ghost");
    reveal->setCursor(Qt::PointingHandCursor);
    connect(reveal, &QPushButton::clicked, this, [this]() {
        if (m_index >= 0 && m_index < m_paths.size())
            emit revealRequested(m_paths.at(m_index));
    });
    actions->addWidget(reveal);

    actions->addStretch(1);
    resultLayout->addLayout(actions);

    m_stack->addWidget(m_emptyPage);
    m_stack->addWidget(m_busyPage);
    m_stack->addWidget(m_resultPage);
    m_stack->setCurrentWidget(m_emptyPage);

    QString dir = AppSettings::instance().outputDir();
    m_saved->setText(QString("Saved to %1").arg(QDir::toNativeSeparators(dir)));
}

void OutputPane::showEmpty()
{
    m_hasResult = false;
    m_stack->setCurrentWidget(m_emptyPage);
}

void OutputPane::clear()
{
    m_paths.clear();
    m_index = 0;
    m_image = QImage();
    m_preview->clear();
    rebuildThumbs();
    showEmpty();
}

void OutputPane::setBusy(bool busy)
{
    if (busy) {
        m_stack->setCurrentWidget(m_busyPage);
        m_bar->setValue(0);
        m_percent->setText("0%");
        m_stage->setText("Queued");
    } else if (m_hasResult) {
        m_stack->setCurrentWidget(m_resultPage);
    } else {
        m_stack->setCurrentWidget(m_emptyPage);
    }
}

void OutputPane::setProgress(int percent, const QString &stage)
{
    m_bar->setValue(percent);
    m_percent->setText(QString("%1%").arg(percent));
    if (!stage.isEmpty())
        m_stage->setText(stage);
}

void OutputPane::showResult(const GenerationResult &result)
{
    if (result.images.isEmpty()) {
        showEmpty();
        return;
    }

    m_paths.clear();
    for (int i = 0; i < result.images.size(); ++i)
        m_paths.append(result.images.at(i).path);

    m_hasResult = true;
    m_index = 0;
    m_image = readImage(m_paths.first());
    rebuildThumbs();
    selectIndex(0);
    m_stack->setCurrentWidget(m_resultPage);
}

void OutputPane::removePath(const QString &path)
{
    const int idx = m_paths.indexOf(path);
    if (idx < 0)
        return;

    m_paths.removeAt(idx);
    if (m_paths.isEmpty()) {
        clear();
        return;
    }

    if (m_thumbButtons.size() > idx) {
        QPushButton *b = m_thumbButtons.takeAt(idx);
        m_thumbLayout->removeWidget(b);
        b->deleteLater();
    }

    m_index = qBound(0, idx, m_paths.size() - 1);
    m_image = readImage(m_paths.at(m_index));
    selectIndex(m_index);
}

void OutputPane::rebuildThumbs()
{
    for (int i = 0; i < m_thumbButtons.size(); ++i) {
        m_thumbLayout->removeWidget(m_thumbButtons.at(i));
        m_thumbButtons.at(i)->deleteLater();
    }
    m_thumbButtons.clear();

    if (m_paths.size() > 1) {
        m_thumbBar->setVisible(true);
        for (int i = 0; i < m_paths.size(); ++i) {
            QPushButton *b = new QPushButton(m_thumbBar);
            b->setCheckable(true);
            b->setAutoExclusive(true);
            b->setFixedSize(56, 56);
            b->setCursor(Qt::PointingHandCursor);
            b->setIconSize(QSize(52, 52));
            b->setIcon(QIcon(thumbnail(m_paths.at(i), 52)));
            b->setToolTip(QFileInfo(m_paths.at(i)).fileName());
            connect(b, &QPushButton::clicked, this, [this, i]() { selectIndex(i); });
            m_thumbLayout->insertWidget(i, b);
            m_thumbButtons.append(b);
        }
    } else {
        m_thumbBar->setVisible(false);
    }

    m_thumbBar->updateGeometry();
}

void OutputPane::selectIndex(int index)
{
    if (m_paths.isEmpty())
        return;
    m_index = qBound(0, index, m_paths.size() - 1);
    m_image = readImage(m_paths.at(m_index));
    refreshPreview();
    updateMeta();
    for (int i = 0; i < m_thumbButtons.size(); ++i)
        m_thumbButtons.at(i)->setChecked(i == m_index);
}

void OutputPane::updateMeta()
{
    if (m_paths.isEmpty())
        return;

    const QString name = QFileInfo(m_paths.at(m_index)).fileName();
    QString text;
    if (m_paths.size() > 1)
        text += QString("%1 of %2   -   ").arg(m_index + 1).arg(m_paths.size());
    if (!m_image.isNull())
        text += QString("%1 x %2   -   ").arg(m_image.width()).arg(m_image.height());
    text += name;
    m_meta->setText(text);

    m_saved->setText(QString("Saved to %1")
                         .arg(QDir::toNativeSeparators(AppSettings::instance().outputDir())));
}

void OutputPane::refreshPreview()
{
    if (m_image.isNull() || !m_preview) {
        if (m_preview)
            m_preview->clear();
        return;
    }

    const QSize available(qMax(20, m_preview->contentsRect().width() - 4),
                          qMax(20, m_preview->contentsRect().height() - 4));
    QPixmap pm = QPixmap::fromImage(m_image);
    if (!pm.isNull()) {
        pm = pm.scaled(available, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_preview->setPixmap(pm);
    }
}

void OutputPane::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    refreshPreview();
}

bool OutputPane::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_preview) {
        if (event->type() == QEvent::Resize) {
            refreshPreview();
            return false;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton && !m_paths.isEmpty()) {
                emit viewRequested(m_paths, m_index);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}
