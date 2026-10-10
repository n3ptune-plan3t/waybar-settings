// waybar-settings: Qt6 Widgets settings manager for Waybar 0.15.0
// Edits ~/.config/waybar/config.jsonc (bar options, module layout, per-module options)
// and an optional managed block in style.css. Unknown keys are preserved.
#include <QtWidgets>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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
  QPushButton *bgBtn, *fgBtn; QColor bg{"#1e1e2e"}, fg{"#cdd6f4"};
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

  void buildStyleTab() {
    auto *w = new QWidget; auto *f = new QFormLayout(w);
    manage = new QCheckBox("Manage a settings block in style.css"); f->addRow(manage);
    font = new QFontComboBox; f->addRow("Font", font);
    fsize = new QSpinBox; fsize->setRange(6, 72); fsize->setValue(13); f->addRow("Font size (px)", fsize);
    bgBtn = new QPushButton; fgBtn = new QPushButton;
    f->addRow("Background", bgBtn); f->addRow("Text color", fgBtn);
    alpha = new QSpinBox; alpha->setRange(0, 100); alpha->setValue(90); f->addRow("Background opacity %", alpha);
    radius = new QSpinBox; radius->setRange(0, 50); f->addRow("Module radius (px)", radius);
    pad = new QSpinBox; pad->setRange(0, 50); pad->setValue(8); f->addRow("Module padding (px)", pad);
    connect(bgBtn, &QPushButton::clicked, this, [this] {
      auto c = QColorDialog::getColor(bg, this); if (c.isValid()) { bg = c; colorBtn(bgBtn, bg); } });
    connect(fgBtn, &QPushButton::clicked, this, [this] {
      auto c = QColorDialog::getColor(fg, this); if (c.isValid()) { fg = c; colorBtn(fgBtn, fg); } });
    colorBtn(bgBtn, bg); colorBtn(fgBtn, fg);
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
      else if (k == "bg") { bg = QColor(v); colorBtn(bgBtn, bg); }
      else if (k == "fg") { fg = QColor(v); colorBtn(fgBtn, fg); }
    }
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
      QString blk = QString("\n\n%1\n/* ws font=\"%2\" size=%3 bg=%4 alpha=%5 fg=%6 radius=%7 pad=%8 */\n")
        .arg(BEGIN, fam).arg(fsize->value()).arg(bg.name()).arg(alpha->value()).arg(fg.name()).arg(radius->value()).arg(pad->value());
      blk += QString("* { font-family: \"%1\"; font-size: %2px; }\n").arg(fam).arg(fsize->value());
      blk += QString("window#waybar { background-color: rgba(%1, %2, %3, %4); color: %5; }\n")
        .arg(bg.red()).arg(bg.green()).arg(bg.blue()).arg(alpha->value() / 100.0, 0, 'f', 2).arg(fg.name());
      blk += QString(".modules-left > widget > *, .modules-center > widget > *, .modules-right > widget > * "
                     "{ padding: 0 %1px; border-radius: %2px; }\n%3\n").arg(pad->value()).arg(radius->value()).arg(END);
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
