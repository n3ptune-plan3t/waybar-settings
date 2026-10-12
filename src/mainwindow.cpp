#include "mainwindow.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QShortcut>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>

namespace ws {
namespace {

const QString BEGIN = QStringLiteral("/* waybar-settings:begin */");
const QString END = QStringLiteral("/* waybar-settings:end */");
const char *SECTION_KEYS[3] = {"modules-left", "modules-center", "modules-right"};

// Transcribed from man/waybar.5.scd.in and src/bar.cpp.
QVector<Opt> barOptions() {
  const char *G = "Placement";
  const char *L = "Look";
  const char *B = "Behaviour";
  const char *A = "Advanced";
  const auto s = [](const char *k, const char *d, const char *h, const char *g) {
    return Opt{k, OptKind::String, d, h, QString(), g};
  };
  const auto ch = [](const char *k, const char *d, const char *choices, const char *h, const char *g) {
    return Opt{k, OptKind::Choice, d, h, choices, g};
  };
  const auto bo = [](const char *k, bool d, const char *h, const char *g) {
    return Opt{k, OptKind::Bool, d ? "true" : "false", h, QString(), g};
  };
  const auto in = [](const char *k, const char *h, const char *g) {
    return Opt{k, OptKind::Int, QString(), h, QString(), g};
  };
  QVector<Opt> v;
  v << s("name", "", "Also a CSS class on the window and the layer-shell namespace", G);
  v << s("output", "", "Output name, make/model/serial, \"!name\" to exclude, \"*\" as wildcard", G);
  v << ch("position", "top", "top|bottom|left|right", "Where the bar sits", G);
  v << ch("layer", "bottom", "bottom|top|overlay", "Wayland layer", G);
  v << ch("mode", "", "default|dock|hide|invisible|overlay", "A Waybar preset, or a custom mode from \"modes\"", L);
  v << in("height", "Bar height in pixels; unset means automatic", G);
  v << in("width", "Bar width for a vertical bar; unset means automatic", G);
  v << in("spacing", "Gap between the module groups", G);
  v << s("margin", "", "CSS shorthand without units, e.g. \"5 10\" or \"4 0 0 8\"", G);
  v << in("margin-top", "Overrides margin for this edge", G);
  v << in("margin-right", "Overrides margin for this edge", G);
  v << in("margin-bottom", "Overrides margin for this edge", G);
  v << in("margin-left", "Overrides margin for this edge", G);
  v << bo("expand-left", false, "Let the left group grow", L);
  v << bo("expand-center", false, "Let the center group grow", L);
  v << bo("expand-right", false, "Let the right group grow", L);
  v << bo("no-center", false, "Skip the center group entirely", L);
  v << Opt{"fixed-center", OptKind::Bool, "true",
           "Place the center group in the real window centre instead of letting it grow",
           QString(), L};
  v << bo("exclusive", true, "Reserve screen space so windows do not cover the bar", B);
  v << bo("passthrough", false, "Let clicks pass through to whatever is behind the bar", B);
  v << Opt{"gtk-layer-shell", OptKind::Bool, "",
           "Use gtk-layer-shell instead of the X11 fallback", QString(), B};
  v << bo("start_hidden", false, "Start hidden until the first IPC command", B);
  v << Opt{"reload_style_on_change", OptKind::Bool, "", "Reload style.css when it changes on disk",
           QString(), B};
  v << bo("ipc", false, "Accept sway bar IPC commands", A);
  v << s("id", "", "sway bar id; empty becomes bar-0", A);
  v << ch("on-sigusr1", "toggle", "show|hide|toggle|reload|noop", "What SIGUSR1 does", A);
  v << ch("on-sigusr2", "reload", "show|hide|toggle|reload|noop", "What SIGUSR2 does", A);
  v << s("output-dimensions", "", "Only outputs whose size matches, e.g. \"1920x1080>1920\"", A);
  v << Opt{"modes", OptKind::Json, "",
           "Custom bar modes, e.g. {\"quiet\":{\"layer\":\"overlay\",\"exclusive\":false}}",
           QString(), A};
  v << Opt{"include", OptKind::Json, "", "Nested config files to merge in", QString(), A};
  return v;
}

}  // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  buildUi();
  load();
}

