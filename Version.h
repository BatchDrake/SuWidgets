//
//    Version.h: SuWidgets versioning info
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

#ifndef VERSION_H
#define VERSION_H

#include <sigutils/version.h>

#define SUWIDGETS_VERSION_MAJOR 0
#define SUWIDGETS_VERSION_MINOR 1
#define SUWIDGETS_VERSION_PATCH 1

#define SUWIDGETS_ABI_VERSION   1

#define SUWIDGETS_VERSION \
  SU_VER(SUWIDGETS_VERSION_MAJOR, SUWIDGETS_VERSION_MINOR, SUWIDGETS_VERSION_PATCH)

#define SUWIDGETS_API_VERSION \
  SU_VER(SUWIDGETS_VERSION_MAJOR, SUWIDGETS_VERSION_MINOR, 0)

#define SUWIDGETS_VERSION_STRING        \
  STRINGIFY(SUWIDGETS_VERSION_MAJOR) "." \
  STRINGIFY(SUWIDGETS_VERSION_MINOR) "." \
  STRINGIFY(SUWIDGETS_VERSION_PATCH)

#endif // VERSION_H
