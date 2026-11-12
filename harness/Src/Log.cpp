#include "Log.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

static const int kMaxEntries = 5000;

Log::Log(QObject *parent)
    : QObject(parent)
    , m_mirrorToFile(true)
{
}

Log &Log::instance()
{
    static Log inst;
    return inst;
}

QString Log::levelName(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug: return QString("debug");
    case LogLevel::Info: return QString("info");
    case LogLevel::Success: return QString("ok");
    case LogLevel::Warn: return QString("warn");
    case LogLevel::Error: return QString("error");
    case LogLevel::Backend: return QString("backend");
    }
    return QString("info");
}

QString Log::levelTag(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug: return QString("[dbg]");
    case LogLevel::Info: return QString("[inf]");
    case LogLevel::Success: return QString("[ ok]");
    case LogLevel::Warn: return QString("[wrn]");
    case LogLevel::Error: return QString("[err]");
    case LogLevel::Backend: return QString("[srv]");
    }
    return QString("[inf]");
}

void Log::setLogFile(const QString &path)
{
    m_logFile = path;
}

LogEntry Log::last() const
{
    if (m_entries.isEmpty()) {
        LogEntry e;
        e.time = QDateTime::currentDateTime();
        e.level = LogLevel::Info;
        e.text = QString();
        return e;
    }
    return m_entries.last();
}

void Log::add(LogLevel level, const QString &text)
{
    LogEntry e;
    e.time = QDateTime::currentDateTime();
    e.level = level;
    e.text = text;

    m_entries.append(e);
    if (m_entries.size() > kMaxEntries)
        m_entries.remove(0, m_entries.size() - kMaxEntries);

    if (m_mirrorToFile && !m_logFile.isEmpty()) {
        QFileInfo fi(m_logFile);
        if (!fi.absoluteDir().exists())
            QDir().mkpath(fi.absolutePath());
        QFile f(m_logFile);
        if (f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << e.time.toString("yyyy-MM-dd HH:mm:ss.zzz") << " "
               << levelTag(level) << " " << text << "\n";
        }
    }

    emit appended(e);
}

void Log::clear()
{
    m_entries.clear();
    if (!m_logFile.isEmpty()) {
        QFile f(m_logFile);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << "# Qwen Image Studio log started " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
        }
    }
    emit cleared();
}

namespace Logging {

void debug(const QString &text) { Log::instance().add(LogLevel::Debug, text); }
void info(const QString &text) { Log::instance().add(LogLevel::Info, text); }
void success(const QString &text) { Log::instance().add(LogLevel::Success, text); }
void warn(const QString &text) { Log::instance().add(LogLevel::Warn, text); }
void error(const QString &text) { Log::instance().add(LogLevel::Error, text); }
void backend(const QString &text) { Log::instance().add(LogLevel::Backend, text); }

} // namespace Logging