void MainWindow::buildUi() {
  setWindowTitle(tr("Waybar settings"));
  resize(1200, 860);

  auto *central = new QWidget;
  auto *v = new QVBoxLayout(central);
  v->setContentsMargins(8, 8, 8, 8);

  auto *toolbar = new QHBoxLayout;
  toolbar->addWidget(new QLabel(tr("Bar")));
  barSel_ = new QComboBox;
  barSel_->setMinimumWidth(220);
  toolbar->addWidget(barSel_);
  addBar_ = new QPushButton(tr("New bar"));
  removeBar_ = new QPushButton(tr("Delete bar"));
  toolbar->addWidget(addBar_);
  toolbar->addWidget(removeBar_);
  toolbar->addStretch();
  auto *reload = new QPushButton(tr("Discard and reload"));
  save_ = new QPushButton(tr("Save"));
  saveReload_ = new QPushButton(tr("Save and reload Waybar"));
  toolbar->addWidget(reload);
  toolbar->addWidget(save_);
  toolbar->addWidget(saveReload_);
  v->addLayout(toolbar);

  pathLabel_ = new QLabel;
  pathLabel_->setStyleSheet(QStringLiteral("color: palette(mid);"));
  pathLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  v->addWidget(pathLabel_);

  tabs_ = new QTabWidget;
  v->addWidget(tabs_, 1);

  layout_ = new LayoutPage;
  tabs_->addTab(layout_, tr("Modules"));

  auto *barPage = new QWidget;
  auto *barScroll = new QScrollArea;
  barScroll->setWidgetResizable(true);
  barScroll->setFrameShape(QFrame::NoFrame);
  barScroll->setWidget(barPage);
  auto *bv = new QVBoxLayout(barPage);
  barForm_ = new OptForm;
  barForm_->setOptions(barOptions());
  bv->addWidget(barForm_);
  auto *extraBox = new QGroupBox(tr("Anything else (raw JSON)"));
  auto *ev = new QVBoxLayout(extraBox);
  barExtra_ = new JsonEditor;
  barExtra_->setMinimumHeight(90);
  ev->addWidget(barExtra_);
  bv->addWidget(extraBox);
  bv->addStretch(1);
  tabs_->addTab(barScroll, tr("Bar"));

  stylePage_ = new StylePage;
  tabs_->addTab(stylePage_, tr("Style"));

  preview_ = new BarPreview;
  preview_->setMinimumHeight(180);
  v->addWidget(preview_);
  setCentralWidget(central);
  statusBar()->showMessage(tr("Ready"));

  connect(barSel_, &QComboBox::currentIndexChanged, this, [this](int i) {
    if (loading_ || i < 0) return;
    commitCurrentModule();
    pushSectionsIntoNode();
    commitBarPage();
    selectBar(i);
  });

  connect(addBar_, &QPushButton::clicked, this, [this] {
    commitCurrentModule();
    pushSectionsIntoNode();
    commitBarPage();
    auto node = JsoncDoc::makeObject();
    JsoncDoc::setValue(node, "layer", QJsonValue("top"));
    JsoncDoc::setValue(node, "position", QJsonValue("top"));
    bars_.append(node);
    setDirty(true);
    selectBar(bars_.size() - 1);
    setDirty(true);
  });

  connect(removeBar_, &QPushButton::clicked, this, [this] {
    if (bars_.size() <= 1) {
      QMessageBox::information(this, tr("Waybar settings"),
                               tr("A configuration always needs at least one bar."));
      return;
    }
    commitCurrentModule();
    pushSectionsIntoNode();
    commitBarPage();
    if (QMessageBox::question(this, tr("Waybar settings"), tr("Delete this bar?")) != QMessageBox::Yes)
      return;
    bars_.removeAt(current_);
    current_ = qMax(0, current_ - 1);
    ensureRootArray();
    selectBar(current_);
    setDirty(true);
  });

  connect(reload, &QPushButton::clicked, this, [this] {
    if (dirty_ && QMessageBox::question(this, tr("Waybar settings"),
                                        tr("Throw away the unsaved changes and reload from disk?")) !=
                      QMessageBox::Yes)
      return;
    load();
  });
  connect(save_, &QPushButton::clicked, this, [this] { save(false); });
  connect(saveReload_, &QPushButton::clicked, this, [this] { save(true); });

  connect(layout_, &LayoutPage::changed, this, [this] {
    if (loading_) return;
    commitCurrentModule();
    pushSectionsIntoNode();
    setDirty(true);
    syncPreview();
  });
  connect(layout_, &LayoutPage::previewChanged, this, [this] {
    if (loading_) return;
    syncPreview();
  });
  connect(layout_, &LayoutPage::moduleSelected, this, &MainWindow::onModuleSelected);

  connect(barForm_, &OptForm::changed, this, [this] {
    if (loading_) return;
    commitBarPage();
    setDirty(true);
    syncPreview();
  });
  connect(barExtra_, &JsonEditor::changed, this, [this] {
    if (loading_) return;
    setDirty(true);
  });
  connect(stylePage_, &StylePage::changed, this, [this] {
    if (loading_) return;
    StyleState s = stylePage_->state();
    s.modules = style_.modules;
    style_ = s;
    setDirty(true);
    syncPreview();
  });
  connect(stylePage_, &StylePage::jumpToModule, this, [this](const QString &ref) {
    tabs_->setCurrentIndex(0);
    layout_->selectModule(ref);
  });

  auto *shortcut = new QShortcut(QKeySequence::Save, this);
  connect(shortcut, &QShortcut::activated, this, [this] { save(false); });
}

