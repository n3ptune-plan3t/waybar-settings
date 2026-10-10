// A faithful, live preview of the bar: real module order, real colours, real
// fonts, and the state fills Waybar actually toggles.
#pragma once

#include <QString>
#include <QStringList>
#include <QWidget>

#include "style.h"

class QPainter;

namespace ws {

class BarPreview : public QWidget {
  Q_OBJECT
 public:
  explicit BarPreview(QWidget *parent = nullptr);

  void setState(const StyleState *state, const Resolved *resolved);
  void setSections(const QStringList &left, const QStringList &center, const QStringList &right);
  void setBarHeight(int px);
  void setBackdrop(const QColor &c);
  QString backdropName() const { return backdropName_; }
  void setBackdropNamed(const QString &name);
  void setHighlight(const QString &ref);

  QSize sizeHint() const override { return QSize(640, 110); }

 protected:
  void paintEvent(QPaintEvent *e) override;

 private:
  struct Chip {
    QString ref;
    QString label;
    QRectF rect;
    QColor fill;
    QColor fg;
    QColor border;
    int borderWidth = 0;
    int radius = 0;
    int fontSize = 0;
    int weight = 400;
    bool italic = false;
  };

  void drawChip(QPainter &p, const Chip &c);

  const StyleState *state_ = nullptr;
  const Resolved *resolved_ = nullptr;
  QStringList left_, center_, right_;
  QColor backdrop_ = QColor(24, 24, 24);
  QString backdropName_ = QStringLiteral("Dark wallpaper");
  int barHeight_ = 34;
  QString highlight_;
};

}  // namespace ws
