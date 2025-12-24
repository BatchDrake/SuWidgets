//
//    Histogram.cpp: Histogram widget
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as
//    published by the Free Software Foundation, either version 3 of the
//    License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this program.  If not, see
//    <http://www.gnu.org/licenses/>
//

#include "Histogram.h"
#include "SuWidgetsHelpers.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

#define CROSS_MARK_REL_DIM .1f
#define RIGHT_MARGIN       .01f
#define LEFT_MARGIN        .01f
#define TOP_MARGIN         .01f
#define BOTTOM_MARGIN      .01f
#define HORIZONTAL_SCALE_INV (1.f + RIGHT_MARGIN + LEFT_MARGIN)
#define HORIZONTAL_SCALE   (1 / HORIZONTAL_SCALE_INV)
#define VERTICAL_SCALE     (1 / (1.f + TOP_MARGIN + BOTTOM_MARGIN))

#define LABEL_PRECISION    3

QPoint
Histogram::floatToScreenPoint(float x, float y)
{
  QPoint qp(
    this->ox + static_cast<int>(
          this->width  * (x * HORIZONTAL_SCALE + LEFT_MARGIN)),
    this->oy - static_cast<int>(
          (this->height - this->legendTextHeight)
          * (y * VERTICAL_SCALE + BOTTOM_MARGIN))
        - this->legendTextHeight);

  return qp;
}

void
Histogram::reset(void)
{
  std::fill(this->history.begin(), this->history.end(), 0);
  this->max = 0;
  this->invalidate();
}

qreal
Histogram::getDataRange(void) const
{
  if (this->dataRangeOverride > 0)
    return this->dataRangeOverride;

  if (this->decider != nullptr
      && this->decider->getDecisionMode() == Decider::ARGUMENT)
    return 2 * M_PI;

  return 1;
}

qreal
Histogram::getDisplayRange(void) const
{
  if (this->displayRangeOverride > 0)
    return this->displayRangeOverride;

  if (this->decider != nullptr
      && this->decider->getDecisionMode() == Decider::ARGUMENT)
    return 360;

  return 1;
}

QString
Histogram::getUnits(void) const
{
  if (this->unitsOverride.size() > 0)
    return this->unitsOverride;

  if (this->decider != nullptr
      && this->decider->getDecisionMode() == Decider::ARGUMENT)
    return "º";

  return "";
}


void
Histogram::recalculateDisplayData(void)
{
  int width, height;

  width  = this->geometry.width();
  height = this->geometry.height();

  // Cache data
  this->width = width;
  this->height = height;

  // Recalculate origin
  this->ox = 0;
  this->oy = this->height - 1;

  if (this->decider != nullptr) {
    qreal range;
    qreal divLen;
    bool degs = std::fabs(this->getDisplayRange() - 360) <
        std::numeric_limits<qreal>::epsilon();

    // Recalculate ranges
    range = static_cast<qreal>(
          this->decider->getMaximum() - this->decider->getMinimum());
    range *= this->getDisplayRange() / this->getDataRange();

    // If we are displaying degrees, we override the automatic behavior
    if (degs) {
      if (range >= 180)
        divLen = 45;
      else if (range >= 90)
        divLen = 15;
      else
        degs = false;
    }

    if (!degs) {
      divLen = pow(10, std::floor(std::log10(range)));

      if (range / divLen < 5) {
        divLen /= 2;
        if (range / divLen < 5) {
          divLen /= 2.5;
          if (range / divLen < 5) {
            divLen /= 4;
          }
        }
      }
    }

    this->hDivDegs = divLen;
  }
}

void
Histogram::drawHorizontalAxes(QPainter &p)
{
  QPen pen(this->axes);

  pen.setStyle(Qt::DotLine);
  p.setPen(pen);

  for (int i = 0; i < 10; ++i)
    p.drawLine(
          this->floatToScreenPoint(0.f, i * .1f),
          this->floatToScreenPoint(1.f, i * .1f));
}

