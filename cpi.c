//
//    cpi.c: Extract fonts from CPI files
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

#include "cpi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/io.h>
#include <stdint.h>

#include "cpi.h"
/* #include "ega9.h" */
#include "pearl-m68k.h"

static inline int
in_bounds (cpi_handle_t *handle, size_t sz)
{
  return sz < handle->cpi_file_size;
}

int
cpi_map_codepage (cpi_handle_t *handle, const char *path)
{
  memset (handle, 0, sizeof (cpi_handle_t));

  if (path == NULL)
  {
    handle->cpi_fd = -1;
    handle->cpi_file_size = sizeof (bin2c_pearl_m68k_cpi_data);
    handle->cpi_file = bin2c_pearl_m68k_cpi_data;
  }
  else
  {
    if ((handle->cpi_fd = open (path, O_RDWR)) == -1)
    {
      perror ("cpi_map_codepage: open");
      return -1;
    }

    if ((handle->cpi_file_size = (size_t) lseek (handle->cpi_fd, 0, SEEK_END))
        == (size_t) -1)
    {
      perror ("cpi_map_codepage: lseek");
      close (handle->cpi_fd);
      return -1;
    }

    if ((handle->cpi_file =
      mmap (
           NULL,
           handle->cpi_file_size,
           PROT_READ | PROT_WRITE,
           MAP_SHARED,
           handle->cpi_fd,
           0)) ==
      (caddr_t) -1)
    {
      perror ("cpi_map_codepage: mmap");
      close (handle->cpi_fd);
      return -1;
    }
  }

  handle->cpi_file_header = (struct cpi_header *) handle->cpi_file;

  if (memcmp (handle->cpi_file_header->tag, CPI_TAG, 8) != 0 &&
      memcmp (handle->cpi_file_header->tag, CPI_TAG_NT, 8) != 0)
  {
    fprintf(stderr, "cpi_map_codepage: invalid file (not a CPI file)\n");
    munmap (handle->cpi_file, handle->cpi_file_size);
    handle->cpi_file = NULL;
    close (handle->cpi_fd);
    return -1;
  }

  handle->font_is_nt =
    memcmp (handle->cpi_file_header->tag, CPI_TAG_NT, 8) == 0;
  return 0;
}

struct cpi_entry *
cpi_get_page (cpi_handle_t *handle, short cp)
{
  struct cpi_entry *entry;
  int entry_count;
  int i;
  uintptr_t p;

  struct cpi_font_info *info;
  struct cpi_disp_font *font;


  if (handle->cpi_file == NULL)
  {
    fprintf(stderr, "cpi_get_page: no previously CPI file selected\n");
    return NULL;
  }

  entry_count = 0;

  for (entry = (struct cpi_entry *) ((char *) handle->cpi_file +
    (handle->font_is_nt ? 0x19 : handle->cpi_file_header->info_off + 2));
       entry_count < handle->cpi_file_header->entry_no;
       )
  {
    if (!in_bounds(handle, (uintptr_t) entry - (uintptr_t) handle->cpi_file))
    {
      fprintf(stderr, "cpi_get_page: entry 0x%lx (%d) out of bounds!\n",
        (long) entry - (long) handle->cpi_file, entry_count);

      return NULL;
    }

    if (entry->device_type == 1 && entry->codepage == cp)
      return entry;

    printf ("%d omissible\n", entry->codepage);

    entry_count++;

    if (handle->font_is_nt)
    {
      info = (struct cpi_font_info *)
        ((char *) entry + sizeof (struct cpi_entry));
      p = (uintptr_t) info + sizeof (struct cpi_font_info);

      for (i = 0; i < info->font_no; i++)
      {
        if (!in_bounds (handle, p - (uintptr_t) handle->cpi_file))
        {
          fprintf(stderr, "cpi_get_page: font 0x%lx (%d) out of bounds!\n",
            p, i);
          return NULL;
        }

        font = (struct cpi_disp_font *) p;

        p += sizeof (struct cpi_disp_font) +
          (uintptr_t) BITS2BYTES (font->chars * font->rows * font->cols);
      }

      entry = (struct cpi_entry *) p;
    }
    else
    {
      entry = (struct cpi_entry *) ((char *) handle->cpi_file + entry->next_entry);
    }
  }

  return NULL;
}

struct cpi_disp_font *
cpi_get_disp_font (cpi_handle_t *handle,
  struct cpi_entry *entry, int rows, int cols)
{
  int i;
  uintptr_t p;
  struct cpi_font_info *info;
  struct cpi_disp_font *font;

  if (!in_bounds (handle, (size_t) entry->font_info_ptr))
  {
    fprintf(stderr, "cpi_get_disp_font: font info out of bounds!\n");
    return NULL;
  }

  info = (struct cpi_font_info *) ((handle->font_is_nt ?
    (char *) entry + sizeof (struct cpi_entry) :
      (char *) handle->cpi_file + entry->font_info_ptr));
  p = (uintptr_t) info + sizeof (struct cpi_font_info);

  for (i = 0; i < info->font_no; i++)
  {
    if (!in_bounds (handle, p - (uintptr_t) handle->cpi_file))
    {
      fprintf(stderr, "cpi_get_disp_font: font 0x%lx (%d) out of bounds!\n",
        p, i);
      return NULL;
    }

    font = (struct cpi_disp_font *) p;

    if (font->cols == cols && font->rows == rows)
      return font;

    p += sizeof (struct cpi_disp_font) +
      (uintptr_t) BITS2BYTES (font->chars * font->rows * font->cols);
  }

  return NULL;
}

struct glyph *
cpi_get_glyph (struct cpi_disp_font *font, short glyph)
{
  uintptr_t relative;

  if (glyph < 0 || glyph >= font->chars)
    return NULL;

  relative = (uintptr_t) BITS2BYTES (glyph * font->rows * font->cols);
  relative += (uintptr_t) font + sizeof (struct cpi_disp_font);

  return (struct glyph *) relative;
}

void
cpi_unmap (cpi_handle_t *handle)
{
  /*
   * FIXME: Release resources. This path is not currently exerced,
   * but may be interesting to complete it in the future
   */
  handle = handle;
}
