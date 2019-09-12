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
#ifndef EGAVIEW_H
#define EGAVIEW_H

#include <ThrottleableWidget.h>
#include <vector>

struct cpi_handle;
struct cpi_entry;
struct cpi_font;

struct glyph;

struct EgaChar {
  struct glyph *glyph = nullptr;
  QRgb foreground = qRgb(127, 127, 127);
  QRgb background = qRgb(0, 0, 0);
};

#define EGA_FONT_WIDTH  8
#define EGA_FONT_HEIGHT 8

class EgaView : public ThrottleableWidget
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

protected:
  // You C++ nerds finally fixed it, huh?
  std::vector<std::vector<EgaChar>> scanLines;
  int rows;
  int cols;
  int rowoff = 0;

  QColor foregroundColor;
  QColor backgroundColor;

  struct cpi_handle *handle = nullptr;
  struct cpi_entry *entry = nullptr;
  struct cpi_disp_font *font = nullptr;

  QImage viewPort;

  void assertDimensions(void);

public:
  EgaView(QWidget *parent = nullptr);
  ~EgaView(void) override;

  void
  setBackgroundColor(const QColor &c)
  {
    this->backgroundColor = c;
  }

  void setForegroundColor(const QColor &c)
  {
    this->foregroundColor = c;
  }

  const QColor &
  getBackgroundColor(void) const
  {
    return this->backgroundColor;
  }

  const QColor &
  getForegroundColor(void) const
  {
    return this->foregroundColor;
  }

  void draw(void) override;
  void paint(void) override;

  void clearBuffer(void);
  void setRowOffset(int offset);
  void write(int x, int y, const uint8_t *text, size_t size);
  void write(
      int x,
      int y,
      const QColor &fg,
      const QColor &bg,
      const uint8_t *text,
      size_t size);
  void write(int x, int y, std::string const &);
  void write(
      int x,
      int y,
      const QColor &fg,
      const QColor &bg,
      std::string const &);


signals:
  void backgroundColorChanged();
  void foregroundColorChanged();
};

#endif // EGAVIEW_H
