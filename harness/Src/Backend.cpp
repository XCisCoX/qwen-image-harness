#include "Backend.h"
#include "Log.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

QString pageModeId(PageMode mode)
{
    switch (mode) {
    case PageMode::Generate: return QString("generate");
    case PageMode::Edit: return QString("edit");
    case PageMode::FaceSwap: return QString("faceswap");
    case PageMode::MaskInfill: return QString("mask");
    }
    return QString("generate");
}

QString pageModeName(PageMode mode)
{
    switch (mode) {
    case PageMode::Generate: return QString("Generate");
    case PageMode::Edit: return QString("Edit");
    case PageMode::FaceSwap: return QString("Face swap");
    case PageMode::MaskInfill: return QString("Mask infill");
    }
    return QString("Generate");
}

PageMode pageModeFromId(const QString &id)
{
    if (id == "edit")
        return PageMode::Edit;
    if (id == "faceswap")
        return PageMode::FaceSwap;
    if (id == "mask")
        return PageMode::MaskInfill;
    return PageMode::Generate;
}

Backend::Backend(QObject *parent)
    : QObject(parent)
    , m_state(Stopped)
{
    qRegisterMetaType<GenerationRequest>("GenerationRequest");
    qRegisterMetaType<GenerationResult>("GenerationResult");
}

Backend::~Backend()
{
}

void Backend::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(int(state));
}

void Backend::discardImage(const QString &imagePath)
{
    if (imagePath.isEmpty())
        return;

    const QFileInfo fi(imagePath);
    if (!fi.exists()) {
        Logging::warn(QString("Discard: file already gone (%1)").arg(fi.fileName()));
        return;
    }

    if (!QFile::remove(imagePath)) {
        Logging::error(QString("Discard failed: %1").arg(QDir::toNativeSeparators(imagePath)));
        return;
    }

    const QString sidecar = fi.absolutePath() + "/" + fi.completeBaseName() + ".json";
    if (QFileInfo(sidecar).exists())
        QFile::remove(sidecar);

    Logging::success(QString("Discarded %1").arg(fi.fileName()));
}
