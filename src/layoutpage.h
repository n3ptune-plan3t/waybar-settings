// The bar layout page: a searchable module palette on the left, the three
// Waybar sections in the middle (drag and drop between them), and the schema
// driven editor for whichever module is selected on the right.
#pragma once

#include <QJsonObject>
#include <QListWidget>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "schema.h"
#include "widgets.h"

class QComboBox;
class QLineEdit;
class QTreeWidget;

namespace ws {

class ModuleList : public QListWidget {
  Q_OBJECT
 public:
  explicit ModuleList(QWidget *parent = nullptr);

 signals:
  void refActivated(const QString &ref);

 protected:
  void startDrag(Qt::DropActions actions) override;
  void dragMoveEvent(QDragMoveEvent *e) override;
  void dropEvent(QDropEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;

 private:
  void renameSelected();
};

class LayoutPage : public QWidget {
  Q_OBJECT
 public:
  explicit LayoutPage(QWidget *parent = nullptr);

  void load(const QJsonObject &bar);
  void store(QJsonObject *bar) const;

  QString currentModule() const { return current_; }
  void selectModule(const QString &ref);
  QStringList modulesIn(int section) const;
  ModuleForm *moduleForm() const { return form_; }

 signals:
  void changed();
  void previewChanged();
  void moduleSelected(const QString &ref);

 private:
  void buildPalette();
  void applyFilter(const QString &text);
  QString chosenPaletteModule() const;
  void addChosen(int section);
  void addCustom(int section);
  void moveSelected(int section, int delta);
  void onListActivated(int section, const QString &ref);
  void onListChanged();
  QStringList allRefs() const;
  QString uniqueAlias(const QString &base) const;

  QLineEdit *search_ = nullptr;
  QTreeWidget *palette_ = nullptr;
  QComboBox *target_ = nullptr;
  ModuleList *lists_[3] = {};
  ModuleForm *form_ = nullptr;
  QString current_;
  bool loading_ = false;
};

}  // namespace ws
