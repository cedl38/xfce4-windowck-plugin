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

#include <common/mypixmap.h>
#include <common/theme.h>

#include "wckbuttons-theme.h"

#define XPM_COLOR_SYMBOL_SIZE 22


static void get_unity_pixbuf (const gchar *themedir, WBPlugin *wb) {
    gint i,j;
    gchar imagename[40];

    static const char *button_names[] = {
      "minimize",
      "unmaximize",
      "maximize",
      "close"
    };

    static const char *button_state_names[] = {
      "unfocused",
      "focused_normal",
      "focused_prelight",
      "focused_pressed"
    };

    for (i = 0; i < IMAGES_BUTTONS; i++)
    {
        for (j = 0; j < IMAGES_STATES; j++)
        {
            g_snprintf(imagename, sizeof (imagename), "%s_%s", button_names[i], button_state_names[j]);
            wb->pixbufs[i][j] = pixbuf_alpha_load (themedir, imagename);
        }
    }
}


gchar *button_layout_filter  (const gchar *string, const gchar *default_layout)
{
    guint i, j;
    gchar layout[BUTTONS] = {0};

    /* WARNING : beware of bluffer overflow !!!  */
    j = 0;
    for (i = 0; i < strlen (string) && j < BUTTONS; i++)
    {
        switch (string[i])
        {
            case 'H':
                layout[j++] = 'H';
                break;
            case 'M':
                layout[j++] = 'M';
                break;
            case 'C':
                layout[j++] = 'C';
        }
    }

    layout[j] = '\0';

    if (layout[0] == '\0')
        return g_strdup (default_layout);

    return g_strdup (layout);
}


gchar *opposite_layout_filter  (const gchar *string)
{
    guint i, j;
    gchar layout[8] = {0};

    /* WARNING : beware of bluffer overflow !!!  */
    j = 0;
    for (i = 0; i < strlen (string) && j < 8; i++)
    {
        switch (string[i])
        {
            case 'H':
            case 'M':
            case 'C':
                break;
            default:
                layout[j] = string[i];
                j++;
        }
    }

    layout[j] = '\0';

    return g_strdup (layout);
}


static int get_button_from_letter (char chr)
{

    TRACE ("entering get_button_from_letter");

    switch (chr)
    {
        case 'H':
                return MINIMIZE_BUTTON;
        case 'M':
                return MAXIMIZE_BUTTON;
        case 'C':
                return CLOSE_BUTTON;
        default:
            return -1;
    }
}


/* Replace buttons accordingly to button_layout and visible state */
void replace_buttons (const gchar *button_layout, WBPlugin *wb)
{
    guint i, j;
    gint button;

    for (i = 0; i < BUTTONS; i++)
        gtk_widget_hide_all(GTK_WIDGET(wb->button[i]->eventbox));

    j = 0;
    for (i = 0; i < strlen (button_layout); i++)
    {
        button = get_button_from_letter (button_layout[i]);
        if (button >= 0 && wb->button[button]->image)
        {
            gtk_box_reorder_child (GTK_BOX (wb->hvbox), GTK_WIDGET(wb->button[button]->eventbox), j);

            gtk_widget_show_all(GTK_WIDGET(wb->button[button]->eventbox));
            j++;
        }
    }
}


gchar *get_rc_button_layout (const gchar *theme)
{
    gchar *wm_themedir;

    wm_themedir = test_theme_dir(theme, "xfwm4", THEMERC);

    if (G_LIKELY(wm_themedir))
    {
        const gchar *rc_button_layout;
        gchar *filename;
        XfceRc       *rc;

        /* check in the rc if the theme supports a custom button layout */
        filename = g_build_filename (wm_themedir, THEMERC, NULL);

        rc = xfce_rc_simple_open (filename, TRUE);
        g_free (filename);

        if (G_LIKELY (rc))
        {
            rc_button_layout = xfce_rc_read_entry (rc, "button_layout", NULL);

            if (rc_button_layout)
                return button_layout_filter (rc_button_layout, NULL);
        }
    }
    g_free (wm_themedir);

    return NULL;
}


