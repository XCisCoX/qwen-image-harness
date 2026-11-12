#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QFont>
#include <QString>

class QWidget;

/*!
 * Central design tokens. Everything visual flows through here so a single
 * accent change re-skins the whole application.
 */
namespace Theme {

extern const QColor Bg0;      // window
extern const QColor Bg1;      // panel
extern const QColor Bg2;      // card
extern const QColor Bg3;      // input / raised
extern const QColor Bg4;      // hover
extern const QColor Line;     // hairline
extern const QColor Line2;    // stronger border
extern const QColor Text;
extern const QColor TextDim;
extern const QColor TextFaint;
extern const QColor Ok;
extern const QColor Warn;
extern const QColor Err;
extern const QColor Info;

QColor accent();
void setAccent(const QColor &c);

//! "rgba(r,g,b,a)" helper usable inside a style sheet.
QString rgba(const QColor &c, int alpha);

QFont ui(int pixels, int weight = QFont::Normal);
QFont mono(int pixels, int weight = QFont::Normal);
QString uiFamily();
QString monoFamily();

QString styleSheet();

//! Applies Fusion style, palette, font and the global style sheet.
void apply();

} // namespace Theme

#endif // THEME_H
