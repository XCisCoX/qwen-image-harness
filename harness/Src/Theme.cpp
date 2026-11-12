#include "Theme.h"

#include <QApplication>
#include <QFontDatabase>
#include <QPalette>
#include <QStyleFactory>
#include <QWidget>

namespace Theme {

const QColor Bg0(0x0F, 0x10, 0x13);
const QColor Bg1(0x14, 0x16, 0x1B);
const QColor Bg2(0x18, 0x1B, 0x22);
const QColor Bg3(0x1E, 0x22, 0x2B);
const QColor Bg4(0x26, 0x2B, 0x36);
const QColor Line(0x22, 0x26, 0x2F);
const QColor Line2(0x2E, 0x34, 0x42);
const QColor Text(0xE6, 0xE9, 0xEF);
const QColor TextDim(0x99, 0xA1, 0xB0);
const QColor TextFaint(0x62, 0x6B, 0x7C);
const QColor Ok(0x4A, 0xDE, 0x80);
const QColor Warn(0xF5, 0xB8, 0x4D);
const QColor Err(0xF8, 0x71, 0x71);
const QColor Info(0x60, 0xA5, 0xFA);

static QColor g_accent(0x8B, 0x7B, 0xFF);

QColor accent() { return g_accent; }

void setAccent(const QColor &c)
{
    g_accent = c;
    if (qApp)
        qApp->setStyleSheet(styleSheet());
}

QString rgba(const QColor &c, int alpha)
{
    return QString("rgba(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(alpha);
}

static QString pickFamily(const QStringList &candidates)
{
    static QStringList families;
    if (families.isEmpty() && qApp)
        families = QFontDatabase().families();
    for (int i = 0; i < candidates.size(); ++i) {
        const QString &c = candidates.at(i);
        if (families.isEmpty() || families.contains(c, Qt::CaseInsensitive))
            return c;
    }
    return qApp ? QApplication::font().family() : QString("Sans Serif");
}

QString uiFamily()
{
    static const QString f = pickFamily(QStringList()
                                        << "Inter" << "Segoe UI Variable Text" << "Segoe UI"
                                        << "SF Pro Text" << "Ubuntu" << "Noto Sans"
                                        << "DejaVu Sans" << "Arial");
    return f;
}

QString monoFamily()
{
    static const QString f = pickFamily(QStringList()
                                        << "JetBrains Mono" << "Cascadia Mono" << "Cascadia Code"
                                        << "Consolas" << "Ubuntu Mono" << "DejaVu Sans Mono"
                                        << "Liberation Mono" << "Courier New");
    return f;
}

QFont ui(int pixels, int weight)
{
    QFont f(uiFamily());
    f.setPixelSize(pixels);
    f.setWeight(weight);
    f.setHintingPreference(QFont::PreferFullHinting);
    return f;
}

QFont mono(int pixels, int weight)
{
    QFont f(monoFamily());
    f.setPixelSize(pixels);
    f.setWeight(weight);
    f.setStyleHint(QFont::Monospace);
    return f;
}

QString styleSheet()
{
    const QColor a = accent();
    const QString accentHover = a.lighter(112).name();
    const QString accentPressed = a.darker(112).name();
    const QString accentSoft = rgba(a, 40);
    const QString accentSofter = rgba(a, 22);
    const QString accentBorder = rgba(a, 90);

    static const char *sheet = R"qss(
* { outline: none; }

#Root { background: @bg0; }
#TitleBar { background: @bg0; border-bottom: 1px solid @line; }
#Sidebar { background: @bg1; border-right: 1px solid @line; }
#StatusStrip { background: @bg0; border-top: 1px solid @line; }
#ContentArea { background: @bg0; }
#PageScroll { background: transparent; border: none; }

QFrame#Card {
    background: @bg2;
    border: 1px solid @line;
    border-radius: 12px;
}
QFrame#CardInner {
    background: @bg1;
    border: 1px solid @line;
    border-radius: 10px;
}
QFrame#Separator { background: @line; border: none; max-height: 1px; min-height: 1px; }
QFrame#VLine { background: @line; border: none; max-width: 1px; min-width: 1px; }

QLabel { color: @text; background: transparent; }
QLabel#PageTitle { font-size: 19px; font-weight: bold; }
QLabel#PageSub { color: @textDim; font-size: 12px; }
QLabel#CardTitle { color: @text; font-size: 13px; font-weight: bold; }
QLabel#SectionLabel { color: @textFaint; font-size: 10px; font-weight: bold; }
QLabel#Hint { color: @textFaint; font-size: 11px; }
QLabel#Dim { color: @textDim; font-size: 12px; }
QLabel#Mono { font-family: "@mono"; font-size: 11px; color: @textDim; }
QLabel#Error { color: @err; font-size: 12px; }
QLabel#Muted { color: @textFaint; }
QLabel#Badge {
    background: @accentSofter;
    color: @accent;
    border: 1px solid @accentBorder;
    border-radius: 9px;
    padding: 3px 9px;
    font-size: 11px;
}

