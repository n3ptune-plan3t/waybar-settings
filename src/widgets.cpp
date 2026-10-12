#include "widgets.h"

#include "jsonc.h"

#include <QAbstractItemModel>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

namespace ws {
  namespace {

    QJsonValue defaultValueOf(const Opt &o) {
      if (o.def.isEmpty()) return QJsonValue(QJsonValue::Undefined);
      QJsonParseError err;
      const QJsonDocument d = QJsonDocument::fromJson(o.def.toUtf8(), &err);
      if (err.error != QJsonParseError::NoError || !d.isArray() || d.array().isEmpty())
        return QJsonValue(o.def);
      return d.array().at(0);
    }

    QString describeDefault(const Opt &o) {
      if (o.def.isEmpty()) return QObject::tr("waybar default");
      if (o.kind == OptKind::Bool)
        return o.def == QLatin1String("true") ? QObject::tr("on in waybar") : QObject::tr("off in waybar");
      if (o.kind == OptKind::Interval) return QObject::tr("%1 s in waybar").arg(o.def);
      return QObject::tr("%1 in waybar").arg(o.def);
    }

    QString variantToText(const QJsonValue &v) {
      if (v.isUndefined() || v.isNull()) return QString();
      if (v.isString()) return v.toString();
      return jsonCompact(v);
    }

    QStringList arrayToStrings(const QJsonValue &v) {
      QStringList out;
      if (!v.isArray()) return out;
      for (const QJsonValue &e : v.toArray()) out << e.toString();
      return out;
    }

  }  // namespace

  // ===========================================================================
  // ColorSpecWidget
  // ===========================================================================

  ColorSpecWidget::ColorSpecWidget(const QString &label, QWidget *parent) : QWidget(parent) {
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(4);

    source_ = new QComboBox;
    source_->addItem(tr("inherit"), int(SpecSource::Inherit));
    source_->addItem(tr("bar role"), int(SpecSource::Bar));
    source_->addItem(tr("other theme"), int(SpecSource::Theme));
    source_->addItem(tr("custom"), int(SpecSource::Custom));
    source_->setToolTip(label);

    theme_ = new QComboBox;
    for (int i = 0; i < themes().size(); ++i) theme_->addItem(themes().at(i).name, i);
    theme_->hide();

    role_ = new QComboBox;
    for (int i = 0; i < RCount; ++i) role_->addItem(roleLabel(i), i);
    role_->hide();

    swatch_ = new QPushButton;
    swatch_->setFlat(true);
    swatch_->setMinimumWidth(78);

    row->addWidget(source_, 3);
    row->addWidget(theme_, 3);
    row->addWidget(role_, 2);
    row->addWidget(swatch_, 0);

    connect(source_, &QComboBox::currentIndexChanged, this, [this] {
      if (source_->currentIndex() < 0) return;
      spec_.source = static_cast<SpecSource>(source_->currentData().toInt());
      if (spec_.source == SpecSource::Custom && !spec_.custom.isValid()) spec_.custom = preview_;
      refresh();
      emit changed();
    });
    connect(theme_, &QComboBox::currentIndexChanged, this, [this] {
      if (theme_->currentIndex() < 0) return;
      spec_.theme = theme_->currentIndex();
      refresh();
      emit changed();
    });
    connect(role_, &QComboBox::currentIndexChanged, this, [this] {
      if (role_->currentIndex() < 0) return;
      spec_.role = role_->currentData().toInt();
      refresh();
      emit changed();
    });
    connect(swatch_, &QPushButton::clicked, this, &ColorSpecWidget::pick);
    refresh();
  }

  void ColorSpecWidget::pick() {
    QColor start = spec_.custom.isValid() ? spec_.custom : preview_;
    if (!start.isValid()) start = QColor(128, 128, 128);
    const QColor c = QColorDialog::getColor(start, this, tr("Pick a colour"));
    if (!c.isValid()) return;
    spec_.custom = c;
    spec_.source = SpecSource::Custom;
    const QSignalBlocker b(source_);
    source_->setCurrentIndex(source_->findData(int(SpecSource::Custom)));
    refresh();
    emit changed();
  }

  void ColorSpecWidget::refresh() {
    const bool isTheme = spec_.source == SpecSource::Theme;
    const bool isRole = spec_.source == SpecSource::Bar || isTheme;
    theme_->setVisible(isTheme);
    role_->setVisible(isRole);
    if (spec_.theme >= 0 && spec_.theme < theme_->count()) theme_->setCurrentIndex(spec_.theme);
    const int roleIndex = role_->findData(spec_.role);
    if (roleIndex >= 0) role_->setCurrentIndex(roleIndex);

    QColor shown = preview_;
    if (isRole) shown = resolveSpecColor(spec_, bar_, themes());
    if (spec_.source == SpecSource::Custom && spec_.custom.isValid()) shown = spec_.custom;
    if (!shown.isValid()) shown = QColor(160, 160, 160);

    swatch_->setText(shown.name());
    swatch_->setStyleSheet(
      QStringLiteral("QPushButton{background:%1;color:%2;border:1px solid palette(mid);border-radius:4px;"
      "padding:2px 6px;}")
      .arg(shown.name(),
           relativeLuminance(shown) > 0.4 ? QStringLiteral("#101010") : QStringLiteral("#f0f0f0")));
    const bool clickable = spec_.source == SpecSource::Custom;
    swatch_->setCursor(clickable ? Qt::PointingHandCursor : Qt::ArrowCursor);
    swatch_->setToolTip(clickable ? tr("Click to pick a colour") : QString());
  }