int MainWindow::barHeight() const {
  const QJsonObject o = JsoncDoc::toObject(bar_);
  const QJsonValue h = o.value(QStringLiteral("height"));
  return h.isDouble() ? qBound(18, h.toInt(), 90) : 34;
}

QStringList MainWindow::presentModules() const {
  QStringList out;
  for (const NodePtr &b : bars_) {
    const QJsonObject o = JsoncDoc::toObject(b);
    for (const char *k : SECTION_KEYS)
      for (const QJsonValue &v : o.value(QLatin1String(k)).toArray())
        if (v.isString() && !out.contains(v.toString())) out << v.toString();
  }
  return out;
}

void MainWindow::syncPreview() {
  // Module style overrides are edited on the modules page, so the style page
  // gets them pushed back into it rather than the other way round.
  StyleState s = stylePage_->state();
  s.modules = style_.modules;
  stylePage_->setModules(s.modules);
  style_ = s;
  if (ModuleForm *f = layout_->moduleForm()) {
    f->setStyleContext(&style_);
    f->setBarColors(style_.color);
  }
  preview_->setState(&style_, &stylePage_->resolved());
  preview_->setSections(layout_->modulesIn(0), layout_->modulesIn(1), layout_->modulesIn(2));
  preview_->setBarHeight(barHeight());
  preview_->setHighlight(layout_->currentModule());
}

void MainWindow::onModuleSelected(const QString &ref) {
  if (ref.isEmpty() || !bar_) return;
  loading_ = true;
  ModuleForm *f = layout_->moduleForm();
  f->load(JsoncDoc::toObject(JsoncDoc::member(bar_, ref)), style_.modules.value(ref));
  loading_ = false;
  syncPreview();
}

void MainWindow::commitCurrentModule() {
  if (loading_) return;
  const QString ref = layout_->currentModule();
  if (ref.isEmpty() || !bar_) return;
  QJsonObject opts;
  ModuleStyle ms;
  layout_->moduleForm()->store(&opts, &ms);
  if (opts.isEmpty())
    JsoncDoc::removeMember(bar_, ref);
  else
    JsoncDoc::setValue(bar_, ref, opts);
  if (ms.isDefault())
    style_.modules.remove(ref);
  else
    style_.modules.insert(ref, ms);
}

