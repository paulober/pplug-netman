/*
 * Network Manager plugin for LXPanel
 *
 * Copyright for relevant code as for LXPanel
 *
 */

/*
Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <glib/gi18n.h>

#include "plugin.h"
#include "nm-default.h"
#include "applet.h"

/* Private context for plugin */

extern void applet_startup (NMApplet *applet);
extern void status_icon_size_changed_cb (NMApplet *applet);
extern void status_icon_activate_cb (NMApplet *applet);
extern void finalize (NMApplet *applet);

/* Handler for system config changed message from panel */
static void nm_configuration_changed (LXPanel *panel, GtkWidget *p)
{
    NMApplet *nm = lxpanel_plugin_get_data (p);
    if (nm->active)
        status_icon_size_changed_cb (nm);
    else
        gtk_widget_hide (nm->plugin);
}

/* Handler for menu button click */
static gboolean nm_button_press_event (GtkWidget *widget, GdkEventButton *event, LXPanel *panel)
{
    NMApplet *nm = lxpanel_plugin_get_data (widget);

    if (event->button == 1)
    {
        status_icon_activate_cb (nm);
        return TRUE;
    }
    else return FALSE;
}

/* Handler for control message */
static gboolean nm_control_msg (GtkWidget *plugin, const char *cmd)
{
    NMApplet *nm = lxpanel_plugin_get_data (plugin);

    if (!g_strcmp0 (cmd, "menu"))
    {
        if (nm_client_get_nm_running (nm->nm_client)) status_icon_activate_cb (nm);
        else system ("lxpanelctl command dhcpcdui menu");
    }

    return TRUE;
}

/* Plugin destructor. */
static void nm_destructor (gpointer user_data)
{
    NMApplet *nm = (NMApplet *) user_data;

    /* Deallocate memory. */
    finalize (nm);
    g_free (nm);
}

/* Plugin constructor. */
static GtkWidget *nm_constructor (LXPanel *panel, config_setting_t *settings)
{
    /* Allocate and initialize plugin context */
    NMApplet *nm = g_new0 (NMApplet, 1);

#ifdef ENABLE_NLS
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

    /* Allocate top level widget and set into plugin widget pointer. */
    nm->panel = panel;
    nm->settings = settings;
    nm->plugin = gtk_button_new ();
    lxpanel_plugin_set_data (nm->plugin, nm, nm_destructor);

    /* Allocate icon as a child of top level */
    nm->status_icon = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (nm->plugin), nm->status_icon);

    /* Set up button */
    gtk_button_set_relief (GTK_BUTTON (nm->plugin), GTK_RELIEF_NONE);

    /* Set up variables */
    nm->icon_size = panel_get_safe_icon_size (panel);

    if (system ("ps ax | grep NetworkManager | grep -qv grep"))
    {
        nm->active = FALSE;
        g_message ("netman: network manager service not running; plugin hidden");
    }
    else
    {
        nm->active = TRUE;
        applet_startup (nm);
    }

    /* Show the widget and return */
    gtk_widget_show_all (nm->plugin);
    return nm->plugin;
}

FM_DEFINE_MODULE (lxpanel_gtk, netman)

/* Plugin descriptor. */
LXPanelPluginInit fm_module_init_lxpanel_gtk = {
    .name = N_("Network Manager"),
    .description = N_("Controller for Network Manager"),
    .new_instance = nm_constructor,
    .reconfigure = nm_configuration_changed,
    .button_press_event = nm_button_press_event,
    .control = nm_control_msg,
    .gettext_package = GETTEXT_PACKAGE
};
