#include "Shell.h"

#include "Icons.h"
#include "Theme.h"

#include <QButtonGroup>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#include <QWindow>
#endif

namespace {

void repolish(QWidget *w)
{
    if (!w || !w->style())
        return;
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

class LogoMark : public QWidget
{
public:
    explicit LogoMark(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(30, 30);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QRectF r(0.5, 0.5, width() - 1.0, height() - 1.0);
        QLinearGradient g(r.topLeft(), r.bottomRight());
        g.setColorAt(0.0, Theme::accent().lighter(128));
        g.setColorAt(1.0, Theme::accent().darker(135));
        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawRoundedRect(r, 8.5, 8.5);

        const QPixmap mark = Icons::pixmap(Icons::Sparkles, 18, QColor(255, 255, 255));
        p.drawPixmap((width() - 18) / 2, (height() - 18) / 2, mark);
    }
};

} // namespace

// ------------------------------------------------------------------ TitleBar

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , m_dragging(false)
    , m_systemMove(false)
    , m_maxButton(nullptr)
{
    setObjectName("TitleBar");
    setFixedHeight(54);
    setMouseTracking(true);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 0, 10, 0);
    layout->setSpacing(10);

    layout->addWidget(new LogoMark(this), 0, Qt::AlignVCenter);

    QVBoxLayout *texts = new QVBoxLayout();
    texts->setContentsMargins(0, 0, 0, 0);
    texts->setSpacing(0);

    QLabel *title = new QLabel("Qwen Image Studio", this);
    title->setFont(Theme::ui(13, QFont::Bold));
    texts->addWidget(title);

    QLabel *subtitle = new QLabel("Qwen-Image 2.1 frontend", this);
    subtitle->setObjectName("SubtitleLabel");
    subtitle->setFont(Theme::ui(10));
    subtitle->setStyleSheet(QString("color: %1;").arg(Theme::TextFaint.name()));
    texts->addWidget(subtitle);

    layout->addLayout(texts);
    layout->addStretch(1);

    const auto makeButton = [this](Icons::Id id, const char *objectName, const QString &tooltip) {
        QPushButton *b = new QPushButton(this);
        b->setObjectName(objectName);
        b->setFixedSize(34, 28);
        b->setCursor(Qt::ArrowCursor);
        b->setFocusPolicy(Qt::NoFocus);
        b->setIcon(QIcon(Icons::pixmap(id, 15, Theme::TextDim)));
        b->setIconSize(QSize(15, 15));
        b->setToolTip(tooltip);
        return b;
    };

    QPushButton *minButton = makeButton(Icons::Minimize, "WindowButton", "Minimize");
    m_maxButton = makeButton(Icons::Maximize, "WindowButton", "Maximize");
    QPushButton *closeButton = makeButton(Icons::Close, "WindowButtonClose", "Close");

    layout->addWidget(minButton);
    layout->addWidget(m_maxButton);
    layout->addWidget(closeButton);

    connect(minButton, &QPushButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maxButton, &QPushButton::clicked, this, &TitleBar::maximizeRequested);
    connect(closeButton, &QPushButton::clicked, this, &TitleBar::closeRequested);
}

void TitleBar::setMaximized(bool on)
{
    if (!m_maxButton)
        return;
    m_maxButton->setIcon(QIcon(Icons::pixmap(on ? Icons::Restore : Icons::Maximize, 15, Theme::TextDim)));
    m_maxButton->setToolTip(on ? "Restore" : "Maximize");
}

void TitleBar::setSubtitle(const QString &text)
{
    QList<QLabel *> labels = findChildren<QLabel *>();
    for (int i = 0; i < labels.size(); ++i) {
        if (labels.at(i)->objectName() == "SubtitleLabel")
            labels.at(i)->setText(text);
    }
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressGlobal = event->globalPos();
        m_windowOrigin = window()->pos();
        m_dragging = true;
        m_systemMove = false;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (QWindow *handle = window()->windowHandle()) {
            handle->startSystemMove();
            m_dragging = false;
            m_systemMove = true;
        }
#endif
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        const QPoint delta = event->globalPos() - m_pressGlobal;
        window()->move(m_windowOrigin + delta);
    }
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    m_systemMove = false;
    QWidget::mouseReleaseEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit maximizeRequested();
    QWidget::mouseDoubleClickEvent(event);
}

