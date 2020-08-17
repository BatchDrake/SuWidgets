//
//    TVDisplay.h: Television Display Widget
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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
#ifndef TVDISPLAY_H
#define TVDISPLAY_H

#include <QFrame>

#include <cmath>
#include <complex.h>
#include <vector>
#include <tgmath.h>
#include <sigutils/types.h>
#include "ThrottleableWidget.h"

#define TVDISPLAY_DEFAULT_BACKGROUND_COLOR QColor(0,     0,   0)
#define TVDISPLAY_DEFAULT_FOREGROUND_COLOR QColor(255, 255, 255)
#define TVDISPLAY_GAMMA_RANGE_SIZE 256

struct sigutils_tv_frame_buffer;

class TVDisplay : public ThrottleableWidget
{
  Q_OBJECT

  Q_PROPERTY(
      QColor backgroundColor
      READ getBackgroundColor
      WRITE setBackgroundColor
      NOTIFY backgroundColorChanged)

  Q_PROPERTY(
      QColor foregroundColor
      READ getForegroundColor
      WRITE setForegroundColor
      NOTIFY foregroundColorChanged)


  // Drawing area properties
  QPixmap contentPixmap;
  QImage  picture;
  QVector<SUFLOAT> frameAccum;
  QSize geometry;
  QSize requestedGeometry;
  qreal aspect = 4. / 3.;
  qreal brightness = 0;
  qreal contrast = 0;
  qreal contrastMul = 1;
  qreal angle = 0;
  qreal pZoom = 1;
  bool mAcummulate = false;
  qint64 mAcumCount = 0;

  bool hFlip = false;
  bool vFlip = false;
  SUFLOAT fBrightness = 0;
  SUFLOAT fContrastMul = 1;
  qreal gammaExp = 1;
  SUFLOAT gammaLookupTable[TVDISPLAY_GAMMA_RANGE_SIZE];

  // Data
  bool dirty = false;

  // PropertiesQResizeEvent
  QColor background;
  QColor foreground;
  QRgb   colors[2];

  void computeGammaLookupTable(void);
  void paintPicture(QPainter &, QPixmap const &);

public:
  void
  setBackgroundColor(const QColor &c)
  {
    colors[0] = c.rgba();
    this->background = c;
    this->invalidate();
    emit backgroundColorChanged();
  }

  const QColor &
  getBackgroundColor(void) const
  {
    return this->background;
  }

  void
  setBrightness(const qreal &val)
  {
    this->brightness = qBound(-1., val, 1.);
    this->fBrightness = SU_ASFLOAT(this->brightness);
    this->invalidate();
    emit brightnessChanged();
  }

  const qreal &
  getBrightness(void) const
  {
    return this->brightness;
  }

  void
  setContrast(const qreal &val)
  {
    this->contrast = qBound(-1., val, 1.);
    this->fContrastMul = SU_ASFLOAT(std::pow(10., this->contrast));
    this->invalidate();
    emit contrastChanged();
  }

  const qreal &
  getContrast(void) const
  {
    return this->contrast;
  }

  void
  setForegroundColor(const QColor &c)
  {
    colors[1] = c.rgba();
    this->foreground = c;
    this->invalidate();
    emit foregroundColorChanged();
  }

  const QColor &
  getForegroundColor(void) const
  {
    return this->foreground;
  }

  bool
  havePicGeometry(void) const
  {
    return this->picture.width() * this->picture.height() > 0;
  }

  void
  setRotation(qreal angle)
  {
    this->angle = angle;
    this->invalidate();
  }

  qreal
  rotation(void) const
  {
    return this->angle;
  }

  void
  setZoom(qreal zoom)
  {
    this->pZoom = qBound(1., zoom, 100.);
    this->invalidate();
  }

  qreal
  zoom(void) const
  {
    return this->pZoom;
  }

  void
  setHorizontalFlip(bool val)
  {
    this->hFlip = val;
    this->invalidate();
  }

  bool
  horizontalFlip(void) const
  {
    return this->hFlip;
  }


  void
  setVerticalFlip(bool val)
  {
    this->vFlip = val;
    this->invalidate();
  }

  bool
  verticalFlip(void) const
  {
    return this->vFlip;
  }

  qreal
  gamma(void) const
  {
    return this->gammaExp;
  }

  void
  setGamma(qreal gamma)
  {
    if (gamma < 0)
      gamma = 0;

    this->gammaExp = gamma;
    this->computeGammaLookupTable();
    this->invalidate();
  }

  QRgb
  tvSampleToRgb(SUFLOAT x)
  {
    int index = qBound(
                    0,
                    static_cast<int>(
                        (TVDISPLAY_GAMMA_RANGE_SIZE - 1)
                        * this->fContrastMul
                        * (x + this->fBrightness)),
                    TVDISPLAY_GAMMA_RANGE_SIZE - 1);

    x = this->gammaLookupTable[index];

    return
        qRgba(
          static_cast<int>(
            (1 - x) * qRed(this->colors[0])   + x * qRed(this->colors[1])),
          static_cast<int>(
            (1 - x) * qGreen(this->colors[0]) + x * qGreen(this->colors[1])),
          static_cast<int>(
            (1 - x) * qBlue(this->colors[0])  + x * qBlue(this->colors[1])),
          static_cast<int>(
            (1 - x) * qAlpha(this->colors[0]) + x * qAlpha(this->colors[1])));
  }

  bool saveToFile(QString path);
  void setAccumulate(bool);
  void setPicGeometry(int width, int height);
  void putLine(int line, const SUFLOAT *data, int size);
  void putFrame(const sigutils_tv_frame_buffer *);
  void draw(void);
  void paint(void);
  void resizeEvent(QResizeEvent *);
  QSize sizeHint(void) const;

  TVDisplay(QWidget *parent = nullptr);

signals:
  void backgroundColorChanged();
  void foregroundColorChanged();
  void brightnessChanged();
  void contrastChanged();

};

#endif
