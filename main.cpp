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

struct Resolved { QColor bg, fg, ac, acTx, wn, wnTx, cr, crTx, gd; double alpha; QStringList notes; };

// col: bg, fg, accent, warning, critical, good. Whatever the scheme/custom colors are, the result is
// guaranteed: text >= 4.5:1 on the bar (also over any wallpaper), state fills >= 3:1 against the bar,
// text on every fill >= 4.5:1.
static Resolved resolve(const QColor col[6], int alphaPct) {
  Resolved r; r.bg = col[0];
  r.fg = lift(col[1], r.bg, 4.5);
  if (r.fg != col[1])
    r.notes << QString("text color adjusted (%1:1 -> %2:1)").arg(contrast(col[1], r.bg), 0, 'f', 1).arg(contrast(r.fg, r.bg), 0, 'f', 1);
  r.ac = lift(col[2], r.bg, 3.0); r.wn = lift(col[3], r.bg, 3.0); r.cr = lift(col[4], r.bg, 3.0);
  if (r.ac != col[2] || r.wn != col[3] || r.cr != col[4]) r.notes << "accent/warning/critical adjusted for visibility";
  r.gd = lift(col[5], r.bg, 4.5);
  if (r.gd != col[5]) r.notes << "good color adjusted for readability";
  r.acTx = textOn(r.ac, r.bg, r.fg); r.wnTx = textOn(r.wn, r.bg, r.fg); r.crTx = textOn(r.cr, r.bg, r.fg);
  double a = alphaPct / 100.0, minA = 1.0;
  for (int p = 100; p >= 0; p--) {  // lowest opacity such that every opacity above it is also readable
    double k = p / 100.0; bool good = true;
    for (QColor t : {QColor(Qt::black), QColor(Qt::white)}) good = good && contrast(r.fg, mix(t, r.bg, k)) >= 4.5;
    if (!good) break;
    minA = k;
  }
  if (a < minA) { a = minA; r.notes << QString("opacity raised to %1% so text stays readable over any wallpaper").arg(qRound(minA * 100)); }
  r.alpha = a;
  return r;
}
static QString previewHtml(const Resolved &r, const QColor &backdrop) {
  QColor eff = mix(backdrop, r.bg, r.alpha);
  auto sp = [](const QColor &bg, const QColor &fg, const QString &t) {
    return QString("<span style=\"background-color:%1;color:%2\">&nbsp;%3&nbsp;</span>&nbsp;").arg(bg.name(), fg.name(), t); };
    return sp(eff, r.fg, "12:34") + sp(r.ac, r.acTx, "1") + sp(eff, r.fg, "2") + sp(r.wn, r.wnTx, "bat 20%") +
    sp(r.cr, r.crTx, "bat 5%") + sp(eff, r.gd, "charging");
}

class Win : public QMainWindow {
  QString cfgPath, cssPath;
  QJsonArray bars; bool single = true, ok = true, loading = false;
  QJsonObject bar; int cur = 0;
  QComboBox *barSel, *addSel, *modSel;
  QTabWidget *tabs;
  QList<Field> fields;
  QListWidget *lists[3];
  const char *lkeys[3] = {"modules-left", "modules-center", "modules-right"};
  QTableWidget *tbl; QString curMod;
  QCheckBox *manage; QFontComboBox *font;
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