void MainWindow::commitBarPage() {
  if (loading_ || !bar_) return;
  const QJsonObject known = barForm_->store();
  for (const Opt &o : barOptions()) {
    if (known.contains(o.key))
      JsoncDoc::setValue(bar_, o.key, known.value(o.key));
    else
      JsoncDoc::removeMember(bar_, o.key);
  }
  const QJsonValue rest = barExtra_->value();
  if (rest.isObject()) {
    // Hold one QJsonObject: begin() and end() of two separate toObject()
    // temporaries never compare equal, so the loop ran off the end.
    const QJsonObject restObj = rest.toObject();
    for (auto it = restObj.begin(); it != restObj.end(); ++it)
      JsoncDoc::setValue(bar_, it.key(), it.value());
  }
}

void MainWindow::pushSectionsIntoNode() {
  if (loading_ || !bar_) return;
  QJsonObject obj;
  layout_->store(&obj);
  for (const char *k : SECTION_KEYS) {
    const QString key = QLatin1String(k);
    QStringList refs;
    for (const QJsonValue &v : obj.value(key).toArray()) refs << v.toString();
    if (refs.isEmpty()) {
      JsoncDoc::removeMember(bar_, key);
      continue;
    }
    NodePtr arr = JsoncDoc::member(bar_, key);
    if (!arr || arr->type != JType::Array) arr = JsoncDoc::ensureMember(bar_, key);
    JsoncDoc::setStringList(arr, refs);
  }
}

void MainWindow::ensureRootArray() {
  QList<NodePtr> items;
  for (const NodePtr &b : bars_) items << b;
  if (singleMode_ && items.size() <= 1) return;
  singleMode_ = false;
  doc_.setRoot(JsoncDoc::makeRootArray(items));
}

void MainWindow::refreshBarList() {
  barSel_->blockSignals(true);
  barSel_->clear();
  for (int i = 0; i < bars_.size(); ++i) {
    const QJsonObject o = JsoncDoc::toObject(bars_.at(i));
    QString label = o.value(QStringLiteral("name")).toString();
    if (label.isEmpty()) label = o.value(QStringLiteral("output")).toString();
    if (label.isEmpty()) label = o.value(QStringLiteral("position")).toString();
    if (label.isEmpty()) label = tr("bar");
    barSel_->addItem(tr("%1: %2").arg(i + 1).arg(label));
  }
  barSel_->setCurrentIndex(current_);
  barSel_->blockSignals(false);
  removeBar_->setEnabled(bars_.size() > 1);
}

void MainWindow::selectBar(int index) {
  if (bars_.isEmpty()) return;
  loading_ = true;
  current_ = qBound(0, index, bars_.size() - 1);
  bar_ = bars_.at(current_);
  const QJsonObject obj = JsoncDoc::toObject(bar_);

  layout_->load(obj);

  QJsonObject rest = obj;
  for (const Opt &o : barOptions()) rest.remove(o.key);
  // Module sections belong to the modules page, not here.
  for (const char *k : SECTION_KEYS)
    for (const QJsonValue &v : obj.value(QLatin1String(k)).toArray()) rest.remove(v.toString());
  barForm_->load(obj);
  barExtra_->setValue(rest);

  loading_ = false;
  refreshBarList();
  layout_->moduleForm()->setStyleContext(&style_);
  layout_->moduleForm()->setBarColors(style_.color);

  // Open the first module so the editor is never empty.
  QString first;
  for (int i = 0; i < 3 && first.isEmpty(); ++i) {
    const QStringList m = layout_->modulesIn(i);
    if (!m.isEmpty()) first = m.first();
  }
  if (!first.isEmpty()) {
    layout_->selectModule(first);
    onModuleSelected(first);
  }
  syncPreview();
}

void MainWindow::setDirty(bool dirty) {
  dirty_ = dirty;
  setWindowTitle(tr("Waybar settings%1").arg(dirty_ ? QStringLiteral(" *") : QString()));
}

