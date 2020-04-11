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

#include "SymView.h"
#include <QPainter>
#include <fstream>
#include <iomanip>

SymView::SymView(QWidget *parent) :
  ThrottleableWidget(parent)
{
  this->setFocusPolicy(Qt::StrongFocus);
  this->setMouseTracking(true);
  this->invalidate();
}

void
SymView::assertImage(void)
{
  if (this->viewPort.width() != this->width()
      || this->viewPort.height() != this->height()) {
    if (this->autoStride)
      this->setStride(static_cast<unsigned int>(this->width()));

    this->viewPort = QImage(
          this->width(),
          this->height(),
          QImage::Format_ARGB32);
  }
}

void
SymView::drawToImage(
    QImage &image,
    unsigned int start,
    unsigned int end,
    unsigned int zoom,
    unsigned int lineSize,
    unsigned int lineSkip,
    unsigned int lineStart)
{
  unsigned int x = 0;
  int y = 0;
  int asInt, convD;
  unsigned int p = start;
  QRgb *scanLine;

  // Calculate conversion coefficients
  convD = (1 << this->bps) - 1;

  scanLine = reinterpret_cast<QRgb *>(image.scanLine(0));

  if (lineSize == 0)
    lineSize = static_cast<unsigned int>(image.width());

  if (this->zoom == 1) {
    while (p < end) {
      asInt = (static_cast<int>(this->buffer[p++]) * 255) / convD;
      if (this->reverse)
        asInt = ~asInt;

      // You like Cobol, right?
      if (x++ >= lineStart)
        scanLine[x - 1 - lineStart] = qRgb(asInt, asInt, asInt);

      if (x >= lineSize) {
        x = 0;
        scanLine = reinterpret_cast<QRgb *>(image.scanLine(++y));
        p += lineSkip; // Skip non visible pixels
      }
    }
  } else {
    // If zoom is bigger than one, we decide which pixel belongs to which
    // symbol instead. We assume the symbol stream to be divided in
    // blocks of lineSize symbols, starting by start. Therefore:
    //
    //  x = i / zoom
    //  y = j / zoom
    //
    //  if (x >= lineSize) p(x, y) = 0
    //  p(x, y) = start + x + y * lineSize
    //

    unsigned int stride = lineSize + lineSkip;
    bool highlight = zoom > 2 && this->hoverX > 0 && this->hoverY > 0;

    int width = static_cast<int>(stride * zoom);
    if (width > image.width())
      width = image.width();

    for (int j = 0; j < image.height(); ++j) {
      unsigned int y = static_cast<unsigned>(j) / zoom;
      scanLine = reinterpret_cast<QRgb *>(image.scanLine(j));
      for (int i = 0; i < width; ++i) {
        unsigned x = static_cast<unsigned>(i) / zoom
            + lineStart;
        if (x < stride) {
          p = start + x + y * stride;
          asInt = (static_cast<int>(this->buffer[p]) * 255) / convD;
          if (p >= end)
            break;
          if (this->reverse)
            asInt = ~asInt;

          scanLine[i] = qRgb(asInt, asInt, asInt);
        }
      }

      if (p > end)
        break;
    }

    if (highlight) {
      unsigned int y = static_cast<unsigned>(this->hoverY) / zoom;
      unsigned int x = static_cast<unsigned>(this->hoverX) / zoom;
      unsigned int highlightPtr = start + x + lineStart + y * stride;
      unsigned int uWidth = stride - lineStart;

      if (highlightPtr >= start
          && highlightPtr < end
          && x < uWidth) {
        x *= zoom;
        y *= zoom;
        uWidth *= zoom;

        emit hoverSymbol(highlightPtr);

        for (unsigned int j = 0; j < zoom; ++j)
          if (y + j < static_cast<unsigned>(image.height())) {
            scanLine = reinterpret_cast<QRgb *>(
                  image.scanLine(static_cast<int>(y + j)));
            if (j == 0 || j == zoom - 1) {
              for (unsigned int i = x; i < qBound(0u, x + zoom, uWidth); ++i)
                scanLine[i] = qRgb(255, 0, 0);
            } else {
              scanLine[x] = qRgb(255, 0, 0);

              if (x + zoom <= uWidth)
                scanLine[x + zoom - 1] = qRgb(255, 0, 0);
            }
          }
      }
    }

  }
}

