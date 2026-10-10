// waybar-settings: Qt6 Widgets settings manager for Waybar 0.15.0
// Edits ~/.config/waybar/config.jsonc (bar options, module layout, per-module options)
// and an optional managed block in style.css. Unknown keys are preserved.
#include <QtWidgets>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

static QByteArray stripJsonc(const QByteArray &in) {
  QByteArray o; int n = in.size(); bool str = false;
  for (int i = 0; i < n; i++) {
    char c = in[i];
    if (str) { o += c; if (c == '\\' && i + 1 < n) o += in[++i]; else if (c == '"') str = false; continue; }
    if (c == '"') { str = true; o += c; continue; }
    if (c == '/' && i + 1 < n && in[i + 1] == '/') { while (i < n && in[i] != '\n') i++; o += '\n'; continue; }
    if (c == '/' && i + 1 < n && in[i + 1] == '*') { i += 2; while (i + 1 < n && !(in[i] == '*' && in[i + 1] == '/')) i++; i++; continue; }
    o += c;
  }
  QByteArray r; str = false;
  for (int i = 0; i < o.size(); i++) {
    char c = o[i];
    if (str) { r += c; if (c == '\\' && i + 1 < o.size()) r += o[++i]; else if (c == '"') str = false; continue; }
    if (c == '"') { str = true; r += c; continue; }
    if (c == ',') { int j = i + 1; while (j < o.size() && isspace((uchar)o[j])) j++; if (j < o.size() && (o[j] == '}' || o[j] == ']')) continue; }
    r += c;
  }
  return r;
}

static QJsonValue parseVal(const QString &t) {
  QJsonParseError e;
  auto d = QJsonDocument::fromJson("[" + t.toUtf8() + "]", &e);
  if (e.error == QJsonParseError::NoError && d.array().size() == 1) return d.array().at(0);
  return QJsonValue(t);
}
static QString showVal(const QJsonValue &v) {
  QString s = QJsonDocument(QJsonArray{v}).toJson(QJsonDocument::Compact);
  return s.mid(1, s.size() - 2);
}

struct Field { QString key; QWidget *w; };
static const QString BEGIN = "/* waybar-settings:begin */";
static const QString END = "/* waybar-settings:end */";

