#include "style.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>

#include "schema.h"

namespace ws {
namespace {

const char *ROLE_KEYS[RCount] = {"bg", "fg", "ac", "wn", "cr", "gd", "mu", "sf"};

struct ThemeRow {
  const char *name;
  const char *hex[RCount];
};

const ThemeRow THEME_ROWS[] = {
    {"Catppuccin Mocha", {"#1e1e2e", "#cdd6f4", "#89b4fa", "#f9e2af", "#f38ba8", "#a6e3a1", "#9399b2", "#313244"}},
    {"Catppuccin Macchiato", {"#24273a", "#cad3f5", "#8aadf4", "#eed49f", "#ed8796", "#a6da95", "#a5adce", "#363a4f"}},
    {"Catppuccin Frappe", {"#303446", "#c6d0f5", "#8caaee", "#e5c890", "#e78284", "#a6d189", "#949cbb", "#414559"}},
    {"Catppuccin Latte", {"#eff1f5", "#4c4f69", "#1e66f5", "#df8e1d", "#d20f39", "#40a02b", "#8c8fa1", "#dce0e8"}},
    {"Gruvbox Dark", {"#282828", "#ebdbb2", "#83a598", "#fabd2f", "#fb4934", "#b8bb26", "#928374", "#3c3836"}},
    {"Gruvbox Light", {"#fbf1c7", "#3c3836", "#076678", "#b57614", "#9d0006", "#79740e", "#7c6f64", "#ebdbb2"}},
    {"Nord", {"#2e3440", "#eceff4", "#88c0d0", "#ebcb8b", "#bf616a", "#a3be8c", "#7b88a1", "#3b4252"}},
    {"Nord Light", {"#f2f4f8", "#2e3440", "#5e81ac", "#b58900", "#bf616a", "#a3be8c", "#7b88a1", "#e5e9f0"}},
    {"Dracula", {"#282a36", "#f8f8f2", "#bd93f9", "#f1fa8c", "#ff5555", "#50fa7b", "#9aa4b2", "#343746"}},
    {"Tokyo Night", {"#1a1b26", "#c0caf5", "#7aa2f7", "#e0af68", "#f7768e", "#9ece6a", "#787fa4", "#24283b"}},
    {"Tokyo Night Light", {"#e1e2e7", "#3760bf", "#2e7de9", "#8c6c3e", "#f52a65", "#587539", "#8990b3", "#d0d0e0"}},
    {"Solarized Dark", {"#002b36", "#93a1a1", "#268bd2", "#b58900", "#dc322f", "#859900", "#657b83", "#073642"}},
    {"Solarized Light", {"#fdf6e3", "#586e75", "#268bd2", "#b58900", "#dc322f", "#859900", "#93a1a1", "#eee8d5"}},
    {"Rose Pine", {"#191724", "#e0def4", "#c4a7e7", "#f6c177", "#eb6f92", "#9ccfd8", "#908caa", "#26233a"}},
    {"Rose Pine Dawn", {"#faf4ed", "#575279", "#907aa9", "#ea9d34", "#d7827e", "#56949f", "#797593", "#f2e9e1"}},
    {"One Dark", {"#282c34", "#abb2bf", "#61afef", "#e5c07b", "#e06c75", "#98c379", "#7a8394", "#333842"}},
    {"One Light", {"#fafafa", "#383a42", "#4078f2", "#c18401", "#e45649", "#50a14f", "#9d9d9f", "#eaeaeb"}},
    {"Everforest Dark", {"#2d353b", "#d3c6aa", "#7fbbb3", "#dbbc7f", "#e67e80", "#a7c080", "#95a08c", "#37414c"}},
    {"Everforest Light", {"#fdf6e3", "#5c6a72", "#3a94c5", "#f57d26", "#f85552", "#8da101", "#939f91", "#f4f0d9"}},
    {"Kanagawa", {"#1f1f28", "#dcd7ba", "#7e9cd8", "#e6c384", "#e46876", "#98bb6c", "#727169", "#2a2a37"}},
    {"Kanagawa Wave", {"#1f1f28", "#dcd7ba", "#957fb8", "#ffa066", "#c84053", "#98bb6c", "#727169", "#2a2a37"}},
    {"Kanzen", {"#2f2f2f", "#d4d4d4", "#87a889", "#e0af68", "#c4746e", "#a9b665", "#8a8a8a", "#3a3a3a"}},
    {"Monokai Pro", {"#2d2a2e", "#fcfcfa", "#ffd866", "#fd971f", "#fb2c2c", "#a9dc76", "#959077", "#3b383b"}},
    {"Ayu Dark", {"#0f1419", "#e6e1cf", "#e6b673", "#ff8f40", "#f26d78", "#aad94c", "#8a9199", "#1c2128"}},
    {"Ayu Light", {"#fbf1e7", "#5c6773", "#ff9940", "#ff8f40", "#e65050", "#86b300", "#8a9199", "#f3e0d9"}},
    {"Material", {"#263238", "#eeffff", "#82b1ff", "#ffcb6b", "#ff5370", "#c3e88d", "#89a4b1", "#37474f"}},
    {"Fennec Dark", {"#30343b", "#f2f4f8", "#e0b050", "#d88c30", "#c66d65", "#8cc265", "#949ba6", "#3b4048"}},
    {"Builtin Dark", {"#1c1c1c", "#dedede", "#61afef", "#e5c07b", "#e06c75", "#98c379", "#9a9a9a", "#2b2b2b"}},
    {"Builtin Light", {"#f2f2f2", "#2b2b2b", "#3d7fd6", "#a06000", "#c0392b", "#4b8b3b", "#767676", "#e2e2e2"}},
    {"Custom", {"#1e1e2e", "#cdd6f4", "#89b4fa", "#f9e2af", "#f38ba8", "#a6e3a1", "#9399b2", "#313244"}},
};

struct StateBucket {
  int role;
  const char *classes;  // '|'-separated
};

const StateBucket STATE_BUCKETS[] = {
    {RWarning,
     "warning|degraded|needs-attention|solving|demoted|medium"},
    {RCritical,
     "critical|urgent|xrun|failed|locked|rejected|empty|critical-charge"},
    {RAccent,
     "focused|active|current_output|visible|occupied|persistent|special|maximized|activated|"
     "on|connected|playing|full|plugged|charging|running|ok|registered|performance|balanced|"
     "power-saver|bluetooth|enabled|fixed|2d|3d|searching|hosting-monitor|workspace-hover"},
    {RGood, "discharging|available|dpms|battery-high|battery-low|battery-full"},
    {RMuted,
     "muted|sink-muted|source-muted|disabled|disconnected|off|paused|stopped|deactivated|"
     "no-controller|fix-none|passive|hidden|truncated|alt|deactivated"},
};

bool isGenericFamily(const QString &n) {
  return n == QLatin1String("sans-serif") || n == QLatin1String("serif") ||
         n == QLatin1String("monospace") || n == QLatin1String("cursive") ||
         n == QLatin1String("fantasy") || n == QLatin1String("system-ui");
}

QString quoteFamily(const QString &n) { return isGenericFamily(n) ? n : QLatin1Char('"') + n + QLatin1Char('"'); }

QString rgba(const QColor &c, double a) {
  return QStringLiteral("rgba(%1, %2, %3, %4)")
      .arg(c.red())
      .arg(c.green())
      .arg(c.blue())
      .arg(a, 0, 'f', 3);
}

QString fontStackCss(const QStringList &stack, const QString &extra) {
  QStringList parts;
  for (const QString &raw : stack + QStringList(extra)) {
    const QString n = raw.trimmed();
    if (!n.isEmpty() && !parts.contains(n)) parts << n;
  }
  if (parts.isEmpty()) parts << QStringLiteral("sans-serif");
  bool generic = false;
  for (const QString &n : parts)
    if (isGenericFamily(n)) generic = true;
  if (!generic) parts << QStringLiteral("sans-serif");
  QStringList quoted;
  for (const QString &n : parts) quoted << quoteFamily(n);
  return quoted.join(QLatin1String(", "));
}

QString weightName(int w) {
  if (w <= 0) return QStringLiteral("normal");
  return QString::number(w);
}

QString weightKeyword(int w) {
  switch (w) {
    case 1: return QStringLiteral("bold");
    case 2: return QStringLiteral("light");
    default: return QString();
  }
}

}  // namespace

// ---- colour maths ---------------------------------------------------------

double relativeLuminance(const QColor &c) {
  auto f = [](double v) { return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
  return 0.2126 * f(c.redF()) + 0.7152 * f(c.greenF()) + 0.0722 * f(c.blueF());
}

double contrastRatio(const QColor &a, const QColor &b) {
  double x = relativeLuminance(a), y = relativeLuminance(b);
  if (x < y) std::swap(x, y);
  return (x + 0.05) / (y + 0.05);
}

QColor blend(const QColor &a, const QColor &b, double t) {
  return QColor::fromRgbF(a.redF() * (1 - t) + b.redF() * t, a.greenF() * (1 - t) + b.greenF() * t,
                          a.blueF() * (1 - t) + b.blueF() * t);
}

QColor liftToContrast(QColor c, const QColor &ref, double minRatio) {
  if (!c.isValid()) c = QColor(Qt::white);
  if (contrastRatio(c, ref) >= minRatio) return c;
  const QColor target = contrastRatio(QColor(Qt::white), ref) >= contrastRatio(QColor(Qt::black), ref)
                            ? QColor(Qt::white)
                            : QColor(Qt::black);
  for (int i = 1; i <= 20; ++i) {
    const QColor n = blend(c, target, i / 20.0);
    if (contrastRatio(n, ref) >= minRatio) return n;
  }
  return target;
}

Resolved resolveTheme(const QColor in[RCount], int opacityPct, bool raisedModules) {
  Resolved r;
  r.c[RBackground] = in[RBackground];
  const QColor bg = in[RBackground];

  QColor fg = liftToContrast(in[RText], bg, 4.5);
  if (fg != in[RText])
    r.notes << QStringLiteral("text colour nudged for readability (%1:1 to %2:1)")
                   .arg(contrastRatio(in[RText], bg), 0, 'f', 1)
                   .arg(contrastRatio(fg, bg), 0, 'f', 1);
  r.c[RAccent] = liftToContrast(in[RAccent], bg, 3.0);
  r.c[RWarning] = liftToContrast(in[RWarning], bg, 3.0);
  r.c[RCritical] = liftToContrast(in[RCritical], bg, 3.0);
  if (r.c[RAccent] != in[RAccent] || r.c[RWarning] != in[RWarning] || r.c[RCritical] != in[RCritical])
    r.notes << QStringLiteral("accent, warning and critical nudged until they stand out from the bar");

  QList<QColor> refs{bg};
  auto readable = [&](QColor c) {
    for (const QColor &ref : refs) c = liftToContrast(c, ref, 4.5);
    auto worst = [&](const QColor &e) {
      double k = 1e9;
      for (const QColor &ref : refs) k = std::min(k, contrastRatio(e, ref));
      return k;
    };
    if (worst(c) < 4.49)
      c = worst(QColor(Qt::white)) >= worst(QColor(Qt::black)) ? QColor(Qt::white) : QColor(Qt::black);
    return c;
  };

  QColor surface = bg;
  if (raisedModules) {
    for (const double tint : {0.10, 0.08, 0.06, 0.04, 0.02, 0.0}) {
      surface = blend(bg, fg, tint);
      refs = {bg, surface};
      bool ok = true;
      for (int i : {RText, RGood, RMuted}) {
        const QColor fixed = readable(in[i]);
        if (fixed != in[i]) ok = false;
      }
      if (ok || tint == 0.0) {
        if (tint < 0.10 && tint > 0.0)
          r.notes << QStringLiteral("module surface tint reduced to %1% to keep the text readable")
                         .arg(qRound(tint * 100));
        if (tint == 0.0)
          r.notes << QStringLiteral("module surface flattened; the scheme colours cannot be tinted and "
                                    "stay readable at the same time");
        break;
      }
    }
  }
  refs = {bg};
  if (raisedModules) refs << surface;
  r.c[RSurface] = surface;
  for (int i = 0; i < RCount; ++i) {
    if (i == RBackground || i == RAccent || i == RWarning || i == RCritical || i == RSurface) continue;
    QColor c = readable(in[i]);
    if (c != in[i]) r.notes << QStringLiteral("%1 adjusted for readability").arg(roleLabel(i));
    r.c[i] = c;
  }
  r.c[RMuted] = readable(blend(in[RText], bg, 0.45));
  r.c[RText] = readable(in[RText]);

  // Legible text on the state fills.
  auto textOn = [&](const QColor &fill, const QColor &fallback) {
    for (const QColor &c : {fallback, QColor(Qt::black), QColor(Qt::white)})
      if (contrastRatio(c, fill) >= 4.5) return c;
    return QColor(Qt::black);
  };
  r.stateTextAccent = textOn(r.c[RAccent], in[RText]);
  r.stateTextWarning = textOn(r.c[RWarning], in[RText]);
  r.stateTextCritical = textOn(r.c[RCritical], in[RText]);

  // Lowest opacity that still keeps every text colour readable over any wallpaper.
  QList<QColor> texts = {r.c[RText], r.c[RMuted], r.c[RGood], r.stateTextAccent, r.stateTextWarning,
                         r.stateTextCritical};
  double minAlpha = 1.0;
  for (int p = 100; p >= 0; --p) {
    const double k = p / 100.0;
    bool ok = true;
    for (const QColor &t : {QColor(Qt::black), QColor(Qt::white)})
      for (const QColor &tx : texts)
        if (contrastRatio(tx, blend(t, bg, k)) < 4.5) {
          ok = false;
          break;
        }
    if (!ok) break;
    minAlpha = k;
  }
  double a = qBound(0, opacityPct, 100) / 100.0;
  if (a < minAlpha) {
    a = minAlpha;
    r.notes << QStringLiteral("bar opacity raised to %1% so the text stays readable over any wallpaper")
                   .arg(qRound(minAlpha * 100));
  }
  r.opacity = a;
  return r;
}

// ---- specs ----------------------------------------------------------------

const char *roleKey(int role) { return (role >= 0 && role < RCount) ? ROLE_KEYS[role] : "bg"; }

QString roleLabel(int role) {
  switch (role) {
    case RBackground: return QStringLiteral("bar background");
    case RText: return QStringLiteral("text");
    case RAccent: return QStringLiteral("accent");
    case RWarning: return QStringLiteral("warning");
    case RCritical: return QStringLiteral("critical");
    case RGood: return QStringLiteral("good");
    case RMuted: return QStringLiteral("muted text");
    case RSurface: return QStringLiteral("module surface");
    default: return QString();
  }
}

int roleFromKey(const QString &key) {
  for (int i = 0; i < RCount; ++i)
    if (key == QLatin1String(ROLE_KEYS[i])) return i;
  return RBackground;
}

const QVector<Theme> &themes() {
  static QVector<Theme> v = [] {
    QVector<Theme> out;
    for (const ThemeRow &row : THEME_ROWS) {
      Theme t;
      t.name = QString::fromUtf8(row.name);
      for (int i = 0; i < RCount; ++i) t.hex[i] = QString::fromUtf8(row.hex[i]);
      out.append(t);
    }
    return out;
  }();
  return v;
}

int themeIndexByName(const QString &name) {
  const QVector<Theme> &t = themes();
  for (int i = 0; i < t.size(); ++i)
    if (t.at(i).name == name) return i;
  return -1;
}

QColor resolveSpecColor(const ColorSpec &spec, const QColor bar[RCount], const QVector<Theme> &palette) {
  switch (spec.source) {
    case SpecSource::Inherit:
      return QColor();
    case SpecSource::Bar:
      return bar[spec.role];
    case SpecSource::Theme: {
      if (spec.theme < 0 || spec.theme >= palette.size()) return QColor();
      return QColor(palette.at(spec.theme).hex[spec.role]);
    }
    case SpecSource::Custom:
      return spec.custom.isValid() ? spec.custom : QColor(Qt::gray);
  }
  return QColor();
}

bool ModuleStyle::isDefault() const {
  return !fill.isSet() && !text.isSet() && !border.isSet() && stateText.isEmpty() &&
         stateFill.isEmpty() && borderWidth == 1 && radius < 0 && padding < 0 && margin < 0 &&
         fontSize == 0 && weight == 0 && !italic;
}

ResolvedModule resolveModule(const ModuleStyle &ms, const Resolved &bar, const QVector<Theme> &palette) {
  ResolvedModule out;
  QColor bg = resolveSpecColor(ms.fill, bar.c, palette);
  QColor fg = resolveSpecColor(ms.text, bar.c, palette);
  QColor bd = resolveSpecColor(ms.border, bar.c, palette);
  if (bg.isValid()) {
    out.hasFill = true;
    out.fill = bg;
  }
  if (bd.isValid()) {
    out.hasBorder = true;
    out.border = bd;
  }
  QList<QColor> refs;
  if (out.hasFill)
    refs << out.fill;
  else {
    refs << bar.c[RBackground];
    if (!QString::isEmpty()) refs << bar.c[RSurface];
  }
  auto worst = [&](const QColor &c) {
    double k = 1e9;
    for (const QColor &ref : refs) k = std::min(k, contrastRatio(c, ref));
    return k;
  };
  auto fix = [&](QColor c) {
    for (const QColor &ref : refs) c = liftToContrast(c, ref, 4.5);
    if (worst(c) < 4.49)
      c = worst(QColor(Qt::white)) >= worst(QColor(Qt::black)) ? QColor(Qt::white) : QColor(Qt::black);
    return c;
  };
  if (fg.isValid()) {
    out.hasText = true;
    out.text = fix(fg);
  } else if (out.hasFill) {
    out.hasText = true;
    out.text = fix(bar.c[RText]);
  }
  out.borderWidth = ms.borderWidth;
  out.radius = ms.radius;
  out.padding = ms.padding;
  out.margin = ms.margin;
  out.fontSize = ms.fontSize;
  out.weight = ms.weight == 1 ? 700 : (ms.weight == 2 ? 300 : 0);
  out.italic = ms.italic;
  return out;
}

QStringList styledModules(const QJsonObject &barObject) {
  static const char *keys[3] = {"modules-left", "modules-center", "modules-right"};
  QStringList out;
  for (const char *k : keys)
    for (const QJsonValue &v : barObject.value(QLatin1String(k)).toArray())
      if (v.isString() && !out.contains(v.toString())) out << v.toString();
  return out;
}

// ---- persistence ----------------------------------------------------------

namespace {

QJsonObject specToJson(const ColorSpec &s) {
  QJsonObject o;
  o.insert(QStringLiteral("src"), int(s.source));
  o.insert(QStringLiteral("role"), s.role);
  o.insert(QStringLiteral("theme"), s.theme);
  o.insert(QStringLiteral("color"), s.custom.name(QColor::HexRgb));
  return o;
}

ColorSpec specFromJson(const QJsonValue &v) {
  ColorSpec s;
  const QJsonObject o = v.toObject();
  s.source = static_cast<SpecSource>(o.value(QStringLiteral("src")).toInt());
  s.role = qBound(0, o.value(QStringLiteral("role")).toInt(RText), RCount - 1);
  s.theme = o.value(QStringLiteral("theme")).toInt(-1);
  s.custom = QColor(o.value(QStringLiteral("color")).toString());
  if (!s.custom.isValid()) s.custom = QColor("#808080");
  return s;
}

QJsonObject moduleStyleToJson(const ModuleStyle &m) {
  QJsonObject o;
  o.insert(QStringLiteral("fill"), specToJson(m.fill));
  o.insert(QStringLiteral("text"), specToJson(m.text));
  o.insert(QStringLiteral("border"), specToJson(m.border));
  o.insert(QStringLiteral("borderWidth"), m.borderWidth);
  o.insert(QStringLiteral("radius"), m.radius);
  o.insert(QStringLiteral("padding"), m.padding);
  o.insert(QStringLiteral("margin"), m.margin);
  o.insert(QStringLiteral("fontSize"), m.fontSize);
  o.insert(QStringLiteral("weight"), m.weight);
  o.insert(QStringLiteral("italic"), m.italic);
  QJsonObject st, sf;
  for (auto it = m.stateText.begin(); it != m.stateText.end(); ++it)
    st.insert(it.key(), specToJson(it.value()));
  for (auto it = m.stateFill.begin(); it != m.stateFill.end(); ++it)
    sf.insert(it.key(), specToJson(it.value()));
  o.insert(QStringLiteral("stateText"), st);
  o.insert(QStringLiteral("stateFill"), sf);
  return o;
}

ModuleStyle moduleStyleFromJson(const QJsonValue &v) {
  ModuleStyle m;
  const QJsonObject o = v.toObject();
  m.fill = specFromJson(o.value(QStringLiteral("fill")));
  m.text = specFromJson(o.value(QStringLiteral("text")));
  m.border = specFromJson(o.value(QStringLiteral("border")));
  m.borderWidth = qBound(0, o.value(QStringLiteral("borderWidth")).toInt(1), 20);
  m.radius = o.value(QStringLiteral("radius")).toInt(-1);
  m.padding = o.value(QStringLiteral("padding")).toInt(-1);
  m.margin = o.value(QStringLiteral("margin")).toInt(-1);
  m.fontSize = o.value(QStringLiteral("fontSize")).toInt(0);
  m.weight = o.value(QStringLiteral("weight")).toInt(0);
  m.italic = o.value(QStringLiteral("italic")).toBool();
  const QJsonObject st = o.value(QStringLiteral("stateText")).toObject();
  for (auto it = st.begin(); it != st.end(); ++it) m.stateText.insert(it.key(), specFromJson(it.value()));
  const QJsonObject sf = o.value(QStringLiteral("stateFill")).toObject();
  for (auto it = sf.begin(); it != sf.end(); ++it) m.stateFill.insert(it.key(), specFromJson(it.value()));
  return m;
}

QColor colorOr(const QJsonValue &v, const QColor &fallback) {
  const QColor c(v.toString());
  return c.isValid() ? c : fallback;
}

}  // namespace

QJsonObject StyleState::toJson() const {
  QJsonObject o;
  o.insert(QStringLiteral("version"), 2);
  o.insert(QStringLiteral("manage"), manageCss);
  o.insert(QStringLiteral("scheme"), themeName);
  QJsonObject cols;
  for (int i = 0; i < RCount; ++i) cols.insert(QLatin1String(ROLE_KEYS[i]), color[i].name(QColor::HexRgb));
  o.insert(QStringLiteral("colors"), cols);
  o.insert(QStringLiteral("fonts"), QJsonArray::fromStringList(fontStack));
  o.insert(QStringLiteral("fontSize"), fontSize);
  o.insert(QStringLiteral("weight"), weight);
  o.insert(QStringLiteral("italic"), italic);
  o.insert(QStringLiteral("radius"), radius);
  o.insert(QStringLiteral("padding"), padding);
  o.insert(QStringLiteral("margin"), margin);
  o.insert(QStringLiteral("borderWidth"), borderWidth);
  o.insert(QStringLiteral("opacity"), opacity);
  o.insert(QStringLiteral("underline"), underlineThickness);
  o.insert(QStringLiteral("underlineAccent"), underlineAccent);
  o.insert(QStringLiteral("underlineLeft"), underlineLeft);
  o.insert(QStringLiteral("underlineCenter"), underlineCenter);
  o.insert(QStringLiteral("underlineRight"), underlineRight);
  o.insert(QStringLiteral("raised"), raised);
  QJsonObject mods;
  for (auto it = modules.begin(); it != modules.end(); ++it)
    mods.insert(it.key(), moduleStyleToJson(it.value()));
  o.insert(QStringLiteral("modules"), mods);
  return o;
}

StyleState StyleState::fromJson(const QJsonObject &o) {
  StyleState s;
  const StyleState dflt;
  s.manageCss = o.value(QStringLiteral("manage")).toBool(dflt.manageCss);
  s.themeName = o.value(QStringLiteral("scheme")).toString(dflt.themeName);
  const QJsonObject cols = o.value(QStringLiteral("colors")).toObject();
  for (int i = 0; i < RCount; ++i)
    s.color[i] = colorOr(cols.value(QLatin1String(ROLE_KEYS[i])), dflt.color[i]);
  const QJsonArray fonts = o.value(QStringLiteral("fonts")).toArray();
  if (!fonts.isEmpty()) {
    s.fontStack.clear();
    for (const QJsonValue &f : fonts) s.fontStack << f.toString();
  }
  s.fontSize = o.value(QStringLiteral("fontSize")).toInt(dflt.fontSize);
  s.weight = o.value(QStringLiteral("weight")).toInt(dflt.weight);
  s.italic = o.value(QStringLiteral("italic")).toBool();
  s.radius = o.value(QStringLiteral("radius")).toInt(dflt.radius);
  s.padding = o.value(QStringLiteral("padding")).toInt(dflt.padding);
  s.margin = o.value(QStringLiteral("margin")).toInt(dflt.margin);
  s.borderWidth = o.value(QStringLiteral("borderWidth")).toInt(dflt.borderWidth);
  s.opacity = qBound(0, o.value(QStringLiteral("opacity")).toInt(dflt.opacity), 100);
  s.underlineThickness = qMax(0, o.value(QStringLiteral("underline")).toInt());
  s.underlineAccent = o.value(QStringLiteral("underlineAccent")).toBool(true);
  s.underlineLeft = o.value(QStringLiteral("underlineLeft")).toBool(true);
  s.underlineCenter = o.value(QStringLiteral("underlineCenter")).toBool(true);
  s.underlineRight = o.value(QStringLiteral("underlineRight")).toBool(true);
  s.raised = o.value(QStringLiteral("raised")).toBool();
  const QJsonObject mods = o.value(QStringLiteral("modules")).toObject();
  for (auto it = mods.begin(); it != mods.end(); ++it)
    s.modules.insert(it.key(), moduleStyleFromJson(it.value()));
  return s;
}

// ---- CSS generation -------------------------------------------------------

QString generateCss(const StyleState &st, const Resolved &r, const QStringList &presentModules) {
  const QVector<Theme> &palette = themes();
  const QString fg = r.c[RText].name();
  const QString accent = r.c[RAccent].name();
  const QString warnFill = r.c[RWarning].name();
  const QString critFill = r.c[RCritical].name();
  const QString warnText = r.stateTextWarning.name();
  const QString critText = r.stateTextCritical.name();
  const QString accentText = r.stateTextAccent.name();
  const QString muted = r.c[RMuted].name();
  const QString good = r.c[RGood].name();
  const QString surface = r.c[RSurface].name();
  const QString barFill = rgba(r.c[RBackground], r.opacity);
  const QString hover = rgba(r.c[RText], 0.16);
  const QString borderColor = rgba(r.c[RText], 0.30);

  const char *sections[3] = {"modules-left", "modules-center", "modules-right"};
  auto joinSections = [&](const QString &suffix, const QString &prefix) {
    QStringList out;
    for (int i = 0; i < 3; ++i) out << prefix + QLatin1String(sections[i]) + suffix;
    return out.join(QStringLiteral(", "));
  };
  auto stateRules = [&](const QString &classes, const QString &decl) {
    QStringList out;
    for (const char *sec : sections)
      for (const QString &c : classes.split(QLatin1Char('|'), Qt::SkipEmptyParts))
        out << QStringLiteral("window#waybar .%1 .%2").arg(QLatin1String(sec), c);
    return out.join(QStringLiteral(", ")) + QStringLiteral(" { ") + decl + QStringLiteral(" }");
  };

  QString css;
  css += QStringLiteral("/* generated by waybar-settings - edit above, not here */\n");
  css += QStringLiteral("* { font-family: %1; }\n").arg(fontStackCss(st.fontStack, QString()));
  css += QStringLiteral("* { font-size: %1px; }\n").arg(st.fontSize);
  if (st.weight != 400)
    css += QStringLiteral("* { font-weight: %1; }\n").arg(weightName(st.weight));
  if (st.italic) css += QStringLiteral("* { font-style: italic; }\n");

  css += QStringLiteral("window#waybar { color: %1; background-color: %2; }\n").arg(fg, barFill);
  css += QStringLiteral("window#waybar.hidden { background-color: %1; }\n").arg(barFill);
  css += QStringLiteral("window#waybar.empty, window#waybar.solo, window#waybar.floating { "
                        "background-color: %1; }\n")
             .arg(barFill);
  css += QStringLiteral("tooltip { background-color: %1; color: %2; border: 1px solid %3; }\n")
             .arg(r.c[RBackground].name(), fg, borderColor);
  css += QStringLiteral("tooltip label { color: %1; }\n").arg(fg);
  css += QStringLiteral("menu, menuitem { background-color: %1; color: %2; }\n")
             .arg(r.c[RBackground].name(), fg);
  css += QStringLiteral("menuitem:hover { background-color: %1; }\n").arg(hover);

  if (st.underlineThickness > 0) {
    const QString c = st.underlineAccent ? accent : fg;
    struct { bool on; const char *name; } parts[3] = {
        {st.underlineLeft, "modules-left"},
        {st.underlineCenter, "modules-center"},
        {st.underlineRight, "modules-right"},
    };
    for (const auto &p : parts)
      if (p.on)
        css += QStringLiteral(".%1 { border-bottom: %2px solid %3; }\n")
                   .arg(QLatin1String(p.name))
                   .arg(st.underlineThickness)
                   .arg(c);
  }

  QString base = QStringLiteral("color: %1; padding: 0 %2px; border-radius: %3px;").arg(fg).arg(st.padding).arg(st.radius);
  if (st.raised) base += QStringLiteral(" background-color: %1; margin: %2px;").arg(surface).arg(st.margin);
  css += joinSections(QStringLiteral(" > widget > *"), QString()) + QStringLiteral(" { ") + base + QStringLiteral(" }\n");
  css += joinSections(QStringLiteral(" button"), QString()) +
         QStringLiteral(" { color: %1; background: transparent; border: none; box-shadow: none; "
                        "text-shadow: none; border-radius: %2px; }\n")
             .arg(fg)
             .arg(st.radius);

  // Per module overrides ---------------------------------------------------
  for (const QString &ref : presentModules) {
    auto it = st.modules.find(ref);
    if (it == st.modules.end()) continue;
    const ModuleStyle &ms = it.value();
    if (ms.isDefault()) continue;
    const ResolvedModule m = resolveModule(ms, r, palette);
    const QString sel = cssSelector(ref);
    QStringList decls;
    if (m.hasFill) decls << QStringLiteral("background-color: %1;").arg(m.fill.name());
    if (m.hasText) decls << QStringLiteral("color: %1;").arg(m.text.name());
    if (m.hasBorder) decls << QStringLiteral("border: %1px solid %2;").arg(qMax(1, m.borderWidth)).arg(m.border.name());
    if (m.radius >= 0) decls << QStringLiteral("border-radius: %1px;").arg(m.radius);
    if (m.padding >= 0) decls << QStringLiteral("padding: 0 %1px;").arg(m.padding);
    if (m.margin >= 0) decls << QStringLiteral("margin: %1px;").arg(m.margin);
    if (m.fontSize > 0) decls << QStringLiteral("font-size: %1px;").arg(m.fontSize);
    const QString w = weightKeyword(m.weight == 700 ? 1 : (m.weight == 300 ? 2 : 0));
    if (!w.isEmpty()) decls << QStringLiteral("font-weight: %1;").arg(w);
    if (m.italic) decls << QStringLiteral("font-style: italic;");
    if (!decls.isEmpty()) css += QStringLiteral("%1 { %2 }\n").arg(sel, decls.join(QLatin1Char(' ')));
    if (m.hasText && m.hasFill) css += QStringLiteral("%1 button { color: %1; }\n").arg(m.text.name());
    for (auto s = ms.stateText.begin(); s != ms.stateText.end(); ++s) {
      if (!s.value().isSet()) continue;
      const QColor c = resolveSpecColor(s.value(), r.c, palette);
      css += QStringLiteral("%1.%2 { color: %3; }\n").arg(sel, s.key(), c.name());
    }
    for (auto s = ms.stateFill.begin(); s != ms.stateFill.end(); ++s) {
      if (!s.value().isSet()) continue;
      const QColor c = resolveSpecColor(s.value(), r.c, palette);
      css += QStringLiteral("%1.%2 { background-color: %3; }\n").arg(sel, s.key(), c.name());
    }
  }

  // Global state classes ---------------------------------------------------
  for (const StateBucket &b : STATE_BUCKETS) {
    const QStringList classes = QString::fromLatin1(b.classes).split(QLatin1Char('|'), Qt::SkipEmptyParts);
    if (b.role == RWarning) {
      css += stateRules(classes.join(QLatin1Char('|')), QStringLiteral("background-color: %1; color: %2;").arg(warnFill, warnText));
    } else if (b.role == RCritical) {
      css += stateRules(classes.join(QLatin1Char('|')), QStringLiteral("background-color: %1; color: %2;").arg(critFill, critText));
    } else if (b.role == RMuted) {
      css += stateRules(classes.join(QLatin1Char('|')), QStringLiteral("color: %1;").arg(muted));
    } else {
      css += stateRules(classes.join(QLatin1Char('|')), QStringLiteral("color: %1;").arg(r.c[b.role].name()));
    }
  }

  // Interaction ------------------------------------------------------------
  css += joinSections(QStringLiteral(" button:hover"), QStringLiteral("window#waybar ")) +
         QStringLiteral(" { background: %1; }\n").arg(hover);
  css += joinSections(QStringLiteral(" button.focused"), QString()) + QStringLiteral(", ") +
         joinSections(QStringLiteral(" button.active"), QString()) +
         QStringLiteral(" { background-color: %1; color: %2; }\n").arg(accent, accentText);
  css += joinSections(QStringLiteral(" button.urgent"), QString()) +
         QStringLiteral(" { background-color: %1; color: %2; }\n").arg(critFill, critText);
  css += joinSections(QStringLiteral(" label.focused"), QString()) + QStringLiteral(", ") +
         joinSections(QStringLiteral(" label.active"), QString()) +
         QStringLiteral(" { color: %1; }\n").arg(accent);

  // Widget specific --------------------------------------------------------
  css += QStringLiteral("#tray > .passive { -gtk-icon-effect: dim; }\n");
  css += QStringLiteral("#tray > .active { background: %1; }\n").arg(hover);
  css += QStringLiteral("#tray > .needs-attention { background-color: %1; }\n").arg(critFill);
  css += QStringLiteral("#keyboard-state label { color: %1; }\n").arg(muted);
  css += QStringLiteral("#keyboard-state label.locked { color: %1; }\n").arg(accent);
  css += QStringLiteral("#privacy-item { padding: 0 2px; }\n");
  css += QStringLiteral("#privacy-item .audio-in { color: %1; }\n").arg(good);
  css += QStringLiteral("#privacy-item .audio-out { color: %1; }\n").arg(warnFill);
  css += QStringLiteral("#privacy-item .location { color: %1; }\n").arg(accent);
  css += QStringLiteral("#privacy-item .screenshare { color: %1; }\n").arg(critFill);
  for (const char *slider : {"pulseaudio-slider", "backlight-slider"})
    css += QStringLiteral("#%1 slider { border: none; border-radius: %2px; min-height: 6px; }\n")
               .arg(QLatin1String(slider))
               .arg(st.radius);
  css += QStringLiteral("#pulseaudio-slider trough, #backlight-slider trough { "
                        "background-color: %1; border-radius: %2px; min-height: 6px; min-width: 100px; }\n")
             .arg(rgba(r.c[RText], 0.25), 4);
  css += QStringLiteral("#pulseaudio-slider highlight, #backlight-slider highlight { "
                        "background-color: %1; border-radius: %2px; }\n")
             .arg(accent, 4);
  css += QStringLiteral("#pulseaudio-slider.muted trough { opacity: 0.5; }\n");
  css += QStringLiteral(".drawer { border-radius: %1px; }\n").arg(st.radius);
  css += QStringLiteral(".drawer-child { padding: 0 2px; }\n");
  css += QStringLiteral("#cpu_graph { -gtk-icon-shadow: none; }\n");

  QString header = QStringLiteral("/* ws scheme=\"%1\" fonts=\"%2\" size=%3 alpha=%4 radius=%5 pad=%6 ")
                       .arg(st.themeName, st.fontStack.join(QLatin1Char('|')))
                       .arg(st.fontSize)
                       .arg(st.opacity)
                       .arg(st.radius)
                       .arg(st.padding);
  header += QStringLiteral("margin=%1 border=%2 weight=%3 italic=%4 raised=%5 underline=%6 ulaccent=%7 "
                           "ulleft=%8 ulcenter=%9 ulright=%10")
                .arg(st.margin)
                .arg(st.borderWidth)
                .arg(st.weight)
                .arg(st.italic ? 1 : 0)
                .arg(st.raised ? 1 : 0)
                .arg(st.underlineThickness)
                .arg(st.underlineAccent ? 1 : 0)
                .arg(st.underlineLeft ? 1 : 0)
                .arg(st.underlineCenter ? 1 : 0)
                .arg(st.underlineRight ? 1 : 0);
  for (int i = 0; i < RCount; ++i)
    header += QStringLiteral(" %1=%2").arg(QLatin1String(ROLE_KEYS[i]), st.color[i].name(QColor::HexRgb));
  header += QStringLiteral(" */\n");
  return header + css;
}

}  // namespace ws
