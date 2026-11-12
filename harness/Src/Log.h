#ifndef LOG_H
#define LOG_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVector>

enum class LogLevel {
    Debug = 0,
    Info,
    Success,
    Warn,
    Error,
    Backend
};

struct LogEntry {
    QDateTime time;
    LogLevel level;
    QString text;
};

/*!
 * Process-wide log bus. The terminal page renders the full history, the status
 * strip shows the newest line, and optionally every line is mirrored to disk.
 */
class Log : public QObject
{
    Q_OBJECT

public:
    static Log &instance();

    void add(LogLevel level, const QString &text);
    void clear();

    QVector<LogEntry> entries() const { return m_entries; }
    LogEntry last() const;

    void setMirrorToFile(bool on) { m_mirrorToFile = on; }
    void setLogFile(const QString &path);
    QString logFile() const { return m_logFile; }

    static QString levelName(LogLevel level);
    static QString levelTag(LogLevel level);

signals:
    void appended(const LogEntry &entry);
    void cleared();

private:
    explicit Log(QObject *parent = nullptr);

    QVector<LogEntry> m_entries;
    QString m_logFile;
    bool m_mirrorToFile;
};

namespace Logging {
void debug(const QString &text);
void info(const QString &text);
void success(const QString &text);
void warn(const QString &text);
void error(const QString &text);
void backend(const QString &text);
} // namespace Logging

#endif // LOG_H