  void ColorSpecWidget::setSpec(const ColorSpec &s) {
    spec_ = s;
    if (spec_.source == SpecSource::Custom && !spec_.custom.isValid() && preview_.isValid())
      spec_.custom = preview_;
    const QSignalBlocker b(source_);
    source_->setCurrentIndex(source_->findData(int(spec_.source)));
    refresh();
  }

  void ColorSpecWidget::setBarColors(const QColor bar[RCount]) {
    for (int i = 0; i < RCount; ++i) bar_[i] = bar[i];
    if (spec_.source == SpecSource::Bar) preview_ = bar_[spec_.role];
    refresh();
  }

  void ColorSpecWidget::setPreviewColor(const QColor &c) {
    if (!c.isValid()) return;
    preview_ = c;
    if (spec_.source == SpecSource::Bar || spec_.source == SpecSource::Theme ||
      (spec_.source == SpecSource::Custom && !spec_.custom.isValid()))
      refresh();
  }

  // ===========================================================================
  // StringListEditor
  // ===========================================================================

  StringListEditor::StringListEditor(const QString &placeholder, QWidget *parent) : QWidget(parent) {
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    list_ = new QListWidget;
    list_->setDragDropMode(QAbstractItemView::InternalMove);
    list_->setMaximumHeight(96);
    entry_ = new QLineEdit;
    entry_->setPlaceholderText(placeholder);
    auto *row = new QHBoxLayout;
    auto *add = new QPushButton(tr("Add"));
    auto *del = new QPushButton(tr("Remove"));
    row->addWidget(entry_, 1);
    row->addWidget(add);
    row->addWidget(del);
    v->addWidget(list_);
    v->addLayout(row);

    const auto commit = [this] {
      const QString t = entry_->text().trimmed();
      if (t.isEmpty()) return;
      if (list_->findItems(t, Qt::MatchExactly).isEmpty()) list_->addItem(t);
      entry_->clear();
      emit changed();
    };
    connect(add, &QPushButton::clicked, this, commit);
    connect(entry_, &QLineEdit::returnPressed, this, commit);
    connect(del, &QPushButton::clicked, this, [this] {
      delete list_->takeItem(list_->currentRow());
      emit changed();
    });
    connect(list_->model(), &QAbstractItemModel::rowsMoved, this, [this] { emit changed(); });
  }

  QStringList StringListEditor::values() const {
    QStringList out;
    for (int i = 0; i < list_->count(); ++i) out << list_->item(i)->text();
    return out;
  }

  void StringListEditor::setValues(const QStringList &v) {
    const QSignalBlocker b(list_->model());
    list_->clear();
    for (const QString &s : v) list_->addItem(s);
  }

  // ===========================================================================
  // IntervalWidget
  // ===========================================================================

  IntervalWidget::IntervalWidget(QWidget *parent) : QWidget(parent) {
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    mode_ = new QComboBox;
    mode_->addItem(tr("seconds"), 1);
    mode_->addItem(tr("event driven"), 0);
    mode_->addItem(tr("once"), 2);
    seconds_ = new QSpinBox;
    seconds_->setRange(0, 3600);
    seconds_->setSuffix(tr(" s"));
    seconds_->setEnabled(false);
    row->addWidget(mode_, 2);
    row->addWidget(seconds_, 2);
    connect(mode_, &QComboBox::currentIndexChanged, this, [this] {
      if (mode_->currentIndex() < 0) return;
      seconds_->setEnabled(mode_->currentData().toInt() == 1);
      emit changed();
    });
    connect(seconds_, &QSpinBox::valueChanged, this, [this] { emit changed(); });
  }

  QJsonValue IntervalWidget::value() const {
    if (mode_->currentIndex() < 0) return QJsonValue(0);
    const int m = mode_->currentData().toInt();
    if (m == 0) return QJsonValue(0);
    if (m == 2) return QJsonValue(QStringLiteral("once"));
    return QJsonValue(seconds_->value());
  }

  void IntervalWidget::setValue(const QJsonValue &v) {
    const QSignalBlocker b1(mode_), b2(seconds_);
    if (v.isString() && v.toString() == QLatin1String("once")) {
      mode_->setCurrentIndex(mode_->findData(2));
      return;
    }
    if (v.isDouble() && v.toInt() == 0) {
      mode_->setCurrentIndex(mode_->findData(0));
      return;
    }
    mode_->setCurrentIndex(mode_->findData(1));
    if (v.isDouble()) seconds_->setValue(v.toInt());
    else seconds_->setValue(1);
  }

