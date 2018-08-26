/*      $Id$

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2, or (at your option)
        any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., Inc., 51 Franklin Street, Fifth Floor, Boston,
        MA 02110-1301, USA.


        xfwm4-windowck-plugin - (c) 2013 Cedric leporcq
 */

#include <glib.h>
#include "mypixmap.h"

GdkPixbuf *
pixbuf_alpha_load (const gchar * dir, const gchar * file)
{
    gchar *filepng;
    gchar *filename;
    int i;

    static const char* image_types[] = {
      "svg",
      "png",
      "gif",
      "jpg",
      "bmp",
      NULL };

    i = 0;
    while ((image_types[i]))
    {
        filepng = g_strdup_printf ("%s.%s", file, image_types[i]);
        filename = g_build_filename (dir, filepng, NULL);
        g_free (filepng);

        if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        {
            return gdk_pixbuf_new_from_file (filename, NULL);
        }
        g_free (filename);
        ++i;
    }
    return NULL;
}