// ------------------------------------------------------------------- Sidebar

Sidebar::Sidebar(QWidget *parent)
    : QWidget(parent)
    , m_group(nullptr)
    , m_dot(nullptr)
    , m_state(nullptr)
    , m_port(nullptr)
    , m_serverButton(nullptr)
{
    setObjectName("Sidebar");
    setFixedWidth(200);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 12);
    layout->setSpacing(2);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    addSection("CREATE");
    addNav("generate", Icons::Sparkles, "Generate");
    addNav("edit", Icons::Pen, "Edit");
    addNav("faceswap", Icons::Swap, "Face swap");
    addNav("mask", Icons::Mask, "Mask infill");

    addSection("LIBRARY");
    addNav("gallery", Icons::Grid, "Gallery");

    addSection("SYSTEM");
    addNav("terminal", Icons::Terminal, "Terminal");
    addNav("settings", Icons::Gear, "Settings");

    layout->addStretch(1);

    QFrame *footer = new QFrame(this);
    footer->setObjectName("CardInner");
    QVBoxLayout *footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(10, 10, 10, 10);
    footerLayout->setSpacing(6);

    QLabel *caption = new QLabel("BACKEND", footer);
    caption->setObjectName("SectionLabel");
    QFont captionFont = caption->font();
    captionFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    caption->setFont(captionFont);
    footerLayout->addWidget(caption);

    QHBoxLayout *stateRow = new QHBoxLayout();
    stateRow->setSpacing(7);
    m_dot = new StatusDot(footer);
    stateRow->addWidget(m_dot);
    m_state = new QLabel("Stopped", footer);
    m_state->setFont(Theme::ui(12, QFont::DemiBold));
    stateRow->addWidget(m_state);
    stateRow->addStretch(1);
    footerLayout->addLayout(stateRow);

    m_port = new QLabel("127.0.0.1:8000", footer);
    m_port->setFont(Theme::mono(10));
    m_port->setStyleSheet(QString("color: %1;").arg(Theme::TextFaint.name()));
    footerLayout->addWidget(m_port);

    m_serverButton = new QPushButton("Start server", footer);
    m_serverButton->setObjectName("Primary");
    m_serverButton->setCursor(Qt::PointingHandCursor);
    m_serverButton->setFixedHeight(32);
    m_serverButton->setIcon(QIcon(Icons::pixmap(Icons::Play, 13, QColor(255, 255, 255))));
    m_serverButton->setIconSize(QSize(13, 13));
    connect(m_serverButton, &QPushButton::clicked, this, &Sidebar::toggleServerRequested);
    footerLayout->addWidget(m_serverButton);

    layout->addWidget(footer);
}

void Sidebar::addSection(const QString &title)
{
    QLabel *label = new QLabel(title, this);
    label->setObjectName("SectionLabel");
    QFont f = label->font();
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    f.setWeight(QFont::Bold);
    label->setFont(f);
    label->setContentsMargins(10, 14, 0, 4);
    qobject_cast<QVBoxLayout *>(layout())->addWidget(label);
}

void Sidebar::addNav(const QString &pageId, Icons::Id icon, const QString &label)
{
    QPushButton *b = new QPushButton(QString("  ") + label, this);
    b->setObjectName("NavItem");
    b->setCheckable(true);
    b->setCursor(Qt::PointingHandCursor);
    b->setFixedHeight(34);
    b->setIconSize(QSize(17, 17));
    b->setIcon(Icons::stateIcon(icon, Theme::TextDim, Theme::accent(), 17));
    b->setFocusPolicy(Qt::NoFocus);

    m_group->addButton(b);
    m_iconIds.insert(pageId, int(icon));
    m_buttons.insert(pageId, b);
    qobject_cast<QVBoxLayout *>(layout())->addWidget(b);

    connect(b, &QPushButton::clicked, this, [this, pageId]() {
        emit pageRequested(pageId);
    });
}

