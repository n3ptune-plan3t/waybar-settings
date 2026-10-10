#include "stylepage.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

namespace ws {
namespace {

const char *ROLE_TOOLTIPS[RCount] = {
    "The bar itself. Waybar draws it with an alpha channel, so a partly transparent background "
    "still has to stay readable.",
    "Default label colour.",
    "Focused and active buttons, and the underline when it uses the accent.",
    "The .warning class, for example a hot CPU or a nearly empty battery.",
    "The .critical and .urgent classes.",
    "Connected, charging, playing and other good news.",
    "Disabled, muted, empty and everything else that should recede.",
    "Background behind a module when the raised (pill) look is on.",
};

QString contrastBadge(const QColor &a, const QColor &b) {
  return QStringLiteral("%1:1").arg(contrastRatio(a, b), 0, 'f', 1);
}

}  // namespace

StylePage::StylePage(QWidget *parent) : QWidget(parent) {
  auto *outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);
  auto *scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  outer->addWidget(scroll);
  auto *host = new QWidget;
  scroll->setWidget(host);
  auto *v = new QVBoxLayout(host);

  auto *manageBox = new QGroupBox(tr("style.css"));
  auto *mv = new QVBoxLayout(manageBox);
  manage_ = new QCheckBox(tr("Manage a generated block in style.css"));
  manage_->setToolTip(tr("Everything outside the waybar-settings markers is left untouched."));
  mv->addWidget(manage_);
  auto *note = new QLabel(tr("The generated block sits between /* waybar-settings:begin */ and "
                             "/* waybar-settings:end */. Remove those two markers to stop managing it."));
  note->setWordWrap(true);
  note->setStyleSheet(QStringLiteral("color: palette(mid);"));
  mv->addWidget(note);
  v->addWidget(manageBox);

  auto *colorBox = new QGroupBox(tr("Colours"));
  auto *cv = new QVBoxLayout(colorBox);
  auto *themeRow = new QHBoxLayout;
  themeRow->addWidget(new QLabel(tr("Theme")));
  theme_ = new QComboBox;
  for (const Theme &t : themes()) theme_->addItem(t.name);
  themeRow->addWidget(theme_, 1);
  cv->addLayout(themeRow);

  auto *grid = new QGridLayout;
  for (int i = 0; i < RCount; ++i) {
    auto *b = new QPushButton;
    b->setMinimumHeight(26);
    b->setToolTip(QString::fromLatin1(ROLE_TOOLTIPS[i]));
    connect(b, &QPushButton::clicked, this, [this, i] {
      const QColor c = QColorDialog::getColor(state_.color[i], this, tr("%1").arg(roleLabel(i)));
      if (!c.isValid()) return;
      loading_ = true;
      theme_->setCurrentIndex(theme_->count() - 1);  // "Custom"
      loading_ = false;
      state_.color[i] = c;
      state_.themeName = QStringLiteral("Custom");
      refresh();
      emit changed();
    });
    swatch_[i] = b;
    grid->addWidget(b, i / 4, i % 4);
  }
  cv->addLayout(grid);
  v->addWidget(colorBox);

  auto *typeBox = new QGroupBox(tr("Typography"));
  auto *tf = new QFormLayout;
  fonts_ = new StringListEditor(tr("Font family, e.g. Inter, sans-serif"));
  fonts_->setToolTip(tr("First match wins. Generic families are never quoted."));
  tf->addRow(tr("Fonts (drag to reorder)"), fonts_);
  auto *sizeRow = new QHBoxLayout;
  fontSize_ = new QSpinBox;
  fontSize_->setRange(6, 96);
  sizeRow->addWidget(fontSize_);
  fontWeight_ = new QComboBox;
  fontWeight_->addItem(tr("normal"), 400);
  fontWeight_->addItem(tr("bold"), 700);
  fontWeight_->addItem(tr("light"), 300);
  sizeRow->addWidget(fontWeight_);
  italic_ = new QCheckBox(tr("italic"));
  sizeRow->addWidget(italic_);
  sizeRow->addStretch();
  tf->addRow(tr("Size"), sizeRow);
  v->addWidget(typeBox);

