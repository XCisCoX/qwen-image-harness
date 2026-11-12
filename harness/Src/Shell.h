#ifndef SHELL_H
#define SHELL_H

#include "Log.h"
#include "Widgets.h"

#include <QHash>
#include <QWidget>

class QButtonGroup;
class QLabel;
class QPushButton;
class QPropertyAnimation;
class QTimer;

/*! Frameless window title bar: brand mark, drag handle and window controls. */
class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);

    void setMaximized(bool on);
    void setSubtitle(const QString &text);

signals:
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QPoint m_pressGlobal;
    QPoint m_windowOrigin;
    bool m_dragging;
    bool m_systemMove;
    QPushButton *m_maxButton;
};

/*! Left navigation rail with the backend power switch pinned to the bottom. */
class Sidebar : public QWidget
{
    Q_OBJECT

public:
    explicit Sidebar(QWidget *parent = nullptr);

    void setActive(const QString &pageId);
    void setBackendState(int state, int port);
    void setBackendHint(const QString &text);
    void refreshIcons();

signals:
    void pageRequested(const QString &pageId);
    void toggleServerRequested();

private:
    void addSection(const QString &title);
    void addNav(const QString &pageId, Icons::Id icon, const QString &label);

    QHash<QString, QPushButton *> m_buttons;
    QHash<QString, int> m_iconIds;
    QButtonGroup *m_group;
    StatusDot *m_dot;
    QLabel *m_state;
    QLabel *m_port;
    QPushButton *m_serverButton;
};

/*! Bottom strip: newest log line, job stage, progress and cancel. */
class StatusStrip : public QWidget
{
    Q_OBJECT

public:
    explicit StatusStrip(QWidget *parent = nullptr);

    void setLastEntry(const LogEntry &entry);
    void setBusy(bool busy);
    void setProgress(int percent, const QString &stage);
    void setServerColor(const QColor &color);

signals:
    void cancelRequested();

private:
    StatusDot *m_dot;
    ElidedLabel *m_line;
    QLabel *m_stage;
    ProgressBar *m_progress;
    QPushButton *m_cancel;
};

/*! Transient feedback bubble floating above the status strip. */
class Toast : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal fade READ fade WRITE setFade)

public:
    explicit Toast(QWidget *parent);

    void showMessage(const QString &text, int kind = 0);

    qreal fade() const { return m_fade; }
    void setFade(qreal value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_text;
    int m_kind;
    qreal m_fade;
    QTimer *m_hold;
    QPropertyAnimation *m_anim;
};

#endif // SHELL_H