void
Histogram::drawVerticalAxes(QPainter &p)
{
  QFont font;
  QFontMetrics metrics(font);
  QRect rect;
  QPen pen(this->axes);
  int axis;

  pen.setStyle(Qt::DotLine);
  p.setPen(pen);
  p.setFont(font);

  if (this->legendTextHeight == 0)
    this->legendTextHeight = metrics.height();

  if (this->hDivDegs > 0) {
    qreal dataRange = this->getDataRange();
    qreal start = static_cast<qreal>(this->decider->getMinimum()) / dataRange;
    qreal end   = static_cast<qreal>(this->decider->getMaximum()) / dataRange;
    qreal range;
    qreal fullRange = this->getDisplayRange();

    start *= fullRange;
    end   *= fullRange;

    range = end - start;

    // Draw axes
    axis = static_cast<int>(std::floor(start / this->hDivDegs));

    while (axis * this->hDivDegs <= end) {
      QPoint pt =
          this->floatToScreenPoint(
            static_cast<float>((axis * this->hDivDegs - start) / range),
            0.);

      if (pt.x() > 0)
        p.drawLine(pt.x(), 0, pt.x(), pt.y());
      ++axis;
    }

    // Draw labels
    p.setPen(this->text);
    axis = static_cast<int>(std::floor(start / this->hDivDegs));

    while (axis * this->hDivDegs <= end) {
      QPoint pt =
          this->floatToScreenPoint(
            static_cast<float>((axis * this->hDivDegs - start) / range),
            1.);

      if (pt.x() > 0) {
        QString label;
        int tw;

        label = SuWidgetsHelpers::formatQuantity(
              axis * this->hDivDegs,
              LABEL_PRECISION,
              this->getUnits(),
              this->getUnits() == "º");

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        tw = metrics.horizontalAdvance(label);
#else
        tw = metrics.width(label);
#endif // QT_VERSION_CHECK

        rect.setRect(
              pt.x() - tw / 2,
              this->geometry.height() - this->legendTextHeight,
              tw,
              this->legendTextHeight);
        p.drawText(rect, Qt::AlignHCenter | Qt::AlignBottom, label);
      }
      ++axis;
    }
  }

}

void
Histogram::drawAxes(void)
{
  QPainter painter(&this->axesPixmap);
  QPen pen(this->axes);

  painter.fillRect(0, 0, this->width, this->height, this->background);

  pen.setStyle(Qt::SolidLine);
  pen.setWidth(1);
  painter.setPen(pen);

  painter.drawLine(
        this->floatToScreenPoint(0, -TOP_MARGIN),
        this->floatToScreenPoint(0, 1.f + TOP_MARGIN));

  painter.drawLine(
        this->floatToScreenPoint(1, -TOP_MARGIN),
        this->floatToScreenPoint(1, 1.f + TOP_MARGIN));

  painter.drawLine(
        this->floatToScreenPoint(-LEFT_MARGIN, 0),
        this->floatToScreenPoint(1.f + RIGHT_MARGIN, 0));

  painter.drawLine(
        this->floatToScreenPoint(-LEFT_MARGIN, 1),
        this->floatToScreenPoint(1.f + RIGHT_MARGIN, 1));

  if (this->decider != nullptr) {
    float delta;

    this->drawVerticalAxes(painter);
    this->drawHorizontalAxes(painter);

    // Draw outer border
    pen.setWidth(1);

    // Draw intervals.
    pen.setStyle(Qt::SolidLine);
    pen.setColor(this->interval);
    painter.setPen(pen);

    delta = 1.f  / this->decider->getIntervals();

    if (this->drawThreshold)
      for (int i = 0; i < this->decider->getIntervals(); ++i)
        painter.drawLine(
              this->floatToScreenPoint(i * delta, 0),
              this->floatToScreenPoint(i * delta, 1));
  }

  this->axesDrawn = true;
}

