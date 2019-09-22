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

#include "EgaConsole.h"

EgaConsole::EgaConsole(QWidget *parent) : EgaView(parent)
{

}

EgaConsole::~EgaConsole(void)
{

}

void
EgaConsole::clear()
{
  this->clearBuffer();
  this->setRowOffset(0);
  this->gotoxy(0, 0);
}

void
EgaConsole::gotoxy(int x, int y)
{
  this->x = x;
  this->y = y;

  if (this->y >= this->mRows) {
    this->setRowOffset(
        static_cast<int>(this->scanLines.size()) - this->mRows);
  }
}

void
EgaConsole::newLine(void)
{
  this->x = 0;
  ++this->y;

  if (this->y - this->rowoff >= this->mRows)
    this->setRowOffset(
        static_cast<int>(this->scanLines.size()) - this->mRows);
}
void
EgaConsole::setBackground(QColor const &bg)
{
  this->setBackgroundColor(bg);
}

void
EgaConsole::setForeground(QColor const &fg)
{
  this->setForegroundColor(fg);
}

void
EgaConsole::scrollTo(int y)
{
  this->setRowOffset(y);
  this->invalidate();
}

int
EgaConsole::length(void) const
{
  return static_cast<int>(this->scanLines.size());
}

void
EgaConsole::put(const char *bytes, size_t size)
{
  size_t chunk;
  while (size > 0) {
    chunk = size;
    if (this->x + static_cast<int>(chunk) > this->mCols)
      chunk = static_cast<size_t>(this->mCols - this->x);

    this->write(
          this->x,
          this->y,
          reinterpret_cast<const uint8_t *>(bytes),
          chunk);

    size -= chunk;
    bytes += chunk;
    this->x += chunk;

    if (this->x >= this->mCols)
      this->newLine();
  }
}

void
EgaConsole::print(const char *text)
{
  while (*text != '\0') {
    if (*text == '\r') {
      this->x = 0;
    } else if (*text == '\n') {
      this->newLine();
    } else if (*text == '\t') {
      this->x = 8 * ((this->x + 8) / 8);
    } else {
      this->put(text, 1);
    }
    ++text;
  }
}
