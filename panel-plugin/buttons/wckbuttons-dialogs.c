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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>

#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <common/theme.h>

#include "wckbuttons.h"
#include "wckbuttons-theme.h"
#include "wckbuttons-dialogs.h"
#include "wckbuttons-dialogs_ui.h"


enum
{
  COL_THEME_NAME,
  COL_THEME_RC,
  N_COLUMNS
};


static void on_only_maximized_toggled(GtkRadioButton *only_maximized, WBPlugin *wb)
{
    wb->prefs->only_maximized = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(only_maximized));
    reload_wnck (wb->win, wb->prefs->only_maximized, wb);
}


static void on_show_on_desktop_toggled(GtkToggleButton *show_on_desktop, WBPlugin *wb)
{
    wb->prefs->show_on_desktop = gtk_toggle_button_get_active(show_on_desktop);
    reload_wnck (wb->win, wb->prefs->only_maximized, wb);
}


static void on_button_layout_changed (GtkEditable *entry, WBPlugin *wb)
{
    if (gtk_widget_get_sensitive (GTK_WIDGET(entry)))
    {
        const gchar *button_layout = gtk_entry_get_text (GTK_ENTRY(entry));
        wb->prefs->button_layout = button_layout_filter (button_layout, wb->prefs->button_layout);

        if (wb->prefs->sync_wm_theme)
        {
            gchar *part;
            const gchar *layout;

            const gchar *wm_button_layout = xfconf_channel_get_string(wb->wm_channel, "/general/button_layout", "O|HMC");

            /* get opposite part of the layout and concatenate it */
            part = opposite_layout_filter (wm_button_layout);
            if (wm_button_layout[0] == part[0])
                layout = g_strconcat (part, wb->prefs->button_layout, NULL);
            else
                layout = g_strconcat (wb->prefs->button_layout, part, NULL);

            xfconf_channel_set_string (wb->wm_channel, "/general/button_layout", layout);
            g_free (part);
        }
        else
        {
            replace_buttons (wb->prefs->button_layout, wb);
            on_wck_state_changed (wb->win->controlwindow, wb);
        }
    }
}


static void
wckbuttons_theme_selection_changed (GtkTreeSelection *selection,
                                       WBPlugin *wb)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;
    const gchar *theme;
    GtkWidget    *entry;

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        gtk_tree_model_get (model, &iter, COL_THEME_NAME, &theme, -1);

        /* set the theme name */
        wb->prefs->theme = g_strdup (theme);

        entry = GTK_WIDGET(gtk_builder_get_object(wb->prefs->builder, "button_layout"));

        if (wb->prefs->sync_wm_theme)
        {
            gchar *button_layout;

            xfconf_channel_set_string (wb->wm_channel, "/general/theme", wb->prefs->theme);
            button_layout = get_rc_button_layout (theme);

            if (button_layout)
            {
                gtk_widget_set_sensitive (entry, FALSE);
                gtk_entry_set_text (GTK_ENTRY(entry), button_layout);
            }
            else
            {
                gtk_entry_set_text (GTK_ENTRY(entry), wb->prefs->button_layout);
                gtk_widget_set_sensitive (entry, TRUE);
            }
            g_free (button_layout);
        }
        else
        {
            load_theme (wb->prefs->theme, wb);
            replace_buttons (wb->prefs->button_layout, wb);
            on_wck_state_changed (wb->win->controlwindow, wb);
        }
    }
}


static void
wckbuttons_load_themes (GtkWidget *theme_name_treeview, WBPlugin *wb)
{
    GtkTreeModel *model;
    GHashTable   *themes;
    GDir         *dir;
    const gchar  *file;
    gchar       **theme_dirs;
    gchar        *themedir;
    gint          i;

    themes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (theme_name_treeview));

    /* clear any previous row */
    gtk_list_store_clear (GTK_LIST_STORE (model));

    xfce_resource_push_path (XFCE_RESOURCE_THEMES, DATADIR G_DIR_SEPARATOR_S "themes");
    theme_dirs = xfce_resource_dirs (XFCE_RESOURCE_THEMES);
    xfce_resource_pop_path (XFCE_RESOURCE_THEMES);

    for (i = 0; theme_dirs[i] != NULL; ++i)
    {
        dir = g_dir_open (theme_dirs[i], 0, NULL);

        if (G_UNLIKELY (dir == NULL))
            continue;

        while ((file = g_dir_read_name (dir)) != NULL)
        {
            /* check if there is not already a theme with the
            * same name in the database */
            if (g_hash_table_lookup (themes, file) == NULL)
            {
                GtkTreeIter   iter;

                if (wb->prefs->sync_wm_theme)
                {
                    if (!test_theme_dir(file, "xfwm4", THEMERC))
                        continue;
                }

                themedir = get_unity_theme_dir (file, NULL);
                if (!themedir)
                    continue;

                g_hash_table_insert (themes, g_strdup (file), GINT_TO_POINTER (1));

                /* insert in the list store */
                gtk_list_store_append (GTK_LIST_STORE (model), &iter);
                gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                                  COL_THEME_NAME, file,
                                  COL_THEME_RC, g_path_get_basename (themedir), -1);

                    if (G_UNLIKELY (g_str_equal (wb->prefs->theme, file)))
                    {
                        GtkTreePath *path = gtk_tree_model_get_path (model, &iter);

                        gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (theme_name_treeview)),
                                                      &iter);
                        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (theme_name_treeview), path, NULL, TRUE, 0.5, 0.5);

                        gtk_tree_path_free (path);
                    }
                g_free (themedir);
            }
        }

      g_dir_close (dir);
    }

  g_strfreev (theme_dirs);
  g_hash_table_destroy (themes);
}