// ---- color schemes + readability guarantees (WCAG contrast) ----
static double lum(const QColor &c) {
  auto f = [](double v) { return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
  return 0.2126 * f(c.redF()) + 0.7152 * f(c.greenF()) + 0.0722 * f(c.blueF());
}
static double contrast(const QColor &a, const QColor &b) {
  double x = lum(a), y = lum(b); if (x < y) std::swap(x, y);
  return (x + 0.05) / (y + 0.05);
}
static QColor mix(const QColor &a, const QColor &b, double t) {  // t=0 -> a, t=1 -> b
  return QColor::fromRgbF(a.redF() * (1 - t) + b.redF() * t, a.greenF() * (1 - t) + b.greenF() * t,
                          a.blueF() * (1 - t) + b.blueF() * t);
}
// Move c toward white or black (whichever contrasts more with ref) until contrast(c, ref) >= min.
static QColor lift(const QColor &c, const QColor &ref, double min) {
  if (contrast(c, ref) >= min) return c;
  QColor t = contrast(Qt::white, ref) >= contrast(Qt::black, ref) ? QColor(Qt::white) : QColor(Qt::black);
  for (double k = 0.05; k <= 1.0001; k += 0.05) { QColor n = mix(c, t, k); if (contrast(n, ref) >= min) return n; }
  return t;
}
// Readable (>= 4.5:1) text color on a filled background: prefer scheme colors, else black/white.
static QColor textOn(const QColor &fill, const QColor &bg, const QColor &fg) {
  for (QColor c : {bg, fg}) if (contrast(c, fill) >= 4.5) return c;
  return contrast(Qt::black, fill) >= contrast(Qt::white, fill) ? QColor(Qt::black) : QColor(Qt::white);
}

struct Scheme { const char *name, *bg, *fg, *ac, *wn, *cr, *gd; };
static const Scheme SCHEMES[] = {
  {"Catppuccin Mocha",     "#1e1e2e", "#cdd6f4", "#89b4fa", "#f9e2af", "#f38ba8", "#a6e3a1"},
  {"Catppuccin Macchiato", "#24273a", "#cad3f5", "#8aadf4", "#eed49f", "#ed8796", "#a6da95"},
  {"Catppuccin Frappe",    "#303446", "#c6d0f5", "#8caaee", "#e5c890", "#e78284", "#a6d189"},
  {"Catppuccin Latte",     "#eff1f5", "#4c4f69", "#1e66f5", "#df8e1d", "#d20f39", "#40a02b"},
  {"Gruvbox Dark",         "#282828", "#ebdbb2", "#83a598", "#fabd2f", "#fb4934", "#b8bb26"},
  {"Gruvbox Light",        "#fbf1c7", "#3c3836", "#076678", "#b57614", "#9d0006", "#79740e"},
  {"Nord",                 "#2e3440", "#eceff4", "#88c0d0", "#ebcb8b", "#bf616a", "#a3be8c"},
  {"Dracula",              "#282a36", "#f8f8f2", "#bd93f9", "#f1fa8c", "#ff5555", "#50fa7b"},
  {"Tokyo Night",          "#1a1b26", "#c0caf5", "#7aa2f7", "#e0af68", "#f7768e", "#9ece6a"},
  {"Solarized Dark",       "#002b36", "#93a1a1", "#268bd2", "#b58900", "#dc322f", "#859900"},
  {"Solarized Light",      "#fdf6e3", "#586e75", "#268bd2", "#b58900", "#dc322f", "#859900"},
  {"Rose Pine",            "#191724", "#e0def4", "#c4a7e7", "#f6c177", "#eb6f92", "#9ccfd8"},
  {"One Dark",             "#282c34", "#abb2bf", "#61afef", "#e5c07b", "#e06c75", "#98c379"},
  {"Everforest Dark",      "#2d353b", "#d3c6aa", "#7fbbb3", "#dbbc7f", "#e67e80", "#a7c080"},
  {"Kanagawa",             "#1f1f28", "#dcd7ba", "#7e9cd8", "#e6c384", "#e46876", "#98bb6c"},
};
static const int NS = sizeof(SCHEMES) / sizeof(SCHEMES[0]);

struct Resolved {
  QColor bg, fg, ac, acTx, wn, wnTx, cr, crTx, gd, surface, mute, mod[5];
  double alpha; int style; QStringList notes;
};

static QColor rot(const QColor &c, int deg) {  // hue rotation, keeps lightness
  float h, sat, l, a; c.getHslF(&h, &sat, &l, &a);
  if (h < 0) h = 0;
  h = std::fmod(h + deg / 360.0f, 1.0f);
  return QColor::fromHslF(h, std::max(sat, 0.45f), l);
}

// col: bg, fg, accent, warning, critical, good. style: 0 plain, 1 pills (raised module backgrounds),
// 2 colored (per-module-group text colors). Whatever the colors are, the result is guaranteed:
// every text color >= 4.5:1 on the bar (and on pills), also over any wallpaper (opacity is raised
// if needed), state fills >= 3:1 against the bar, text on every fill >= 4.5:1.
static Resolved resolve(const QColor col[6], int alphaPct, int style, const QList<QColor> &extra = {}) {
  Resolved r; r.style = style; r.bg = col[0];
  QColor fg0 = lift(col[1], r.bg, 4.5);
  if (fg0 != col[1])
    r.notes << QString("text color adjusted (%1:1 -> %2:1)").arg(contrast(col[1], r.bg), 0, 'f', 1).arg(contrast(fg0, r.bg), 0, 'f', 1);
  r.ac = lift(col[2], r.bg, 3.0); r.wn = lift(col[3], r.bg, 3.0); r.cr = lift(col[4], r.bg, 3.0);
  if (r.ac != col[2] || r.wn != col[3] || r.cr != col[4]) r.notes << "accent/warning/critical adjusted for visibility";
  // Text colors must be readable on the bar and (pills) on the pill background.
  auto fix = [&](QColor c, const QColor &sf) {
    c = lift(c, r.bg, 4.5);
    if (style == 1 && contrast(c, sf) < 4.5) c = lift(c, sf, 4.5);
    if (contrast(c, r.bg) < 4.5 || (style == 1 && contrast(c, sf) < 4.5)) {
      auto m = [&](const QColor &e) { return style == 1 ? std::min(contrast(e, r.bg), contrast(e, sf)) : contrast(e, r.bg); };
      c = m(Qt::white) >= m(Qt::black) ? QColor(Qt::white) : QColor(Qt::black);
    }
    return c;
  };
  auto okc = [&](const QColor &c, const QColor &sf) { return contrast(c, r.bg) >= 4.49 && (style != 1 || contrast(c, sf) >= 4.49); };
  bool modAdj = false, gdAdj = false;
  for (double k : {0.10, 0.07, 0.04, 0.0}) {  // pill tint is reduced until all text stays readable
    r.surface = mix(r.bg, fg0, k);
    r.fg = fix(fg0, r.surface); r.gd = fix(col[5], r.surface); r.mute = fix(mix(fg0, r.bg, 0.4), r.surface);
    gdAdj = r.gd != col[5]; modAdj = false;
    for (int i = 0; i < 5; i++) { QColor c = i ? rot(col[2], 72 * i) : QColor(col[2]); r.mod[i] = fix(c, r.surface); modAdj = modAdj || r.mod[i] != c; }
    bool good = okc(r.fg, r.surface) && okc(r.gd, r.surface) && okc(r.mute, r.surface);
    for (auto &m : r.mod) good = good && okc(m, r.surface);
    if (style != 1 || good) { if (style == 1 && k < 0.10) r.notes << "pill tint reduced to keep text readable"; break; }
  }
  if (gdAdj) r.notes << "good color adjusted for readability";
  if (style == 2 && modAdj) r.notes << "module colors adjusted for readability";
  r.acTx = textOn(r.ac, r.bg, r.fg); r.wnTx = textOn(r.wn, r.bg, r.fg); r.crTx = textOn(r.cr, r.bg, r.fg);
  QList<QColor> texts{r.fg, r.gd, r.mute};
  if (style == 2) for (auto &m : r.mod) texts << m;
  texts << extra;
  double a = alphaPct / 100.0, minA = 1.0;
  for (int p = 100; p >= 0; p--) {  // lowest opacity such that every opacity above it is also readable
    double k = p / 100.0; bool good = true;
    for (QColor t : {QColor(Qt::black), QColor(Qt::white)})
      for (auto &tx : texts) good = good && contrast(tx, mix(t, r.bg, k)) >= 4.5;
      if (!good) break;
      minA = k;
  }
  if (a < minA) { a = minA; r.notes << QString("opacity raised to %1% so text stays readable over any wallpaper").arg(qRound(minA * 100)); }
  r.alpha = a;
  return r;
}
static QString previewHtml(const Resolved &r, const QColor &backdrop) {
  QColor eff = mix(backdrop, r.bg, r.alpha);
  QColor mbg = r.style == 1 ? r.surface : eff;
  auto tx = [&](int i) { return r.style == 2 ? r.mod[i] : r.fg; };
  auto sp = [](const QColor &bg, const QColor &fg, const QString &t) {
    return QString("<span style=\"background-color:%1;color:%2\">&nbsp;%3&nbsp;</span>&nbsp;").arg(bg.name(), fg.name(), t); };
    return sp(mbg, tx(0), "12:34") + sp(r.ac, r.acTx, "1") + sp(eff, r.fg, "2") + sp(mbg, tx(1), "cpu 8%") +
    sp(mbg, tx(2), "net") + sp(mbg, tx(3), "vol 40%") + sp(r.wn, r.wnTx, "bat 20%") + sp(r.cr, r.crTx, "bat 5%") +
    sp(mbg, r.gd, "charging");
}


// ---- per-module style overrides ----
// A color can come from: inherit (-1), the bar-wide scheme (-2), any preset scheme (0..NS-1) or a custom color (NS).
struct ColSpec { int src = -1; int role = 0; QColor custom; };  // role: bg, text, accent, warning, critical, good
struct ModStyle {
  ColSpec c[3];  // background, text, border
  int bw = 1, radius = -1, pad = -1, margin = -1, fsize = 0; QString font; bool bold = false;
  ModStyle() { c[1].role = 1; c[2].role = 2; }
  bool isDefault() const {
    return c[0].src == -1 && c[1].src == -1 && c[2].src == -1 && radius == -1 && pad == -1 && margin == -1 &&
    fsize == 0 && font.trimmed().isEmpty() && !bold; }
};
static QColor specColor(const ColSpec &sp, const QColor bar[6]) {
  if (sp.src == -1) return QColor();
  if (sp.src == NS) return sp.custom;
  if (sp.src == -2) return bar[sp.role];
  const Scheme &sc = SCHEMES[sp.src];
  const char *v[6] = {sc.bg, sc.fg, sc.ac, sc.wn, sc.cr, sc.gd};
  return QColor(v[sp.role]);
}
static QString srcName(int sv) { return sv == -1 ? "inherit" : sv == -2 ? "bar" : sv == NS ? "custom" : SCHEMES[sv].name; }
static int srcFromName(const QString &n) {
  if (n == "bar") return -2;
  if (n == "custom") return NS;
  for (int i = 0; i < NS; i++) if (n == SCHEMES[i].name) return i;
  return -1;
}
static QJsonObject msToJson(const ModStyle &m) {
  QJsonObject o; const char *k[3] = {"bg", "fg", "border"};
  for (int i = 0; i < 3; i++)
    o[k[i]] = QJsonObject{{"src", srcName(m.c[i].src)}, {"role", m.c[i].role}, {"color", m.c[i].custom.name()}};
  o["bw"] = m.bw; o["radius"] = m.radius; o["pad"] = m.pad; o["margin"] = m.margin; o["fsize"] = m.fsize;
  o["font"] = m.font; o["bold"] = m.bold;
  return o;
}
static ModStyle msFromJson(const QJsonObject &o) {
  ModStyle m; const char *k[3] = {"bg", "fg", "border"};
  for (int i = 0; i < 3; i++) {
    QJsonObject c = o.value(k[i]).toObject();
    m.c[i].src = srcFromName(c.value("src").toString());
    m.c[i].role = qBound(0, c.value("role").toInt(m.c[i].role), 5);
    m.c[i].custom = QColor(c.value("color").toString());
  }
  m.bw = o.value("bw").toInt(1); m.radius = o.value("radius").toInt(-1); m.pad = o.value("pad").toInt(-1);
  m.margin = o.value("margin").toInt(-1); m.fsize = o.value("fsize").toInt(0);
  m.font = o.value("font").toString(); m.bold = o.value("bold").toBool();
  return m;
}
// Text must stay readable (>= 4.5:1) against every background it can sit on.
static QColor readable(QColor c, const QList<QColor> &refs) {
  for (auto &rf : refs) c = lift(c, rf, 4.5);
  auto minC = [&](const QColor &e) { double k = 1e9; for (auto &rf : refs) k = std::min(k, contrast(e, rf)); return k; };
  if (minC(c) < 4.49) c = minC(Qt::white) >= minC(Qt::black) ? QColor(Qt::white) : QColor(Qt::black);
  return c;
}
struct ModOut { bool hasBg = false, hasFg = false, hasBd = false; QColor bg, fg, bd; };
static ModOut modResolve(const ModStyle &m, const QColor bar[6], const Resolved &r) {
  ModOut o; QColor bg = specColor(m.c[0], bar), fg = specColor(m.c[1], bar), bd = specColor(m.c[2], bar);
  if (bg.isValid()) { o.hasBg = true; o.bg = bg; }
  if (bd.isValid()) { o.hasBd = true; o.bd = bd; }
  QList<QColor> refs;
  if (o.hasBg) refs << o.bg; else { refs << r.bg; if (r.style == 1) refs << r.surface; }
  if (fg.isValid()) { o.hasFg = true; o.fg = readable(fg, refs); }
  else if (o.hasBg) { o.hasFg = true; o.fg = readable(r.fg, refs); }  // inherited text must still suit the new background
  return o;
}
// Waybar CSS selector for a module name: sway/workspaces -> #workspaces, custom/foo -> #custom-foo, clock#2 -> #clock.2
static QString cssSel(const QString &name) {
  QString base = name, alias; int h = name.indexOf('#');
  if (h >= 0) { alias = name.mid(h + 1); base = name.left(h); }
  QString id = base;
  int sl = base.indexOf('/');
  if (sl >= 0) {
    QString pre = base.left(sl), rest = base.mid(sl + 1);
    id = (pre == "custom" || pre == "cffi") ? pre + "-" + rest : (rest == "slider") ? pre + "-slider" : rest;
  }
  return "#" + id + (alias.isEmpty() ? QString() : "." + alias);
}
static QString fontFamilyCss(const QString &text) {
  QStringList q;
  for (auto n : text.split(',', Qt::SkipEmptyParts)) {
    n = n.trimmed().remove('"'); if (n.isEmpty()) continue;
    q << ((n == "sans-serif" || n == "serif" || n == "monospace") ? n : "\"" + n + "\"");
  }
  return q.join(", ");
}

class Win : public QMainWindow {
  QString cfgPath, cssPath, msPath;
  QMap<QString, ModStyle> mstyles; QString curMs; bool msLoading = false;
  QComboBox *msSel; QComboBox *csrc[3], *crole[3]; QPushButton *cbtn3[3]; QColor ccustom[3];
  QSpinBox *msBw, *msRadius, *msPad, *msMargin, *msFsize; QLineEdit *msFont; QCheckBox *msBold; QLabel *msPrev, *msNote;
  QJsonArray bars; bool single = true, ok = true, loading = false;
  QJsonObject bar; int cur = 0;
  QComboBox *barSel, *modSel; QTreeWidget *avail; QLineEdit *customMod;
  QTabWidget *tabs;
  QList<Field> fields;
  QListWidget *lists[3];
  const char *lkeys[3] = {"modules-left", "modules-center", "modules-right"};
  QTableWidget *tbl; QString curMod;
  QCheckBox *manage; QFontComboBox *fontPick; QListWidget *fontList; QComboBox *mstyle;
  QSpinBox *fsize, *alpha, *radius, *pad;
  QComboBox *schemeSel; QPushButton *cbtn[6]; QColor col[6]; QLabel *prevDark, *prevLight, *note;
  QPushButton *saveBtn, *applyBtn;

  void addField(QFormLayout *f, const QString &k, QWidget *w) { f->addRow(k, w); fields << Field{k, w}; }
  QComboBox *combo(const QStringList &o) { auto *c = new QComboBox; c->addItem(""); c->addItems(o); return c; }
  QComboBox *boolBox() { return combo({"true", "false"}); }

  void buildBarTab() {
    auto *w = new QWidget; auto *f = new QFormLayout(w);
    addField(f, "name", new QLineEdit);
    addField(f, "output", new QLineEdit);
    addField(f, "position", combo({"top", "bottom", "left", "right"}));
    addField(f, "layer", combo({"top", "bottom", "overlay"}));
    addField(f, "mode", combo({"dock", "hide", "invisible", "overlay"}));
    for (auto k : {"height", "width", "spacing"}) {
      auto *s = new QSpinBox; s->setRange(-1, 10000); s->setSpecialValueText("(unset)"); addField(f, k, s);
    }
    addField(f, "margin", new QLineEdit);
    for (auto k : {"exclusive", "passthrough", "gtk-layer-shell", "fixed-center", "reload_style_on_change", "start_hidden", "ipc"})
      addField(f, k, boolBox());
      tabs->addTab(w, "Bar");
  }

  // All modules shipped with Waybar 0.15 (compositor-specific ones in their own groups).
  // "custom/NAME", "group/NAME" and "cffi/NAME" are templates: rename NAME after adding.
  void fillAvail() {
    static const struct { const char *cat, *mods; } cats[] = {
      {"System & info", "clock,cpu,memory,disk,load,temperature,gps,user,systemd-failed-units,privacy,gamemode,keyboard-state,idle_inhibitor,inhibitor,image"},
      {"Network", "network,bluetooth,wwan"},
      {"Audio, media & display", "pulseaudio,pulseaudio/slider,wireplumber,backlight,backlight/slider,mpd,mpris,cava,sndio,jack"},
      {"Power", "battery,upower,power-profiles-daemon"},
      {"Tray & user-defined", "tray,custom/example,group/example,cffi/example"},
      {"Sway", "sway/workspaces,sway/window,sway/mode,sway/language,sway/scratchpad"},
      {"Hyprland", "hyprland/workspaces,hyprland/window,hyprland/language,hyprland/submap"},
      {"Niri", "niri/workspaces,niri/window,niri/language"},
      {"River", "river/tags,river/window,river/layout,river/mode"},
      {"DWL", "dwl/tags,dwl/window"},
      {"Mango", "mango/workspaces,mango/window,mango/keymode,mango/layout"},
      {"Generic Wayland (wlroots / ext)", "ext/workspaces,wlr/taskbar"}};
      for (auto &c : cats) {
        auto *top = new QTreeWidgetItem(avail, QStringList(c.cat));
        QFont f = top->font(0); f.setBold(true); top->setFont(0, f);
        top->setFlags(top->flags() & ~Qt::ItemIsSelectable);
        for (auto &m : QString(c.mods).split(',')) new QTreeWidgetItem(top, QStringList(m));
      }
      avail->expandAll();
  }
  QString pickModule() {
    QString t = customMod->text().trimmed();
    if (!t.isEmpty()) { customMod->clear(); return t; }
    auto *it = avail->currentItem();
    return (it && it->parent()) ? it->text(0) : QString();
  }

  void buildModTab() {
    auto *w = new QWidget; auto *top = new QHBoxLayout(w);
    auto *lv = new QVBoxLayout; top->addLayout(lv, 2);
    lv->addWidget(new QLabel("Available modules (select, then Add to a section):"));
    avail = new QTreeWidget; avail->setHeaderHidden(true); lv->addWidget(avail, 1);
    fillAvail();
    customMod = new QLineEdit; customMod->setPlaceholderText("or type any name, e.g. custom/foo, clock#2");
    lv->addWidget(customMod);
    auto *h = new QHBoxLayout; top->addLayout(h, 3);
    const char *names[3] = {"Left", "Center", "Right"};
    for (int i = 0; i < 3; i++) {
      auto *c = new QVBoxLayout; h->addLayout(c);
      c->addWidget(new QLabel(names[i]));
      lists[i] = new QListWidget; lists[i]->setDragDropMode(QAbstractItemView::DragDrop);
      lists[i]->setDefaultDropAction(Qt::MoveAction);
      c->addWidget(lists[i], 1);
      auto *r = new QHBoxLayout; c->addLayout(r);
      auto *a = new QPushButton("Add"), *d = new QPushButton("Remove");
      r->addWidget(a); r->addWidget(d);
      connect(a, &QPushButton::clicked, this, [this, i] {
        auto t = pickModule(); if (!t.isEmpty()) lists[i]->addItem(t); });
      connect(d, &QPushButton::clicked, this, [this, i] { delete lists[i]->takeItem(lists[i]->currentRow()); });
    }
    tabs->addTab(w, "Modules");
  }

  void buildOptTab() {
    auto *w = new QWidget; auto *v = new QVBoxLayout(w);
    modSel = new QComboBox; v->addWidget(modSel);
    tbl = new QTableWidget(0, 2); tbl->setHorizontalHeaderLabels({"Option", "Value (JSON; unquoted text = string)"});
    tbl->horizontalHeader()->setStretchLastSection(true);
    tbl->setColumnWidth(0, 200);
    v->addWidget(tbl, 1);
    auto *r = new QHBoxLayout; v->addLayout(r);
    auto *a = new QPushButton("Add option"), *d = new QPushButton("Remove option");
    r->addWidget(a); r->addWidget(d); r->addStretch();
    connect(a, &QPushButton::clicked, this, [this] { tbl->insertRow(tbl->rowCount()); });
    connect(d, &QPushButton::clicked, this, [this] { tbl->removeRow(tbl->currentRow()); });
    connect(modSel, &QComboBox::currentTextChanged, this, [this](const QString &t) {
      if (loading) return;
      commitOptions(); curMod = t; loadOptions(); });
      tabs->addTab(w, "Module options");
  }

  void colorBtn(QPushButton *b, QColor &c) {
    b->setStyleSheet(QString("background:%1;color:%2").arg(c.name(), c.lightness() > 128 ? "#000" : "#fff"));
    b->setText(c.name());
  }

  void applyScheme(const Scheme &sc) {
    const char *v[6] = {sc.bg, sc.fg, sc.ac, sc.wn, sc.cr, sc.gd};
    for (int i = 0; i < 6; i++) col[i] = QColor(v[i]);
    refreshColors();
  }
  void refreshColors() {
    for (int i = 0; i < 6; i++) colorBtn(cbtn[i], col[i]);
    updatePreview();
  }
  void updatePreview() {
    auto r = resolve(col, alpha->value(), mstyle->currentIndex());
    prevDark->setText(previewHtml(r, Qt::black)); prevLight->setText(previewHtml(r, Qt::white));
    QString n = QString("Text contrast %1:1 (minimum enforced: 4.5:1)").arg(contrast(r.fg, r.bg), 0, 'f', 1);
    if (!r.notes.isEmpty()) n += "\nAuto-adjusted: " + r.notes.join("; ");
    note->setText(n);
    if (msSel) msChanged();
  }

  void buildStyleTab() {
    auto *w = new QWidget; auto *f = new QFormLayout(w);
    manage = new QCheckBox("Manage a settings block in style.css"); f->addRow(manage);
    schemeSel = new QComboBox;
    for (int i = 0; i < NS; i++) schemeSel->addItem(SCHEMES[i].name);
    schemeSel->addItem("Custom");
    f->addRow("Color scheme", schemeSel);
    const char *cn[6] = {"Background", "Text", "Accent (focused workspace)", "Warning", "Critical", "Good (charging)"};
    for (int i = 0; i < 6; i++) {
      cbtn[i] = new QPushButton; f->addRow(cn[i], cbtn[i]);
      connect(cbtn[i], &QPushButton::clicked, this, [this, i] {
        auto c = QColorDialog::getColor(col[i], this); if (!c.isValid()) return;
        col[i] = c;
        schemeSel->blockSignals(true); schemeSel->setCurrentIndex(NS); schemeSel->blockSignals(false);
        refreshColors(); });
    }
    mstyle = new QComboBox; mstyle->addItems({"Plain (scheme text color on every module)", "Pills (raised background per module)", "Colored (distinct color per module group)"});
    f->addRow("Module style", mstyle);
    fontPick = new QFontComboBox; fontPick->setEditable(true);
    auto *addF = new QPushButton("Add"), *delF = new QPushButton("Remove");
    fontList = new QListWidget; fontList->setDragDropMode(QAbstractItemView::InternalMove); fontList->setMaximumHeight(110);
    fontList->addItem(QApplication::font().family());
    auto *fr = new QHBoxLayout; fr->addWidget(fontPick, 1); fr->addWidget(addF); fr->addWidget(delF);
    auto *fv = new QVBoxLayout; fv->addLayout(fr); fv->addWidget(fontList);
    f->addRow("Fonts (fallback order, drag to reorder)", fv);
    connect(addF, &QPushButton::clicked, this, [this] {
      QString t = fontPick->currentText().trimmed();
      if (!t.isEmpty() && fontList->findItems(t, Qt::MatchExactly).isEmpty()) fontList->addItem(t); });
    connect(delF, &QPushButton::clicked, this, [this] { delete fontList->takeItem(fontList->currentRow()); });
    fsize = new QSpinBox; fsize->setRange(6, 72); fsize->setValue(13); f->addRow("Font size (px)", fsize);
    alpha = new QSpinBox; alpha->setRange(0, 100); alpha->setValue(90); f->addRow("Background opacity %", alpha);
    radius = new QSpinBox; radius->setRange(0, 50); f->addRow("Module radius (px)", radius);
    pad = new QSpinBox; pad->setRange(0, 50); pad->setValue(8); f->addRow("Module padding (px)", pad);
    prevDark = new QLabel; prevLight = new QLabel;
    prevDark->setStyleSheet("background:#000;padding:6px"); prevLight->setStyleSheet("background:#fff;padding:6px");
    note = new QLabel; note->setWordWrap(true);
    f->addRow("Preview, dark wallpaper", prevDark); f->addRow("Preview, light wallpaper", prevLight); f->addRow(note);
    connect(schemeSel, &QComboBox::currentIndexChanged, this, [this](int i) { if (i >= 0 && i < NS) applyScheme(SCHEMES[i]); });
    connect(alpha, &QSpinBox::valueChanged, this, [this] { updatePreview(); });
    connect(mstyle, &QComboBox::currentIndexChanged, this, [this] { updatePreview(); });
    applyScheme(SCHEMES[0]);
    tabs->addTab(w, "Style");
  }

  // ---- Module styles tab ----
  int idxFromSrc(int sv) { return sv == -1 ? 0 : sv == -2 ? 1 : sv == NS ? NS + 2 : sv + 2; }
  int srcFromIdx(int i) { return i == 0 ? -1 : i == 1 ? -2 : i == NS + 2 ? NS : i - 2; }
  ColSpec readSpec(int i) {
    ColSpec c; c.src = srcFromIdx(csrc[i]->currentIndex()); c.role = crole[i]->currentIndex(); c.custom = ccustom[i]; return c; }
    ModStyle readModStyle() {
      ModStyle m; for (int i = 0; i < 3; i++) m.c[i] = readSpec(i);
      m.bw = msBw->value(); m.radius = msRadius->value(); m.pad = msPad->value(); m.margin = msMargin->value();
      m.fsize = msFsize->value(); m.font = msFont->text(); m.bold = msBold->isChecked();
      return m; }
      void msChanged() {
        if (msLoading) return;
        for (int i = 0; i < 3; i++) {
          int ix = csrc[i]->currentIndex();
          crole[i]->setEnabled(ix >= 1 && ix <= NS + 1);
          QColor c = specColor(readSpec(i), col);
          if (c.isValid()) colorBtn(cbtn3[i], c); else { cbtn3[i]->setStyleSheet(""); cbtn3[i]->setText("(inherit)"); }
        }
        ModStyle m = readModStyle();
        Resolved r = resolve(col, alpha->value(), mstyle->currentIndex());
        ModOut o = modResolve(m, col, r);
        QColor bgc = o.hasBg ? o.bg : r.bg, fgc = o.hasFg ? o.fg : r.fg;
        QString ss = QString("background:%1;color:%2;padding:4px %3px;border-radius:%4px;").arg(bgc.name(), fgc.name())
        .arg(m.pad >= 0 ? m.pad : pad->value()).arg(m.radius >= 0 ? m.radius : radius->value());
        if (o.hasBd) ss += QString("border:%1px solid %2;").arg(qMax(1, m.bw)).arg(o.bd.name());
        if (m.bold) ss += "font-weight:bold;";
        if (m.fsize > 0) ss += QString("font-size:%1px;").arg(m.fsize);
        msPrev->setStyleSheet(ss);
        QColor want = specColor(m.c[1], col);
        msNote->setText(want.isValid() && want != o.fg ? "Text color adjusted to stay readable on this background." : QString());
      }
      void commitModStyle() {
        if (curMs.isEmpty()) return;
        ModStyle m = readModStyle();
        if (m.isDefault()) mstyles.remove(curMs); else mstyles[curMs] = m;
      }
      void loadModStyle() {
        msLoading = true;
        ModStyle m = mstyles.value(curMs);
        for (int i = 0; i < 3; i++) {
          csrc[i]->setCurrentIndex(idxFromSrc(m.c[i].src)); crole[i]->setCurrentIndex(m.c[i].role);
          ccustom[i] = m.c[i].custom.isValid() ? m.c[i].custom : QColor("#808080");
        }
        msBw->setValue(m.bw); msRadius->setValue(m.radius); msPad->setValue(m.pad); msMargin->setValue(m.margin);
        msFsize->setValue(m.fsize); msFont->setText(m.font); msBold->setChecked(m.bold);
        msLoading = false; msChanged();
      }
      void refreshStyleMods() {
        commitModStyle();
        QStringList names;
        for (int i = 0; i < 3; i++)
          for (int j = 0; j < lists[i]->count(); j++) { auto t = lists[i]->item(j)->text(); if (!names.contains(t)) names << t; }
          msLoading = true;
        msSel->clear(); msSel->addItems(names);
        int i = msSel->findText(curMs); msSel->setCurrentIndex(i < 0 ? 0 : i);
        curMs = msSel->currentText();
        msLoading = false;
        loadModStyle();
      }
      void buildModStyleTab() {
        auto *w = new QWidget; auto *v = new QVBoxLayout(w);
        v->addWidget(new QLabel("Style one module on its own. Each color can come from the bar scheme, any other scheme, or a custom color."));
        msSel = new QComboBox; v->addWidget(msSel);
        auto *f = new QFormLayout; v->addLayout(f);
        const char *labels[3] = {"Background", "Text", "Border color"};
        for (int i = 0; i < 3; i++) {
          csrc[i] = new QComboBox; csrc[i]->addItems({"(inherit)", "Bar scheme"});
          for (int k = 0; k < NS; k++) csrc[i]->addItem(SCHEMES[k].name);
          csrc[i]->addItem("Custom");
          crole[i] = new QComboBox; crole[i]->addItems({"Background", "Text", "Accent", "Warning", "Critical", "Good"});
          cbtn3[i] = new QPushButton("(inherit)"); ccustom[i] = QColor("#808080");
          auto *row = new QHBoxLayout; row->addWidget(csrc[i], 2); row->addWidget(crole[i], 1); row->addWidget(cbtn3[i], 1);
          f->addRow(labels[i], row);
          connect(csrc[i], &QComboBox::currentIndexChanged, this, [this] { msChanged(); });
          connect(crole[i], &QComboBox::currentIndexChanged, this, [this] { msChanged(); });
          connect(cbtn3[i], &QPushButton::clicked, this, [this, i] {
            QColor start = specColor(readSpec(i), col); if (!start.isValid()) start = ccustom[i];
            auto c = QColorDialog::getColor(start, this); if (!c.isValid()) return;
            ccustom[i] = c; csrc[i]->setCurrentIndex(NS + 2); msChanged(); });
        }
        auto spin = [&](const QString &lbl, int lo, int hi, const QString &special, int val) {
          auto *sb = new QSpinBox; sb->setRange(lo, hi); sb->setValue(val); if (!special.isEmpty()) sb->setSpecialValueText(special);
          f->addRow(lbl, sb); connect(sb, &QSpinBox::valueChanged, this, [this] { msChanged(); }); return sb; };
          msBw = spin("Border width (px)", 0, 20, QString(), 1);
          msRadius = spin("Radius (px)", -1, 50, "(inherit)", -1);
          msPad = spin("Padding (px)", -1, 50, "(inherit)", -1);
          msMargin = spin("Margin (px)", -1, 50, "(inherit)", -1);
          msFsize = spin("Font size (px)", 0, 72, "(inherit)", 0);
          msFont = new QLineEdit; msFont->setPlaceholderText("inherit; or e.g. Noto Sans, Symbols Nerd Font");
          f->addRow("Fonts (fallback order)", msFont);
          connect(msFont, &QLineEdit::textChanged, this, [this] { msChanged(); });
          msBold = new QCheckBox("Bold"); f->addRow(msBold);
          connect(msBold, &QCheckBox::toggled, this, [this] { msChanged(); });
          msPrev = new QLabel("  12:34  sample  "); msPrev->setAlignment(Qt::AlignCenter);
          msNote = new QLabel; msNote->setWordWrap(true);
          f->addRow("Preview", msPrev); f->addRow(msNote);
          auto *reset = new QPushButton("Reset this module to bar style");
          v->addWidget(reset); v->addStretch();
          connect(reset, &QPushButton::clicked, this, [this] { mstyles.remove(curMs); loadModStyle(); });
          connect(msSel, &QComboBox::currentTextChanged, this, [this](const QString &t) {
            if (msLoading) return;
            commitModStyle(); curMs = t; loadModStyle(); });
            tabs->addTab(w, "Module styles");
      }

      void setField(const Field &f, const QJsonObject &b) {
        QJsonValue v = b.value(f.key);
        if (auto *c = qobject_cast<QComboBox *>(f.w)) {
          QString s = v.isBool() ? (v.toBool() ? "true" : "false") : v.toString();
          int i = c->findText(s); c->setCurrentIndex(i < 0 ? 0 : i);
        } else if (auto *s = qobject_cast<QSpinBox *>(f.w)) s->setValue(v.isDouble() ? v.toInt() : -1);
        else if (auto *e = qobject_cast<QLineEdit *>(f.w))
          e->setText(v.isString() ? v.toString() : v.isDouble() ? QString::number(v.toInt()) : QString());
      }
      void getField(const Field &f, QJsonObject &b) {
        if (auto *c = qobject_cast<QComboBox *>(f.w)) {
          QString t = c->currentText();
          if (t.isEmpty()) b.remove(f.key);
          else if (t == "true" || t == "false") b[f.key] = (t == "true");
          else b[f.key] = t;
        } else if (auto *s = qobject_cast<QSpinBox *>(f.w)) {
          if (s->value() < 0) b.remove(f.key); else b[f.key] = s->value();
        } else if (auto *e = qobject_cast<QLineEdit *>(f.w)) {
          QString t = e->text().trimmed();
          if (t.isEmpty()) b.remove(f.key);
          else { bool n; int i = t.toInt(&n); b[f.key] = n ? QJsonValue(i) : QJsonValue(t); }
        }
      }

      void commitLists() {
        for (int i = 0; i < 3; i++) {
          QJsonArray a;
          for (int j = 0; j < lists[i]->count(); j++) a << lists[i]->item(j)->text();
          if (a.isEmpty()) bar.remove(lkeys[i]); else bar[lkeys[i]] = a;
        }
      }
      void commitOptions() {
        if (curMod.isEmpty()) return;
        QJsonObject o;
        for (int r = 0; r < tbl->rowCount(); r++) {
          auto *k = tbl->item(r, 0), *v = tbl->item(r, 1);
          if (k && !k->text().trimmed().isEmpty()) o[k->text().trimmed()] = parseVal(v ? v->text() : "");
        }
        if (o.isEmpty()) bar.remove(curMod); else bar[curMod] = o;
      }
      void commitAll() {
        for (auto &f : fields) getField(f, bar);
        commitLists(); commitOptions(); commitModStyle();
        bars[cur] = bar;
      }
      void loadOptions() {
        tbl->setRowCount(0);
        if (curMod.isEmpty()) return;
        QJsonObject o = bar.value(curMod).toObject();
        for (auto k : o.keys()) {
          int r = tbl->rowCount(); tbl->insertRow(r);
          tbl->setItem(r, 0, new QTableWidgetItem(k));
          tbl->setItem(r, 1, new QTableWidgetItem(showVal(o[k])));
        }
      }
      void refreshMods() {
        commitOptions(); commitLists();
        QStringList names;
        for (int i = 0; i < 3; i++)
          for (int j = 0; j < lists[i]->count(); j++) {
            auto t = lists[i]->item(j)->text(); if (!names.contains(t)) names << t; }
            loading = true;
            modSel->clear(); modSel->addItems(names);
            int i = modSel->findText(curMod); modSel->setCurrentIndex(i < 0 ? 0 : i);
            curMod = modSel->currentText();
            loading = false;
            loadOptions();
      }

      void populate() {
        loading = true;
        for (auto &f : fields) setField(f, bar);
        for (int i = 0; i < 3; i++) {
          lists[i]->clear();
          for (auto v : bar.value(lkeys[i]).toArray()) lists[i]->addItem(v.toString());
        }
        curMod.clear(); modSel->clear(); tbl->setRowCount(0);
        curMs.clear(); msSel->clear();
        loading = false;
      }

      void load() {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/waybar";
        cssPath = dir + "/style.css"; cfgPath = dir + "/config.jsonc"; msPath = dir + "/waybar-settings.json";
        for (auto n : {"config.jsonc", "config.json", "config"})
          if (QFile::exists(dir + "/" + n)) { cfgPath = dir + "/" + n; break; }
          bars = QJsonArray(); ok = true;
          QFile f(cfgPath);
          if (f.open(QIODevice::ReadOnly)) {
            QJsonParseError e;
            auto d = QJsonDocument::fromJson(stripJsonc(f.readAll()), &e);
            if (e.error != QJsonParseError::NoError) {
              ok = false;
              QMessageBox::warning(this, "Waybar settings", "Cannot parse " + cfgPath + ":\n" + e.errorString() + "\nSaving is disabled.");
            } else if (d.isArray()) { bars = d.array(); single = false; }
            else { bars = QJsonArray{d.object()}; single = true; }
          }
          if (bars.isEmpty()) bars = QJsonArray{QJsonObject{{"layer", "top"}, {"position", "top"},
          {"modules-right", QJsonArray{"clock"}}}};
          barSel->blockSignals(true); barSel->clear();
          for (int i = 0; i < bars.size(); i++) {
            auto o = bars[i].toObject();
            barSel->addItem(QString("Bar %1 %2").arg(i + 1).arg(o.value("name").toString(o.value("output").toString())));
          }
          barSel->blockSignals(false);
          cur = 0; bar = bars[0].toObject();
          populate(); loadStyle();
          mstyles.clear();
          { QFile mf(msPath);
            if (mf.open(QIODevice::ReadOnly)) {
              auto md = QJsonDocument::fromJson(mf.readAll()).object().value("modules").toObject();
              for (auto k : md.keys()) mstyles[k] = msFromJson(md[k].toObject());
            } }
            saveBtn->setEnabled(ok); applyBtn->setEnabled(ok);
      }

      void loadStyle() {
        QFile f(cssPath); QString s;
        if (f.open(QIODevice::ReadOnly)) s = QString::fromUtf8(f.readAll());
        int a = s.indexOf(BEGIN), b = s.indexOf(END);
        manage->setChecked(a >= 0 && b > a);
        if (a < 0 || b < a) return;
        auto it = QRegularExpression("(\\w+)=(?:\"([^\"]*)\"|(\\S+))").globalMatch(s.mid(a, b - a));
        while (it.hasNext()) {
          auto m = it.next(); QString k = m.captured(1), v = m.captured(2).isEmpty() ? m.captured(3) : m.captured(2);
          if (k == "fonts" || k == "font") {
            fontList->clear();
            for (auto &x : v.split('|', Qt::SkipEmptyParts)) fontList->addItem(x);
          } else if (k == "mstyle") mstyle->setCurrentIndex(qBound(0, v.toInt(), 2));
          else if (k == "size") fsize->setValue(v.toInt());
          else if (k == "alpha") alpha->setValue(v.toInt());
          else if (k == "radius") radius->setValue(v.toInt());
          else if (k == "pad") pad->setValue(v.toInt());
          else if (k == "scheme") {
            int i = schemeSel->findText(v);
            if (i >= 0) { schemeSel->blockSignals(true); schemeSel->setCurrentIndex(i); schemeSel->blockSignals(false); }
          } else {
            static const QStringList ck = {"bg", "fg", "ac", "wn", "cr", "gd"};
            int i = ck.indexOf(k); if (i >= 0 && QColor(v).isValid()) col[i] = QColor(v);
          }
        }
        refreshColors();
      }

      bool writeFile(const QString &p, const QByteArray &d) {
        QDir().mkpath(QFileInfo(p).path());
        if (QFile::exists(p)) { QFile::remove(p + ".bak"); QFile::copy(p, p + ".bak"); }
        QFile f(p);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) { QMessageBox::critical(this, "Error", "Cannot write " + p); return false; }
        f.write(d); return true;
      }

      void save(bool reload) {
        commitAll();
        QJsonDocument d = (single && bars.size() == 1) ? QJsonDocument(bars[0].toObject()) : QJsonDocument(bars);
        if (!writeFile(cfgPath, d.toJson(QJsonDocument::Indented))) return;
        { QJsonObject mo; for (auto it = mstyles.begin(); it != mstyles.end(); ++it) mo[it.key()] = msToJson(it.value());
          if (mo.isEmpty() && !QFile::exists(msPath)) {}
          else if (!writeFile(msPath, QJsonDocument(QJsonObject{{"modules", mo}}).toJson(QJsonDocument::Indented))) return; }
          if (manage->isChecked()) {
            QString old; QFile f(cssPath);
            if (f.open(QIODevice::ReadOnly)) { old = QString::fromUtf8(f.readAll()); f.close(); }
            old.remove(QRegularExpression(QString("\\n*%1.*?%2\\n*").arg(QRegularExpression::escape(BEGIN), QRegularExpression::escape(END)),
                                          QRegularExpression::DotMatchesEverythingOption));
            QStringList fl, q; bool generic = false;
            for (int i = 0; i < fontList->count(); i++) {
              QString n = fontList->item(i)->text().trimmed().remove('"'); if (n.isEmpty()) continue;
              bool g = n == "sans-serif" || n == "serif" || n == "monospace";
              generic = generic || g; fl << n; q << (g ? n : "\"" + n + "\"");
            }
            if (!generic) q << "sans-serif";
            // Modules present in any bar that have their own style.
            QStringList present;
            for (auto bv : bars) for (auto k : lkeys) for (auto mv : bv.toObject().value(k).toArray())
              if (mstyles.contains(mv.toString()) && !present.contains(mv.toString())) present << mv.toString();
              Resolved r0 = resolve(col, alpha->value(), mstyle->currentIndex());
            QList<QColor> extra;  // module text sitting directly on the bar must stay readable over any wallpaper
            for (auto &nm : present) { ModOut o = modResolve(mstyles[nm], col, r0); if (o.hasFg && !o.hasBg) extra << o.fg; }
            Resolved r = resolve(col, alpha->value(), mstyle->currentIndex(), extra);
            auto rgba = [](const QColor &c, double al) {
              return QString("rgba(%1, %2, %3, %4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(al, 0, 'f', 2); };
              QString hdr = QString("/* ws scheme=\"%1\" fonts=\"%2\" size=%3 alpha=%4 radius=%5 pad=%6 mstyle=%7")
              .arg(schemeSel->currentText(), fl.join('|')).arg(fsize->value()).arg(alpha->value()).arg(radius->value()).arg(pad->value()).arg(mstyle->currentIndex());
              const char *ck[6] = {"bg", "fg", "ac", "wn", "cr", "gd"};
              for (int i = 0; i < 6; i++) hdr += QString(" %1=%2").arg(ck[i], col[i].name());
              hdr += " */\n";
            // Selectors covering every module (any module type, custom/*, groups) in all three sections.
            auto sel = [](const QString &suf, const QString &pre = QString()) {
              QStringList l; for (auto c : {".modules-left", ".modules-center", ".modules-right"}) l << pre + c + suf;
              return l.join(", "); };
              auto st = [&](const QStringList &classes, const QString &decl) {
                QStringList l;
                for (auto c : {".modules-left", ".modules-center", ".modules-right"})
                  for (auto &k : classes) l << "window#waybar " + QString(c) + " ." + k;
                  return l.join(", ") + " { " + decl + " }\n"; };
                const QString P = "window#waybar ";
                bool pills = r.style == 1;
                QString css;
                css += "* { font-family: " + q.join(", ") + "; font-size: @SIZE@px; }\n";
                css += "window#waybar { background-color: @BAR@; color: @FG@; }\n";
                css += "tooltip { background-color: @BG@; color: @FG@; border: 1px solid @BORDER@; }\ntooltip label { color: @FG@; }\n";
                css += "menu, menuitem { background-color: @BG@; color: @FG@; }\nmenuitem:hover { background-color: @HOVER@; }\n";
                css += sel(" > widget > *") + " { color: @FG@; padding: 0 @PAD@px; border-radius: @RAD@px;" +
                (pills ? " background-color: @SURF@; margin: 2px;" : "") + " }\n";
                css += sel(" button") + " { color: @FG@; background: transparent; border: none; box-shadow: none; text-shadow: none; border-radius: @RAD@px; }\n";
                if (r.style == 2) {
                  static const struct { const char *ids; int c; } groups[] = {
                    {"clock,user,window,language,mode,submap,scratchpad,keyboard-state,privacy,systemd-failed-units,image", 0},
                    {"cpu,memory,disk,load,temperature,gps", 1},
                    {"network,bluetooth,wwan", 2},
                    {"pulseaudio,wireplumber,backlight,mpris,mpd,cava,sndio,jack", 3},
                    {"battery,upower,power-profiles-daemon,idle_inhibitor,inhibitor,gamemode", 4}};
                    for (auto &g : groups) {
                      QStringList l; for (auto &id : QString(g.ids).split(',')) l << "#" + id;
                      css += l.join(", ") + " { color: @M" + QString::number(g.c) + "@; }\n";
                    }
                }
                for (auto &nm : present) {  // per-module overrides (win over the bar-wide rules above)
                  const ModStyle &m = mstyles[nm]; ModOut o = modResolve(m, col, r);
                  QString ms = cssSel(nm); QStringList d;
                  if (o.hasBg) d << "background-color: " + o.bg.name() + ";";
                  if (o.hasFg) d << "color: " + o.fg.name() + ";";
                  if (o.hasBd) d << QString("border: %1px solid %2;").arg(qMax(1, m.bw)).arg(o.bd.name());
                  if (m.radius >= 0) d << QString("border-radius: %1px;").arg(m.radius);
                  if (m.pad >= 0) d << QString("padding: 0 %1px;").arg(m.pad);
                  if (m.margin >= 0) d << QString("margin: %1px;").arg(m.margin);
                  QString ff = fontFamilyCss(m.font);
                  if (!ff.isEmpty()) d << "font-family: " + ff + ";";
                  if (m.fsize > 0) d << QString("font-size: %1px;").arg(m.fsize);
                  if (m.bold) d << "font-weight: bold;";
                  if (d.isEmpty()) continue;
                  css += "/* module " + nm + " */\n" + ms + " { " + d.join(" ") + " }\n";
                  if (o.hasFg) css += ms + " button { color: " + o.fg.name() + "; }\n";
                }
                css += sel(" button:hover", P) + " { background: @HOVER@; }\n";
                css += st({"muted", "disabled", "disconnected", "off", "paused", "stopped", "empty", "deactivated"}, "color: @MUTE@;");
                css += st({"charging", "plugged", "full", "connected", "activated", "playing", "on", "performance"}, "color: @GD@;");
                css += sel(" button.focused", P) + ", " + sel(" button.active", P) + " { background: @AC@; color: @ACTX@; }\n";
                css += st({"warning"}, "background-color: @WN@; color: @WNTX@;");
                css += st({"critical", "urgent"}, "background-color: @CR@; color: @CRTX@;");
                css += "#tray > .passive { -gtk-icon-effect: dim; }\n#tray > .needs-attention { background-color: @CR@; }\n";
                css.replace("@SIZE@", QString::number(fsize->value()))
                .replace("@BAR@", rgba(r.bg, r.alpha)).replace("@BG@", r.bg.name()).replace("@FG@", r.fg.name())
                .replace("@BORDER@", rgba(r.fg, 0.35)).replace("@HOVER@", rgba(r.fg, 0.18))
                .replace("@SURF@", r.surface.name()).replace("@MUTE@", r.mute.name())
                .replace("@PAD@", QString::number(pad->value())).replace("@RAD@", QString::number(radius->value()))
                .replace("@ACTX@", r.acTx.name()).replace("@AC@", r.ac.name())
                .replace("@WNTX@", r.wnTx.name()).replace("@WN@", r.wn.name())
                .replace("@CRTX@", r.crTx.name()).replace("@CR@", r.cr.name()).replace("@GD@", r.gd.name());
                for (int i = 0; i < 5; i++) css.replace("@M" + QString::number(i) + "@", r.mod[i].name());
                QString blk = "\n\n" + BEGIN + "\n" + hdr + css + END + "\n";
            if (!writeFile(cssPath, (old.trimmed() + blk).toUtf8())) return;
          }
          if (reload) QProcess::startDetached("pkill", {"-USR2", "waybar"});
          statusBar()->showMessage(reload ? "Saved and reloaded Waybar" : "Saved", 4000);
      }

public:
  Win() {
    setWindowTitle("Waybar Settings (Waybar 0.15.0)"); resize(760, 600);
    auto *cw = new QWidget; auto *v = new QVBoxLayout(cw); setCentralWidget(cw);
    barSel = new QComboBox; v->addWidget(barSel);
    tabs = new QTabWidget; v->addWidget(tabs, 1);
    buildBarTab(); buildModTab(); buildOptTab(); buildModStyleTab(); buildStyleTab();
    auto *h = new QHBoxLayout; v->addLayout(h);
    auto *rl = new QPushButton("Reload from disk");
    saveBtn = new QPushButton("Save"); applyBtn = new QPushButton("Save && Reload Waybar");
    h->addWidget(rl); h->addStretch(); h->addWidget(saveBtn); h->addWidget(applyBtn);
    connect(rl, &QPushButton::clicked, this, [this] { load(); });
    connect(saveBtn, &QPushButton::clicked, this, [this] { save(false); });
    connect(applyBtn, &QPushButton::clicked, this, [this] { save(true); });
    connect(tabs, &QTabWidget::currentChanged, this, [this](int i) { if (i == 2) refreshMods(); else if (i == 3) refreshStyleMods(); else if (i == 4) msChanged(); });
    connect(barSel, &QComboBox::currentIndexChanged, this, [this](int i) {
      if (loading || i < 0) return;
      commitAll(); cur = i; bar = bars[i].toObject(); populate(); });
      load();
  }
};

int main(int argc, char **argv) {
  QApplication a(argc, argv);
  Win w; w.show();
  return a.exec();
}