  // ===========================================================================
  // StatesEditor
  // ===========================================================================

  struct StatesEditor::Row {
    QString name;
    QLineEdit *nameEdit = nullptr;
    QSpinBox *spin = nullptr;
  };

  StatesEditor::StatesEditor(QWidget *parent) : QWidget(parent) {
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    enabled_ = new QCheckBox(tr("define state classes"));
    list_ = new QListWidget;
    list_->setMaximumHeight(150);
    list_->setEnabled(false);
    auto *row = new QHBoxLayout;
    auto *add = new QPushButton(tr("Add class"));
    auto *del = new QPushButton(tr("Remove"));
    row->addStretch();
    row->addWidget(add);
    row->addWidget(del);
    v->addWidget(enabled_);
    v->addWidget(list_);
    v->addLayout(row);

    connect(enabled_, &QCheckBox::toggled, this, [this] {
      list_->setEnabled(enabled_->isChecked());
      emit changed();
    });
    connect(add, &QPushButton::clicked, this, [this] { addRow(QString(), 0); });
    connect(del, &QPushButton::clicked, this, [this] {
      const int i = list_->currentRow();
      if (i < 0 || i >= rows_.size()) return;
      rows_.removeAt(i);
      delete list_->takeItem(i);
      emit changed();
    });
  }

  void StatesEditor::addRow(const QString &name, int threshold) {
    auto r = QSharedPointer<Row>(new Row);
    r->name = name;
    auto *w = new QWidget;
    auto *h = new QHBoxLayout(w);
    h->setContentsMargins(2, 0, 2, 0);
    r->nameEdit = new QLineEdit(name);
    r->nameEdit->setPlaceholderText(tr("css class name"));
    r->spin = new QSpinBox;
    r->spin->setRange(0, 1000000);
    r->spin->setValue(threshold);
    h->addWidget(r->nameEdit, 2);
    h->addWidget(r->spin, 1);
    rows_.append(r);
    auto *item = new QListWidgetItem(list_);
    item->setSizeHint(w->sizeHint());
    list_->setItemWidget(item, w);
    connect(r->nameEdit, &QLineEdit::textChanged, this, [this, r](const QString &t) {
      r->name = t;
      emit changed();
    });
    connect(r->spin, &QSpinBox::valueChanged, this, [this] { emit changed(); });
  }

  bool StatesEditor::isSet() const { return enabled_->isChecked(); }

  QStringList StatesEditor::names() const {
    QStringList out;
    if (!enabled_->isChecked()) return out;
    for (const auto &r : rows_) {
      const QString n = r->name.trimmed();
      if (!n.isEmpty()) out << n;
    }
    return out;
  }

  QJsonObject StatesEditor::value() const {
    QJsonObject o;
    if (!enabled_->isChecked()) return o;
    for (const auto &r : rows_) {
      const QString n = r->name.trimmed();
      if (!n.isEmpty()) o.insert(n, r->spin->value());
    }
    return o;
  }

  void StatesEditor::setValue(const QJsonValue &v) {
    const QSignalBlocker be(enabled_), bl(list_);
    list_->clear();
    rows_.clear();
    const QJsonObject o = v.isObject() ? v.toObject() : QJsonObject();
    enabled_->setChecked(!o.isEmpty());
    list_->setEnabled(!o.isEmpty());
    for (auto it = o.begin(); it != o.end(); ++it) addRow(it.key(), it.value().toInt());
  }

  // ===========================================================================
  // JsonEditor
  // ===========================================================================

  JsonEditor::JsonEditor(QWidget *parent) : QPlainTextEdit(parent) {
    setTabChangesFocus(true);
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setMinimumHeight(70);
    connect(this, &QPlainTextEdit::textChanged, this, &JsonEditor::validate);
    validate();
  }

  void JsonEditor::validate() {
    if (updating_) return;
    const QString text = toPlainText().trimmed();
    QJsonParseError err;
    QJsonDocument::fromJson(text.toUtf8(), &err);
    valid_ = err.error == QJsonParseError::NoError || text.isEmpty();
    setStyleSheet(valid_ ? QString() : QStringLiteral("QPlainTextEdit{border:1px solid palette(critical);}"));
    setToolTip(valid_ ? QString() : err.errorString());
    emit changed();
  }

  QJsonValue JsonEditor::value() const {
    const QString text = toPlainText().trimmed();
    if (text.isEmpty()) return QJsonValue(QJsonValue::Undefined);
    QJsonParseError err;
    const QJsonDocument wrapped = QJsonDocument::fromJson(("[ " + text + " ]").toUtf8(), &err);
    if (err.error == QJsonParseError::NoError && wrapped.isArray() && wrapped.array().size() == 1)
      return wrapped.array().at(0);
    err = QJsonParseError();
    const QJsonDocument d = QJsonDocument::fromJson(text.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) return QJsonValue(text);
    if (d.isObject()) return d.object();
    if (d.isArray()) return d.array();
    return QJsonValue(text);
  }