void
SymView::draw(void)
{
  unsigned int available;
  int zoom = static_cast<int>(this->zoom);
  if (!this->size().isValid())
    return;

  // Assert a few things before going on
  this->assertImage();

  int lineSize = this->stride > this->viewPort.width() / zoom
      ? this->viewPort.width() / zoom
      : this->stride;
  unsigned int lineSkip = static_cast<unsigned int>(this->stride - lineSize);
  unsigned int lineStart = static_cast<unsigned int>(this->hOffset);

  // lineSize: Number of visible symbols
  // lineSkip: Number of invisible symbols
  // If I start beyond the number of visible symbols, I will fall beyond
  // the line limits.

  if (lineStart > lineSkip)
    lineStart = lineSkip;

  unsigned int visibleLines =
      (static_cast<unsigned>(this->height()) + this->zoom - 1) / this->zoom;
  unsigned visible = static_cast<unsigned>(this->stride) * visibleLines;

  this->viewPort.fill(Qt::black); // Fill in black

  if (this->bps > 0 && this->buffer.size() > this->offset) {
    available = static_cast<unsigned>(this->buffer.size()) - this->offset;

    // Calculate how many symbols are visible
    if (visible > available)
      visible = available;

    this->drawToImage(
          this->viewPort,
          this->offset,
          this->offset + visible,
          this->zoom,
          static_cast<unsigned int>(lineSize) + lineStart,
          lineSkip - lineStart,
          lineStart);
  }
}

void
SymView::clear(void)
{
  this->buffer.clear();
  this->offset = 0;
  this->invalidate();
}

// Can you belive that some people out there code like this for a living?
void
SymView::save(QString const &dest, FileFormat format)
{
  QFile file(dest);
  char b;
  QImage img;

  file.open(QIODevice::WriteOnly);

  if (!file.isOpen())
    throw std::ios_base::failure("Failed to save file " + dest.toStdString());

  if (format > FILE_FORMAT_C_ARRAY) {
    // Initialize image
    img = QImage(this->stride, this->getLines(), QImage::Format_ARGB32);
    drawToImage(
          img,
          this->offset % static_cast<unsigned>(this->stride),
          static_cast<unsigned>(this->buffer.size()));
  }

  switch (format) {
    case FILE_FORMAT_TEXT:
      for (auto p = this->buffer.begin();
           p != this->buffer.end();
           ++p) {
        b = '0' + static_cast<char>(*p);
        file.write(&b, 1);
      }
      break;

    case FILE_FORMAT_RAW:
      for (auto p = this->buffer.begin();
           p != this->buffer.end();
           ++p) {
        b = static_cast<char>(*p & ((1 << this->bps) - 1));
        file.write(&b, 1);
      }
      break;

    case FILE_FORMAT_C_ARRAY:
      char buf[8];
      file.write("#include <stdint.h>\n\n");
      file.write(
            ("static uint8_t data[" + QString::number(this->buffer.size()) + "] = {\n")
            .toUtf8());

      for (unsigned int i = 0; i < this->buffer.size(); ++i) {
        if (i % 16 == 0)
          file.write("  ");
        snprintf(buf, 8, "0x%02x, ", static_cast<unsigned int>(this->buffer[i]));
        file.write(buf);

        if (i % 16 == 15)
          file.write("\n");
      }

      file.write("};\n");
      break;

    case FILE_FORMAT_BMP:
      img.save(&file, "BMP");
      break;

    case FILE_FORMAT_PNG:
      img.save(&file, "PNG");
      break;

    case FILE_FORMAT_JPEG:
      img.save(&file, "JPEG");
      break;

    case FILE_FORMAT_PPM:
      img.save(&file, "PPM");
      break;
  }
}

void
SymView::paint(void)
{
  QPainter painter(this);

  painter.drawImage(0, 0, this->viewPort);
}

void
SymView::scrollToBottom(void)
{
  unsigned int new_offset = 0;

  int size  = static_cast<int>(this->buffer.size());
  int lines = (size + this->stride - 1) / this->stride;

  if (lines > this->height() / static_cast<int>(this->zoom))
    new_offset = static_cast<unsigned int>(
          (lines - this->height()  / static_cast<int>(this->zoom))
          * this->stride);

  this->setOffset(new_offset);
}

