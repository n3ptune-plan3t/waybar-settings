// Metadata describing every Waybar module and every option it accepts.
//
// The option tables are transcribed from the Waybar sources (src/modules/*,
// src/ALabel.cpp, src/AModule.cpp, src/ASlider.cpp, src/AGraph.cpp) and are used
// to build typed editors instead of a raw key/value table. Options not covered
// here still round-trip untouched through the "extra options" JSON editor.
#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace ws {

enum class OptKind {
  Bool,      // true / false / unset
  Int,
  Double,
  String,
  Choice,    // one of a fixed set
  List,      // array of strings
  Json,      // free-form JSON
  Command,   // shell command
  Path,      // filesystem path (browse button)
  Interval,  // seconds, "once", or event driven (0)
  States,    // { name: threshold } -> CSS classes
};

struct Opt {
  QString key;
  OptKind kind = OptKind::String;
  QString def;       // default written as JSON text; empty means "no default"
  QString help;
  QString choices;   // '|'-separated, OptKind::Choice
  QString group = QStringLiteral("General");
};

struct ModuleDef {
  QString id;             // "cpu", "custom/*", "group/*", ...
  QString label;          // human readable
  QString category;
  QString cssId;          // Waybar CSS id, e.g. "cpu", "custom-foo"
  QString summary;
  QStringList states;     // CSS classes the module toggles on/off
  QVector<Opt> opts;      // module specific options
  bool labelBased = true;
  bool iconBased = false;
  bool graph = false;
  bool slider = false;
  bool template_ = false; // needs a name before it can be added (custom/* ...)
  QString suggestedName;  // for templates
};

// ---- module registry -------------------------------------------------------
const QVector<ModuleDef> &modules();
const ModuleDef *findModule(const QString &id);

// ---- option assembly -------------------------------------------------------
QVector<Opt> optionsFor(const ModuleDef &m);

// ---- reference helpers -----------------------------------------------------
QString moduleType(const QString &ref);      // "clock#2" -> "clock", "custom/x" -> "custom/*"
QString cssSelector(const QString &ref);     // "sway/workspaces#w" -> "#workspaces.w"
QString cssIdFor(const QString &ref);        // "sway/workspaces" -> "workspaces"
QString displayName(const QString &ref);     // "hyprland/workspaces" -> "Workspaces (Hyprland)"
QString sampleText(const QString &ref);      // representative label text for previews
bool isTemplateRef(const QString &ref);

}  // namespace ws
