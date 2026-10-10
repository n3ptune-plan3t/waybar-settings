// Colour roles, readability guarantees and the persistent style model.
//
// Everything the app writes into style.css is derived from a StyleState. A
// single resolve() pass turns the picked colours into a palette in which every
// text colour clears 4.5:1 against the bar, against any module fill, and against
// a fully black or fully white wallpaper - if a pick cannot satisfy that, the
// opacity is raised (or the colour nudged) and the change is reported back.
#pragma once

#include <QColor>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

namespace ws {

enum Role {
  RBackground = 0,
  RText,
  RAccent,
  RWarning,
  RCritical,
  RGood,
  RMuted,
  RSurface,
  RCount
};

const char *roleKey(int role);
QString roleLabel(int role);
int roleFromKey(const QString &key);

struct Theme {
  QString name;
  QString hex[RCount];
};

const QVector<Theme> &themes();
int themeIndexByName(const QString &name);

// ---- colour maths ---------------------------------------------------------
double relativeLuminance(const QColor &c);
double contrastRatio(const QColor &a, const QColor &b);
QColor blend(const QColor &a, const QColor &b, double t);
QColor liftToContrast(QColor c, const QColor &ref, double minRatio);

struct Resolved {
  QColor c[RCount];
  QColor stateTextAccent;   // legible text on the accent fill
  QColor stateTextWarning;  // legible text on the warning fill
  QColor stateTextCritical; // legible text on the critical fill
  double opacity = 1.0;
  QStringList notes;
};

Resolved resolveTheme(const QColor in[RCount], int opacityPct, bool raisedModules);

// ---- per module style -----------------------------------------------------
enum class SpecSource { Inherit = 0, Bar, Theme, Custom };

struct ColorSpec {
  SpecSource source = SpecSource::Inherit;
  int role = RText;
  QColor custom;

  bool isSet() const { return source != SpecSource::Inherit; }
  bool operator==(const ColorSpec &o) const {
    if (source != o.source) return false;
    if (source == SpecSource::Inherit) return true;
    if (source == SpecSource::Theme && theme != o.theme) return false;
    if (source == SpecSource::Custom && custom != o.custom) return false;
    if ((source == SpecSource::Bar || source == SpecSource::Theme) && role != o.role) return false;
    return true;
  }
  bool operator!=(const ColorSpec &o) const { return !(*this == o); }

  int theme = -1;  // SpecSource::Theme
};

QColor resolveSpecColor(const ColorSpec &spec, const QColor bar[RCount], const QVector<Theme> &palette);

struct ModuleStyle {
  ColorSpec fill;
  ColorSpec text;
  ColorSpec border;
  QMap<QString, ColorSpec> stateText;   // CSS state class -> text colour
  QMap<QString, ColorSpec> stateFill;   // CSS state class -> fill colour
  int borderWidth = 1;
  int radius = -1;    // -1 inherits
  int padding = -1;   // -1 inherits
  int margin = -1;    // -1 inherits
  int fontSize = 0;   // 0 inherits
  int weight = 0;     // 0 normal, 1 bold, 2 light
  bool italic = false;

  bool isDefault() const;
};

struct ResolvedModule {
  bool hasFill = false, hasText = false, hasBorder = false;
  QColor fill, text, border;
  int borderWidth = 1;
  int radius = 0, padding = 0, margin = 0, fontSize = 0;
  int weight = 400;
  bool italic = false;
};

ResolvedModule resolveModule(const ModuleStyle &ms, const Resolved &bar, const QVector<Theme> &palette);

struct StyleState {
  bool manageCss = true;
  QString themeName = QStringLiteral("Catppuccin Mocha");
  QColor color[RCount] = {QColor("#1e1e2e"), QColor("#cdd6f4"), QColor("#89b4fa"), QColor("#f9e2af"),
                          QColor("#f38ba8"), QColor("#a6e3a1"), QColor("#9399b2"), QColor("#313244")};
  QStringList fontStack = {QStringLiteral("sans-serif")};
  int fontSize = 13;
  int weight = 400;   // bar wide weight
  bool italic = false;
  int radius = 6;
  int padding = 6;
  int margin = 0;
  int borderWidth = 1;
  int opacity = 100;
  int underlineThickness = 0;
  bool underlineAccent = true;
  bool underlineLeft = true, underlineCenter = true, underlineRight = true;
  bool raised = false;  // give every module a raised (pill) background
  QMap<QString, ModuleStyle> modules;

  QJsonObject toJson() const;
  static StyleState fromJson(const QJsonObject &o);
};

// Every module reference that currently has a style, in configuration order.
QStringList styledModules(const QJsonObject &barObject);

// Writes the managed block (header comment + rules) for the given state.
QString generateCss(const StyleState &st, const Resolved &r, const QStringList &presentModules);

}  // namespace ws
