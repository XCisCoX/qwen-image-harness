#ifndef WIDGETS_H
#define WIDGETS_H

#include "Icons.h"
#include "Theme.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QWidget>

class QVBoxLayout;
class QHBoxLayout;
class QToolButton;
class QPropertyAnimation;

/*! Rounded panel with an optional header row. */
class Card : public QFrame
{
    Q_OBJECT

public:
    explicit Card(const QString &title = QString(), QWidget *parent = nullptr);

    QVBoxLayout *body() const { return m_body; }
    QHBoxLayout *header() const { return m_header; }
    void addHeaderWidget(QWidget *w);
    void setTitle(const QString &text);
    void setBodyMargins(int l, int t, int r, int b);

private:
    QLabel *m_title;
    QVBoxLayout *m_body;
    QHBoxLayout *m_header;
};

/*! Animated pill switch used for every boolean setting. */
class ToggleSwitch : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(qreal position READ position WRITE setPosition)

public:
    explicit ToggleSwitch(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

    qreal position() const { return m_pos; }
    void setPosition(qreal p);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void animateToggled(bool on);

private:
    qreal m_pos;
    QPropertyAnimation *m_anim;
};

/*! Thin flat progress bar with an indeterminate mode. */
class ProgressBar : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressBar(QWidget *parent = nullptr);

    void setValue(int value);
    int value() const { return m_value; }
    void setIndeterminate(bool on);
    void setBarColor(const QColor &c) { m_color = c; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void tick();

private:
    int m_value;
    bool m_indeterminate;
    QColor m_color;
    int m_phase;
};

/*! QComboBox that keeps the styled frame but paints its own chevron. */
class ThemeCombo : public QComboBox
{
    Q_OBJECT

public:
    explicit ThemeCombo(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

/*! Small status lamp with a soft halo. */
class StatusDot : public QWidget
{
    Q_OBJECT

public:
    explicit StatusDot(QWidget *parent = nullptr);

    void setColor(const QColor &c);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_color;
};

/*! Single line label that elides with an ellipsis instead of clipping. */
class ElidedLabel : public QWidget
{
    Q_OBJECT

public:
    explicit ElidedLabel(QWidget *parent = nullptr);

    void setText(const QString &text);
    void setTextColor(const QColor &c);
    void setFont(const QFont &f);
    QString text() const { return m_text; }

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    QString m_text;
    QColor m_color;
    QFont m_font;
};

namespace Widgets {
QToolButton *iconButton(Icons::Id id, const QString &tooltip, QWidget *parent, int size = 18);
QLabel *sectionLabel(const QString &text, QWidget *parent);
QLabel *hintLabel(const QString &text, QWidget *parent);
QFrame *separator(QWidget *parent);
QFrame *verticalSeparator(QWidget *parent);
} // namespace Widgets

#endif // WIDGETS_H