void MainWindow::readStyleFromCss() {
  QFile cf(cssPath_);
  if (!cf.open(QIODevice::ReadOnly)) return;
  const QString css = QString::fromUtf8(cf.readAll());
  cf.close();
  const int a = css.indexOf(BEGIN), b = css.indexOf(END);
  if (a < 0 || b <= a) return;

  StyleState s;
  QMap<QString, QString> kv;
  const QRegularExpression re(QStringLiteral("(\\w+)=(?:\"([^\"]*)\"|(\\S+))"));
  QRegularExpressionMatchIterator it = re.globalMatch(css.mid(a, b - a));
  while (it.hasNext()) {
    const QRegularExpressionMatch m = it.next();
    kv.insert(m.captured(1),
              m.captured(2).isEmpty() ? m.captured(3) : m.captured(2));
  }
  for (int i = 0; i < RCount; ++i) {
    const QString c = kv.value(QLatin1String(roleKey(i)));
    const QColor col(c);
    if (col.isValid()) s.color[i] = col;
  }
  if (kv.contains(QStringLiteral("scheme"))) s.themeName = kv.value(QStringLiteral("scheme"));
  if (kv.contains(QStringLiteral("fonts")))
    s.fontStack = kv.value(QStringLiteral("fonts")).split(QLatin1Char('|'), Qt::SkipEmptyParts);
  if (kv.contains(QStringLiteral("size"))) s.fontSize = kv.value(QStringLiteral("size")).toInt();
  if (kv.contains(QStringLiteral("alpha"))) s.opacity = kv.value(QStringLiteral("alpha")).toInt();
  if (kv.contains(QStringLiteral("radius"))) s.radius = kv.value(QStringLiteral("radius")).toInt();
  if (kv.contains(QStringLiteral("pad"))) s.padding = kv.value(QStringLiteral("pad")).toInt();
  s.manageCss = true;
  style_ = s;
}

void MainWindow::load() {
  dir_ = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/waybar");
  cssPath_ = dir_ + QStringLiteral("/style.css");
  stylePath_ = dir_ + QStringLiteral("/waybar-settings.json");
  configPath_ = dir_ + QStringLiteral("/config.jsonc");
  for (const char *n : {"config.jsonc", "config.json", "config"}) {
    const QString candidate = dir_ + QLatin1Char('/') + QLatin1String(n);
    if (QFile::exists(candidate)) {
      configPath_ = candidate;
      break;
    }
  }

  bars_.clear();
  bar_.reset();
  parseOk_ = true;

  QString err;
  QString text;
  QFile f(configPath_);
  if (f.open(QIODevice::ReadOnly)) {
    text = QString::fromUtf8(f.readAll());
    f.close();
  }
  if (text.trimmed().isEmpty()) text = QStringLiteral("{}");
  if (!doc_.parse(text, &err)) {
    parseOk_ = false;
    QMessageBox::warning(this, tr("Waybar settings"),
                         tr("%1 cannot be parsed:\n%2\n\nSaving is disabled so nothing is lost.")
                             .arg(configPath_, err));
    doc_ = JsoncDoc();
    doc_.parse(QStringLiteral("{}"), &err);
  }

  const NodePtr root = doc_.root();
  if (root && root->type == JType::Array) {
    singleMode_ = false;
    for (const NodePtr &item : root->items)
      if (item->type == JType::Object) bars_.append(item);
  } else {
    singleMode_ = true;
    bars_.clear();
    bars_.append(root && root->type == JType::Object ? root : JsoncDoc::makeObject());
    if (!root) doc_.setRoot(bars_.at(0));
  }
  if (bars_.isEmpty()) {
    bars_.append(JsoncDoc::makeObject());
    if (singleMode_) doc_.setRoot(bars_.at(0));
  }

  style_ = StyleState();
  QFile sf(stylePath_);
  if (sf.open(QIODevice::ReadOnly)) {
    const QJsonObject o = QJsonDocument::fromJson(sf.readAll()).object();
    sf.close();
    if (!o.isEmpty()) style_ = StyleState::fromJson(o);
  }
  if (!QFile::exists(stylePath_)) {
    readStyleFromCss();
    QFile probe(cssPath_);
    if (!probe.open(QIODevice::ReadOnly)) style_.manageCss = false;
  }

  stylePage_->setState(style_);
  pathLabel_->setText(configPath_);
  current_ = 0;
  selectBar(0);
  setDirty(false);
  save_->setEnabled(parseOk_);
  saveReload_->setEnabled(parseOk_);
  statusBar()->showMessage(
      parseOk_ ? tr("Loaded %1").arg(configPath_) : tr("%1 could not be parsed").arg(configPath_));
}

