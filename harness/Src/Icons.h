#ifndef ICONS_H
#define ICONS_H

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

/*!
 * Vector icon set painted at runtime. Keeping icons procedural avoids binary
 * assets and makes every glyph follow the current accent colour.
 */
namespace Icons {

enum Id {
    Sparkles,
    Pen,
    Swap,
    Mask,
    Grid,
    Terminal,
    Gear,
    Play,
    Stop,
    Plus,
    Close,
    Minimize,
    Maximize,
    Restore,
    ChevronDown,
    ChevronRight,
    Folder,
    Trash,
    Refresh,
    Copy,
    Save,
    Undo,
    Redo,
    Brush,
    Eraser,
    Image,
    Check,
    Alert,
    Dice,
    Lock,
    Star,
    External,
    ZoomIn,
    ZoomOut,
    Fit,
    Server,
    Info,
    Pause,
    ArrowLeft,
    ArrowRight,
    Download,
    Layers
};

QPixmap pixmap(Id id, int size, const QColor &color);

//! QIcon with distinct Off/On states, for checkable buttons.
QIcon stateIcon(Id id, const QColor &off, const QColor &on, int size = 18);

} // namespace Icons

#endif // ICONS_H
