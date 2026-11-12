#ifndef TERMINALPAGE_H
#define TERMINALPAGE_H

#include "Log.h"

#include <QWidget>

class Card;
class ElidedLabel;
class QLabel;
class QPlainTextEdit;
class ThemeCombo;
class ToggleSwitch;

/*! Full application log with level filtering and autoscroll. */
class TerminalPage : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalPage(QWidget *parent = nullptr);

private slots:
    void onAppended(const LogEntry &entry);
    void onCleared();
    void rebuild();
    void copyAll();

private:
    bool passes(LogLevel level) const;
    void appendEntry(const LogEntry &entry);

    QPlainTextEdit *m_view;
    ThemeCombo *m_filter;
    ToggleSwitch *m_autoScroll;
    ElidedLabel *m_path;
    QLabel *m_count;
    int m_shown;
    bool m_rebuilding;
};

#endif // TERMINALPAGE_H