  void buildModTab() {
    auto *w = new QWidget; auto *v = new QVBoxLayout(w);
    addSel = new QComboBox; addSel->setEditable(true);
    addSel->addItems({"clock", "battery", "cpu", "memory", "disk", "load", "temperature", "network", "bluetooth",
      "pulseaudio", "wireplumber", "backlight", "tray", "idle_inhibitor", "power-profiles-daemon", "upower", "mpris",
      "mpd", "keyboard-state", "user", "cava", "gamemode", "privacy", "systemd-failed-units", "image",
      "hyprland/workspaces", "hyprland/window", "hyprland/language", "hyprland/submap",
      "niri/workspaces", "niri/window", "niri/language", "sway/workspaces", "sway/window", "sway/mode",
      "river/tags", "river/window", "ext/workspaces", "dwl/tags", "dwl/window", "custom/example"});
    v->addWidget(new QLabel("Add module (type any name, e.g. custom/foo, clock#2). Drag to reorder / move between sections."));
    v->addWidget(addSel);
    auto *h = new QHBoxLayout; v->addLayout(h, 1);
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
        auto t = addSel->currentText().trimmed(); if (!t.isEmpty()) lists[i]->addItem(t); });
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
    auto r = resolve(col, alpha->value());
    prevDark->setText(previewHtml(r, Qt::black)); prevLight->setText(previewHtml(r, Qt::white));
    QString n = QString("Text contrast %1:1 (minimum enforced: 4.5:1)").arg(contrast(r.fg, r.bg), 0, 'f', 1);
    if (!r.notes.isEmpty()) n += "\nAuto-adjusted: " + r.notes.join("; ");
    note->setText(n);
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
    font = new QFontComboBox; f->addRow("Font", font);
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
    applyScheme(SCHEMES[0]);
    tabs->addTab(w, "Style");
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
    commitLists(); commitOptions();
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
    loading = false;
  }

  void load() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/waybar";
    cssPath = dir + "/style.css"; cfgPath = dir + "/config.jsonc";
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
      if (k == "font") font->setCurrentFont(QFont(v));
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
    if (manage->isChecked()) {
      QString old; QFile f(cssPath);
      if (f.open(QIODevice::ReadOnly)) { old = QString::fromUtf8(f.readAll()); f.close(); }
      old.remove(QRegularExpression(QString("\\n*%1.*?%2\\n*").arg(QRegularExpression::escape(BEGIN), QRegularExpression::escape(END)),
                                    QRegularExpression::DotMatchesEverythingOption));
      QString fam = font->currentFont().family();
      Resolved r = resolve(col, alpha->value());
      auto rgba = [](const QColor &c, double al) {
        return QString("rgba(%1, %2, %3, %4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(al, 0, 'f', 2); };
        QString hdr = QString("/* ws scheme=\"%1\" font=\"%2\" size=%3 alpha=%4 radius=%5 pad=%6")
        .arg(schemeSel->currentText(), fam).arg(fsize->value()).arg(alpha->value()).arg(radius->value()).arg(pad->value());
        const char *ck[6] = {"bg", "fg", "ac", "wn", "cr", "gd"};
        for (int i = 0; i < 6; i++) hdr += QString(" %1=%2").arg(ck[i], col[i].name());
        hdr += " */\n";
      QString css = QString::fromUtf8(R"(* { font-family: "@FAM@"; font-size: @SIZE@px; }
window#waybar { background-color: @BAR@; color: @FG@; }
tooltip { background-color: @BG@; color: @FG@; border: 1px solid @BORDER@; }
tooltip label { color: @FG@; }
.modules-left > widget > *, .modules-center > widget > *, .modules-right > widget > * { padding: 0 @PAD@px; border-radius: @RAD@px; }
#workspaces button, #tags button { color: @FG@; background: transparent; border: none; box-shadow: none; text-shadow: none; }
#workspaces button:hover, #tags button:hover { background: @HOVER@; }
#workspaces button.focused, #workspaces button.active, #tags button.focused { background: @AC@; color: @ACTX@; }
#workspaces button.urgent, #tags button.urgent { background: @CR@; color: @CRTX@; }
#battery.warning, #cpu.warning, #memory.warning, #disk.warning, #load.warning, #temperature.warning, #network.warning { background-color: @WN@; color: @WNTX@; }
#battery.critical, #cpu.critical, #memory.critical, #disk.critical, #load.critical, #temperature.critical, #network.critical { background-color: @CR@; color: @CRTX@; }
#battery.charging, #battery.plugged { background-color: transparent; color: @GD@; }
      )");
      css.replace("@FAM@", fam).replace("@SIZE@", QString::number(fsize->value()))
      .replace("@BAR@", rgba(r.bg, r.alpha)).replace("@BG@", r.bg.name()).replace("@FG@", r.fg.name())
      .replace("@BORDER@", rgba(r.fg, 0.35)).replace("@HOVER@", rgba(r.fg, 0.18))
      .replace("@PAD@", QString::number(pad->value())).replace("@RAD@", QString::number(radius->value()))
      .replace("@ACTX@", r.acTx.name()).replace("@AC@", r.ac.name())
      .replace("@WNTX@", r.wnTx.name()).replace("@WN@", r.wn.name())
      .replace("@CRTX@", r.crTx.name()).replace("@CR@", r.cr.name()).replace("@GD@", r.gd.name());
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
    buildBarTab(); buildModTab(); buildOptTab(); buildStyleTab();
    auto *h = new QHBoxLayout; v->addLayout(h);
    auto *rl = new QPushButton("Reload from disk");
    saveBtn = new QPushButton("Save"); applyBtn = new QPushButton("Save && Reload Waybar");
    h->addWidget(rl); h->addStretch(); h->addWidget(saveBtn); h->addWidget(applyBtn);
    connect(rl, &QPushButton::clicked, this, [this] { load(); });
    connect(saveBtn, &QPushButton::clicked, this, [this] { save(false); });
    connect(applyBtn, &QPushButton::clicked, this, [this] { save(true); });
    connect(tabs, &QTabWidget::currentChanged, this, [this](int i) { if (i == 2) refreshMods(); });
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
