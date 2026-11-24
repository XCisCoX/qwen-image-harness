#include "MaskFullView.h"

#include "Icons.h"
#include "MaskCanvas.h"
#include "Theme.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QSizePolicy>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

MaskFullView::MaskFullView(QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog)
    , m_canvas(nullptr)
    , m_toolbar(nullptr)
{
    setObjectName("MaskFullView");
    setModal(false);
    setStyleSheet(QString("QDialog#MaskFullView { background: %1; }")
                      .arg(Theme::Bg0.name()));

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(16, 12, 16, 12);
    m_layout->setSpacing(10);

    m_header = new QWidget(this);
    m_header->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    QHBoxLayout *header = new QHBoxLayout(m_header);
    header->setContentsMargins(0, 0, 0, 0);
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(10);

    QLabel *title = new QLabel("Mask editor", m_header);
    title->setFont(Theme::ui(14, QFont::DemiBold));
    header->addWidget(title);

    m_hint = new QLabel("Paint the area to regenerate - Esc returns to the page", m_header);
    m_hint->setObjectName("Hint");
    header->addWidget(m_hint);
    header->addStretch(1);

    QToolButton *close = new QToolButton(m_header);
    close->setObjectName("Ghost");
    close->setCursor(Qt::PointingHandCursor);
    close->setIcon(QIcon(Icons::pixmap(Icons::Close, 16, Theme::Text)));
    close->setIconSize(QSize(16, 16));
    close->setText("Close");
    close->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    close->setToolTip("Back to the page (Esc)");
    connect(close, &QToolButton::clicked, this, &MaskFullView::close);
    header->addWidget(close);

    m_layout->addWidget(m_header, 0);
}

void MaskFullView::showEditor(MaskCanvas *canvas, QWidget *toolbar, const QRect &geometry)
{
    m_canvas = canvas;
    m_toolbar = toolbar;

    if (m_canvas) {
        m_canvas->setParent(this);
        m_canvas->setMinimumHeight(200);
        // The canvas owns the free space; without an Expanding policy the toolbar's
        // internal layout claims it and the editor collapses to a sliver.
        m_canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_layout->addWidget(m_canvas, 1);
        // The page hides the canvas while it is in full view (the placeholder takes its
        // slot), and a reparented widget keeps that explicit hide state.
        m_canvas->show();
    }
    if (m_toolbar) {
        m_toolbar->setParent(this);
        m_toolbar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        m_layout->addWidget(m_toolbar, 0);
        m_toolbar->show();
    }

    setGeometry(geometry);
    show();
    raise();
    activateWindow();
    if (m_canvas) {
        m_canvas->setFocus();
        // fitToView() reads the widget size, which is still the old card size until the
        // dialog has been laid out - defer one event loop turn.
        QPointer<MaskCanvas> canvas = m_canvas;
        QTimer::singleShot(0, this, [canvas]() {
            if (canvas)
                canvas->fitToView();
        });
    }
}

bool MaskFullView::releaseEditor()
{
    if (!m_canvas)
        return false;

    // The page re-inserts both widgets into its own layout, which reparents them.
    m_layout->removeWidget(m_canvas);
    if (m_toolbar)
        m_layout->removeWidget(m_toolbar);
    m_canvas->setParent(nullptr);
    if (m_toolbar)
        m_toolbar->setParent(nullptr);
    m_canvas = nullptr;
    m_toolbar = nullptr;
    hide();
    return true;
}

QWidget *MaskFullView::hostedCanvas() const
{
    return m_canvas;
}

void MaskFullView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QDialog::keyPressEvent(event);
}

void MaskFullView::closeEvent(QCloseEvent *event)
{
    emit closing();
    QDialog::closeEvent(event);
}
