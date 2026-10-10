#include "layoutpage.h"

#include <QAbstractItemModel>
#include <QComboBox>
#include <QDrag>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHash>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace ws {
namespace {

const char *MODULE_MIME = "application/x-waybar-settings-module";
const char *SECTION_KEYS[3] = {"modules-left", "modules-center", "modules-right"};
const char *SECTION_NAMES[3] = {"Left", "Center", "Right"};

}  // namespace

// ===========================================================================
// ModuleList
// ===========================================================================

ModuleList::ModuleList(QWidget *parent) : QListWidget(parent) {
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setDragDropMode(QAbstractItemView::DragDrop);
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
  setDefaultDropAction(Qt::MoveAction);
  setDragDropOverwriteMode(false);
}

void ModuleList::startDrag(Qt::DropActions actions) {
  if (!currentItem()) return;
  auto *mime = new QMimeData;
  QStringList refs;
  for (QListWidgetItem *sel : selectedItems()) refs << sel->text();
  mime->setData(QLatin1String(MODULE_MIME), refs.join(QLatin1Char('\n')).toUtf8());
  auto *drag = new QDrag(this);
  drag->setMimeData(mime);
  drag->exec(actions);
}

void ModuleList::dragMoveEvent(QDragMoveEvent *e) {
  if (!e->mimeData()->hasFormat(QLatin1String(MODULE_MIME))) {
    e->ignore();
    return;
  }
  QListView::dragMoveEvent(e);
}

void ModuleList::dropEvent(QDropEvent *e) {
  if (!e->mimeData()->hasFormat(QLatin1String(MODULE_MIME))) {
    e->ignore();
    return;
  }
  const QStringList refs = QString::fromUtf8(e->mimeData()->data(QLatin1String(MODULE_MIME)))
                               .split(QLatin1Char('\n'), Qt::SkipEmptyParts);
  QListWidgetItem *at = itemAt(e->position().toPoint());
  int row = at ? indexRow(at) : count();
  if (at) {
    const int h = qMax(1, sizeHintForRow(indexRow(at)));
    const int yInItem = e->position().toPoint().y() - visualItemRect(at).top();
    if (yInItem > h / 2) ++row;
  }
  for (const QString &r : refs) insertItem(row++, r);
  e->acceptProposedAction();
  if (!refs.isEmpty()) emit refActivated(refs.first());
}

void ModuleList::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key_F2) {
    renameSelected();
    return;
  }
  if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
    if (QListWidgetItem *it = currentItem()) emit refActivated(it->text());
    return;
  }
  QListWidget::keyPressEvent(e);
}

void ModuleList::renameSelected() {
  QListWidgetItem *it = currentItem();
  if (!it) return;
  bool ok = false;
  const QString name = QInputDialog::getText(this, tr("Rename module"), tr("Module reference"),
                                             QLineEdit::Normal, it->text(), &ok);
  if (!ok) return;
  const QString trimmed = name.trimmed();
  if (trimmed.isEmpty() || trimmed == it->text() || trimmed.contains(QLatin1Char('*'))) return;
  it->setText(trimmed);
  emit refActivated(trimmed);
}

// ===========================================================================
// LayoutPage
// ===========================================================================