void Sidebar::setActive(const QString &pageId)
{
    QPushButton *b = m_buttons.value(pageId, nullptr);
    if (b && !b->isChecked())
        b->setChecked(true);
}

void Sidebar::refreshIcons()
{
    QHash<QString, QPushButton *>::const_iterator it = m_buttons.constBegin();
    for (; it != m_buttons.constEnd(); ++it) {
        const int id = m_iconIds.value(it.key(), int(Icons::Sparkles));
        it.value()->setIcon(Icons::stateIcon(Icons::Id(id), Theme::TextDim, Theme::accent(), 17));
    }
}

void Sidebar::setBackendHint(const QString &text)
{
    m_port->setText(text);
}

void Sidebar::setBackendState(int state, int port)
{
    m_port->setText(QString("127.0.0.1:%1").arg(port));

    QColor color = Theme::TextFaint;
    QString text = "Stopped";
    QString action = "Start server";
    Icons::Id icon = Icons::Play;
    bool enabled = true;

    switch (state) {
    case 1:
        color = Theme::Warn;
        text = "Starting";
        action = "Starting";
        icon = Icons::Play;
        enabled = false;
        break;
    case 2:
        color = Theme::Ok;
        text = "Running";
        action = "Stop server";
        icon = Icons::Stop;
        break;
    case 3:
        color = Theme::Warn;
        text = "Stopping";
        action = "Stopping";
        icon = Icons::Stop;
        enabled = false;
        break;
    case 4:
        color = Theme::Err;
        text = "Failed";
        action = "Retry";
        icon = Icons::Play;
        break;
    default:
        break;
    }

    m_dot->setColor(color);
    m_state->setText(text);
    m_serverButton->setText(action);
    m_serverButton->setEnabled(enabled);

    const bool running = (state == 2);
    const QColor iconColor = running ? QColor(255, 255, 255) : QColor(255, 255, 255);
    m_serverButton->setIcon(QIcon(Icons::pixmap(icon, 13, iconColor)));

    const QString wanted = running ? "Danger" : "Primary";
    if (m_serverButton->objectName() != wanted) {
        m_serverButton->setObjectName(wanted);
        repolish(m_serverButton);
    }
}

// ---------------------------------------------------------------- StatusStrip

StatusStrip::StatusStrip(QWidget *parent)
    : QWidget(parent)
    , m_dot(nullptr)
    , m_line(nullptr)
    , m_stage(nullptr)
    , m_progress(nullptr)
    , m_cancel(nullptr)
{
    setObjectName("StatusStrip");
    setFixedHeight(34);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 0, 10, 0);
    layout->setSpacing(9);

    m_dot = new StatusDot(this);
    layout->addWidget(m_dot);

    m_line = new ElidedLabel(this);
    m_line->setText("Ready");
    m_line->setTextColor(Theme::TextFaint);
    layout->addWidget(m_line, 1);

    m_stage = new QLabel(this);
    m_stage->setFont(Theme::ui(11));
    m_stage->setStyleSheet(QString("color: %1;").arg(Theme::TextDim.name()));
    layout->addWidget(m_stage);

    m_progress = new ProgressBar(this);
    m_progress->setFixedWidth(150);
    m_progress->setVisible(false);
    layout->addWidget(m_progress);

    m_cancel = new QPushButton("Cancel", this);
    m_cancel->setObjectName("Ghost");
    m_cancel->setCursor(Qt::PointingHandCursor);
    m_cancel->setFixedHeight(24);
    m_cancel->setVisible(false);
    connect(m_cancel, &QPushButton::clicked, this, &StatusStrip::cancelRequested);
    layout->addWidget(m_cancel);

    connect(&Log::instance(), &Log::appended, this, &StatusStrip::setLastEntry);
}

