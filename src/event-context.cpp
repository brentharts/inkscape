#define __SP_EVENT_CONTEXT_C__

/*
 * Base class for event processors
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>

#include "display/sp-canvas.h"
#include "xml/node-event-vector.h"
#include "xml/repr.h"
#include "sp-cursor.h"
#include "shortcuts.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "selection.h"
#include "sp-item.h"
#include "zoom-context.h"
#include "select-context.h"
#include "file.h"
#include "interface.h"
#include <glibmm/i18n.h>
#include "selection-chemistry.h"
#include "dialogs/desktop-properties.h"
#include "macros.h"
#include "tools-switch.h"
#include "prefs-utils.h"
#include "message-context.h"
#include "widgets/spw-utilities.h"
#include "gradient-drag.h"

#include "event-context.h"

static void sp_event_context_class_init(SPEventContextClass *klass);
static void sp_event_context_init(SPEventContext *event_context);
static void sp_event_context_dispose(GObject *object);

static void sp_event_context_private_setup(SPEventContext *ec);
static gint sp_event_context_private_root_handler(SPEventContext *event_context, GdkEvent *event);
static gint sp_event_context_private_item_handler(SPEventContext *event_context, SPItem *item, GdkEvent *event);

static void set_event_location(SPDesktop * desktop, GdkEvent * event);

static GObjectClass *parent_class;

// globals for temporary switching to selector by space
static gboolean selector_toggled = FALSE;
static int switch_selector_to = 0;

static gint xp = 0, yp = 0; // where drag started
static gint tolerance = 0;
static bool within_tolerance = false;

// globals for keeping track of keyboard scroll events in order to accelerate
static guint32 scroll_event_time = 0;
static gdouble scroll_multiply = 1;
static guint scroll_keyval = 0;

GType
sp_event_context_get_type(void)
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPEventContextClass),
            NULL, NULL,
            (GClassInitFunc) sp_event_context_class_init,
            NULL, NULL,
            sizeof(SPEventContext),
            4,
            (GInstanceInitFunc) sp_event_context_init,
            NULL,    /* value_table */
        };
        type = g_type_register_static(G_TYPE_OBJECT, "SPEventContext", &info, (GTypeFlags)0);
    }
    return type;
}

static void
sp_event_context_class_init(SPEventContextClass *klass)
{
    GObjectClass *object_class;

    object_class = (GObjectClass *) klass;

    parent_class = (GObjectClass*)g_type_class_peek_parent(klass);

    object_class->dispose = sp_event_context_dispose;

    klass->setup = sp_event_context_private_setup;
    klass->root_handler = sp_event_context_private_root_handler;
    klass->item_handler = sp_event_context_private_item_handler;
}

static void
sp_event_context_init(SPEventContext *event_context)
{
    event_context->desktop = NULL;
    event_context->cursor = NULL;
    event_context->_message_context = NULL;
    event_context->_selcue = NULL;
    event_context->_grdrag = NULL;
}

