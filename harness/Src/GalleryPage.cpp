#include "GalleryPage.h"

#include "Icons.h"
#include "Settings.h"
#include "Theme.h"
#include "Widgets.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace {

const int kThumb = 172;
const int kCell = 200;

QStringList imageFilters()
{
    return QStringList() << "*.png" << "*.jpg" << "*.jpeg" << "*.webp"
                         << "*.bmp" << "*.gif" << "*.tif" << "*.tiff";
}

QPixmap thumbFor(const QString &path)
{
    QImageReader reader(path);
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    reader.setAutoTransform(true);
#endif
    const QSize original = reader.size();
    if (original.isValid())
        reader.setScaledSize(original.scaled(kThumb, kThumb, Qt::KeepAspectRatioByExpanding));

    QImage img = reader.read();
    if (img.isNull())
        return QPixmap();

    img = img.scaled(QSize(kThumb, kThumb), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const int x = qMax(0, (img.width() - kThumb) / 2);
    const int y = qMax(0, (img.height() - kThumb) / 2);

    QImage rounded(kThumb, kThumb, QImage::Format_ARGB32_Premultiplied);
    rounded.fill(Qt::transparent);
    {
        QPainter painter(&rounded);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.addRoundedRect(QRectF(0, 0, kThumb, kThumb), 10, 10);
        painter.setClipPath(path);
        painter.drawImage(0, 0, img.copy(x, y, kThumb, kThumb));
    }
    return QPixmap::fromImage(rounded);
}

} // namespace

GalleryPage::GalleryPage(QWidget *parent)
    : QWidget(parent)
    , m_stack(nullptr)
    , m_list(nullptr)
    , m_count(nullptr)
    , m_watcher(nullptr)
    , m_debounce(nullptr)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(22, 16, 22, 16);
    root->setSpacing(14);

    // Header
    QHBoxLayout *header = new QHBoxLayout();
    header->setSpacing(10);

    QVBoxLayout *titles = new QVBoxLayout();
    titles->setSpacing(2);
    QLabel *title = new QLabel("Gallery", this);
    title->setObjectName("PageTitle");
    titles->addWidget(title);
    QLabel *subtitle = new QLabel("Everything in your output directory, newest first.", this);
    subtitle->setObjectName("PageSub");
    titles->addWidget(subtitle);
    header->addLayout(titles);
    header->addStretch(1);

    m_count = new QLabel("0 images", this);
    m_count->setObjectName("Badge");
    header->addWidget(m_count);

    QPushButton *refreshButton = new QPushButton("Refresh", this);
    refreshButton->setObjectName("Ghost");
    refreshButton->setCursor(Qt::PointingHandCursor);
    refreshButton->setIcon(QIcon(Icons::pixmap(Icons::Refresh, 14, Theme::TextDim)));
    connect(refreshButton, &QPushButton::clicked, this, &GalleryPage::refresh);
    header->addWidget(refreshButton);

    QPushButton *folder = new QPushButton("Open folder", this);
    folder->setObjectName("Ghost");
    folder->setCursor(Qt::PointingHandCursor);
    folder->setIcon(QIcon(Icons::pixmap(Icons::Folder, 14, Theme::TextDim)));
    connect(folder, &QPushButton::clicked, this, [this]() {
        emit revealRequested(QString());
    });
    header->addWidget(folder);

    root->addLayout(header);

    // Content
    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack, 1);

    QWidget *empty = new QWidget(m_stack);
    QVBoxLayout *emptyLayout = new QVBoxLayout(empty);
    emptyLayout->addStretch(1);
    QLabel *emptyIcon = new QLabel(empty);
    emptyIcon->setPixmap(Icons::pixmap(Icons::Grid, 42, QColor(0x3A, 0x41, 0x50)));
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyIcon);
    QLabel *emptyTitle = new QLabel("Nothing here yet", empty);
    emptyTitle->setFont(Theme::ui(14, QFont::DemiBold));
    emptyTitle->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyTitle);
    QLabel *emptyText = new QLabel("Generated images are saved to your output directory\n"
                                   "and show up here automatically.",
                                   empty);
    emptyText->setObjectName("Hint");
    emptyText->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyText);
    emptyLayout->addStretch(1);

    m_list = new QListWidget(m_stack);
    m_list->setViewMode(QListView::IconMode);
    m_list->setResizeMode(QListView::Adjust);
    m_list->setMovement(QListView::Static);
    m_list->setUniformItemSizes(true);
    m_list->setIconSize(QSize(kThumb, kThumb));
    m_list->setGridSize(QSize(kCell, kCell));
    m_list->setSpacing(4);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setWordWrap(false);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        openItem(m_list->row(item));
    });
    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        openItem(m_list->row(item));
    });
    connect(m_list, &QListWidget::customContextMenuRequested, this, &GalleryPage::showContextMenu);

    m_stack->addWidget(empty);
    m_stack->addWidget(m_list);

    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &GalleryPage::onDirectoryChanged);

    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(450);
    connect(m_debounce, &QTimer::timeout, this, &GalleryPage::rebuild);

    refresh();
}

