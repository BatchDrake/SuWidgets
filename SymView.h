//
//    filename: description
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

#ifndef SYMVIEW_H
#define SYMVIEW_H

#include <QFrame>
#include "Decider.h"
#include <QResizeEvent>
#include "ThrottleableWidget.h"

class SymView : public ThrottleableWidget
{
  Q_OBJECT

    // Symbol buffer
    std::vector<Symbol> buffer; // TODO: Allow loans
    // Behavior
    bool autoScroll = true;
    bool autoStride = true;
    bool reverse    = false;
    bool pad[6];

    // Representation properties
    unsigned int bps = 1;     // Bits per symbol.
    unsigned int zoom = 1;    // Pixels per symbol
    unsigned int offset = 0;  // Offset (wrt buffer)
    int stride = 1;           // Image stride
    unsigned int pad2;
    QImage viewPort;          // Current view. Matches geometry

    // Private methods
    void assertImage(void);
    void drawToImage(
        QImage &image,
        unsigned int start,
        unsigned int end,
        unsigned int stride = 0,
        unsigned int skip = 0);

public:
  enum FileFormat {
    FILE_FORMAT_TEXT,
    FILE_FORMAT_RAW,
    FILE_FORMAT_C_ARRAY,
    FILE_FORMAT_BMP,
    FILE_FORMAT_PNG,
    FILE_FORMAT_JPEG,
    FILE_FORMAT_PPM
  };

  void clear(void);
  void save(QString const &dest, FileFormat format);

  unsigned long
  getLength(void) const
  {
    return this->buffer.size();
  }

  void
  setAutoScroll(bool val)
  {
    this->autoScroll = val;

    if (val)
      this->scrollToBottom();
  }

  void
  setAutoStride(bool val)
  {
    this->autoStride = val;

    if (val)
      this->setStride(static_cast<unsigned int>(this->width() / this->zoom));
  }

  bool
  getReverse(void) const
  {
    return this->reverse;
  }

  void
  setReverse(bool rev)
  {
    this->reverse = rev;
    if (this->buffer.size() > 0)
      this->invalidate();
  }

  bool
  getAutoScroll(void) const
  {
    return this->autoStride;
  }

  bool
  getAutoStride(void) const
  {
    return this->autoStride;
  }

  int
  getLines(void) const
  {
    return (static_cast<int>(this->buffer.size()) + this->stride - 1)
        / this->stride;
  }

  void
  setStride(unsigned int stride)
  {
    if (this->stride != static_cast<int>(stride)) {
      this->stride = static_cast<int>(stride);
      emit strideChanged(stride);
      this->invalidate();
    }
  }

  unsigned int
  getStride(void) const
  {
    return static_cast<unsigned int>(this->stride);
  }

  unsigned int
  getOffset(void) const
  {
    return this->offset;
  }

  void
  setBitsPerSymbol(unsigned int bps)
  {
    if (this->bps != bps) {
      this->bps = bps;
      this->invalidate();
    }
  }

  unsigned int
  getBitsPerSymbol(void) const
  {
    return this->bps;
  }

  void
  setOffset(unsigned int offset)
  {
    if (offset >= buffer.size())
      offset = static_cast<unsigned int>(buffer.size());

    if (offset != this->offset) {
      this->offset = offset;
      this->invalidate();
      emit offsetChanged(offset);
    }
  }

  void
  setZoom(unsigned int zoom)
  {
    if (zoom > 0 && zoom != this->zoom) {
      this->zoom = zoom;
      this->setAutoStride(this->autoStride);
      this->invalidate();
    }
  }

  unsigned int
  getZoom(void) const
  {
    return this->zoom;
  }

  SymView(QWidget *parent = nullptr);

  void scrollToBottom(void);
  void feed(std::vector<Symbol> const &x);
  void feed(const Symbol *data, unsigned int length);

  // Virtual overrides
  void draw(void);
  void paint(void);
  void mousePressEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void wheelEvent(QWheelEvent *event);

  signals:
  void offsetChanged(unsigned int);
  void strideChanged(unsigned int);

};

#endif