  auto *metricBox = new QGroupBox(tr("Metrics"));
  auto *mf = new QFormLayout;
  radius_ = new QSpinBox; radius_->setRange(0, 100); radius_->setSuffix(tr(" px"));
  padding_ = new QSpinBox; padding_->setRange(0, 100); padding_->setSuffix(tr(" px"));
  margin_ = new QSpinBox; margin_->setRange(0, 100); margin_->setSuffix(tr(" px"));
  borderWidth_ = new QSpinBox; borderWidth_->setRange(0, 20); borderWidth_->setSuffix(tr(" px"));
  opacity_ = new QSpinBox; opacity_->setRange(0, 100); opacity_->setSuffix(tr(" %"));
  mf->addRow(tr("Module radius"), radius_);
  mf->addRow(tr("Module padding"), padding_);
  mf->addRow(tr("Module margin"), margin_);
  mf->addRow(tr("Default border width"), borderWidth_);
  mf->addRow(tr("Bar opacity"), opacity_);
  raised_ = new QCheckBox(tr("Give every module a raised (pill) background"));
  mf->addRow(raised_);
  v->addWidget(metricBox);

  auto *lineBox = new QGroupBox(tr("Underline"));
  auto *lf = new QFormLayout;
  underline_ = new QSpinBox; underline_->setRange(0, 10); underline_->setSuffix(tr(" px"));
  underlineAccent_ = new QCheckBox(tr("Use the accent colour"));
  underlineLeft_ = new QCheckBox(tr("Left"));
  underlineCenter_ = new QCheckBox(tr("Center"));
  underlineRight_ = new QCheckBox(tr("Right"));
  auto *sections = new QHBoxLayout;
  sections->addWidget(underlineLeft_);
  sections->addWidget(underlineCenter_);
  sections->addWidget(underlineRight_);
  sections->addStretch();
  lf->addRow(tr("Thickness"), underline_);
  lf->addRow(QString(), underlineAccent_);
  lf->addRow(tr("Sections"), sections);
  v->addWidget(lineBox);

  auto *previewBox = new QGroupBox(tr("Readability"));
  auto *pv = new QVBoxLayout(previewBox);
  notes_ = new QLabel;
  notes_->setWordWrap(true);
  pv->addWidget(notes_);
  v->addWidget(previewBox);

  overrides_ = new QWidget;
  overridesLayout_ = new QVBoxLayout(overrides_);
  overridesLayout_->setContentsMargins(0, 0, 0, 0);
  auto *overrideBox = new QGroupBox(tr("Module style overrides"));
  auto *ov = new QVBoxLayout(overrideBox);
  ov->addWidget(overrides_);
  v->addWidget(overrideBox);

  v->addStretch(1);

  const auto emitChanged = [this] {
    if (loading_) return;
    refresh();
    emit changed();
  };
  connect(manage_, &QCheckBox::toggled, this, emitChanged);
  connect(theme_, &QComboBox::currentIndexChanged, this, [this](int i) {
    if (loading_ || i < 0) return;
    applyTheme(i);
    refresh();
    emit changed();
  });
  connect(fonts_, &StringListEditor::changed, this, emitChanged);
  connect(fontSize_, &QSpinBox::valueChanged, this, emitChanged);
  connect(fontWeight_, &QComboBox::currentIndexChanged, this, emitChanged);
  connect(italic_, &QCheckBox::toggled, this, emitChanged);
  connect(radius_, &QSpinBox::valueChanged, this, emitChanged);
  connect(padding_, &QSpinBox::valueChanged, this, emitChanged);
  connect(margin_, &QSpinBox::valueChanged, this, emitChanged);
  connect(borderWidth_, &QSpinBox::valueChanged, this, emitChanged);
  connect(opacity_, &QSpinBox::valueChanged, this, emitChanged);
  connect(raised_, &QCheckBox::toggled, this, emitChanged);
  connect(underline_, &QSpinBox::valueChanged, this, emitChanged);
  connect(underlineAccent_, &QCheckBox::toggled, this, emitChanged);
  connect(underlineLeft_, &QCheckBox::toggled, this, emitChanged);
  connect(underlineCenter_, &QCheckBox::toggled, this, emitChanged);
  connect(underlineRight_, &QCheckBox::toggled, this, emitChanged);
}