  void JsonEditor::setValue(const QJsonValue &v) {
    updating_ = true;
    if (v.isObject())
      setPlainText(QString::fromUtf8(QJsonDocument(v.toObject()).toJson(QJsonDocument::Indented)));
    else if (v.isArray())
      setPlainText(QString::fromUtf8(QJsonDocument(v.toArray()).toJson(QJsonDocument::Indented)));
    else if (v.isString())
      setPlainText(v.toString());
    else if (v.isUndefined() || v.isNull())
      setPlainText(QStringLiteral("{}"));
    else
      setPlainText(jsonCompact(v));
    updating_ = false;
    validate();
  }

  // ===========================================================================
  // OptForm
  // ===========================================================================

  void OptForm::Row::clearToDefault() {
    const QJsonValue def = defaultValueOf(opt);
    if (auto *cb = qobject_cast<QCheckBox *>(editor)) cb->setChecked(def.toBool());
    else if (auto *sb = qobject_cast<QSpinBox *>(editor)) sb->setValue(def.toInt());
    else if (auto *dsb = qobject_cast<QDoubleSpinBox *>(editor)) dsb->setValue(def.toDouble());
    else if (auto *cb = qobject_cast<QComboBox *>(editor)) cb->setCurrentText(def.toString());
    else if (auto *le = qobject_cast<QLineEdit *>(editor)) le->setText(def.toString());
    else if (auto *sl = qobject_cast<StringListEditor *>(editor)) sl->setValues(QStringList());
    else if (auto *iv = qobject_cast<IntervalWidget *>(editor)) iv->setValue(def);
    else if (auto *se = qobject_cast<StatesEditor *>(editor)) se->setValue(QJsonValue());
    else if (auto *je = qobject_cast<JsonEditor *>(editor)) je->setValue(def);
    present = false;
    absent = readRaw();
  }

  QJsonValue OptForm::Row::readRaw() const {
    if (auto *w = qobject_cast<QCheckBox *>(editor)) return QJsonValue(w->isChecked());
    if (auto *w = qobject_cast<QSpinBox *>(editor)) return QJsonValue(w->value());
    if (auto *w = qobject_cast<QDoubleSpinBox *>(editor)) return QJsonValue(w->value());
    if (auto *w = qobject_cast<QComboBox *>(editor)) {
      const QString t = w->currentText();
      return t.isEmpty() ? QJsonValue(QJsonValue::Undefined) : QJsonValue(t);
    }
    if (auto *w = qobject_cast<QLineEdit *>(editor)) {
      const QString t = w->text().trimmed();
      return t.isEmpty() ? QJsonValue(QJsonValue::Undefined) : QJsonValue(t);
    }
    if (auto *w = qobject_cast<StringListEditor *>(editor)) {
      const QStringList v = w->values();
      return v.isEmpty() ? QJsonValue(QJsonValue::Undefined) : QJsonValue(QJsonArray::fromStringList(v));
    }
    if (auto *w = qobject_cast<IntervalWidget *>(editor)) return w->value();
    if (auto *w = qobject_cast<StatesEditor *>(editor)) {
      const QJsonObject o = w->value();
      return o.isEmpty() ? QJsonValue(QJsonValue::Undefined) : QJsonValue(o);
    }
    if (auto *w = qobject_cast<JsonEditor *>(editor)) return w->value();
    return QJsonValue(QJsonValue::Undefined);
  }

  QJsonValue OptForm::Row::read() const {
    const QJsonValue v = readRaw();
    // Keys that were absent when the file was loaded stay absent until edited,
    // so opening and saving a config never rewrites it.
    if (!present && v == absent) return QJsonValue(QJsonValue::Undefined);
    return v;
  }

  OptForm::OptForm(QWidget *parent) : QWidget(parent) {
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(6);
  }

  void OptForm::setOptions(const QVector<Opt> &opts) {
    while (layout()->count() > 0) {
      QLayoutItem *item = layout()->takeAt(0);
      if (QWidget *w = item->widget()) w->deleteLater();
      delete item;
    }
    rows_.clear();

    QStringList groups;
    for (const Opt &o : opts)
      if (!groups.contains(o.group)) groups << o.group;

      for (const QString &g : groups) {
        auto *box = new QGroupBox(g);
        auto *form = new QFormLayout(box);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        for (const Opt &o : opts) {
          if (o.group != g) continue;
          Row row;
          row.opt = o;
          row.editor = makeEditor(o);
          row.name = new QLabel(o.key);
          row.reset = new QToolButton;
          row.reset->setText(QStringLiteral("\xE2\x86\x96"));
          row.reset->setToolTip(tr("Clear this option"));
          row.reset->setAutoRaise(true);
          row.reset->setMaximumWidth(24);
          auto *w = new QWidget;
          auto *h = new QHBoxLayout(w);
          h->setContentsMargins(0, 0, 0, 0);
          h->addWidget(row.editor, 1);
          h->addWidget(row.reset);
          form->addRow(row.name, w);
          const int idx = rows_.size();
          connect(row.reset, &QToolButton::clicked, this, [this, idx] {
            if (idx < 0 || idx >= rows_.size()) return;
            rows_[idx].clearToDefault();
            applyRowState(&rows_[idx]);
            emit changed();
          });
          rows_.append(row);
          applyRowState(&rows_.last());
        }
        layout()->addWidget(box);
      }
      static_cast<QVBoxLayout *>(layout())->addStretch(1);
  }

