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
SymView::draw(void)
{
  int convD, asInt;
  unsigned int p;
  int x, y;
  size_t available, last;
  QRgb *scanLine;

  if (!this->size().isValid())
    return;

  // Assert a few things before going on
  this->assertImage();

  int lineSize = this->stride > this->viewPort.width()
      ? this->viewPort.width()
      : this->stride;
  unsigned int lineSkip = static_cast<unsigned int>(this->stride - lineSize);

  size_t visible = static_cast<size_t>(this->stride * this->viewPort.height());

  this->viewPort.fill(Qt::black); // Fill in black

  if (this->bps > 0 && this->buffer.size() > this->offset) {
    available = this->buffer.size() - this->offset;

    // Calculate how many symbols are visible
    if (visible > available)
      visible = available;

    // Calculate conversion coefficients
    convD = (1 << this->bps) - 1;

    x = y = 0;
    p = this->offset;
    last = p + visible;

    // Wow, C++ is truly dense
    scanLine = (QRgb *) this->viewPort.scanLine(0);

    while (p < last) {
      asInt = (static_cast<int>(this->buffer[p++]) * 255) / convD;

      // You like Cobol, right?

      scanLine[x++] = qRgb(asInt, asInt, asInt);
      if (x >= lineSize) {
        x = 0;
        scanLine = (QRgb *) this->viewPort.scanLine(++y);
        p += lineSkip; // Skip non visible pixels
      }
    }
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
  std::ofstream fs;

  fs.open(dest.toStdString(), std::ios::binary);

  if (!fs)
    throw std::ios_base::failure("Failed to save file " + dest.toStdString());

  switch (format) {
    case FILE_FORMAT_TEXT:
      for (auto p = this->buffer.begin();
           p != this->buffer.end();
           ++p)
        fs << static_cast<char>('0' + *p);
      break;

    case FILE_FORMAT_RAW:
      for (auto p = this->buffer.begin();
           p != this->buffer.end();
           ++p)
        fs << static_cast<char>(*p & ((1 << this->bps) - 1));
      break;

    case FILE_FORMAT_C_ARRAY:
      fs << "#include <stdint.h>" << std::endl << std::endl;
      fs << "static uint8_t data[" << this->buffer.size() << "] = {" << std::endl;

      for (unsigned int i = 0; i < this->buffer.size(); ++i) {
        if (i % 16 == 0)
          fs << "  ";
        fs << "0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned int>(this->buffer[i]) << ",";
        if (i % 16 == 15)
          fs << std::endl;
      }

      fs << "};" << std::endl;
      break;
  }

  fs.close();
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
