#define __SP_WINDOW_C__

/*
 * Generic window implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <gtk/gtkwindow.h>

#include "inkscape.h"
#include "shortcuts.h"

#include "window.h"

static gboolean
sp_window_key_press (GtkWidget *widget, GdkEventKey *event)
{
	unsigned int shortcut;
	shortcut = event->keyval;
	shortcut = event->keyval |
	           ( event->state & GDK_SHIFT_MASK ?
	             SP_SHORTCUT_SHIFT_MASK : 0 ) |
	           ( event->state & GDK_CONTROL_MASK ?
	             SP_SHORTCUT_CONTROL_MASK : 0 ) |
	           ( event->state & GDK_MOD1_MASK ?
	             SP_SHORTCUT_ALT_MASK : 0 );
	return sp_shortcut_invoke (shortcut, SP_VIEW (SP_ACTIVE_DESKTOP));
}

GtkWidget *
sp_window_new (const gchar *title, unsigned int resizeable)
{
	GtkWidget *window;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title ((GtkWindow *) window, title);
	gtk_window_set_resizable ((GtkWindow *) window, resizeable);
	g_signal_connect_after ((GObject *) window, "key_press_event", (GCallback) sp_window_key_press, NULL);

	return window;
}