static gint
wckbuttons_theme_sort_func (GtkTreeModel *model,
                               GtkTreeIter  *iter1,
                               GtkTreeIter  *iter2,
                               void *unused)
{
  gchar *str1 = NULL;
  gchar *str2 = NULL;

  gtk_tree_model_get (model, iter1, 0, &str1, -1);
  gtk_tree_model_get (model, iter2, 0, &str2, -1);

  if (str1 == NULL) str1 = g_strdup ("");
  if (str2 == NULL) str2 = g_strdup ("");

  return g_utf8_collate (str1, str2);
}


static void on_sync_wm_theme_toggled(GtkToggleButton *sync_wm_theme, WBPlugin *wb)
{
    GtkWidget   *theme_name_treeview;

    theme_name_treeview = GTK_WIDGET(gtk_builder_get_object(wb->prefs->builder, "theme_name_treeview"));

    wb->prefs->sync_wm_theme = gtk_toggle_button_get_active(sync_wm_theme);

    init_theme (wb);
    wckbuttons_load_themes (theme_name_treeview, wb);

    if (!wb->prefs->sync_wm_theme)
    {
        GtkWidget    *entry;

        entry = GTK_WIDGET(gtk_builder_get_object(wb->prefs->builder, "button_layout"));
        gtk_widget_set_sensitive (entry, TRUE);
        gtk_entry_set_text (GTK_ENTRY(entry), wb->prefs->button_layout);
    }
}


