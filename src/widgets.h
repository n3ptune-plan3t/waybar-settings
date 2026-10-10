// Reusable editing widgets: colour sources, string lists, interval pickers,
// state-class tables, JSON blobs and the schema driven option form that both the
// bar page and the module page are built from.
#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSharedPointer>

#include <functional>

#include "schema.h"
#include "style.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QScrollArea;
class QSpinBox;
class QToolButton;
class QVBoxLayout;

namespace ws {

// ---- colour source: inherit / bar role / other theme role / custom ---------
class ColorSpecWidget : public QWidget {
  Q_OBJECT
 public:
  explicit ColorSpecWidget(const QString &label, QWidget *parent = nullptr);

  ColorSpec spec() const { return spec_; }
  void setSpec(const ColorSpec &s);
  void setBarColors(const QColor bar[RCount]);
  void setPreviewColor(const QColor &c);

 signals:
  void changed();

 private:
  void refresh();
  void pick();

  QPushButton *swatch_ = nullptr;
  QComboBox *source_ = nullptr;
  QComboBox *theme_ = nullptr;
  QComboBox *role_ = nullptr;
  ColorSpec spec_;
  QColor bar_[RCount] = {};
  QColor preview_;
};

// ---- ordered list of strings ---------------------------------------------
class StringListEditor : public QWidget {
  Q_OBJECT
 public:
  explicit StringListEditor(const QString &placeholder, QWidget *parent = nullptr);
  QStringList values() const;
  void setValues(const QStringList &v);

 signals:
  void changed();

 private:
  QLineEdit *entry_;
  QListWidget *list_;
};

// ---- seconds / event driven / once ---------------------------------------
class IntervalWidget : public QWidget {
  Q_OBJECT
 public:
  explicit IntervalWidget(QWidget *parent = nullptr);
  QJsonValue value() const;
  void setValue(const QJsonValue &v);

 signals:
  void changed();

 private:
  QComboBox *mode_;
  QSpinBox *seconds_;
};

// ---- name -> threshold, each name is a CSS class -------------------------
class StatesEditor : public QWidget {
  Q_OBJECT
 public:
  explicit StatesEditor(QWidget *parent = nullptr);
  bool isSet() const;
  QJsonObject value() const;
  QStringList names() const;
  void setValue(const QJsonValue &v);

 signals:
  void changed();

 private:
  struct Row;
  void addRow(const QString &name, int threshold);

  QCheckBox *enabled_ = nullptr;
  QListWidget *list_ = nullptr;
  QVector<QSharedPointer<Row>> rows_;
};

// ---- free form JSON -------------------------------------------------------
class JsonEditor : public QPlainTextEdit {
  Q_OBJECT
 public:
  explicit JsonEditor(QWidget *parent = nullptr);
  bool isValid() const { return valid_; }
  QJsonValue value() const;
  void setValue(const QJsonValue &v);

 signals:
  void changed();

 private:
  void validate();
  bool valid_ = true;
  bool updating_ = false;
};

// ---- schema driven option form -------------------------------------------
// Only writes keys that differ from the documented waybar default, so the
// generated config stays as small as the one waybar's own docs show.
class OptForm : public QWidget {
  Q_OBJECT
 public:
  explicit OptForm(QWidget *parent = nullptr);

  void setOptions(const QVector<Opt> &opts);
  void load(const QJsonObject &options);
  QJsonObject store() const;
  bool hasInvalidJson() const;

 signals:
  void changed();

 private:
  struct Row {
    Opt opt;
    QWidget *editor = nullptr;
    QLabel *name = nullptr;
    QToolButton *reset = nullptr;
    bool present = true;   // the key was present in the file we loaded
    QJsonValue absent;     // what the editor showed while the key was absent
    QJsonValue readRaw() const;
    QJsonValue read() const;
    void clearToDefault();
  };

  QWidget *makeEditor(const Opt &o);
  void applyRowState(Row *row);

  QVBoxLayout *layout_ = nullptr;
  QVector<Row> rows_;
};

// ---- module page: title + options + style override ------------------------
class ModuleForm : public QWidget {
  Q_OBJECT
 public:
  explicit ModuleForm(QWidget *parent = nullptr);

  void setReference(const QString &ref);
  QString reference() const { return ref_; }
  QString cssSelector() const { return ::ws::cssSelector(ref_); }

  void load(const QJsonObject &options, const ModuleStyle &style);
  void store(QJsonObject *options, ModuleStyle *style) const;
  bool hasInvalidJson() const;

  void setBarColors(const QColor bar[RCount]);
  void setStyleContext(const StyleState *state);

 signals:
  void changed();
  void styleChanged();

 private:
  void rebuild(const QString &ref);
  void buildStyleBox(const std::function<void(QWidget *)> &add);
  void applyPreview();

  QString ref_;
  const ModuleDef *def_ = nullptr;
  OptForm *optForm_ = nullptr;
  JsonEditor *extra_ = nullptr;
  QLabel *styleNote_ = nullptr;
  const StyleState *ctx_ = nullptr;
  QColor barColors_[RCount] = {};

  QScrollArea *scroll_ = nullptr;
  QWidget *content_ = nullptr;
  QVBoxLayout *contentLayout_ = nullptr;

  ColorSpecWidget *styleFill_ = nullptr;
  ColorSpecWidget *styleText_ = nullptr;
  ColorSpecWidget *styleBorder_ = nullptr;
  QSpinBox *styleBorderWidth_ = nullptr;
  QSpinBox *styleRadius_ = nullptr;
  QSpinBox *stylePadding_ = nullptr;
  QSpinBox *styleMargin_ = nullptr;
  QSpinBox *styleFontSize_ = nullptr;
  QComboBox *styleWeight_ = nullptr;
  QCheckBox *styleItalic_ = nullptr;
  QVector<QPair<QString, ColorSpecWidget *>> stateTextEdits_;
  QVector<QPair<QString, ColorSpecWidget *>> stateFillEdits_;
};

}  // namespace ws
