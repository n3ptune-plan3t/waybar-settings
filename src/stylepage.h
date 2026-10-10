// The style page: colour roles, typography, metrics and the underline, plus the
// readability report for the palette that ends up in style.css.
#pragma once

#include <QWidget>

#include "style.h"
#include "widgets.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QSpinBox;

namespace ws {

class StylePage : public QWidget {
  Q_OBJECT
 public:
  explicit StylePage(QWidget *parent = nullptr);

  StyleState state() const { return state_; }
  const Resolved &resolved() const { return resolved_; }
  void setState(const StyleState &s);
  void setModules(const QMap<QString, ModuleStyle> &modules);

 signals:
  void changed();
  void jumpToModule(const QString &ref);

 private:
  void applyTheme(int index);
  void refreshColors();
  void refresh();
  void showOverrides();

  StyleState state_;
  Resolved resolved_;
  bool loading_ = false;

  QCheckBox *manage_ = nullptr;
  QComboBox *theme_ = nullptr;
  QPushButton *swatch_[RCount] = {};

  StringListEditor *fonts_ = nullptr;
  QSpinBox *fontSize_ = nullptr;
  QComboBox *fontWeight_ = nullptr;
  QCheckBox *italic_ = nullptr;

  QSpinBox *radius_ = nullptr;
  QSpinBox *padding_ = nullptr;
  QSpinBox *margin_ = nullptr;
  QSpinBox *borderWidth_ = nullptr;
  QSpinBox *opacity_ = nullptr;
  QCheckBox *raised_ = nullptr;

  QSpinBox *underline_ = nullptr;
  QCheckBox *underlineAccent_ = nullptr;
  QCheckBox *underlineLeft_ = nullptr;
  QCheckBox *underlineCenter_ = nullptr;
  QCheckBox *underlineRight_ = nullptr;

  QLabel *notes_ = nullptr;
  QWidget *overrides_ = nullptr;
  QVBoxLayout *overridesLayout_ = nullptr;
};

}  // namespace ws