void
Histogram::drawHistogram(void)
{
  QPainter painter(&this->contentPixmap);
  QPen pen(this->foreground);
  float old = 0, curr;
  float K;
  float max = this->max == 0 ? 1. : this->max;

  pen.setStyle(Qt::SolidLine);
  pen.setWidth(1);
  painter.setPen(pen);
  painter.setRenderHint(QPainter::Antialiasing);

  K = static_cast<float>(1.f / (this->history.size() - 1));

  pen.setColor(QColor(255, 0, 0));
  painter.setPen(pen);
  if (this->model.size() == this->history.size()) {
    for (unsigned int i = 0; i < this->model.size(); ++i) {
      curr = this->model[i];
      if (i > 0) {
        painter.drawLine(
              this->floatToScreenPoint((i - 1) * K, old),
              this->floatToScreenPoint(i * K, curr));
      }
      old = curr;
    }
  }

  pen.setColor(this->foreground);
  painter.setPen(pen);
  for (unsigned int i = 0; i < this->history.size(); ++i) {
    curr = this->history[i] / max;
    if (i > 0) {
      painter.drawLine(
            this->floatToScreenPoint((i - 1) * K, old),
            this->floatToScreenPoint(i * K, curr));
    }
    old = curr;
  }

  if (this->selecting) {
    QColor copy = this->foreground;
    copy.setAlpha(127);
    pen.setWidth(2);
    pen.setColor(copy);
    painter.setPen(pen);

    QPainterPath path;
    painter.drawLine(
          this->floatToScreenPoint(this->sStart, 0),
          this->floatToScreenPoint(this->sStart, 1));

    painter.drawLine(
          this->floatToScreenPoint(this->sEnd, 0),
          this->floatToScreenPoint(this->sEnd, 1));

    painter.drawLine(
          this->floatToScreenPoint(this->sStart, 0.5f),
          this->floatToScreenPoint(this->sEnd, 0.5f));

    path.addEllipse(
          this->floatToScreenPoint(this->sStart, 0.5f),
          4.,
          4.);

    path.addEllipse(
          this->floatToScreenPoint(this->sEnd, 0.5f),
          4.,
          4.);

    painter.fillPath(path, this->axes);
  }
}


void
Histogram::draw(void)
{
  if (!this->size().isValid())
    return;

  if (this->geometry != this->size()) {
    this->geometry = this->size();
    this->history.resize(static_cast<unsigned>(this->geometry.width()));
    this->reset();
    this->invalidate();
    this->contentPixmap  = QPixmap(this->geometry.width(), this->geometry.height());
    this->axesPixmap = QPixmap(this->geometry.width(), this->geometry.height());
    this->axesDrawn = false;
    emit blanked();
  }

  if (!this->axesDrawn) {
    this->recalculateDisplayData();
    this->drawAxes();
    emit this->axesUpdated();
  }

  /* Dump to content pixmap */
  this->contentPixmap = this->axesPixmap.copy(
        0,
        0,
        this->geometry.width(),
        this->geometry.height());

  this->drawHistogram();
}


void
Histogram::paint(void)
{
  QPainter painter(this);
  painter.drawPixmap(0, 0, this->contentPixmap);
}

void
Histogram::setDecider(Decider *decider)
{
  // Important to ALWAYS INVALIDATE this.
  this->decider = decider;
  this->setOrderHint(this->decider->getBps());
  this->axesDrawn = false;
  this->invalidate();
}

void
Histogram::setUpdateDecider(bool upd)
{
  this->updateDecider = upd;
}

void
Histogram::setDrawThreshold(bool draw)
{
  this->drawThreshold = draw;
  this->invalidate();
}

// TODO: Use templates!!

void
Histogram::feed(const SUFLOAT *data, unsigned int length)
{
  if (this->decider != nullptr && length > 0) {
    int bin;
    bool invalidate = false;
    unsigned long hlen = this->history.size();
    float arg;
    float mAngl = this->decider->getMinimum();
    float delta = this->decider->getMaximum() - mAngl;

    for (auto i = 0u; i < length; ++i) {
      arg = data[i];
      arg = (arg - mAngl) / delta;
      bin = static_cast<int>(hlen * arg);

      if (bin >= 0 && bin < static_cast<int>(hlen)) {
        if (++this->history[static_cast<unsigned>(bin)] > this->max)
          this->max = this->history[static_cast<unsigned>(bin)];
        invalidate = true;
      }
    }

    if (invalidate)
      this->invalidate();
  }
}

void
Histogram::feed(const SUCOMPLEX *samples, unsigned int length)
{
  if (this->decider != nullptr && length > 0) {
    int bin;
    bool invalidate = false;
    unsigned long hlen = this->history.size();
    float arg;
    float min   = this->decider->getMinimum();
    float delta = this->decider->getMaximum() - min;

    switch (this->decider->getDecisionMode()) {
      case Decider::ARGUMENT:
        for (auto i = 0u; i < length; ++i) {
          SUWIDGETS_DETECT_ARGUMENT(arg, samples[i]);

          arg = (arg - min) / delta;
          bin = static_cast<int>(hlen * arg);

          if (bin >= 0 && bin < static_cast<int>(hlen)) {
            if (++this->history[static_cast<unsigned>(bin)] > this->max)
              this->max = this->history[static_cast<unsigned>(bin)];
            invalidate = true;
          }
        }
        break;

      case Decider::MODULUS:
        for (auto i = 0u; i < length; ++i) {
          SUWIDGETS_DETECT_MODULUS(arg, samples[i]);

          arg = (arg - min) / delta;
          bin = static_cast<int>(hlen * arg);

          if (bin >= 0 && bin < static_cast<int>(hlen)) {
            if (++this->history[static_cast<unsigned>(bin)] > this->max)
              this->max = this->history[static_cast<unsigned>(bin)];
            invalidate = true;
          }
        }
        break;
    }


    if (invalidate)
      this->invalidate();
  }
}