LayoutPage::LayoutPage(QWidget *parent) : QWidget(parent) {
  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  auto *split = new QSplitter(Qt::Horizontal);
  root->addWidget(split);

  auto *left = new QWidget;
  auto *lv = new QVBoxLayout(left);
  lv->setContentsMargins(4, 4, 4, 4);
  search_ = new QLineEdit;
  search_->setPlaceholderText(tr("Search modules"));
  search_->setClearButtonEnabled(true);
  palette_ = new QTreeWidget;
  palette_->setHeaderHidden(true);
  palette_->setSelectionMode(QAbstractItemView::SingleSelection);
  lv->addWidget(search_);
  lv->addWidget(palette_, 1);

  auto *targetRow = new QHBoxLayout;
  targetRow->addWidget(new QLabel(tr("Add to")));
  target_ = new QComboBox;
  target_->addItem(tr("Left"), 0);
  target_->addItem(tr("Center"), 1);
  target_->addItem(tr("Right"), 2);
  targetRow->addWidget(target_, 1);
  lv->addLayout(targetRow);

  auto *addBtn = new QPushButton(tr("Add selected module"));
  auto *customBtn = new QPushButton(tr("Add module by name..."));
  lv->addWidget(addBtn);
  lv->addWidget(customBtn);
  buildPalette();
  split->addWidget(left);

  auto *middle = new QWidget;
  auto *mv = new QVBoxLayout(middle);
  mv->setContentsMargins(0, 0, 0, 0);
  auto *listsRow = new QHBoxLayout;

  for (int i = 0; i < 3; ++i) {
    auto *col = new QWidget;
    auto *cv = new QVBoxLayout(col);
    cv->setContentsMargins(0, 0, 0, 0);
    auto *title = new QLabel(QStringLiteral("<b>%1</b>").arg(QLatin1String(SECTION_NAMES[i])));
    title->setAlignment(Qt::AlignCenter);
    cv->addWidget(title);
    auto *list = new ModuleList;
    lists_[i] = list;
    cv->addWidget(list, 1);
    auto *row = new QHBoxLayout;
    auto *up = new QPushButton(tr("Up"));
    auto *down = new QPushButton(tr("Down"));
    auto *dup = new QPushButton(tr("Duplicate"));
    auto *del = new QPushButton(tr("Remove"));
    row->addWidget(up);
    row->addWidget(down);
    row->addWidget(dup);
    row->addWidget(del);
    cv->addLayout(row);
    listsRow->addWidget(col);

    connect(list, &QListWidget::currentTextChanged, this, [this, i] {
      if (loading_) return;
      QListWidgetItem *it = list->currentItem();
      onListActivated(i, it ? it->text() : QString());
    });
    connect(list, &ModuleList::refActivated, this,
            [this, i](const QString &ref) { onListActivated(i, ref); });
    connect(list->model(), &QAbstractItemModel::rowsInserted, this, &LayoutPage::onListChanged);
    connect(list->model(), &QAbstractItemModel::rowsRemoved, this, &LayoutPage::onListChanged);
    connect(list->model(), &QAbstractItemModel::modelReset, this, &LayoutPage::onListChanged);
    connect(up, &QPushButton::clicked, this, [this, i] { moveSelected(i, -1); });
    connect(down, &QPushButton::clicked, this, [this, i] { moveSelected(i, 1); });
    connect(del, &QPushButton::clicked, this, [this, i] {
      const int r = lists_[i]->currentRow();
      if (r >= 0) delete lists_[i]->takeItem(r);
    });
    connect(dup, &QPushButton::clicked, this, [this, i] {
      QListWidgetItem *it = lists_[i]->currentItem();
      if (!it) return;
      const QString ref = uniqueAlias(it->text());
      lists_[i]->insertItem(i + 1, ref);
      lists_[i]->setCurrentRow(i + 1);
    });
  }
  mv->addLayout(listsRow, 1);
  split->addWidget(middle);

  form_ = new ModuleForm;
  split->addWidget(form_);
  split->setStretchFactor(0, 1);
  split->setStretchFactor(1, 3);
  split->setStretchFactor(2, 4);

  connect(search_, &QLineEdit::textChanged, this, &LayoutPage::applyFilter);
  connect(palette_, &QTreeWidget::itemDoubleClicked, this,
          [this] { addChosen(target_->currentData().toInt()); });
  connect(addBtn, &QPushButton::clicked, this, [this] { addChosen(target_->currentData().toInt()); });
  connect(customBtn, &QPushButton::clicked, this,
          [this] { addCustom(target_->currentData().toInt()); });
  connect(form_, &ModuleForm::changed, this, [this] {
    if (loading_) return;
    emit changed();
    emit previewChanged();
  });
  connect(form_, &ModuleForm::styleChanged, this, [this] {
    if (loading_) return;
    emit changed();
    emit previewChanged();
  });
}

QStringList LayoutPage::allRefs() const {
  QStringList out;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < lists_[i]->count(); ++j) out << lists_[i]->item(j)->text();
  return out;
}

QString LayoutPage::uniqueAlias(const QString &base) const {
  const QStringList used = allRefs();
  int n = 2;
  QString candidate = base + QStringLiteral("#%1").arg(n);
  while (used.contains(candidate)) candidate = base + QStringLiteral("#%1").arg(++n);
  return candidate;
}

void LayoutPage::buildPalette() {
  QHash<QString, QTreeWidgetItem *> categories;
  for (const ModuleDef &m : modules()) {
    QTreeWidgetItem *parent = categories.value(m.category);
    if (!parent) {
      parent = new QTreeWidgetItem(palette_, QStringList(m.category));
      QFont f = parent->font(0);
      f.setBold(true);
      parent->setFont(0, f);
      parent->setFlags(parent->flags() & ~Qt::ItemIsSelectable);
      categories.insert(m.category, parent);
    }
    auto *item = new QTreeWidgetItem(parent, QStringList(m.id));
    item->setToolTip(0, QStringLiteral("%1\n%2").arg(m.summary, cssSelector(m.id)));
    if (m.template_) item->setForeground(0, QBrush(QColor(130, 130, 130)));
  }
  palette_->expandAll();
}

