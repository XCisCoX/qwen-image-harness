#include "TerminalPage.h"

#include "Icons.h"
#include "Settings.h"
#include "Theme.h"
#include "Widgets.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTextCursor>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QColor colorFor(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug: return Theme::TextFaint;
    case LogLevel::Info: return Theme::TextDim;
    case LogLevel::Success: return Theme::Ok;
    case LogLevel::Warn: return Theme::Warn;
    case LogLevel::Error: return Theme::Err;
    case LogLevel::Backend: return Theme::Info;
    }
    return Theme::TextDim;
}

} // namespace

TerminalPage::TerminalPage(QWidget *parent)
    : QWidget(parent)
    , m_view(nullptr)
    , m_filter(nullptr)
    , m_autoScroll(nullptr)
    , m_path(nullptr)
    , m_count(nullptr)
    , m_shown(0)
    , m_rebuilding(false)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(22, 16, 22, 16);
    root->setSpacing(14);

    QHBoxLayout *header = new QHBoxLayout();
    header->setSpacing(10);

    QVBoxLayout *titles = new QVBoxLayout();
    titles->setSpacing(2);
    QLabel *title = new QLabel("Terminal", this);
    title->setObjectName("PageTitle");
    titles->addWidget(title);
    QLabel *subtitle = new QLabel("Everything the app and the backend report, live.", this);
    subtitle->setObjectName("PageSub");
    titles->addWidget(subtitle);
    header->addLayout(titles);
    header->addStretch(1);

    m_count = new QLabel("0 lines", this);
    m_count->setObjectName("Badge");
    header->addWidget(m_count);

    QPushButton *copy = new QPushButton("Copy", this);
    copy->setObjectName("Ghost");
    copy->setCursor(Qt::PointingHandCursor);
    copy->setIcon(QIcon(Icons::pixmap(Icons::Copy, 14, Theme::TextDim)));
    connect(copy, &QPushButton::clicked, this, &TerminalPage::copyAll);
    header->addWidget(copy);

    QPushButton *clear = new QPushButton("Clear", this);
    clear->setObjectName("Ghost");
    clear->setCursor(Qt::PointingHandCursor);
    clear->setIcon(QIcon(Icons::pixmap(Icons::Trash, 14, Theme::TextDim)));
    connect(clear, &QPushButton::clicked, this, [this]() { Log::instance().clear(); });
    header->addWidget(clear);

    QPushButton *openLog = new QPushButton("Open log", this);
    openLog->setObjectName("Ghost");
    openLog->setCursor(Qt::PointingHandCursor);
    openLog->setIcon(QIcon(Icons::pixmap(Icons::External, 14, Theme::TextDim)));
    connect(openLog, &QPushButton::clicked, this, [this]() {
        const QString path = Log::instance().logFile();
        if (!path.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    header->addWidget(openLog);

    root->addLayout(header);

    Card *card = new Card("Live log", this);

    QLabel *showLabel = new QLabel("Show", card);
    showLabel->setObjectName("Hint");
    card->addHeaderWidget(showLabel);

    m_filter = new ThemeCombo(card);
    m_filter->addItems(QStringList() << "All" << "No debug" << "Warnings + errors"
                                     << "Errors only" << "Backend only");
    m_filter->setFixedWidth(150);
    connect(m_filter, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &TerminalPage::rebuild);
    card->addHeaderWidget(m_filter);

    QLabel *scrollLabel = new QLabel("Auto-scroll", card);
    scrollLabel->setObjectName("Hint");
    card->addHeaderWidget(scrollLabel);

    m_autoScroll = new ToggleSwitch(card);
    m_autoScroll->setChecked(true);
    card->addHeaderWidget(m_autoScroll);

    m_view = new QPlainTextEdit(card);
    m_view->setObjectName("TerminalView");
    m_view->setReadOnly(true);
    m_view->setFont(Theme::mono(11));
    m_view->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_view->setMaximumBlockCount(6000);
    m_view->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    card->body()->addWidget(m_view, 1);

    root->addWidget(card, 1);

    QHBoxLayout *footer = new QHBoxLayout();
    footer->setSpacing(8);
    m_path = new ElidedLabel(this);
    m_path->setTextColor(Theme::TextFaint);
    m_path->setText(QString());
    footer->addWidget(m_path, 1);

    QPushButton *openFolder = new QPushButton("Open log folder", this);
    openFolder->setObjectName("Ghost");
    openFolder->setCursor(Qt::PointingHandCursor);
    connect(openFolder, &QPushButton::clicked, this, [this]() {
        const QString path = Log::instance().logFile();
        if (!path.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
    });
    footer->addWidget(openFolder);

    root->addLayout(footer);

    connect(&Log::instance(), &Log::appended, this, &TerminalPage::onAppended);
    connect(&Log::instance(), &Log::cleared, this, &TerminalPage::onCleared);

    rebuild();
}

bool TerminalPage::passes(LogLevel level) const
{
    switch (m_filter->currentIndex()) {
    case 0: return true;
    case 1: return level != LogLevel::Debug;
    case 2: return level == LogLevel::Warn || level == LogLevel::Error;
    case 3: return level == LogLevel::Error;
    case 4: return level == LogLevel::Backend;
    default: return true;
    }
}

void TerminalPage::appendEntry(const LogEntry &entry)
{
    QTextCursor cursor(m_view->document());
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat timeFormat;
    timeFormat.setForeground(Theme::TextFaint);
    timeFormat.setFont(Theme::mono(11));
    cursor.insertText(entry.time.toString("HH:mm:ss") + "   ", timeFormat);

    QTextCharFormat tagFormat;
    tagFormat.setForeground(colorFor(entry.level));
    tagFormat.setFont(Theme::mono(11));
    tagFormat.setFontWeight(QFont::Bold);
    QString tag = Log::levelTag(entry.level);
    cursor.insertText(tag.leftJustified(5, ' ') + "  ", tagFormat);

    QTextCharFormat textFormat;
    QColor textColor = colorFor(entry.level);
    if (entry.level == LogLevel::Info)
        textColor = Theme::Text;
    textFormat.setForeground(textColor);
    textFormat.setFont(Theme::mono(11));
    cursor.insertText(entry.text + "\n", textFormat);

    ++m_shown;

    if (!m_rebuilding && m_autoScroll->isChecked()) {
        QScrollBar *bar = m_view->verticalScrollBar();
        bar->setValue(bar->maximum());
    }
}

void TerminalPage::onAppended(const LogEntry &entry)
{
    if (!passes(entry.level))
        return;
    appendEntry(entry);
    m_count->setText(QString("%1 line%2").arg(m_shown).arg(m_shown == 1 ? "" : "s"));
}

void TerminalPage::onCleared()
{
    rebuild();
}

void TerminalPage::rebuild()
{
    m_rebuilding = true;
    m_view->clear();
    m_shown = 0;

    const QVector<LogEntry> entries = Log::instance().entries();
    for (int i = 0; i < entries.size(); ++i) {
        if (passes(entries.at(i).level))
            appendEntry(entries.at(i));
    }

    m_rebuilding = false;
    m_count->setText(QString("%1 line%2").arg(m_shown).arg(m_shown == 1 ? "" : "s"));

    const QString logFile = Log::instance().logFile();
    m_path->setText(logFile.isEmpty() ? QString("Log file not configured")
                                      : QDir::toNativeSeparators(logFile));

    QScrollBar *bar = m_view->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void TerminalPage::copyAll()
{
    QApplication::clipboard()->setText(m_view->toPlainText());
    Logging::info("Terminal contents copied to clipboard");
}