/* load the theme according to xfwm4 theme format */
void load_theme (const gchar *theme, WBPlugin *wb)
{
    gint i;
    gchar *themedir;

    /* get theme dir */
    themedir = get_unity_theme_dir (wb->prefs->theme, DEFAULT_THEME);

    if (themedir)
    {
        get_unity_pixbuf (themedir, wb);
        g_free (themedir);

            /* try to replace missing images */
        
            for (i = 0; i < IMAGES_STATES; i++)
            {
                if (!wb->pixbufs[IMAGE_UNMAXIMIZE][i])
                    wb->pixbufs[IMAGE_UNMAXIMIZE][i] = wb->pixbufs[IMAGE_MAXIMIZE][i];
            }
            for (i = 0; i < IMAGES_BUTTONS; i++)
            {
                if (!wb->pixbufs[i][IMAGE_UNFOCUSED] || !wb->pixbufs[i][IMAGE_PRESSED])
                    wb->pixbufs[i][IMAGE_UNFOCUSED] = wb->pixbufs[i][IMAGE_FOCUSED];
        
                if (!wb->pixbufs[i][IMAGE_PRELIGHT])
                    wb->pixbufs[i][IMAGE_PRELIGHT] = wb->pixbufs[i][IMAGE_PRESSED];
            }
    }
}


static void apply_wm_theme (WBPlugin *wb)
{
   const gchar *wm_theme = xfconf_channel_get_string (wb->wm_channel, "/general/theme", NULL);

    if (G_LIKELY(wm_theme))
    {
        gchar *button_layout;

        wb->prefs->theme = g_strdup (wm_theme);
        load_theme (wb->prefs->theme, wb);

        button_layout = get_rc_button_layout (wm_theme);

        if (button_layout)
        {
            replace_buttons (button_layout, wb);
        }
        else
        {
            const gchar *wm_buttons_layout = xfconf_channel_get_string (wb->wm_channel, "/general/button_layout", wb->prefs->button_layout);
            wb->prefs->button_layout = button_layout_filter (wm_buttons_layout, wb->prefs->button_layout);

            replace_buttons (wb->prefs->button_layout, wb);
        }
        g_free (button_layout);
    }

    on_wck_state_changed (wb->win->controlwindow, wb);
}


static void
on_x_chanel_property_changed (XfconfChannel *x_channel, const gchar *property_name, const GValue *value, WBPlugin *wb)
{
    if (g_str_has_prefix(property_name, "/Net/") == TRUE)
    {
        const gchar *name = &property_name[5];
        switch (G_VALUE_TYPE(value))
        {
            case G_TYPE_STRING:
                if (!strcmp (name, "ThemeName"))
                {
                    apply_wm_theme (wb);
                }
                break;
            default:
                g_warning("The property '%s' is not supported", property_name);
                break;
        }
    }
}


static void
on_xfwm_channel_property_changed (XfconfChannel *wm_channel, const gchar *property_name, const GValue *value, WBPlugin *wb)
{
    if (g_str_has_prefix(property_name, "/general/") == TRUE)
    {
        const gchar *name = &property_name[9];
        switch (G_VALUE_TYPE(value))
        {
            case G_TYPE_STRING:
                if (!strcmp (name, "theme")
                    || !strcmp (name, "button_layout"))
                {
                    apply_wm_theme (wb);
                }
                break;
            default:
                g_warning("The property '%s' is not supported", property_name);
                break;
        }
    }
}


void init_theme (WBPlugin *wb)
{
    /* get the xfwm4 chanel */
    wb->wm_channel = wck_properties_get_channel (G_OBJECT (wb->plugin), "xfwm4");

    /* try to use the xfwm4 theme */
    if (wb->wm_channel && wb->prefs->sync_wm_theme)
    {
        apply_wm_theme (wb);

        g_signal_connect (wb->wm_channel, "property-changed", G_CALLBACK (on_xfwm_channel_property_changed), wb);
    }
    else
    {
        load_theme (wb->prefs->theme, wb);
        replace_buttons (wb->prefs->button_layout, wb);
    }

    /* get the xsettings chanel to update the gtk theme */
    wb->x_channel = wck_properties_get_channel (G_OBJECT (wb->plugin), "xsettings");

    if (wb->x_channel)
        g_signal_connect (wb->x_channel, "property-changed", G_CALLBACK (on_x_chanel_property_changed), wb);
}