void LayoutPage::applyFilter(const QString &text) {
  const QString needle = text.trimmed();
  for (int i = 0; i < palette_->topLevelItemCount(); ++i) {
    QTreeWidgetItem *cat = palette_->topLevelItem(i);
    bool anyVisible = false;
    for (int j = 0; j < cat->childCount(); ++j) {
      QTreeWidgetItem *child = cat->child(j);
      const bool match = needle.isEmpty() || child->text(0).contains(needle, Qt::CaseInsensitive) ||
                         cat->text(0).contains(needle, Qt::CaseInsensitive);
      child->setHidden(!match);
      anyVisible = anyVisible || match;
    }
    cat->setHidden(!anyVisible);
    cat->setExpanded(true);
  }
}

QString LayoutPage::chosenPaletteModule() const {
  auto *item = palette_->currentItem();
  if (!item) return QString();
  return item->text(0);
}

void LayoutPage::addChosen(int section) {
  const QString id = chosenPaletteModule();
  if (id.isEmpty()) return;
  if (id.endsWith(QLatin1Char('*'))) {
    // A template such as custom/* needs a name before it can be used.
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Name this module"), tr("Module name"),
                                               QLineEdit::Normal, QString(), &ok);
    if (!ok) return;
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) return;
    const QString prefix = id.left(id.length() - 1);
    const QString ref = trimmed.contains(QLatin1Char('/')) ? trimmed : prefix + trimmed;
    lists_[section]->addItem(ref);
    lists_[section]->setCurrentRow(lists_[section]->count() - 1);
    onListChanged();
    return;
  }
  lists_[section]->addItem(id);
  lists_[section]->setCurrentRow(lists_[section]->count() - 1);
  onListChanged();
}

void LayoutPage::addCustom(int section) {
  bool ok = false;
  const QString name = QInputDialog::getText(
      this, tr("Add a module"),
      tr("Module reference, for example custom/volume, group/hardware or clock#2"), QLineEdit::Normal,
      QString(), &ok);
  if (!ok) return;
  const QString ref = name.trimmed();
  if (ref.isEmpty() || ref.contains(QLatin1Char('*'))) {
    QMessageBox::warning(this, tr("Add a module"), tr("That is not a valid module name."));
    return;
  }
  lists_[section]->addItem(ref);
  lists_[section]->setCurrentRow(lists_[section]->count() - 1);
  onListChanged();
}

void LayoutPage::moveSelected(int section, int delta) {
  QListWidget *list = lists_[section];
  const int row = list->currentRow();
  const int target = row + delta;
  if (row < 0 || target < 0 || target >= list->count()) return;
  QListWidgetItem *item = list->takeItem(row);
  list->insertItem(target, item);
  list->setCurrentRow(target);
  onListChanged();
}

void LayoutPage::onListActivated(int section, const QString &ref) {
  Q_UNUSED(section);
  if (ref.isEmpty()) return;
  if (current_ == ref) return;
  current_ = ref;
  form_->setReference(ref);
  emit moduleSelected(ref);
  emit previewChanged();
}

void LayoutPage::onListChanged() {
  if (loading_) return;
  emit changed();
  emit previewChanged();
}

QStringList LayoutPage::modulesIn(int section) const {
  QStringList out;
  for (int i = 0; i < lists_[section]->count(); ++i) out << lists_[section]->item(i)->text();
  return out;
}

void LayoutPage::load(const QJsonObject &bar) {
  loading_ = true;
  for (int i = 0; i < 3; ++i) {
    lists_[i]->clear();
    for (const QJsonValue &v : bar.value(QLatin1String(SECTION_KEYS[i])).toArray())
      lists_[i]->addItem(v.toString());
  }
  loading_ = false;
  current_.clear();
  form_->setReference(QString());
}

void LayoutPage::store(QJsonObject *bar) const {
  for (int i = 0; i < 3; ++i) {
    QStringList refs;
    for (int j = 0; j < lists_[i]->count(); ++j) refs << lists_[i]->item(j)->text();
    if (refs.isEmpty())
      bar->remove(QLatin1String(SECTION_KEYS[i]));
    else
      (*bar)[QLatin1String(SECTION_KEYS[i])] = QJsonArray::fromStringList(refs);
  }
}

void LayoutPage::selectModule(const QString &ref) {
  for (int i = 0; i < 3; ++i) {
    const int idx = lists_[i]->findText(ref);
    if (idx >= 0) {
      lists_[i]->setCurrentRow(idx);
      return;
    }
  }
}

}  // namespace ws