void
SymView::feed(const Symbol *data, unsigned int length)
{
  this->buffer.insert(
        this->buffer.end(),
        data,
        data + length);

  if (length > 0) {
    if (this->autoScroll)
      this->scrollToBottom();

    this->invalidate();
  }
}

void
SymView::feed(std::vector<Symbol> const &x)
{
  this->feed(x.data(), static_cast<unsigned int>(x.size()));
}

void
SymView::mousePressEvent(QMouseEvent *)
{

}

void
SymView::mouseMoveEvent(QMouseEvent *event)
{
  if (this->zoom > 2) {
    hoverX = event->x();
    hoverY = event->y();
    this->invalidate();
  }
}

void
SymView::keyPressEvent(QKeyEvent *event)
{
  unsigned int lineSize = static_cast<unsigned>(this->stride);
  unsigned int lineCount = static_cast<unsigned>(this->height()) / this->zoom;
  unsigned int pageSize = lineSize * lineCount;
  unsigned int visible = static_cast<unsigned>(this->width()) / this->zoom;

  switch (event->key()) {
    case Qt::Key_PageUp:
      this->setOffset(
            this->offset < pageSize
            ? 0
            : this->offset - pageSize);
      break;

    case Qt::Key_PageDown:
      if (this->getLength() > pageSize) {
        this->setOffset(
              static_cast<unsigned int>(
              this->offset < this->getLength() - pageSize
              ? this->offset + pageSize
              : this->getLength() - pageSize));
      }
      break;

    case Qt::Key_Up:
      this->setOffset(
            this->offset < lineSize
            ? 0
            : this->offset - lineSize);
      break;

    case Qt::Key_Down:
      if (this->getLength() > pageSize) {
        this->setOffset(
              static_cast<unsigned int>(
              this->offset + lineSize < this->getLength() - pageSize
              ? this->offset + lineSize
              : this->getLength() - pageSize));
      }
      break;

    case Qt::Key_Home:
      this->setOffset(0);
      break;

    case Qt::Key_End:
      this->setOffset(static_cast<unsigned int>(this->getLength() - pageSize));
      break;

    case Qt::Key_Left:
      if (this->hOffset > 0)
        this->setHOffset(this->hOffset - 1);
      break;

    case Qt::Key_Right:
      if (static_cast<unsigned>(this->hOffset) + visible <= lineSize)
        this->setHOffset(this->hOffset + 1);
      break;

    case Qt::Key_Plus:
      if (event->modifiers() & Qt::ControlModifier)
        this->setZoom(this->zoom + 1);
      break;

    case Qt::Key_Minus:
      if (event->modifiers() & Qt::ControlModifier && this->zoom > 1)
        this->setZoom(this->zoom - 1);
      break;
  }
}

void
SymView::wheelEvent(QWheelEvent *event)
{
  unsigned int lineSize = static_cast<unsigned>(this->stride);
  unsigned int lineCount = static_cast<unsigned>(this->height()) / this->zoom;
  unsigned int pageSize = lineSize * lineCount;
  int count = (event->delta() + 119) / 120;

  if (event->modifiers() & Qt::ControlModifier) {
    if (count <= 0) {
      unsigned int delta = static_cast<unsigned>(-count) + 1;
      this->setZoom(delta < this->zoom ? this->zoom - delta : 1);
    } else {
      unsigned int delta = static_cast<unsigned>(count);
      this->setZoom(
            this->zoom + delta > SYMVIEW_MAX_ZOOM
            ? SYMVIEW_MAX_ZOOM
            : this->zoom + delta);
    }
  } else {
    if (count > 0) {
      unsigned int delta = static_cast<unsigned>(count);
      unsigned int step = 5 * delta * lineSize * this->zoom;
      this->setOffset(
            this->offset < step
            ? 0
            : this->offset - step);
    } else {
      unsigned int delta = static_cast<unsigned>(-count) + 1;
      unsigned int step = 5 * delta * lineSize * this->zoom;
      if (this->getLength() > pageSize) {
        this->setOffset(
              static_cast<unsigned int>(
                this->offset + step < this->getLength() - pageSize
                ? this->offset + step
                : this->getLength() - pageSize));
      }
    }
  }
}