void StatusStrip::setLastEntry(const LogEntry &entry)
{
    QColor color = Theme::TextDim;
    switch (entry.level) {
    case LogLevel::Success: color = Theme::Ok; break;
    case LogLevel::Warn: color = Theme::Warn; break;
    case LogLevel::Error: color = Theme::Err; break;
    case LogLevel::Backend: color = Theme::Info; break;
    case LogLevel::Debug: color = Theme::TextFaint; break;
    default: color = Theme::TextDim; break;
    }

    m_dot->setColor(color);
    m_line->setText(entry.text);
    m_line->setTextColor(color);
}

void StatusStrip::setServerColor(const QColor &color)
{
    m_dot->setColor(color);
}

void StatusStrip::setBusy(bool busy)
{
    m_progress->setVisible(busy);
    m_cancel->setVisible(busy);
    if (busy) {
        m_progress->setValue(0);
    } else {
        m_stage->setText(QString());
    }
}

void StatusStrip::setProgress(int percent, const QString &stage)
{
    m_progress->setVisible(true);
    m_cancel->setVisible(true);
    m_progress->setValue(percent);
    m_stage->setText(stage);
}

// ---------------------------------------------------------------------- Toast

Toast::Toast(QWidget *parent)
    : QWidget(parent)
    , m_kind(0)
    , m_fade(0.0)
    , m_hold(nullptr)
    , m_anim(nullptr)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setVisible(false);

    m_hold = new QTimer(this);
    m_hold->setSingleShot(true);
    m_hold->setInterval(2200);
    connect(m_hold, &QTimer::timeout, this, [this]() {
        if (!m_anim)
            return;
        m_anim->stop();
        m_anim->setStartValue(m_fade);
        m_anim->setEndValue(0.0);
        m_anim->start();
    });

    m_anim = new QPropertyAnimation(this, "fade", this);
    m_anim->setDuration(160);
    connect(m_anim, &QPropertyAnimation::finished, this, [this]() {
        if (m_fade <= 0.01)
            setVisible(false);
    });
}

void Toast::setFade(qreal value)
{
    m_fade = value;
    update();
}

void Toast::showMessage(const QString &text, int kind)
{
    m_text = text;
    m_kind = kind;

    const QFontMetrics fm(Theme::ui(12, QFont::DemiBold));
    const int maxWidth = 560;
    const int textWidth = qMin(maxWidth, fm.width(text));
    const int width = textWidth + 54;
    const int height = 38;

    QWidget *host = parentWidget();
    const int x = host ? (host->width() - width) / 2 : 0;
    const int y = host ? host->height() - height - 34 - 14 : 0;
    setGeometry(x, y, width, height);

    setVisible(true);
    raise();

    m_hold->stop();
    m_hold->start();

    if (m_anim) {
        m_anim->stop();
        m_anim->setDuration(140);
        m_anim->setStartValue(m_fade);
        m_anim->setEndValue(1.0);
        m_anim->start();
    } else {
        setFade(1.0);
    }
}

void Toast::paintEvent(QPaintEvent *)
{
    if (m_fade <= 0.0)
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setOpacity(m_fade);

    const QRectF r(0.5, 0.5, width() - 1.0, height() - 1.0);
    QColor bg = Theme::Bg3;
    bg.setAlpha(245);
    p.setPen(QPen(Theme::Line2, 1.0));
    p.setBrush(bg);
    p.drawRoundedRect(r, 9.0, 9.0);

    QColor accent = Theme::accent();
    Icons::Id icon = Icons::Info;
    if (m_kind == 1) {
        accent = Theme::Ok;
        icon = Icons::Check;
    } else if (m_kind == 2) {
        accent = Theme::Err;
        icon = Icons::Alert;
    }

    const int iconSize = 16;
    p.drawPixmap(14, (height() - iconSize) / 2, Icons::pixmap(icon, iconSize, accent));

    p.setPen(Theme::Text);
    p.setFont(Theme::ui(12, QFont::DemiBold));
    p.drawText(QRect(40, 0, width() - 52, height()), Qt::AlignLeft | Qt::AlignVCenter,
               QFontMetrics(p.font()).elidedText(m_text, Qt::ElideRight, width() - 52));
}