QPushButton {
    background: @bg3;
    color: @text;
    border: 1px solid @line2;
    border-radius: 8px;
    padding: 7px 13px;
    font-size: 12px;
}
QPushButton:hover { background: @bg4; border-color: #3B4356; }
QPushButton:pressed { background: #191C24; }
QPushButton:disabled { color: #59606E; background: #171A21; border-color: #23272F; }
QPushButton:focus { border-color: @accentBorder; }

QPushButton#Primary {
    background: @accent;
    border: 1px solid @accent;
    color: #FFFFFF;
    font-weight: bold;
    font-size: 13px;
    padding: 10px 16px;
    border-radius: 9px;
}
QPushButton#Primary:hover { background: @accentHover; border-color: @accentHover; }
QPushButton#Primary:pressed { background: @accentPressed; border-color: @accentPressed; }
QPushButton#Primary:disabled { background: #23262F; border-color: #23262F; color: #5B6272; }

QPushButton#Ghost {
    background: transparent; border: 1px solid transparent; color: @textDim; padding: 6px 10px;
}
QPushButton#Ghost:hover { background: @bg3; color: @text; border-color: @line; }
QPushButton#Ghost:checked { background: @bg3; color: @text; border-color: @line2; }

QPushButton#Danger {
    background: rgba(248,113,113,30); border: 1px solid rgba(248,113,113,70); color: @err;
}
QPushButton#Danger:hover { background: rgba(248,113,113,50); }

QPushButton#NavItem {
    text-align: left;
    background: transparent;
    border: 1px solid transparent;
    border-radius: 8px;
    padding: 0px 10px;
    color: @textDim;
    font-size: 12px;
}
QPushButton#NavItem:hover { background: @bg3; color: @text; }
QPushButton#NavItem:checked { background: @accentSofter; color: @text; border-color: @accentSofter; }

QPushButton#WindowButton {
    background: transparent; border: none; border-radius: 6px; padding: 0px;
}
QPushButton#WindowButton:hover { background: @bg3; }
QPushButton#WindowButtonClose:hover { background: #C0392B; }

QToolButton {
    background: transparent; border: 1px solid transparent; border-radius: 7px; padding: 5px;
}
QToolButton:hover { background: @bg3; border-color: @line; }
QToolButton:pressed { background: #191C24; }
QToolButton:checked { background: @accentSofter; border-color: @accentBorder; }

QLineEdit, QPlainTextEdit, QTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
    background: #0D0F13;
    border: 1px solid @line2;
    border-radius: 8px;
    padding: 7px 10px;
    color: @text;
    selection-background-color: @accent;
    selection-color: #FFFFFF;
    font-size: 12px;
}
QLineEdit:hover, QPlainTextEdit:hover, QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover {
    border-color: #394154;
}
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus,
QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus { border-color: @accentBorder; }
QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {
    color: @textFaint; background: #14171C; border-color: #22262E;
}

QPlainTextEdit#Prompt {
    border-radius: 10px; padding: 10px; font-size: 13px; background: #0D0F13;
}
QPlainTextEdit#TerminalView {
    background: #0B0D10;
    border: 1px solid @line;
    border-radius: 10px;
    padding: 10px;
    font-size: 11px;
}
QLabel#Preview {
    background: #0B0D10;
    border: 1px solid @line;
    border-radius: 10px;
    padding: 6px;
}

QSpinBox::up-button, QSpinBox::down-button,
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width: 0px; border: none; }

QComboBox::drop-down { border: none; width: 26px; }
QComboBox::down-arrow { image: none; width: 0px; height: 0px; }
QComboBox QAbstractItemView {
    background: @bg2; border: 1px solid @line2; border-radius: 8px;
    padding: 4px; outline: none; color: @text;
    selection-background-color: @accentSofter; selection-color: @text;
}