  bool OptForm::hasInvalidJson() const {
    for (const Row &row : rows_) {
      auto *je = qobject_cast<JsonEditor *>(row.editor);
      if (je && !je->isValid()) return true;
    }
    return false;
  }

  QWidget *OptForm::makeEditor(const Opt &o) {
    QWidget *w = nullptr;
    switch (o.kind) {
      case OptKind::Bool:
        w = new QCheckBox;
        break;
      case OptKind::Int: {
        auto *sb = new QSpinBox;
        sb->setRange(0, 1000000);
        w = sb;
        break;
      }
      case OptKind::Double: {
        auto *sb = new QDoubleSpinBox;
        sb->setRange(0, 1000000);
        sb->setDecimals(2);
        sb->setSingleStep(0.5);
        w = sb;
        break;
      }
      case OptKind::Choice: {
        auto *cb = new QComboBox;
        cb->addItem(QString());
        cb->addItems(o.choices.split(QLatin1Char('|'), Qt::SkipEmptyParts));
        w = cb;
        break;
      }
      case OptKind::List:
        w = new StringListEditor(o.help);
        break;
      case OptKind::States:
        w = new StatesEditor;
        break;
      case OptKind::Interval:
        w = new IntervalWidget;
        break;
      case OptKind::Json:
        w = new JsonEditor;
        break;
      case OptKind::Command:
      case OptKind::Path:
      case OptKind::String:
      default: {
        auto *le = new QLineEdit;
        le->setPlaceholderText(o.help);
        w = le;
        break;
      }
    }
    w->setToolTip(o.help);

    if (o.kind == OptKind::Path || o.kind == OptKind::Command) {
      const bool isPath = o.kind == OptKind::Path;
      const QString hint = o.help;
      const QString title = isPath ? tr("Select a file") : tr("Shell command");
      auto *box = new QWidget;
      auto *h = new QHBoxLayout(box);
      h->setContentsMargins(0, 0, 0, 0);
      h->addWidget(w, 1);
      auto *b = new QPushButton(QStringLiteral("..."));
      b->setMaximumWidth(36);
      b->setToolTip(hint);
      h->addWidget(b);
      connect(b, &QPushButton::clicked, this, [this, w, isPath, hint, title] {
        auto *le = qobject_cast<QLineEdit *>(w);
        if (!le) return;
        if (isPath) {
          const QString f = QFileDialog::getOpenFileName(this, title, le->text());
          if (f.isEmpty()) return;
          le->setText(f);
        } else {
          bool ok = false;
          const QString cmd = QInputDialog::getText(this, title, hint, QLineEdit::Normal, le->text(), &ok);
          if (!ok || cmd.isEmpty()) return;
          le->setText(cmd);
        }
        emit changed();
      });
    }

    if (auto *cb = qobject_cast<QCheckBox *>(w))
      connect(cb, &QCheckBox::toggled, this, &OptForm::changed);
    else if (auto *sb = qobject_cast<QSpinBox *>(w))
      connect(sb, &QSpinBox::valueChanged, this, &OptForm::changed);
    else if (auto *dsb = qobject_cast<QDoubleSpinBox *>(w))
      connect(dsb, &QDoubleSpinBox::valueChanged, this, &OptForm::changed);
    else if (auto *cb = qobject_cast<QComboBox *>(w))
      connect(cb, &QComboBox::currentIndexChanged, this, &OptForm::changed);
    else if (auto *le = qobject_cast<QLineEdit *>(w))
      connect(le, &QLineEdit::textChanged, this, &OptForm::changed);
    else if (auto *sl = qobject_cast<StringListEditor *>(w))
      connect(sl, &StringListEditor::changed, this, &OptForm::changed);
    else if (auto *iv = qobject_cast<IntervalWidget *>(w))
      connect(iv, &IntervalWidget::changed, this, &OptForm::changed);
    else if (auto *se = qobject_cast<StatesEditor *>(w))
      connect(se, &StatesEditor::changed, this, &OptForm::changed);
    else if (auto *je = qobject_cast<JsonEditor *>(w))
      connect(je, &JsonEditor::changed, this, &OptForm::changed);
    return w;
  }

  void OptForm::applyRowState(Row *row) {
    if (!row->name) return;
    const QJsonValue v = row->read();
    const bool isDefault = v.isUndefined() || v == defaultValueOf(row->opt);
    QFont f = row->name->font();
    f.setItalic(isDefault);
    row->name->setFont(f);
    row->name->setToolTip(row->opt.help + QStringLiteral("\n\n") + describeDefault(row->opt));
  }

