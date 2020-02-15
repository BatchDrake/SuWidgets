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
    unsigned int lineSize,
    unsigned int lineSkip)
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

  while (p < end) {
    asInt = (static_cast<int>(this->buffer[p++]) * 255) / convD;
    if (this->reverse)
      asInt = ~asInt;

    // You like Cobol, right?

    scanLine[x++] = qRgb(asInt, asInt, asInt);
    if (x >= lineSize) {
      x = 0;
      scanLine = reinterpret_cast<QRgb *>(image.scanLine(++y));
      p += lineSkip; // Skip non visible pixels
    }
  }
}

void
SymView::draw(void)
{
  unsigned int available;

  if (!this->size().isValid())
    return;

  // Assert a few things before going on
  this->assertImage();

  int lineSize = this->stride > this->viewPort.width()
      ? this->viewPort.width()
      : this->stride;
  unsigned int lineSkip = static_cast<unsigned int>(this->stride - lineSize);

  unsigned visible = static_cast<unsigned>(this->stride * this->viewPort.height());

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
          static_cast<unsigned int>(lineSize),
          lineSkip);
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

  if (lines > this->height())
    new_offset = static_cast<unsigned int>(
          (lines - this->height()) * this->stride);

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
SymView::keyPressEvent(QKeyEvent *)
{

}

void
SymView::wheelEvent(QWheelEvent *)
{

}
