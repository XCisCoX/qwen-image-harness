#include "Widgets.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

// ---------------------------------------------------------------- Card

Card::Card(const QString &title, QWidget *parent)
    : QFrame(parent)
    , m_title(nullptr)
    , m_header(nullptr)
{
    setObjectName("Card");

    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(14, 12, 14, 14);
    outer->setSpacing(10);

    if (!title.isEmpty()) {
        m_header = new QHBoxLayout();
        m_header->setSpacing(8);
        m_header->setContentsMargins(0, 0, 0, 0);
        m_title = new QLabel(title, this);
        m_title->setObjectName("CardTitle");
        m_header->addWidget(m_title);
        m_header->addStretch(1);
        outer->addLayout(m_header);
    }

    m_body = new QVBoxLayout();
    m_body->setSpacing(10);
    m_body->setContentsMargins(0, 0, 0, 0);
    outer->addLayout(m_body, 1);
}

void Card::addHeaderWidget(QWidget *w)
{
    if (!m_header) {
        m_header = new QHBoxLayout();
        m_header->setSpacing(8);
        m_header->setContentsMargins(0, 0, 0, 0);
        m_header->addStretch(1);
        qobject_cast<QVBoxLayout *>(layout())->insertLayout(0, m_header);
    }
    m_header->addWidget(w);
}

void Card::setTitle(const QString &text)
{
    if (!m_title)
        return;
    m_title->setText(text);
    m_title->setVisible(!text.isEmpty());
}

void Card::setBodyMargins(int l, int t, int r, int b)
{
    m_body->setContentsMargins(l, t, r, b);
}

// ------------------------------------------------------------ ToggleSwitch

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QAbstractButton(parent)
    , m_pos(0.0)
    , m_anim(nullptr)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);
    connect(this, &QAbstractButton::toggled, this, &ToggleSwitch::animateToggled);
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(38, 22);
}

void ToggleSwitch::setPosition(qreal p)
{
    m_pos = qBound(0.0, p, 1.0);
    update();
}

void ToggleSwitch::animateToggled(bool on)
{
    if (!m_anim) {
        m_anim = new QPropertyAnimation(this, "position", this);
        m_anim->setDuration(140);
        m_anim->setEasingCurve(QEasingCurve::OutCubic);
    }
    m_anim->stop();
    m_anim->setStartValue(m_pos);
    m_anim->setEndValue(on ? 1.0 : 0.0);
    m_anim->start();
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->pos()))
        QAbstractButton::mouseReleaseEvent(event);
}

void ToggleSwitch::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal h = height();
    const qreal w = width();
    const qreal radius = h / 2.0;

    const QColor offTrack = Theme::Bg4;
    const QColor onTrack = Theme::accent();
    QColor track = offTrack;
    if (m_pos > 0.0) {
        track = QColor(
            offTrack.red() + int((onTrack.red() - offTrack.red()) * m_pos),
            offTrack.green() + int((onTrack.green() - offTrack.green()) * m_pos),
            offTrack.blue() + int((onTrack.blue() - offTrack.blue()) * m_pos));
    }
    if (!isEnabled())
        track.setAlpha(90);

    p.setPen(Qt::NoPen);
    p.setBrush(track);
    p.drawRoundedRect(QRectF(0, 0, w, h), radius, radius);

    if (m_pos < 1.0) {
        QColor border = Theme::Line2;
        border.setAlpha(int(180 * (1.0 - m_pos)));
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(border, 1.0));
        p.drawRoundedRect(QRectF(0.5, 0.5, w - 1.0, h - 1.0), radius - 0.5, radius - 0.5);
    }

    const qreal d = h - 6.0;
    const qreal x = 3.0 + m_pos * (w - 6.0 - d);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 45));
    p.drawEllipse(QRectF(x, 4.0, d, d));

    p.setBrush(isEnabled() ? QColor(0xFF, 0xFF, 0xFF) : QColor(0xD0, 0xD3, 0xDA));
    p.drawEllipse(QRectF(x, 3.0, d, d));
}

// ------------------------------------------------------------- ProgressBar