  void OptForm::load(const QJsonObject &options) {
    for (Row &row : rows_) {
      const QJsonValue v = options.value(row.opt.key);
      const QJsonValue def = defaultValueOf(row.opt);
      const QJsonValue use = v.isUndefined() ? def : v;
      if (auto *cb = qobject_cast<QCheckBox *>(row.editor)) cb->setChecked(use.toBool());
      else if (auto *sb = qobject_cast<QSpinBox *>(row.editor)) sb->setValue(use.toInt());
      else if (auto *dsb = qobject_cast<QDoubleSpinBox *>(row.editor)) dsb->setValue(use.toDouble());
      else if (auto *cb = qobject_cast<QComboBox *>(row.editor)) cb->setCurrentText(use.toString());
      else if (auto *le = qobject_cast<QLineEdit *>(row.editor)) le->setText(variantToText(v));
      else if (auto *sl = qobject_cast<StringListEditor *>(row.editor)) sl->setValues(arrayToStrings(v));
      else if (auto *iv = qobject_cast<IntervalWidget *>(row.editor)) iv->setValue(use);
      else if (auto *se = qobject_cast<StatesEditor *>(row.editor)) se->setValue(v);
      else if (auto *je = qobject_cast<JsonEditor *>(row.editor)) je->setValue(v.isUndefined() ? def : v);
      row.present = options.contains(row.opt.key);
      row.absent = row.readRaw();
      applyRowState(&row);
    }
  }

  QJsonObject OptForm::store() const {
    QJsonObject out;
    for (const Row &row : rows_) {
      const QJsonValue v = row.read();
      if (v.isUndefined()) continue;
      out.insert(row.opt.key, v);
    }
    return out;
  }

  // ===========================================================================
  // ModuleForm
  // ===========================================================================