static void
sp_event_context_dispose(GObject *object)
{
    SPEventContext *ec;

    ec = SP_EVENT_CONTEXT(object);

    if (ec->_message_context) {
        delete ec->_message_context;
    }

    if (ec->cursor != NULL) {
        gdk_cursor_unref(ec->cursor);
        ec->cursor = NULL;
    }

    if (ec->desktop) {
        ec->desktop = NULL;
    }

    if (ec->repr) {
        sp_repr_remove_listener_by_data(ec->repr, ec);
        sp_repr_unref(ec->repr);
        ec->repr = NULL;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

void
sp_event_context_update_cursor(SPEventContext *ec)
{
    GtkWidget *w = GTK_WIDGET(SP_DT_CANVAS(ec->desktop));
    if (w->window) {
        /* fixme: */
        if (ec->cursor_shape) {
            GdkBitmap *bitmap = NULL;
            GdkBitmap *mask = NULL;
            sp_cursor_bitmap_and_mask_from_xpm(&bitmap, &mask, ec->cursor_shape);
            if ((bitmap != NULL) && (mask != NULL)) {
                if (ec->cursor)
                    gdk_cursor_unref (ec->cursor);
                ec->cursor = gdk_cursor_new_from_pixmap(bitmap, mask,
                                                        &w->style->black, &w->style->white,
                                                        ec->hot_x, ec->hot_y);
                g_object_unref (bitmap);
                g_object_unref (mask);
            }
        }
        gdk_window_set_cursor(w->window, ec->cursor);
    }
}

static void
sp_event_context_private_setup(SPEventContext *ec)
{
    sp_event_context_update_cursor(ec);
}

static void 
sp_toggle_selector(SPDesktop *dt)
{
    if (!dt->event_context) return;

    if (tools_isactive(dt, TOOLS_SELECT)) {
        if (selector_toggled) {
            if (switch_selector_to) tools_switch (dt, switch_selector_to);
            selector_toggled = FALSE;
        } else return;
    } else {
        selector_toggled = TRUE;
        switch_selector_to = tools_active(dt);
        tools_switch (dt, TOOLS_SELECT);
    }
}

/**
\brief   Gobbles next key events on the queue with the same keyval and mask. Returns the number of events consumed.
*/
gint gobble_key_events(guint keyval, gint mask)
{
    GdkEvent *event_next;
    gint i = 0;

    event_next = gdk_event_get();
    // while the next event is also a key notify with the same keyval and mask,
    while (event_next && event_next->type == GDK_KEY_PRESS
           && event_next->key.keyval == keyval && (event_next->key.state & mask)) {
        // kill it
        gdk_event_free(event_next);
        // get next
        event_next = gdk_event_get();
        i ++;
    }
    // otherwise, put it back onto the queue
    if (event_next) gdk_event_put(event_next);

    return i;
}

/**
\brief   Gobbles next motion notify events on the queue with the same mask. Returns the number of events consumed.
*/
gint gobble_motion_events(gint mask)
{
    GdkEvent *event_next;
    gint i = 0;

    event_next = gdk_event_get();
    // while the next event is also a key notify with the same keyval and mask,
    while (event_next && event_next->type == GDK_MOTION_NOTIFY
           && (event_next->motion.state & mask)) {
        // kill it
        gdk_event_free(event_next);
        // get next
        event_next = gdk_event_get();
        i ++;
    }
    // otherwise, put it back onto the queue
    if (event_next) gdk_event_put(event_next);

    return i;
}

gdouble accelerate_scroll(GdkEvent *event, gdouble acceleration)
{
    guint32 time_diff = ((GdkEventKey *) event)->time - scroll_event_time;

    /* key pressed within 500ms ? (1/2 second) */
    if (time_diff > 500 || event->key.keyval != scroll_keyval) {
        scroll_multiply = 1;
    } else {
        scroll_multiply += acceleration;
    }

    scroll_event_time = ((GdkEventKey *) event)->time;
    scroll_keyval = event->key.keyval;

    return scroll_multiply;
}

// This is a hack that is necessary because when middle-clicking too fast, button_press
// events come for all clicks but there's button_release only for the first one. So
// after a release, we must prohibit the next grab for some time, or the grab will
// stuck.  Perhaps this is caused by some wrong handling of events among contexts and
// not by a GDK bug; if someone can fix this properly this would be great.
gint dontgrab = 0;
gboolean 
grab_allow_again() 
{
    dontgrab--; 
    if (dontgrab < 0) dontgrab = 0;
    return FALSE; // so that it is only called once
}

static gint sp_event_context_private_root_handler(SPEventContext *event_context, GdkEvent *event)
{
    static NR::Point button_w;
    static unsigned int panning = 0;

    SPDesktop *desktop = event_context->desktop;

    tolerance = prefs_get_int_attribute_limited("options.dragtolerance", "value", 0, 0, 100);
    double const zoom_inc = prefs_get_double_attribute_limited("options.zoomincrement", "value", M_SQRT2, 1.01, 10);
    double const acceleration = prefs_get_double_attribute_limited("options.scrollingacceleration", "value", 0, 0, 6);
    int const key_scroll = prefs_get_int_attribute_limited("options.keyscroll", "value", 10, 0, 1000);
    int const wheel_scroll = prefs_get_int_attribute_limited("options.wheelscroll", "value", 40, 0, 1000);

    gint ret = FALSE;

    switch (event->type) {
        case GDK_2BUTTON_PRESS:
            if (panning) {
                panning = 0;
                sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate), event->button.time);
                ret = TRUE;
            } else {
                /* sp_desktop_dialog(); */
            }
            break;
        case GDK_BUTTON_PRESS:

            // save drag origin
            xp = (gint) event->button.x; 
            yp = (gint) event->button.y;
            within_tolerance = true;

            switch (event->button.button) {
                case 2:

                    if (dontgrab) { // double-click, still not permitted to grab; increase the counter to guard against triple click
                        dontgrab ++; 
                        gtk_timeout_add(250, (GtkFunction) grab_allow_again, NULL);
                        break;
                    }

                    button_w = NR::Point(event->button.x,
                                         event->button.y);
                    panning = 2;
                    sp_canvas_item_grab(SP_CANVAS_ITEM(desktop->acetate),
                                        GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK,
                                        NULL, event->button.time-1);

                    ret = TRUE;
                    break;
                case 3:
                    if (event->button.state & GDK_SHIFT_MASK || event->button.state & GDK_CONTROL_MASK) {
                        button_w = NR::Point(event->button.x,
                                             event->button.y);
                        panning = 3;
                        sp_canvas_item_grab(SP_CANVAS_ITEM(desktop->acetate),
                                            GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK,
                                            NULL, event->button.time);
                        ret = TRUE;
                    } else {
                        sp_event_root_menu_popup(desktop, NULL, event);
                    }
                    break;
                default:
                    break;
            }
            break;
        case GDK_MOTION_NOTIFY:
            if (panning) {
                if ((panning == 2 && !(event->motion.state & GDK_BUTTON2_MASK)) ||
                    (panning == 3 && !(event->motion.state & GDK_BUTTON3_MASK))) {
                    /* Gdk seems to lose button release for us sometimes :-( */
                    panning = 0;
                    dontgrab = 0;
                    sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate), event->button.time);
                    ret = TRUE;
                } else {

                    if ( within_tolerance
                         && ( abs( (gint) event->motion.x - xp ) < tolerance )
                         && ( abs( (gint) event->motion.y - yp ) < tolerance ) ) {
                        break; // do not drag if we're within tolerance from origin
                    }
                    // Once the user has moved farther than tolerance from the original location 
                    // (indicating they intend to move the object, not click), then always process the 
                    // motion notify coordinates as given (no snapping back to origin)
                    within_tolerance = false; 

                    // gobble subsequent motion events to prevent "sticking" when scrolling is slow
                    gobble_motion_events(panning == 2 ? GDK_BUTTON2_MASK : GDK_BUTTON3_MASK);

                    NR::Point const motion_w(event->motion.x,
                                             event->motion.y);
                    NR::Point const moved_w( motion_w - button_w );
                    sp_desktop_scroll_world(event_context->desktop, moved_w);
                    ret = TRUE;
                }
            }
            break;
        case GDK_BUTTON_RELEASE:
            if (panning == event->button.button) {
                panning = 0;
                sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate), event->button.time);

                if (within_tolerance) {
                    dontgrab ++;
                    NR::Point const event_w(event->button.x, event->button.y);
                    NR::Point const event_dt(sp_desktop_w2d_xy_point(desktop, event_w));
                    double const zoom_power = ( (event->button.state & GDK_SHIFT_MASK)
                                                ? -dontgrab
                                                : dontgrab );
                    sp_desktop_zoom_relative_keep_point(desktop, event_dt,
                                                        pow(zoom_inc, zoom_power));
                    gtk_timeout_add(250, (GtkFunction) grab_allow_again, NULL);
                }

                ret = TRUE;
            } 
            xp = yp = 0; 
            break;
        case GDK_KEY_PRESS:
            switch (get_group0_keyval(&event->key)) {
                unsigned int shortcut;
                case GDK_F1:
                    /* Grab it away from Gtk */
                    shortcut = get_group0_keyval(&event->key);
                    if (event->key.state & GDK_SHIFT_MASK) shortcut |= SP_SHORTCUT_SHIFT_MASK;
                    if (event->key.state & GDK_CONTROL_MASK) shortcut |= SP_SHORTCUT_CONTROL_MASK;
                    if (event->key.state & GDK_MOD1_MASK) shortcut |= SP_SHORTCUT_ALT_MASK;
                    ret = sp_shortcut_invoke(shortcut, SP_VIEW(SP_EVENT_CONTEXT_DESKTOP(event_context)));
                case GDK_Tab: // disable tab/shift-tab which cycle widget focus
                case GDK_ISO_Left_Tab: // they will get different functions
                    if (!(MOD__CTRL_ONLY || (MOD__CTRL && MOD__SHIFT))) {
                        ret = TRUE;
                    } else {
                        /* Grab it away from Gtk */
                        shortcut = get_group0_keyval(&event->key);
                        if (event->key.state & GDK_SHIFT_MASK) shortcut |= SP_SHORTCUT_SHIFT_MASK;
                        if (event->key.state & GDK_CONTROL_MASK) shortcut |= SP_SHORTCUT_CONTROL_MASK;
                        if (event->key.state & GDK_MOD1_MASK) shortcut |= SP_SHORTCUT_ALT_MASK;
                        ret = sp_shortcut_invoke(shortcut, SP_VIEW(SP_EVENT_CONTEXT_DESKTOP(event_context)));
                    }
                    break;
                case GDK_W:
                case GDK_w:
                case GDK_F4:
                    /* Close view */
                    if (event->key.state & GDK_CONTROL_MASK) {
                        sp_ui_close_view(NULL);
                        ret = TRUE;
                    }
                    break;
                    // FIXME: make import a verb
                case GDK_i: // Ctrl i - import file
                    if (event->key.state & GDK_CONTROL_MASK) {
                        sp_file_import(NULL);
                        ret = TRUE;
                    }
                    break;
                case GDK_Left: // Ctrl Left 
                case GDK_KP_Left:
                case GDK_KP_4:
                    if (event->key.state & GDK_CONTROL_MASK) {
                        int i = (int) floor(key_scroll * accelerate_scroll(event, acceleration));
                        gobble_key_events(get_group0_keyval(&event->key), GDK_CONTROL_MASK);
                        sp_desktop_scroll_world(event_context->desktop, i, 0);
                        ret = TRUE;
                    }
                    break;
                case GDK_Up: // Ctrl Up
                case GDK_KP_Up:
                case GDK_KP_8:
                    if (event->key.state & GDK_CONTROL_MASK) {
                        int i = (int) floor(key_scroll * accelerate_scroll(event, acceleration));
                        gobble_key_events(get_group0_keyval(&event->key), GDK_CONTROL_MASK);
                        sp_desktop_scroll_world(event_context->desktop, 0, i);
                        ret = TRUE;
                    }
                    break;
                case GDK_Right: // Ctrl Right
                case GDK_KP_Right:
                case GDK_KP_6:
                    if (event->key.state & GDK_CONTROL_MASK) {
                        int i = (int) floor(key_scroll * accelerate_scroll(event, acceleration));
                        gobble_key_events(get_group0_keyval(&event->key), GDK_CONTROL_MASK);
                        sp_desktop_scroll_world(event_context->desktop, -i, 0);
                        ret = TRUE;
                    }
                    break;
                case GDK_Down: // Ctrl Down
                case GDK_KP_Down:
                case GDK_KP_2:
                    if (event->key.state & GDK_CONTROL_MASK) {
                        int i = (int) floor(key_scroll * accelerate_scroll(event, acceleration));
                        gobble_key_events(get_group0_keyval(&event->key), GDK_CONTROL_MASK);
                        sp_desktop_scroll_world(event_context->desktop, 0, -i);
                        ret = TRUE;
                    }
                    break;
                case GDK_F10:
                    if (event->key.state & GDK_SHIFT_MASK) {
                        sp_event_root_menu_popup(desktop, NULL, event);
                        ret= TRUE;
                    } 
                    break;
                case GDK_space:
                    sp_toggle_selector(desktop);
                    ret= TRUE;
                    break;
                case GDK_z:
                case GDK_Z:
                    if (MOD__ALT_ONLY) {
                        gpointer hb = gtk_object_get_data(GTK_OBJECT(desktop->owner), "altz");
                        if (hb && GTK_IS_WIDGET(hb)) {
                            gtk_widget_grab_focus(GTK_WIDGET(hb));
                        }
                        ret = TRUE;
                    }
                    break;
                default:
                    break;
            }
            break;
        case GDK_SCROLL:
            /* shift + wheel, pan left--right */
            if (event->scroll.state & GDK_SHIFT_MASK) {
                switch (event->scroll.direction) {
                    case GDK_SCROLL_UP:
                        sp_desktop_scroll_world(desktop, wheel_scroll, 0);
                        break;
                    case GDK_SCROLL_DOWN:
                        sp_desktop_scroll_world(desktop, -wheel_scroll, 0);
                        break;
                    default:
                        break;
                }

                /* ctrl + wheel, zoom in--out */
            } else if (event->scroll.state & GDK_CONTROL_MASK) {
                double rel_zoom;
                switch (event->scroll.direction) {
                    case GDK_SCROLL_UP:   
                        rel_zoom = zoom_inc;
                        break;
                    case GDK_SCROLL_DOWN: 
                        rel_zoom = 1 / zoom_inc;
                        break;
                    default:              
                        rel_zoom = 0.0;
                        break;
                }
                if (rel_zoom != 0.0) {
                    NR::Point const scroll_dt = sp_desktop_point(desktop);
                    sp_desktop_zoom_relative_keep_point(desktop, scroll_dt, rel_zoom);
                }

                /* no modifier, pan up--down (left--right on multiwheel mice?) */
            } else {
                switch (event->scroll.direction) {
                    case GDK_SCROLL_UP:
                        sp_desktop_scroll_world(desktop, 0, wheel_scroll);
                        break;
                    case GDK_SCROLL_DOWN:
                        sp_desktop_scroll_world(desktop, 0, -wheel_scroll);
                        break;
                    case GDK_SCROLL_LEFT:
                        sp_desktop_scroll_world(desktop, wheel_scroll, 0);
                        break;
                    case GDK_SCROLL_RIGHT:
                        sp_desktop_scroll_world(desktop, -wheel_scroll, 0);
                        break;
                }
            }
            break;
        default:
            break;
    }

    return ret;
}

