//
//    EgaView.cpp: EGA display view
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
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

#include "EgaView.h"
#include <QPainter>
#include <cpi.h>

EgaView::EgaView(QWidget *parent) : ThrottleableWidget(parent)
{
  this->handle = new cpi_handle;

  if (cpi_map_codepage(this->handle, nullptr) == -1)
    throw std::runtime_error("Failed to load codepage file");

  if ((this->entry = cpi_get_page(this->handle, 850)) == nullptr)
    throw std::runtime_error("Failed to select codepage 850");

  if ((this->font = cpi_get_disp_font(
         this->handle,
         this->entry,
         EGA_FONT_HEIGHT,
         EGA_FONT_WIDTH))
      == nullptr)
    throw std::runtime_error("Failed to select font (14px)");
}

EgaView::~EgaView(void)
{
  if (this->handle != nullptr) {
    cpi_unmap(this->handle);
    delete this->handle;
  }
}

void
EgaView::draw(void)
{
  int x, y = 0;
  int xp, yp;
  int index;
  QRgb *scanLine;
  this->assertDimensions();

  // This is going to hurt

  this->viewPort.fill(this->backgroundColor);

  for (auto i = 0; i < this->mRows; ++i) {
    index = i + this->rowoff;

    if (index >= 0 && index < static_cast<int>(this->scanLines.size())) {
      std::vector<EgaChar> &line = this->scanLines[static_cast<size_t>(index)];

      for (yp = 0; yp < EGA_FONT_HEIGHT && y < this->viewPort.height(); ++yp) {
        scanLine = reinterpret_cast<QRgb *>(this->viewPort.scanLine(y++));

        x = 0;
        for (unsigned int j = 0; j < line.size(); ++j) {
          struct glyph *glyph = line[j].glyph;
          QRgb fg = line[j].foreground;
          QRgb bg = line[j].background;
          char bits = glyph->bits[yp];
          for (xp = 0; xp < EGA_FONT_WIDTH && x < this->viewPort.width(); ++xp)
            scanLine[x++] = (bits & (1 << (7 - xp))) ? fg : bg;
        }
      }
    }
  }
}

void
EgaView::paint(void)
{
  if (!this->viewPort.isNull()) {
    QPainter painter(this);

    painter.drawImage(0, 0, this->viewPort);
  }
}

void
EgaView::clearBuffer(void)
{
  this->scanLines.clear();
  this->invalidate();
}

void
EgaView::setRowOffset(int offset)
{
  this->rowoff = offset;
  this->invalidate();
}

void
EgaView::assertDimensions(void)
{
  int width = this->width();
  int height = this->height();

  if (width < 1)
    width = 1;

  if (height < 1)
    height = 1;

  if (this->viewPort.width() != this->width()
      || this->viewPort.height() != this->height()) {

    this->viewPort = QImage(
          this->width(),
          this->height(),
          QImage::Format_ARGB32);

    this->mRows = (this->height() + EGA_FONT_HEIGHT - 1)  / EGA_FONT_HEIGHT;
    this->mCols = (this->width() + EGA_FONT_WIDTH - 1) / EGA_FONT_WIDTH;

    if (this->mRows < 1)
      this->mRows = 1;

    if (this->mCols < 1)
      this->mCols = 1;

    if (this->mRows > static_cast<int>(this->scanLines.size()))
      this->scanLines.resize(static_cast<size_t>(this->mRows));
  }
}

int
EgaView::rows(void) const
{
  return this->mRows;
}

int
EgaView::cols(void) const
{
  return this->mCols;
}

void
EgaView::write(
    int x,
    int y,
    const QColor &fg,
    const QColor &bg,
    const uint8_t *text,
    size_t size)
{
  this->assertDimensions();

  if (y >= 0 && x > -static_cast<int>(size) && x < this->mCols) {
    if (y >= static_cast<int>(this->scanLines.size()))
      this->scanLines.resize(static_cast<unsigned>(y + 1));

    if (x < 0) {
      text -= x;
      size -= static_cast<size_t>(-x);
    }

    if (x + static_cast<int>(size) > this->mCols)
      size = static_cast<size_t>(this->mCols - x);

    std::vector<EgaChar> &line = this->scanLines[static_cast<size_t>(y)];

    if (line.size() < (static_cast<size_t>(x) + size))
      line.resize(static_cast<size_t>(x) + size);

    for (size_t i = 0; i < size; ++i) {
      EgaChar &thisChar = line[i + static_cast<size_t>(x)];

      thisChar.glyph = cpi_get_glyph(this->font, text[i]);
      thisChar.foreground = fg.rgb();
      thisChar.background = bg.rgb();
    }

    this->invalidate();
  }
}

void
EgaView::write(int x, int y, const uint8_t *text, size_t size)
{
  this->write(x, y, this->foregroundColor, this->backgroundColor, text, size);
}

void
EgaView::write(int x, int y, std::string const &text)
{
  this->write(
        x,
        y,
        reinterpret_cast<const uint8_t *>(text.data()),
        text.size());
}

void
EgaView::write(
    int x,
    int y,
    const QColor &fg,
    const QColor &bg,
    std::string const &text)
{
  this->write(
        x,
        y,
        fg,
        bg,
        reinterpret_cast<const uint8_t *>(text.data()),
        text.size());
}
