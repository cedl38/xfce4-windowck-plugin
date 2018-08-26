/*  $Id$
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Copyright (C) 2013 Cedric Leporcq  <cedl38@gmail.com>
 *
 *  Parts of this code is derived from xfwm4 sources (setting.c)
 *
 */

#include <libxfce4util/libxfce4util.h>

#include "theme.h"

#define UNITY_TEST_FILE_PNG         "close_focused_normal.png"
#define UNITY_TEST_FILE_SVG         "close_focused_normal.svg"


gchar *test_theme_dir (const gchar *theme, const char *themedir, const gchar *file) {
    gchar *test_file, *abs_path, *path;

    path = g_build_filename (theme, themedir, file, NULL);

    xfce_resource_push_path (XFCE_RESOURCE_THEMES,
                             DATADIR G_DIR_SEPARATOR_S "themes");
    test_file = xfce_resource_lookup (XFCE_RESOURCE_THEMES, path);
    xfce_resource_pop_path (XFCE_RESOURCE_THEMES);

    g_free (path);

    if (test_file)
    {
        abs_path = g_path_get_dirname (test_file);
        g_free (test_file);

        return abs_path;
    }

    return NULL;
}


gchar *
get_unity_theme_dir (const gchar *theme, const gchar *default_theme)
{
    gchar *abs_path;

    if (g_path_is_absolute (theme))
    {
        if (g_file_test (theme, G_FILE_TEST_IS_DIR))
        {
            return g_strdup (theme);
        }
    }

    abs_path = test_theme_dir (theme, "unity", UNITY_TEST_FILE_PNG);

    if (!abs_path)
        abs_path = test_theme_dir (theme, "unity", UNITY_TEST_FILE_SVG);

    if (abs_path)
        return abs_path;

    /* Pfew, really can't find that theme nowhere! */
    if (default_theme)
        return g_build_filename (DATADIR, "themes", default_theme, "unity", NULL);

    return NULL;
}