bool MainWindow::writeFile(const QString &path, const QByteArray &data, bool keepBackup) {
  QDir().mkpath(QFileInfo(path).path());
  if (keepBackup && QFile::exists(path)) {
    QFile::remove(path + QStringLiteral(".bak"));
    QFile::copy(path, path + QStringLiteral(".bak"));
  }
  QFile out(path);
  if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    QMessageBox::critical(this, tr("Error"), tr("Cannot write %1").arg(path));
    return false;
  }
  return out.write(data) == data.size();
}

bool MainWindow::save(bool reloadWaybar) {
  if (!parseOk_) return false;
  if ((barForm_ && barForm_->hasInvalidJson()) || !barExtra_->isValid() ||
      (layout_->moduleForm() && layout_->moduleForm()->hasInvalidJson())) {
    QMessageBox::warning(this, tr("Waybar settings"),
                         tr("One of the raw JSON boxes does not parse. Fix the highlighted box "
                            "before saving; nothing was written."));
    return false;
  }
  commitCurrentModule();
  commitBarPage();
  pushSectionsIntoNode();
  if (bars_.size() > 1) ensureRootArray();
  syncPreview();  // folds the module page overrides back into style_

  const QString text = doc_.render();

  // Never hand Waybar something we cannot read back.
  JsoncDoc verify;
  QString err;
  if (!verify.parse(text, &err)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("The generated configuration is not valid JSON (%1).\nNothing was written.")
                              .arg(err));
    return false;
  }
  if (!writeFile(configPath_, text.toUtf8(), true)) return false;

  if (!writeFile(stylePath_, QJsonDocument(style_.toJson()).toJson(QJsonDocument::Indented), true))
    return false;

  const QRegularExpression block(QStringLiteral("\\n*%1[\\s\\S]*?%2\\n*")
                                      .arg(QRegularExpression::escape(BEGIN),
                                           QRegularExpression::escape(END)));
  QFile cf(cssPath_);
  QString css;
  if (cf.open(QIODevice::ReadOnly)) {
    css = QString::fromUtf8(cf.readAll());
    cf.close();
  }

  if (style_.manageCss) {
    css.remove(block);
    const QString generated = generateCss(style_, stylePage_->resolved(), presentModules());
    const QString body = css.trimmed().isEmpty() ? QString() : css.trimmed() + QStringLiteral("\n\n");
    const QString out = body + BEGIN + QStringLiteral("\n") + generated + END + QStringLiteral("\n");
    if (!writeFile(cssPath_, out.toUtf8(), true)) return false;
  } else if (css.contains(BEGIN)) {
    css.remove(block);
    if (!writeFile(cssPath_, css.toUtf8(), true)) return false;
  }

  setDirty(false);
  if (reloadWaybar)
    QProcess::startDetached(QStringLiteral("pkill"),
                            {QStringLiteral("-USR2"), QStringLiteral("waybar")});
  statusBar()->showMessage(reloadWaybar ? tr("Saved and reloaded Waybar")
                                        : tr("Saved %1").arg(configPath_),
                           5000);
  return true;
}

void MainWindow::closeEvent(QCloseEvent *e) {
  if (dirty_ &&
      QMessageBox::question(this, tr("Waybar settings"), tr("There are unsaved changes. Close anyway?")) !=
          QMessageBox::Yes) {
    e->ignore();
    return;
  }
  e->accept();
}

}  // namespace ws