  ModuleForm::ModuleForm(QWidget *parent) : QWidget(parent) {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    scroll_ = new QScrollArea;
    scroll_->setWidgetResizable(true);
    scroll_->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll_);
    content_ = new QWidget;
    scroll_->setWidget(content_);
    contentLayout_ = new QVBoxLayout(content_);
    contentLayout_->setContentsMargins(6, 6, 6, 6);
  }

  void ModuleForm::setReference(const QString &ref) {
    if (!ref.isEmpty() && ref == ref_) return;
    ref_ = ref;
    rebuild(ref);
  }

  void ModuleForm::rebuild(const QString &ref) {
    if (!scroll_) return;
    content_ = new QWidget;
    scroll_->setWidget(content_);
    contentLayout_ = new QVBoxLayout(content_);
    contentLayout_->setContentsMargins(6, 6, 6, 6);
    optForm_ = nullptr;
    extra_ = nullptr;
    styleNote_ = nullptr;
    styleFill_ = styleText_ = styleBorder_ = nullptr;
    stateTextEdits_.clear();
    stateFillEdits_.clear();

    def_ = findModule(moduleType(ref));
    const auto add = [this](QWidget *w) { contentLayout_->addWidget(w); };

    if (ref.isEmpty()) {
      auto *l = new QLabel(tr("Select a module in one of the three sections to configure it."));
      l->setWordWrap(true);
      l->setAlignment(Qt::AlignTop);
      add(l);
      contentLayout_->addStretch(1);
      return;
    }

    if (!def_) {
      auto *l = new QLabel(tr("\"%1\" is not a module waybar-settings has a description for.\n"
      "Its options can still be edited as raw JSON below.")
      .arg(ref));
      l->setWordWrap(true);
      add(l);
    } else {
      auto *title = new QLabel(QStringLiteral("<b>%1</b> &nbsp; <code>%2</code>")
      .arg(displayName(ref).toHtmlEscaped(),
           ::ws::cssSelector(ref).toHtmlEscaped()));
      title->setTextFormat(Qt::RichText);
      add(title);
      if (!def_->summary.isEmpty()) {
        auto *s = new QLabel(def_->summary);
        s->setWordWrap(true);
        s->setStyleSheet(QStringLiteral("color: palette(mid);"));
        add(s);
      }
      optForm_ = new OptForm;
      optForm_->setOptions(optionsFor(*def_));
      connect(optForm_, &OptForm::changed, this, &ModuleForm::changed);
      add(optForm_);
      buildStyleBox(add);
    }

    auto *extraBox = new QGroupBox(tr("Anything else (raw JSON)"));
    auto *ev = new QVBoxLayout(extraBox);
    extra_ = new JsonEditor;
    extra_->setMinimumHeight(90);
    extra_->setToolTip(tr("Options waybar-settings has no description for are kept here untouched."));
    ev->addWidget(extra_);
    connect(extra_, &JsonEditor::changed, this, &ModuleForm::changed);
    add(extraBox);
    contentLayout_->addStretch(1);
  }

  void ModuleForm::buildStyleBox(const std::function<void(QWidget *)> &add) {
    auto *box = new QGroupBox(tr("Style override"));
    auto *v = new QVBoxLayout(box);
    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    styleFill_ = new ColorSpecWidget(tr("Background"));
    styleText_ = new ColorSpecWidget(tr("Text"));
    styleBorder_ = new ColorSpecWidget(tr("Border"));
    form->addRow(tr("Background"), styleFill_);
    form->addRow(tr("Text"), styleText_);
    form->addRow(tr("Border"), styleBorder_);

    const auto spin = [&](const QString &lbl, int lo, int hi, const QString &special) {
      auto *sb = new QSpinBox;
      sb->setRange(lo, hi);
      if (!special.isEmpty()) sb->setSpecialValueText(special);
      form->addRow(lbl, sb);
      return sb;
    };
    styleBorderWidth_ = spin(tr("Border width (px)"), 0, 20, QString());
    styleRadius_ = spin(tr("Radius (px)"), -1, 100, tr("inherit"));
    stylePadding_ = spin(tr("Padding (px)"), -1, 100, tr("inherit"));
    styleMargin_ = spin(tr("Margin (px)"), -1, 100, tr("inherit"));
    styleFontSize_ = spin(tr("Font size (px)"), 0, 96, tr("inherit"));
    styleWeight_ = new QComboBox;
    styleWeight_->addItem(tr("inherit"), 0);
    styleWeight_->addItem(tr("bold"), 1);
    styleWeight_->addItem(tr("light"), 2);
    form->addRow(tr("Weight"), styleWeight_);
    styleItalic_ = new QCheckBox(tr("italic"));
    form->addRow(styleItalic_);
    v->addLayout(form);

    if (!def_->states.isEmpty()) {
      QStringList escaped;
      for (const QString &c : def_->states) escaped << QStringLiteral("<code>%1</code>").arg(c.toHtmlEscaped());
      auto *l = new QLabel(tr("Classes this module sets: %1.\n"
      "Give one of them a colour to override the global rule for it.")
      .arg(escaped.join(QStringLiteral(", "))));
      l->setWordWrap(true);
      v->addWidget(l);
      auto *sform = new QFormLayout;
      sform->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
      for (const QString &cls : def_->states) {
        auto *t = new ColorSpecWidget(cls);
        auto *f = new ColorSpecWidget(cls);
        stateTextEdits_.append({cls, t});
        stateFillEdits_.append({cls, f});
        sform->addRow(QStringLiteral(".<code>%1</code> text").arg(cls.toHtmlEscaped()), t);
        sform->addRow(QStringLiteral(".<code>%1</code> fill").arg(cls.toHtmlEscaped()), f);
      }
      v->addLayout(sform);
    }

    styleNote_ = new QLabel;
    styleNote_->setWordWrap(true);
    v->addWidget(styleNote_);

    auto *clearStyle = new QPushButton(tr("Reset this module to the bar style"));
    v->addWidget(clearStyle);

    const auto onStyleChanged = [this] {
      applyPreview();
      emit styleChanged();
    };
    for (ColorSpecWidget *w : {styleFill_, styleText_, styleBorder_})
      connect(w, &ColorSpecWidget::changed, this, onStyleChanged);
      for (const auto &p : stateTextEdits_)
        connect(p.second, &ColorSpecWidget::changed, this, onStyleChanged);
    for (const auto &p : stateFillEdits_)
      connect(p.second, &ColorSpecWidget::changed, this, onStyleChanged);
    connect(styleBorderWidth_, &QSpinBox::valueChanged, this, onStyleChanged);
    connect(styleRadius_, &QSpinBox::valueChanged, this, onStyleChanged);
    connect(stylePadding_, &QSpinBox::valueChanged, this, onStyleChanged);
    connect(styleMargin_, &QSpinBox::valueChanged, this, onStyleChanged);
    connect(styleFontSize_, &QSpinBox::valueChanged, this, onStyleChanged);
    connect(styleWeight_, &QComboBox::currentIndexChanged, this, onStyleChanged);
    connect(styleItalic_, &QCheckBox::toggled, this, onStyleChanged);
    connect(clearStyle, &QPushButton::clicked, this, [this] {
      styleFill_->setSpec(ColorSpec());
      styleText_->setSpec(ColorSpec());
      styleBorder_->setSpec(ColorSpec());
      styleBorderWidth_->setValue(1);
      styleRadius_->setValue(-1);
      stylePadding_->setValue(-1);
      styleMargin_->setValue(-1);
      styleFontSize_->setValue(0);
      styleWeight_->setCurrentIndex(0);
      styleItalic_->setChecked(false);
      for (const auto &p : stateTextEdits_) p.second->setSpec(ColorSpec());
      for (const auto &p : stateFillEdits_) p.second->setSpec(ColorSpec());
      applyPreview();
      emit styleChanged();
    });
    add(box);
  }

  void ModuleForm::applyPreview() {
    if (ctx_ && styleFill_) {
      styleFill_->setPreviewColor(resolveSpecColor(styleFill_->spec(), ctx_->color, themes()));
      styleText_->setPreviewColor(resolveSpecColor(styleText_->spec(), ctx_->color, themes()));
      styleBorder_->setPreviewColor(resolveSpecColor(styleBorder_->spec(), ctx_->color, themes()));
      for (const auto &p : stateTextEdits_)
        p.second->setPreviewColor(resolveSpecColor(p.second->spec(), ctx_->color, themes()));
      for (const auto &p : stateFillEdits_)
        p.second->setPreviewColor(resolveSpecColor(p.second->spec(), ctx_->color, themes()));
    }
    if (!styleNote_ || !styleText_) return;
    const ColorSpec want = styleText_->spec();
    if (want.isSet()) {
      ModuleStyle probe;
      probe.fill = styleFill_->spec();
      probe.text = want;
      QColor bar[RCount];
      for (int i = 0; i < RCount; ++i) bar[i] = ctx_ ? ctx_->color[i] : QColor(Qt::black);
      const Resolved dummy = resolveTheme(bar, 100, ctx_ && ctx_->raised);
      const ResolvedModule m = resolveModule(probe, dummy, themes());
      const QColor asked = resolveSpecColor(want, bar, themes());
      if (m.hasText && m.text != asked)
        styleNote_->setText(tr("The written text colour is %1 so it stays readable on this background.")
        .arg(m.text.name()));
      else
        styleNote_->clear();
    } else if (styleFill_ && styleFill_->spec().isSet()) {
      styleNote_->setText(tr("Only the background changes; the bar text colour is kept, nudged just enough "
      "to stay readable on the new background."));
    } else {
      styleNote_->clear();
    }
  }

  bool ModuleForm::hasInvalidJson() const {
    if (optForm_ && optForm_->hasInvalidJson()) return true;
    return extra_ && !extra_->isValid();
  }

  void ModuleForm::setBarColors(const QColor bar[RCount]) {
    for (int i = 0; i < RCount; ++i) barColors_[i] = bar[i];
    if (!styleFill_) return;
    styleFill_->setBarColors(barColors_);
    styleText_->setBarColors(barColors_);
    styleBorder_->setBarColors(barColors_);
  }

  void ModuleForm::setStyleContext(const StyleState *s) {
    ctx_ = s;
    if (styleFill_) applyPreview();
  }

  void ModuleForm::load(const QJsonObject &options, const ModuleStyle &style) {
    if (!extra_) return;
    if (optForm_) {
      optForm_->load(options);
      const QVector<Opt> opts = optionsFor(*def_);
      QStringList known;
      for (const Opt &o : opts) known << o.key;
      QJsonObject rest;
      for (auto it = options.begin(); it != options.end(); ++it)
        if (!known.contains(it.key())) rest.insert(it.key(), it.value());
        extra_->setValue(rest);
    } else {
      extra_->setValue(options);
    }

    if (styleFill_) {
      styleFill_->setSpec(style.fill);
      styleText_->setSpec(style.text);
      styleBorder_->setSpec(style.border);
      styleBorderWidth_->setValue(style.borderWidth);
      styleRadius_->setValue(style.radius);
      stylePadding_->setValue(style.padding);
      styleMargin_->setValue(style.margin);
      styleFontSize_->setValue(style.fontSize);
      styleWeight_->setCurrentIndex(style.weight);
      styleItalic_->setChecked(style.italic);
      for (const auto &p : stateTextEdits_) p.second->setSpec(style.stateText.value(p.first));
      for (const auto &p : stateFillEdits_) p.second->setSpec(style.stateFill.value(p.first));
    }
    applyPreview();
  }

  void ModuleForm::store(QJsonObject *options, ModuleStyle *style) const {
    QJsonObject out = optForm_ ? optForm_->store() : QJsonObject();
    if (extra_) {
      const QJsonValue ev = extra_->value();
      if (ev.isObject()) {
        const QJsonObject evObj = ev.toObject();
        for (auto it = evObj.begin(); it != evObj.end(); ++it) out.insert(it.key(), it.value());
      }
    }
    if (options) *options = out;

    if (!style || !styleFill_) return;
    ModuleStyle ms;
    ms.fill = styleFill_->spec();
    ms.text = styleText_->spec();
    ms.border = styleBorder_->spec();
    ms.borderWidth = styleBorderWidth_->value();
    ms.radius = styleRadius_->value();
    ms.padding = stylePadding_->value();
    ms.margin = styleMargin_->value();
    ms.fontSize = styleFontSize_->value();
    ms.weight = styleWeight_->currentData().toInt();
    ms.italic = styleItalic_->isChecked();
    for (const auto &p : stateTextEdits_)
      if (p.second->spec().isSet()) ms.stateText.insert(p.first, p.second->spec());
      for (const auto &p : stateFillEdits_)
        if (p.second->spec().isSet()) ms.stateFill.insert(p.first, p.second->spec());
        *style = ms;
  }

}  // namespace ws