QScrollArea { background: transparent; border: none; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px 2px 2px 0; }
QScrollBar::handle:vertical { background: #2B3140; border-radius: 3px; min-height: 28px; }
QScrollBar::handle:vertical:hover { background: #3B4356; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 0 2px 2px 2px; }
QScrollBar::handle:horizontal { background: #2B3140; border-radius: 3px; min-width: 28px; }
QScrollBar::handle:horizontal:hover { background: #3B4356; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0px; height: 0px; border: none; background: none; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QSlider::groove:horizontal { height: 4px; background: @bg4; border-radius: 2px; }
QSlider::sub-page:horizontal { background: @accent; border-radius: 2px; }
QSlider::handle:horizontal {
    width: 13px; height: 13px; margin: -5px 0; border-radius: 6px;
    background: #FFFFFF; border: 2px solid @accent;
}
QSlider::handle:horizontal:hover { background: @accentHover; }

QToolTip {
    background: @bg3; color: @text; border: 1px solid @line2;
    border-radius: 6px; padding: 5px 8px; font-size: 11px;
}

QMenu {
    background: @bg2; border: 1px solid @line2; border-radius: 8px; padding: 5px;
}
QMenu::item { padding: 6px 22px 6px 12px; border-radius: 5px; color: @text; font-size: 12px; }
QMenu::item:selected { background: @accentSofter; }
QMenu::separator { height: 1px; background: @line; margin: 5px 8px; }

QSplitter::handle { background: transparent; }
QSplitter::handle:horizontal { width: 10px; }
QSplitter::handle:vertical { height: 10px; }

QDialog { background: @bg0; }

QListWidget {
    background: transparent;
    border: none;
    outline: none;
}
QListWidget::item {
    border-radius: 10px;
    padding: 8px;
    color: @textDim;
}
QListWidget::item:hover { background: @bg3; }
QListWidget::item:selected {
    background: @accentSofter;
    border: 1px solid @accentBorder;
    color: @text;
}
QToolButton#Swatch {
    border: 2px solid transparent;
    border-radius: 15px;
    padding: 2px;
}
QToolButton#Swatch:hover { border-color: @line2; }
QToolButton#Swatch:checked { border-color: @text; }
)qss";

    QString out = QString::fromUtf8(sheet);
    out.replace("@bg0", Bg0.name());
    out.replace("@bg1", Bg1.name());
    out.replace("@bg2", Bg2.name());
    out.replace("@bg3", Bg3.name());
    out.replace("@bg4", Bg4.name());
    out.replace("@line2", Line2.name());
    out.replace("@line", Line.name());
    out.replace("@textDim", TextDim.name());
    out.replace("@textFaint", TextFaint.name());
    out.replace("@text", Text.name());
    out.replace("@accentHover", accentHover);
    out.replace("@accentPressed", accentPressed);
    out.replace("@accentBorder", accentBorder);
    out.replace("@accentSofter", accentSofter);
    out.replace("@accentSoft", accentSoft);
    out.replace("@accent", a.name());
    out.replace("@err", Err.name());
    out.replace("@warn", Warn.name());
    out.replace("@ok", Ok.name());
    out.replace("@mono", monoFamily());
    return out;
}

void apply()
{
    if (!qApp)
        return;

    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window, Bg0);
    p.setColor(QPalette::WindowText, Text);
    p.setColor(QPalette::Base, QColor(0x0D, 0x0F, 0x13));
    p.setColor(QPalette::AlternateBase, Bg2);
    p.setColor(QPalette::Text, Text);
    p.setColor(QPalette::Button, Bg3);
    p.setColor(QPalette::ButtonText, Text);
    p.setColor(QPalette::BrightText, Qt::white);
    p.setColor(QPalette::Highlight, accent());
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase, Bg3);
    p.setColor(QPalette::ToolTipText, Text);
    p.setColor(QPalette::Link, accent());
    p.setColor(QPalette::Disabled, QPalette::Text, TextFaint);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, TextFaint);
    p.setColor(QPalette::Disabled, QPalette::WindowText, TextFaint);
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    p.setColor(QPalette::PlaceholderText, TextFaint);
#endif
    qApp->setPalette(p);

    qApp->setFont(ui(13));
    qApp->setStyleSheet(styleSheet());
}

} // namespace Theme