void GalleryPage::refresh()
{
    const QString dir = AppSettings::instance().outputDir();
    if (!dir.isEmpty() && QFileInfo(dir).isDir()) {
        const QStringList watched = m_watcher->directories();
        if (!watched.contains(dir)) {
            if (!watched.isEmpty())
                m_watcher->removePaths(watched);
            m_watcher->addPath(dir);
        }
    }
    rebuild();
}

void GalleryPage::onDirectoryChanged()
{
    m_debounce->start();
}

void GalleryPage::rebuild()
{
    const QString dir = AppSettings::instance().outputDir();
    QDir d(dir);
    if (dir.isEmpty() || !d.exists()) {
        m_paths.clear();
        m_list->clear();
        m_count->setText("0 images");
        m_stack->setCurrentIndex(0);
        return;
    }

    const QFileInfoList files = d.entryInfoList(imageFilters(), QDir::Files | QDir::Readable, QDir::Time);
    const int limit = qMin(files.size(), 400);

    m_list->setUpdatesEnabled(false);
    m_list->clear();
    m_paths.clear();

    for (int i = 0; i < limit; ++i) {
        const QFileInfo &fi = files.at(i);
        QListWidgetItem *item = new QListWidgetItem(m_list);
        item->setIcon(QIcon(thumbFor(fi.absoluteFilePath())));
        item->setSizeHint(QSize(kCell, kCell));
        item->setToolTip(fi.fileName());
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        m_paths.append(fi.absoluteFilePath());
    }

    m_list->setUpdatesEnabled(true);

    m_count->setText(QString("%1 image%2").arg(m_paths.size()).arg(m_paths.size() == 1 ? "" : "s"));
    m_stack->setCurrentIndex(m_paths.isEmpty() ? 0 : 1);
}

void GalleryPage::openItem(int row)
{
    if (row < 0 || row >= m_paths.size())
        return;
    emit viewRequested(m_paths, row);
}

void GalleryPage::openCurrent()
{
    openItem(m_list->currentRow());
}

void GalleryPage::removePath(const QString &path)
{
    const int idx = m_paths.indexOf(path);
    if (idx >= 0) {
        m_paths.removeAt(idx);
        QListWidgetItem *item = m_list->takeItem(idx);
        delete item;
        m_count->setText(QString("%1 image%2").arg(m_paths.size()).arg(m_paths.size() == 1 ? "" : "s"));
        if (m_paths.isEmpty())
            m_stack->setCurrentIndex(0);
    }
}

void GalleryPage::showContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_list->itemAt(pos);
    if (!item)
        return;

    const int row = m_list->row(item);
    if (row < 0 || row >= m_paths.size())
        return;

    const QString path = m_paths.at(row);

    QMenu menu(this);
    QAction *open = menu.addAction("Open");
    QAction *reveal = menu.addAction("Show in folder");
    menu.addSeparator();
    QAction *discard = menu.addAction("Discard");

    QAction *chosen = menu.exec(m_list->mapToGlobal(pos));
    if (chosen == open)
        openItem(row);
    else if (chosen == reveal)
        emit revealRequested(path);
    else if (chosen == discard)
        emit discardRequested(path);
}