static GtkWidget * build_properties_area(WBPlugin *wb, const gchar *buffer, gsize length) {
    GError *error = NULL;
    GObject *area = NULL;
    GtkRadioButton *only_maximized, *active_window;
    GtkToggleButton *show_on_desktop, *sync_wm_theme;
    GtkTreeSelection *selection;
    GtkCellRenderer *renderer;
    GtkListStore *list_store;
    GtkWidget *theme_name_treeview;
    GtkEntry *button_layout;

    wb->prefs->builder = gtk_builder_new();

    if (gtk_builder_add_from_string(wb->prefs->builder, buffer, length, &error)) {
        area = gtk_builder_get_object(wb->prefs->builder, "alignment0");

        if (G_LIKELY (area != NULL))
        {
            only_maximized = GTK_RADIO_BUTTON(gtk_builder_get_object(wb->prefs->builder, "only_maximized"));
            active_window = GTK_RADIO_BUTTON(gtk_builder_get_object(wb->prefs->builder, "active_window"));

            if (G_LIKELY (only_maximized != NULL))
            {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(only_maximized), wb->prefs->only_maximized);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active_window), !wb->prefs->only_maximized);
                g_signal_connect(only_maximized, "toggled", G_CALLBACK(on_only_maximized_toggled), wb);
            }
            else {
                DBG("No widget with the name \"only_maximized\" found");
            }

            show_on_desktop = GTK_TOGGLE_BUTTON(gtk_builder_get_object(wb->prefs->builder, "show_on_desktop"));

            if (G_LIKELY (show_on_desktop != NULL)) {
                gtk_toggle_button_set_active(show_on_desktop, wb->prefs->show_on_desktop);
                g_signal_connect(show_on_desktop, "toggled", G_CALLBACK(on_show_on_desktop_toggled), wb);
            }
            else {
                DBG("No widget with the name \"show_on_desktop\" found");
            }

            /* Style widgets */
            theme_name_treeview = GTK_WIDGET (gtk_builder_get_object (wb->prefs->builder, "theme_name_treeview"));

            /* theme name */
            {
            list_store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
            gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (list_store), COL_THEME_NAME,
                                             wckbuttons_theme_sort_func,
                                             NULL, NULL);
            gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (list_store), COL_THEME_NAME, GTK_SORT_ASCENDING);
            gtk_tree_view_set_model (GTK_TREE_VIEW (theme_name_treeview), GTK_TREE_MODEL (list_store));
            g_object_unref (G_OBJECT (list_store));

            renderer = gtk_cell_renderer_text_new ();

            gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (theme_name_treeview),
                                                         0, _("Directory"), renderer, "text", 1, NULL);
            gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (theme_name_treeview),
                                                         0, _("Themes usable"), renderer, "text", 0, NULL);
            selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (theme_name_treeview));
            g_signal_connect (selection, "changed", G_CALLBACK (wckbuttons_theme_selection_changed),
                              wb);
            gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
            wckbuttons_load_themes (theme_name_treeview, wb);
            }

            sync_wm_theme = GTK_TOGGLE_BUTTON(gtk_builder_get_object(wb->prefs->builder, "sync_wm_theme"));

            if (G_LIKELY (sync_wm_theme != NULL))
            {
                if (wb->wm_channel)
                {
                    gtk_toggle_button_set_active(sync_wm_theme, wb->prefs->sync_wm_theme);
                    g_signal_connect(sync_wm_theme, "toggled", G_CALLBACK(on_sync_wm_theme_toggled), wb);
                }
                else {
                    gtk_widget_set_sensitive (GTK_WIDGET(sync_wm_theme), FALSE);
                }
            }
            else {
                DBG("No widget with the name \"sync_wm_theme\" found");
            }

            button_layout = GTK_ENTRY(gtk_builder_get_object(wb->prefs->builder, "button_layout"));

            if (G_LIKELY (button_layout != NULL))
            {
                gtk_entry_set_text(button_layout, wb->prefs->button_layout);
                g_signal_connect(GTK_EDITABLE(button_layout), "changed", G_CALLBACK(on_button_layout_changed), wb);
            }
            else {
                DBG("No widget with the name \"button_layout\" found");
            }

            return GTK_WIDGET(area) ;
        }
        else {
            g_set_error_literal(&error, 0, 0, "No widget with the name \"contentarea\" found");
        }
    }

    g_critical("Faild to construct the builder for plugin %s-%d: %s.", xfce_panel_plugin_get_name (wb->plugin), xfce_panel_plugin_get_unique_id (wb->plugin), error->message);
    g_error_free(error);
    g_object_unref(G_OBJECT (wb->prefs->builder) );

    return NULL ;
}


static void
wckbuttons_configure_response (GtkWidget *dialog, gint response, WBPlugin *wb)
{
    gboolean result;

    if (response == GTK_RESPONSE_HELP)
    {
        /* show help */
        result = g_spawn_command_line_async ("exo-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);

        if (G_UNLIKELY (result == FALSE))
            g_warning (_("Unable to open the following url: %s"), PLUGIN_WEBSITE);
    }
    else
    {
        /* remove the dialog data from the plugin */
        g_object_set_data (G_OBJECT (wb->plugin), "dialog", NULL);

        /* unlock the panel menu */
        xfce_panel_plugin_unblock_menu (wb->plugin);

        /* save the plugin */
        wckbuttons_save (wb->plugin, wb);

        /* destroy the properties dialog */
        gtk_widget_destroy (dialog);
    }
}


void wckbuttons_configure (XfcePanelPlugin *plugin, WBPlugin *wb)
{
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *ca;
    const gchar *name;

    /* block the plugin menu */
    xfce_panel_plugin_block_menu (plugin);

    /* create the dialog */
    name = xfce_panel_plugin_get_display_name (plugin);
    dialog = xfce_titled_dialog_new_with_buttons (_(name),
                                                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                                                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                                                GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                                NULL);

    /* center dialog on the screen */
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

    /* set dialog icon */
    gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");

    /* link the dialog to the plugin, so we can destroy it when the plugin
    * is closed, but the dialog is still open */
    g_object_set_data (G_OBJECT (plugin), "dialog", dialog);

    /* connect the reponse signal to the dialog */
    g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK(wckbuttons_configure_response), wb);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog) );

    ca = build_properties_area (wb, wckbuttons_dialogs_ui, wckbuttons_dialogs_ui_length);
    if (G_LIKELY (ca != NULL))
        gtk_container_add(GTK_CONTAINER (content_area), ca);
    else
        DBG("Failed to create content area");

    /* show the entire dialog */
    gtk_widget_show (dialog);
}
