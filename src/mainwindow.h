// Main window: owns the JSONC document, the bars inside it and the style state,
// and wires the layout, bar and style pages to them.
#pragma once

#include <QMainWindow>

#include "jsonc.h"
#include "layoutpage.h"
#include "preview.h"
#include "schema.h"
#include "style.h"
#include "stylepage.h"
#include "widgets.h"

class QComboBox;
class QLabel;
class QPushButton;
class QTabWidget;

namespace ws {

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QWidget *parent = nullptr);

 protected:
  void closeEvent(QCloseEvent *e) override;

 private:
  void buildUi();
  void load();
  bool save(bool reloadWaybar);
  void onModuleSelected(const QString &ref);
  void commitCurrentModule();
  void commitBarPage();
  void pushSectionsIntoNode();
  void selectBar(int index);
  void refreshBarList();
  void syncPreview();
  void setDirty(bool dirty);
  void ensureRootArray();
  QStringList presentModules() const;
  int barHeight() const;
  bool writeFile(const QString &path, const QByteArray &data, bool keepBackup);
  void readStyleFromCss();

  QString dir_, configPath_, cssPath_, stylePath_;
  JsoncDoc doc_;
  QVector<NodePtr> bars_;
  NodePtr bar_;
  int current_ = 0;
  bool singleMode_ = true;
  bool parseOk_ = true;
  bool loading_ = false;
  bool dirty_ = false;
  StyleState style_;

  QComboBox *barSel_ = nullptr;
  QTabWidget *tabs_ = nullptr;
  LayoutPage *layout_ = nullptr;
  OptForm *barForm_ = nullptr;
  JsonEditor *barExtra_ = nullptr;
  StylePage *stylePage_ = nullptr;
  BarPreview *preview_ = nullptr;
  QPushButton *save_ = nullptr;
  QPushButton *saveReload_ = nullptr;
  QPushButton *addBar_ = nullptr;
  QPushButton *removeBar_ = nullptr;
  QLabel *pathLabel_ = nullptr;
};

}  // namespace ws