ProgressBar::ProgressBar(QWidget *parent)
    : QWidget(parent)
    , m_value(0)
    , m_indeterminate(false)
    , m_color(Theme::accent())
    , m_phase(0)
{
    setFixedHeight(6);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void ProgressBar::setValue(int value)
{
    const int v = qBound(0, value, 100);
    if (v == m_value)
        return;
    m_value = v;
    update();
}

void ProgressBar::setIndeterminate(bool on)
{
    if (m_indeterminate == on)
        return;
    m_indeterminate = on;
    if (on) {
        QTimer *t = new QTimer(this);
        t->setObjectName("indeterminateTimer");
        t->setInterval(16);
        connect(t, &QTimer::timeout, this, &ProgressBar::tick);
        t->start();
    } else {
        QTimer *t = findChild<QTimer *>("indeterminateTimer");
        if (t) {
            t->stop();
            t->deleteLater();
        }
        m_phase = 0;
    }
    update();
}

void ProgressBar::tick()
{
    m_phase = (m_phase + 6) % 1000;
    update();
}

void ProgressBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal h = height();
    const qreal r = h / 2.0;

    p.setPen(Qt::NoPen);
    p.setBrush(Theme::Bg4);
    p.drawRoundedRect(QRectF(0, 0, width(), h), r, r);

    if (m_indeterminate) {
        const qreal seg = qMax<qreal>(40.0, width() * 0.28);
        const qreal travel = width() + seg;
        qreal x = (m_phase / 1000.0) * travel - seg;
        QColor c = m_color;
        c.setAlpha(220);
        p.setBrush(c);
        p.drawRoundedRect(QRectF(x, 0, seg, h), r, r);
        return;
    }

    if (m_value <= 0)
        return;

    const qreal w = width() * (m_value / 100.0);
    QColor c = m_color;
    c.setAlpha(230);
    p.setBrush(c);
    p.drawRoundedRect(QRectF(0, 0, qMax(w, h), h), r, r);
}

// -------------------------------------------------------------- ThemeCombo

ThemeCombo::ThemeCombo(QWidget *parent)
    : QComboBox(parent)
{
    setCursor(Qt::PointingHandCursor);
}

void ThemeCombo::paintEvent(QPaintEvent *event)
{
    QComboBox::paintEvent(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(isEnabled() ? Theme::TextDim : Theme::TextFaint, 1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);

    const qreal cx = width() - 16.0;
    const qreal cy = height() / 2.0;
    p.drawPolyline(QPolygonF() << QPointF(cx - 4.0, cy - 2.0)
                               << QPointF(cx, cy + 2.4)
                               << QPointF(cx + 4.0, cy - 2.0));
}

// --------------------------------------------------------------- StatusDot

StatusDot::StatusDot(QWidget *parent)
    : QWidget(parent)
    , m_color(Theme::TextFaint)
{
    setFixedSize(14, 14);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

void StatusDot::setColor(const QColor &c)
{
    if (m_color == c)
        return;
    m_color = c;
    update();
}

void StatusDot::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QColor halo = m_color;
    halo.setAlpha(55);
    p.setPen(Qt::NoPen);
    p.setBrush(halo);
    p.drawEllipse(QPointF(width() / 2.0, height() / 2.0), 6.0, 6.0);

    p.setBrush(m_color);
    p.drawEllipse(QPointF(width() / 2.0, height() / 2.0), 3.4, 3.4);
}

// ------------------------------------------------------------- ElidedLabel

ElidedLabel::ElidedLabel(QWidget *parent)
    : QWidget(parent)
    , m_color(Theme::TextDim)
    , m_font(Theme::ui(11))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(16);
}

void ElidedLabel::setText(const QString &text)
{
    if (m_text == text)
        return;
    m_text = text;
    update();
}

void ElidedLabel::setTextColor(const QColor &c)
{
    m_color = c;
    update();
}

void ElidedLabel::setFont(const QFont &f)
{
    m_font = f;
    update();
}

QSize ElidedLabel::sizeHint() const
{
    const QFontMetrics fm(m_font);
    return QSize(fm.width(m_text), fm.height());
}

void ElidedLabel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setFont(m_font);
    p.setPen(m_color);
    const QFontMetrics fm(m_font);
    const QString t = fm.elidedText(m_text, Qt::ElideRight, width());
    p.drawText(rect(), Qt::AlignLeft | Qt::AlignVCenter, t);
}

// ---------------------------------------------------------------- helpers

namespace Widgets {

QToolButton *iconButton(Icons::Id id, const QString &tooltip, QWidget *parent, int size)
{
    QToolButton *b = new QToolButton(parent);
    b->setIcon(QIcon(Icons::pixmap(id, size, Theme::TextDim)));
    b->setIconSize(QSize(size, size));
    b->setFixedSize(size + 14, size + 14);
    b->setCursor(Qt::PointingHandCursor);
    b->setToolTip(tooltip);
    return b;
}

QLabel *sectionLabel(const QString &text, QWidget *parent)
{
    QLabel *l = new QLabel(text, parent);
    l->setObjectName("SectionLabel");
    QFont f = l->font();
    f.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    f.setWeight(QFont::Bold);
    l->setFont(f);
    return l;
}

QLabel *hintLabel(const QString &text, QWidget *parent)
{
    QLabel *l = new QLabel(text, parent);
    l->setObjectName("Hint");
    l->setWordWrap(true);
    return l;
}

QFrame *separator(QWidget *parent)
{
    QFrame *f = new QFrame(parent);
    f->setObjectName("Separator");
    f->setFrameShape(QFrame::NoFrame);
    f->setFixedHeight(1);
    return f;
}

QFrame *verticalSeparator(QWidget *parent)
{
    QFrame *f = new QFrame(parent);
    f->setObjectName("VLine");
    f->setFrameShape(QFrame::NoFrame);
    f->setFixedWidth(1);
    return f;
}

} // namespace Widgets
