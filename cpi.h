//
//    cpi.h: Extract fonts from CPI files
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

#ifndef CPI_H
#define CPI_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <sys/types.h>

#define CPI_TAG    "\xff" "FONT   "
#define CPI_TAG_NT "\xff" "FONT.NT"

#define CPI_PACKED __attribute__ ((packed))
#define BITS2BYTES(x) (((x) + 7) / (8))

#define FONT_HEIGHT 14

struct cpi_header
{
  char  tag[8]; /* "\xffFONT   " */
  char  reserved[8];
  short pointer_no CPI_PACKED; /* 1 */
  char  pointer_type; /* 1 */
  int   info_off CPI_PACKED;
  short entry_no CPI_PACKED;
} CPI_PACKED;

struct cpi_entry
{
  short code_size CPI_PACKED;
  int   next_entry;
  short device_type CPI_PACKED;
  char  device_name[8];
  short codepage CPI_PACKED;
  char  reserved[6];
  int   font_info_ptr CPI_PACKED;
}  CPI_PACKED;

struct cpi_font_info
{
  short reserved CPI_PACKED;
  short font_no CPI_PACKED;
  short font_data_size CPI_PACKED;
} CPI_PACKED;

struct cpi_disp_font
{
  char rows;
  char cols;
  short aspect CPI_PACKED;
  short chars CPI_PACKED;
}  CPI_PACKED;

struct glyph
{
  char bits[16];
};

typedef struct cpi_handle
{
  void *cpi_file;
  int   cpi_fd;
  size_t cpi_file_size;

  int   font_is_nt;

  struct cpi_header *cpi_file_header;
}
cpi_handle_t;

int cpi_map_codepage (cpi_handle_t *, const char *);

struct cpi_entry *cpi_get_page (cpi_handle_t *, short);
struct cpi_disp_font *cpi_get_disp_font
  (cpi_handle_t *, struct cpi_entry *, int, int);
struct glyph *cpi_get_glyph (struct cpi_disp_font *, short);
void cpi_unmap (cpi_handle_t *);
void cpi_puts  (struct cpi_disp_font *font,
                int, int,
                int, int,
                void *,
                int,
                int,
                unsigned int,
                unsigned int,
                const char *);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // CPI_H
