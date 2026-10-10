#include "preview.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QVector>

#include <algorithm>

#include "schema.h"

namespace ws {
namespace {

QColor mixAlpha(const QColor &fg, const QColor &bg, double a) {
  return QColor::fromRgbF(fg.redF() * a + bg.redF() * (1 - a), fg.greenF() * a + bg.greenF() * (1 - a),
                          fg.blueF() * a + bg.blueF() * (1 - a));
}

QFont buildFont(const QStringList &stack, int size, int weight, bool italic) {
  QFont f(QStringLiteral("sans-serif"));
  for (const QString &raw : stack) {
    const QString n = raw.trimmed();
    if (n.isEmpty()) continue;
    f = QFont(n);
    break;
  }
  if (f.family().isEmpty()) f = QFont(QStringLiteral("sans-serif"));
  f.setPixelSize(qBound(6, size, 96));
  f.setWeight(weight >= 700 ? QFont::Bold : (weight <= 300 && weight > 0 ? QFont::Light : QFont::Normal));
  f.setItalic(italic);
  f.setStyleStrategy(QFont::PreferAntialias);
  return f;
}

}  // namespace

BarPreview::BarPreview(QWidget *parent) : QWidget(parent) {
  setMinimumHeight(120);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void BarPreview::setState(const StyleState *state, const Resolved *resolved) {
  state_ = state;
  resolved_ = resolved;
  update();
}

void BarPreview::setSections(const QStringList &left, const QStringList &center, const QStringList &right) {
  left_ = left;
  center_ = center;
  right_ = right;
  update();
}

void BarPreview::setBarHeight(int px) {
  barHeight_ = qBound(18, px, 90);
  update();
}

void BarPreview::setBackdrop(const QColor &c) {
  if (!c.isValid()) return;
  backdrop_ = c;
  update();
}

void BarPreview::setBackdropNamed(const QString &name) {
  backdropName_ = name;
  if (name == QLatin1String("Dark wallpaper"))
    setBackdrop(QColor(20, 20, 22));
  else if (name == QLatin1String("Light wallpaper"))
    setBackdrop(QColor(238, 238, 240));
  else if (name == QLatin1String("Mid wallpaper"))
    setBackdrop(QColor(128, 128, 130));
  else
    update();
}

void BarPreview::setHighlight(const QString &ref) {
  if (highlight_ == ref) return;
  highlight_ = ref;
  update();
}

void BarPreview::drawChip(QPainter &p, const Chip &c) {
  if (c.fill.isValid()) {
    QPainterPath path;
    path.addRoundedRect(c.rect, c.radius, c.radius);
    p.fillPath(path, c.fill);
  }
  if (c.border.isValid() && c.borderWidth > 0) {
    QPainterPath path;
    path.addRoundedRect(c.rect, c.radius, c.radius);
    p.setPen(QPen(c.border, c.borderWidth));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
  }
  p.setFont(buildFont(state_->fontStack, c.fontSize, c.weight, c.italic));
  p.setPen(c.text);
  p.drawText(c.rect, Qt::AlignCenter, c.text);
}

void BarPreview::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  p.setRenderHint(QPainter::TextAntialiasing);
  p.fillRect(rect(), backdrop_);

  if (!state_ || !resolved_) {
    p.setPen(backdrop_.lightness() > 128 ? QColor(60, 60, 60) : QColor(150, 150, 150));
    p.drawText(rect(), Qt::AlignCenter, tr("No bar selected"));
    return;
  }
  const StyleState &st = *state_;
  const Resolved &r = *resolved_;
  const QColor barFill = mixAlpha(r.c[RBackground], backdrop_, r.opacity);

  p.fillRect(QRectF(0, 0, width(), barHeight_), barFill);

  // ---- build the chips ----------------------------------------------------
  auto build = [&](const QStringList &refs, bool reverse) {
    QVector<Chip> chips;
    for (const QString &ref : refs) {
      Chip c;
      c.ref = ref;
      c.text = sampleText(ref);
      const ModuleStyle ms = st.modules.value(ref);
      const ResolvedModule m = resolveModule(ms, r, themes());
      c.fill = m.hasFill ? m.fill : (st.raised ? r.c[RSurface] : QColor());
      c.text = m.hasText ? m.text : r.c[RText];
      c.border = m.hasBorder ? m.border : QColor();
      c.borderWidth = m.hasBorder ? qMax(1, m.borderWidth) : 0;
      c.radius = m.radius >= 0 ? m.radius : st.radius;
      c.fontSize = m.fontSize > 0 ? m.fontSize : st.fontSize;
      c.weight = m.weight ? m.weight : st.weight;
      c.italic = m.italic || st.italic;
      QFont f = buildFont(st.fontStack, c.fontSize, c.weight, c.italic);
      const QFontMetricsF fm(f);
      c.rect = QRectF(0, 0, fm.horizontalAdvance(c.text) + st.padding * 2 + 6, fm.height() + 6);
      chips.append(c);
    }
    if (reverse) std::reverse(chips.begin(), chips.end());
    return chips;
  };

  QVector<Chip> left = build(left_, false);
  QVector<Chip> center = build(center_, false);
  QVector<Chip> right = build(right_, true);

  auto total = [](const QVector<Chip> &v) {
    double w = 0;
    for (const Chip &c : v) w += c.rect.width() + 4;
    return v.isEmpty() ? 0.0 : w - 4;
  };
  const double leftW = total(left), centerW = total(center), rightW = total(right);

  double chipTop = 0.0, chipH = 0.0;
  for (const QVector<Chip> *v : {&left, &center, &right})
    for (const Chip &c : *v) chipH = qMax(chipH, c.rect.height());
  chipH = chipH > 0 ? chipH : barHeight_ - 4;
  chipTop = (barHeight_ - chipH) / 2.0;

  auto place = [&](QVector<Chip> &chips, double x) {
    for (Chip &c : chips) {
      c.rect.moveTo(x, chipTop);
      x += c.rect.width() + 4;
    }
  };
  place(left, 8);
  place(center, qMax(8.0, (width() - centerW) / 2.0));
  place(right, qMax(8.0, width() - rightW - 8));

  // ---- underline ----------------------------------------------------------
  if (st.underlineThickness > 0) {
    const QColor uc = st.underlineAccent ? r.c[RAccent] : r.c[RText];
    const double t = st.underlineThickness;
    const struct {
      bool on;
      double x;
      double w;
    } parts[3] = {{st.underlineLeft, 4.0, leftW + 8.0},
                  {st.underlineCenter, qMax(4.0, (width() - centerW) / 2.0 - 4), centerW + 8.0},
                  {st.underlineRight, qMax(4.0, width() - rightW - 12), rightW + 8.0}};
    for (const auto &part : parts)
      if (part.on && part.w > 0)
        p.fillRect(QRectF(part.x, barHeight_ - t, part.w, t), uc);
  }

  // ---- chips --------------------------------------------------------------
  auto paint = [&](QVector<Chip> &chips) {
    for (Chip &c : chips) {
      drawChip(p, c);
      if (!highlight_.isEmpty() && c.ref == highlight_) {
        p.setPen(QPen(r.c[RAccent], 1.5, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(c.rect.adjusted(-1.5, -1.5, 1.5, 1.5), c.radius + 1, c.radius + 1);
      }
    }
  };
  paint(left);
  paint(center);
  paint(right);

  // ---- legend -------------------------------------------------------------
  struct Legend {
    QString label;
    QColor fill;
    QColor text;
  };
  const QVector<Legend> legend = {
      {tr("warning"), r.c[RWarning], r.stateTextWarning},
      {tr("critical"), r.c[RCritical], r.stateTextCritical},
      {tr("focused"), r.c[RAccent], r.stateTextAccent},
      {tr("muted"), QColor(), r.c[RMuted]},
      {tr("connected"), QColor(), r.c[RGood]},
  };
  const double y = barHeight_ + 14;
  QFont lf = buildFont(st.fontStack, 11, 400, false);
  p.setFont(lf);
  const QFontMetricsF lfm(lf);
  double x = 8;
  for (const Legend &lg : legend) {
    if (x + lfm.horizontalAdvance(lg.label) + 24 > width() - 4) break;
    const QRectF box(x, y, lfm.horizontalAdvance(lg.label) + 18, 18);
    QPainterPath path;
    path.addRoundedRect(box, 4, 4);
    if (lg.fill.isValid())
      p.fillPath(path, lg.fill);
    else
      p.fillPath(path, barFill);
    p.setPen(lg.text);
    p.drawText(box, Qt::AlignCenter, lg.label);
    x += box.width() + 8;
  }

  p.setPen(backdrop_.lightness() > 128 ? QColor(80, 80, 80) : QColor(150, 150, 150));
  p.setFont(buildFont(st.fontStack, 10, 400, false));
  p.drawText(QRectF(4, height() - 18, width() - 8, 14), Qt::AlignRight,
             tr("height %1 px · opacity %2 % · text contrast %3:1 · %4")
                 .arg(barHeight_)
                 .arg(qRound(r.opacity * 100))
                 .arg(contrastRatio(r.c[RText], r.c[RBackground]), 0, 'f', 1)
                 .arg(backdropName_));
}

}  // namespace ws