/* fixme: do context sensitive popup menu on items */

static gint
sp_event_context_private_item_handler(SPEventContext *ec, SPItem *item, GdkEvent *event)
{
    int ret = FALSE;

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            if ((event->button.button == 3) && !(event->button.state & GDK_SHIFT_MASK || event->button.state & GDK_CONTROL_MASK)) {
                sp_event_root_menu_popup(ec->desktop, item, event);
                ret = TRUE;
            }
            break;
        default:
            break;
    }

    return ret;
}

static void
sp_ec_repr_attr_changed(Inkscape::XML::Node *repr, gchar const *key, gchar const *oldval, gchar const *newval,
                        bool is_interactive, gpointer data)
{
    SPEventContext *ec;

    ec = SP_EVENT_CONTEXT(data);

    if (((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->set) {
        ((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->set(ec, key, newval);
    }
}

Inkscape::XML::NodeEventVector sp_ec_event_vector = {
    NULL, /* Child added */
    NULL, /* Child removed */
    sp_ec_repr_attr_changed,
    NULL, /* Content changed */
    NULL /* Order changed */
};

SPEventContext *
sp_event_context_new(GType type, SPDesktop *desktop, Inkscape::XML::Node *repr, unsigned int key)
{
    g_return_val_if_fail(g_type_is_a(type, SP_TYPE_EVENT_CONTEXT), NULL);
    g_return_val_if_fail(desktop != NULL, NULL);
    g_return_val_if_fail(SP_IS_DESKTOP(desktop), NULL);

    SPEventContext *const ec = (SPEventContext*)g_object_new(type, NULL);

    ec->desktop = desktop;
    ec->_message_context = new Inkscape::MessageContext(desktop->messageStack());
    ec->key = key;
    ec->repr = repr;
    if (ec->repr) {
        sp_repr_ref(ec->repr);
        sp_repr_add_listener(ec->repr, &sp_ec_event_vector, ec);
    }

    if (((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->setup)
        ((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->setup(ec);

    return ec;
}

void
sp_event_context_finish(SPEventContext *ec)
{
    g_return_if_fail(ec != NULL);
    g_return_if_fail(SP_IS_EVENT_CONTEXT(ec));

    ec->enableSelectionCue(false);

    if (ec->next) {
        g_warning("Finishing event context with active link\n");
    }

    if (((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->finish)
        ((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->finish(ec);
}

void SPEventContext::enableSelectionCue(bool enable) {
    if (enable) {
        if (!_selcue) {
            _selcue = new SPSelCue(desktop);
        }
    } else {
        if (_selcue) {
            delete _selcue;
            _selcue = NULL;
        }
    }
}

void SPEventContext::enableGrDrag(bool enable) {
    if (enable) {
        if (!_grdrag) {
            _grdrag = new GrDrag(desktop);
        }
    } else {
        if (_grdrag) {
            delete _grdrag;
            _grdrag = NULL;
        }
    }
}

void
sp_event_context_read(SPEventContext *ec, gchar const *key)
{
    g_return_if_fail(ec != NULL);
    g_return_if_fail(SP_IS_EVENT_CONTEXT(ec));
    g_return_if_fail(key != NULL);

    if (ec->repr) {
        gchar const *val = ec->repr->attribute(key);
        if (((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->set)
            ((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->set(ec, key, val);
    }
}

void
sp_event_context_activate(SPEventContext *ec)
{
    g_return_if_fail(ec != NULL);
    g_return_if_fail(SP_IS_EVENT_CONTEXT(ec));

    if (((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->activate)
        ((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->activate(ec);
}

void
sp_event_context_deactivate(SPEventContext *ec)
{
    g_return_if_fail(ec != NULL);
    g_return_if_fail(SP_IS_EVENT_CONTEXT(ec));

    if (((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->activate)
        ((SPEventContextClass *) G_OBJECT_GET_CLASS(ec))->activate(ec);
}

gint
sp_event_context_root_handler(SPEventContext * event_context, GdkEvent * event)
{
    gint ret;

    ret = ((SPEventContextClass *) G_OBJECT_GET_CLASS(event_context))->root_handler(event_context, event);

    set_event_location(event_context->desktop, event);

    return ret;
}

gint
sp_event_context_item_handler(SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
    gint ret;

    ret = ((SPEventContextClass *) G_OBJECT_GET_CLASS(event_context))->item_handler(event_context, item, event);

    if (! ret) {
        ret = sp_event_context_root_handler(event_context, event);
    } else {
        set_event_location(event_context->desktop, event);
    }

    return ret;
}

static void set_event_location(SPDesktop *desktop, GdkEvent *event)
{
    if (event->type != GDK_MOTION_NOTIFY) {
        return;
    }

    NR::Point const button_w(event->button.x, event->button.y);
    NR::Point const button_dt(sp_desktop_w2d_xy_point(desktop, button_w));
    sp_view_set_position(SP_VIEW(desktop), button_dt);
    sp_desktop_set_coordinate_status(desktop, button_dt, 0);
}

void
sp_event_root_menu_popup(SPDesktop *desktop, SPItem *item, GdkEvent *event)
{
    GtkWidget *menu;

    /* fixme: This is not what I want but works for now (Lauris) */
    if (event->type == GDK_KEY_PRESS) {
        item = SP_DT_SELECTION(desktop)->singleItem();
    }
    menu = sp_ui_context_menu(SP_VIEW(desktop), item);
    gtk_widget_show(menu);

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            gtk_menu_popup(GTK_MENU(menu), NULL, NULL, 0, NULL, event->button.button, event->button.time);
            break;
        case GDK_KEY_PRESS:
            gtk_menu_popup(GTK_MENU(menu), NULL, NULL, 0, NULL, 0, event->key.time);
            break;
        default:
            break;
    }
}

void
sp_event_show_modifier_tip(Inkscape::MessageContext *message_context, GdkEvent *event,
                           gchar const *ctrl_tip, gchar const *shift_tip, gchar const *alt_tip)
{
    guint keyval = get_group0_keyval(&event->key);
    bool ctrl = ctrl_tip && (MOD__CTRL || (keyval == GDK_Control_L) || (keyval == GDK_Control_R));
    bool shift = shift_tip && (MOD__SHIFT || (keyval == GDK_Shift_L) || (keyval == GDK_Shift_R));
    bool alt = alt_tip && (MOD__ALT || (keyval == GDK_Alt_L) || (keyval == GDK_Alt_R) || (keyval == GDK_Meta_L) || (keyval == GDK_Meta_R));

    gchar *tip = g_strdup_printf("%s%s%s%s%s", 
                                 ( ctrl ? ctrl_tip : "" ),
                                 ( ctrl && (shift || alt) ? "; " : "" ),
                                 ( shift ? shift_tip : "" ),
                                 ( (ctrl || shift) && alt ? "; " : "" ),
                                 ( alt ? alt_tip : "" ));

    if (strlen(tip) > 0) {
        message_context->flash(Inkscape::INFORMATION_MESSAGE, tip);
    }

    g_free(tip);
}

/**
Return the keyval corresponding to the key event in group 0, i.e. in the main (English)
layout.  Use this instead of simply event->keyval, so that your keyboard shortcuts work
regardless of layouts (e.g. in Cyrillic).
 */
guint
get_group0_keyval(GdkEventKey *event) 
{
    guint keyval = 0;
    gdk_keymap_translate_keyboard_state(gdk_keymap_get_for_display(gdk_display_get_default()),
                                        event->hardware_keycode, (GdkModifierType) event->state, 0/*event->key.group*/,
                                        &keyval, NULL, NULL, NULL);
    return keyval;
}

/**
Returns item at point p in desktop; if state includes alt key mask, cyclically selects under; honors into_groups
*/
SPItem *
sp_event_context_find_item (SPDesktop *desktop, NR::Point const p, int state, gboolean into_groups)
{
    SPItem *item;

    if (state & GDK_MOD1_MASK) { // select under
        SPItem *selected_at_point = sp_desktop_item_from_list_at_point_bottom (desktop,
                                                                               SP_DT_SELECTION(desktop)->itemList(), p);
        item = sp_desktop_item_at_point(desktop, p, into_groups, selected_at_point);
        if (item == NULL) { // we may have reached bottom, flip over to the top
            item = sp_desktop_item_at_point(desktop, p, into_groups, NULL);
        }
    } else 
        item = sp_desktop_item_at_point(desktop, p, into_groups, NULL);

    return item;
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