void StylePage::applyTheme(int index) {
  const QVector<Theme> &t = themes();
  if (index < 0 || index >= t.size()) return;
  for (int i = 0; i < RCount; ++i) state_.color[i] = QColor(t.at(index).hex[i]);
  state_.themeName = t.at(index).name;
}

void StylePage::refreshColors() {
  for (int i = 0; i < RCount; ++i) {
    const QColor c = state_.color[i];
    if (!c.isValid()) continue;
    swatch_[i]->setText(QStringLiteral("%1\n%2").arg(roleLabel(i), c.name()));
    swatch_[i]->setStyleSheet(
        QStringLiteral("QPushButton{background:%1;color:%2;border:1px solid palette(mid);"
                      "border-radius:4px;}")
            .arg(c.name(),
                 relativeLuminance(c) > 0.4 ? QStringLiteral("#101010") : QStringLiteral("#f0f0f0")));
  }
}

void StylePage::refresh() {
  refreshColors();
  resolved_ = resolveTheme(state_.color, state_.opacity, state_.raised);

  QStringList notes;
  notes << tr("Text on the bar: %1 · muted text: %2 · warning fill: %3 · critical fill: %4")
               .arg(contrastBadge(state_.color[RText], state_.color[RBackground]),
                    contrastBadge(state_.color[RMuted], state_.color[RBackground]),
                    contrastBadge(resolved_.stateTextWarning, resolved_.c[RWarning]),
                    contrastBadge(resolved_.stateTextCritical, resolved_.c[RCritical]));
  for (const QString &n : resolved_.notes) notes << tr("Adjusted: %1").arg(n);
  notes_->setText(notes.join(QLatin1Char('\n')));
  showOverrides();
}

void StylePage::showOverrides() {
  while (overridesLayout_->count() > 0) {
    QLayoutItem *item = overridesLayout_->takeAt(0);
    if (QWidget *w = item->widget()) w->deleteLater();
    delete item;
  }
  if (state_.modules.isEmpty()) {
    auto *l = new QLabel(tr("No module overrides yet. Pick a module and use \"Style override\" on "
                            "the layout page."));
    l->setWordWrap(true);
    overridesLayout_->addWidget(l);
    return;
  }
  for (auto it = state_.modules.constBegin(); it != state_.modules.constEnd(); ++it) {
    if (it.value().isDefault()) continue;
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    auto *l = new QLabel(QStringLiteral("<code>%1</code> → <code>%2</code>")
                             .arg(cssSelector(it.key()).toHtmlEscaped(),
                                  displayName(it.key()).toHtmlEscaped()));
    l->setTextFormat(Qt::RichText);
    auto *b = new QPushButton(tr("Edit"));
    h->addWidget(l, 1);
    h->addWidget(b);
    overridesLayout_->addWidget(row);
    connect(b, &QPushButton::clicked, this, [this, ref = it.key()] { emit jumpToModule(ref); });
  }
}

void StylePage::setModules(const QMap<QString, ModuleStyle> &modules) {
  state_.modules = modules;
  showOverrides();
}

void StylePage::setState(const StyleState &s) {
  loading_ = true;
  state_ = s;
  refreshColors();
  manage_->setChecked(state_.manageCss);
  const int ti = themeIndexByName(state_.themeName);
  theme_->setCurrentIndex(ti >= 0 ? ti : theme_->count() - 1);
  fonts_->setValues(state_.fontStack);
  fontSize_->setValue(state_.fontSize);
  const int wi = fontWeight_->findData(state_.weight);
  fontWeight_->setCurrentIndex(wi >= 0 ? wi : 0);
  italic_->setChecked(state_.italic);
  radius_->setValue(state_.radius);
  padding_->setValue(state_.padding);
  margin_->setValue(state_.margin);
  borderWidth_->setValue(state_.borderWidth);
  opacity_->setValue(state_.opacity);
  raised_->setChecked(state_.raised);
  underline_->setValue(state_.underlineThickness);
  underlineAccent_->setChecked(state_.underlineAccent);
  underlineLeft_->setChecked(state_.underlineLeft);
  underlineCenter_->setChecked(state_.underlineCenter);
  underlineRight_->setChecked(state_.underlineRight);
  loading_ = false;
  refresh();
}

}  // namespace ws