void
Histogram::setSNRModel(std::vector<float> const &model)
{
  if (model.size() == this->history.size()) {
    this->model.resize(model.size());
    this->model = model;
  }
}

void
Histogram::resetDecider(void)
{
  if (this->decider !=  nullptr) {
    if (this->updateDecider) {
      if (this->decider->getDecisionMode() == Decider::MODULUS) {
        this->decider->setMinimum(0);
        this->decider->setMaximum(static_cast<float>(this->getDataRange()));
      } else {
        this->decider->setMinimum(-static_cast<float>(.5 * this->getDataRange()));
        this->decider->setMaximum(static_cast<float>(.5 * this->getDataRange()));
      }
      this->axesDrawn = false;
      this->reset();
      emit blanked();
    }

    emit resetLimits();
  }
}

// Decider configuration
void
Histogram::mouseMoveEvent(QMouseEvent *event)
{
  if (this->selecting) {
    float x;
    x = HORIZONTAL_SCALE_INV * (
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          static_cast<float>(event->position().x())  / this->width - LEFT_MARGIN);
#else
          static_cast<float>(event->x())  / this->width - LEFT_MARGIN);
#endif
    this->sEnd = x;
    this->invalidateHard();
  }
}

void
Histogram::mousePressEvent(QMouseEvent *event)
{  if (event->button() == Qt::MouseButton::LeftButton) {
    float x;
    x = HORIZONTAL_SCALE_INV * (
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          static_cast<float>(event->position().x())  / this->width - LEFT_MARGIN);
#else
          static_cast<float>(event->x())  / this->width - LEFT_MARGIN);
#endif
    this->selecting = true;
    this->sStart = x;
    this->sEnd = x;
  } else if (event->button() == Qt::MouseButton::RightButton) {
    this->selecting = false;
    this->resetDecider();
  }

  this->invalidateHard();
}

void
Histogram::mouseReleaseEvent(QMouseEvent *event)
{
  if (this->selecting) {
    float x;
    float add;
    int intervals = 1 << this->bits;
    x = HORIZONTAL_SCALE_INV * (
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          static_cast<float>(event->position().x())  / this->width - LEFT_MARGIN);
#else
          static_cast<float>(event->x())  / this->width - LEFT_MARGIN);
#endif
    this->sEnd = x;
    this->selecting = false;

    if (this->sStart > this->sEnd) {
      x = this->sEnd;
      this->sEnd = this->sStart;
      this->sStart = x;
    }

    // Adjust limits according to current bps config
    add = (this->sEnd - this->sStart) / (2 * intervals);

    this->sStart -= add;
    this->sEnd   += add;

    if (this->decider != nullptr) {
      float min = this->decider->getMinimum();
      float max = this->decider->getMaximum();
      float range = max - min;

      if (this->updateDecider) {
        this->decider->setMinimum(min + this->sStart * range);
        this->decider->setMaximum(min + this->sEnd   * range);
        this->axesDrawn = false;
        this->reset();
        emit blanked();
      }

      emit newLimits(
            min + (this->sStart + add) * range,
            min + (this->sEnd - add)   * range);
    }
  }

  this->invalidateHard();
}

Histogram::Histogram(QWidget *parent) :
  ThrottleableWidget(parent)
{
  this->contentPixmap = QPixmap(0, 0);
  this->axesPixmap = QPixmap(0, 0);

  this->history.resize(HISTOGRAM_DEFAULT_HISTORY_SIZE);

  this->background = HISTOGRAM_DEFAULT_BACKGROUND_COLOR;
  this->foreground = HISTOGRAM_DEFAULT_FOREGROUND_COLOR;
  this->axes       = HISTOGRAM_DEFAULT_AXES_COLOR;
  this->text       = HISTOGRAM_DEFAULT_TEXT_COLOR;
  this->interval   = HISTOGRAM_DEFAULT_INTERVAL_COLOR;

  this->invalidate();
}
