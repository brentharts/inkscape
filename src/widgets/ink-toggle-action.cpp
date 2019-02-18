// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "ink-toggle-action.h"
#include "ui/icon-loader.h"

static void ink_toggle_action_finalize(GObject* obj);
static void ink_toggle_action_get_property(GObject* obj, guint propId, GValue* value, GParamSpec * pspec);
static void ink_toggle_action_set_property(GObject* obj, guint propId, const GValue *value, GParamSpec* pspec);

static GtkWidget* ink_toggle_action_create_menu_item(GtkAction* action);
static GtkWidget* ink_toggle_action_create_tool_item(GtkAction* action);

static void ink_toggle_action_update_icon(InkToggleAction* action);

typedef struct {
    gchar* iconId;
    GtkIconSize iconSize;
} InkToggleActionPrivate;

#define INK_TOGGLE_ACTION_GET_PRIVATE(o) \
    reinterpret_cast<InkToggleActionPrivate *>(ink_toggle_action_get_instance_private (o))

G_DEFINE_TYPE_WITH_PRIVATE (InkToggleAction, ink_toggle_action, GTK_TYPE_TOGGLE_ACTION);

enum {
    PROP_INK_ID = 1,
    PROP_INK_SIZE
};

static void ink_toggle_action_class_init(InkToggleActionClass* klass)
{
    if (klass) {
        GObjectClass * objClass = G_OBJECT_CLASS(klass);

        objClass->finalize = ink_toggle_action_finalize;
        objClass->get_property = ink_toggle_action_get_property;
        objClass->set_property = ink_toggle_action_set_property;

        klass->parent_class.parent_class.create_menu_item = ink_toggle_action_create_menu_item;
        klass->parent_class.parent_class.create_tool_item = ink_toggle_action_create_tool_item;
        /*klass->parent_class.connect_proxy = connect_proxy;*/
        /*klass->parent_class.disconnect_proxy = disconnect_proxy;*/

        g_object_class_install_property(objClass,
                                         PROP_INK_ID,
                                         g_param_spec_string("iconId",
                                                              "Icon ID",
                                                              "The id for the icon",
                                                              "",
                                                              (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT)));

        g_object_class_install_property(objClass,
                                         PROP_INK_SIZE,
                                         g_param_spec_int("iconSize",
                                                           "Icon Size",
                                                           "The size the icon",
                                                           GTK_ICON_SIZE_MENU,
                                                           (int)99,
                                                           GTK_ICON_SIZE_SMALL_TOOLBAR,
                                                           (GParamFlags)(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT)));
    }
}

static void ink_toggle_action_init(InkToggleAction* action)
{
    auto priv = INK_TOGGLE_ACTION_GET_PRIVATE(action);
    priv->iconId = nullptr;
    priv->iconSize = GTK_ICON_SIZE_SMALL_TOOLBAR;
}

static void ink_toggle_action_finalize(GObject* obj)
{
    InkToggleAction* action = INK_TOGGLE_ACTION(obj);
    auto priv = INK_TOGGLE_ACTION_GET_PRIVATE(action);

    g_free(priv->iconId);
    g_free(priv);
}

/**
 * \brief Create a new toggle action
 *
 * \param[in] name    The name of the Action
 * \param[in] label   The label text to display on the Action's tool item
 * \param[in] tooltip The tooltip text for the Action's tool item
 * \param[in] inkId
 * \param[in] size    The size of the tool item to display
 *
 * \detail The name is used by the UI Manager to look up the action when specified in a UI XML
 *         file.
 *
 * \deprecated GtkActions are deprecated.  Use a Gtk::ToggleToolButton instead.
 */
InkToggleAction* ink_toggle_action_new(const gchar *name,
                           const gchar *label,
                           const gchar *tooltip,
                           const gchar *inkId,
                           GtkIconSize  size,
                           SPAttributeEnum attr)
{
    GObject* obj = (GObject*)g_object_new(INK_TOGGLE_ACTION_TYPE,
                                           "name", name,
                                           "label", label,
                                           "tooltip", tooltip,
                                           "iconId", inkId,
                                           "iconSize", size,
                                           //"SP_ATTR_INKSCAPE", attr, // Why doesn't this work and do I need to use g_object_set_data below?
                                           NULL);

    g_object_set_data(obj, "SP_ATTR_INKSCAPE", GINT_TO_POINTER(attr));
    InkToggleAction* action = INK_TOGGLE_ACTION(obj);

    return action;
}

static void ink_toggle_action_get_property(GObject* obj, guint propId, GValue* value, GParamSpec * pspec)
{
    InkToggleAction* action = INK_TOGGLE_ACTION(obj);
    auto priv = INK_TOGGLE_ACTION_GET_PRIVATE(action);

    switch (propId) {
        case PROP_INK_ID:
        {
            g_value_set_string(value, priv->iconId);
        }
        break;

        case PROP_INK_SIZE:
        {
            g_value_set_int(value, priv->iconSize);
        }
        break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, propId, pspec);
    }
}

void ink_toggle_action_set_property(GObject* obj, guint propId, const GValue *value, GParamSpec* pspec)
{
    InkToggleAction* action = INK_TOGGLE_ACTION(obj);
    auto priv = INK_TOGGLE_ACTION_GET_PRIVATE(action);

    switch (propId) {
        case PROP_INK_ID:
        {
            gchar* tmp = priv->iconId;
            priv->iconId = g_value_dup_string(value);
            g_free(tmp);

            ink_toggle_action_update_icon(action);
        }
        break;

        case PROP_INK_SIZE:
        {
            priv->iconSize = (GtkIconSize)g_value_get_int(value);
        }
        break;

        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, propId, pspec);
        }
    }
}

static GtkWidget* ink_toggle_action_create_menu_item(GtkAction* action)
{
    GtkWidget* item = GTK_TOGGLE_ACTION_CLASS(ink_toggle_action_parent_class)->parent_class.create_menu_item(action);

    return item;
}

static GtkWidget* ink_toggle_action_create_tool_item(GtkAction* action)
{
    InkToggleAction* act = INK_TOGGLE_ACTION(action);
    auto priv = INK_TOGGLE_ACTION_GET_PRIVATE(act);

    GtkWidget* item = GTK_TOGGLE_ACTION_CLASS(ink_toggle_action_parent_class)->parent_class.create_tool_item(action);
    if (GTK_IS_TOOL_BUTTON(item)) {
        GtkToolButton* button = GTK_TOOL_BUTTON(item);
        if (priv->iconId) {
            GtkWidget *child = sp_get_icon_image(priv->iconId, priv->iconSize);

            gtk_widget_set_hexpand(child, FALSE);
            gtk_widget_set_vexpand(child, FALSE);
            gtk_tool_button_set_icon_widget(button, child);
        } else {
            gchar *label = nullptr;
            g_object_get(G_OBJECT(action), "short_label", &label, NULL);
            gtk_tool_button_set_label(button, label);
            g_free(label);
            label = nullptr;
        }
    } else {
        // For now trigger a warning but don't do anything else
        GtkToolButton* button = GTK_TOOL_BUTTON(item);
        (void)button;
    }
    gtk_widget_show_all(item);

    return item;
}


static void ink_toggle_action_update_icon(InkToggleAction* action)
{
    auto priv = INK_TOGGLE_ACTION_GET_PRIVATE(action);

    if (action) {
        GSList* proxies = gtk_action_get_proxies(GTK_ACTION(action));
        while (proxies) {
            if (GTK_IS_TOOL_ITEM(proxies->data)) {
                if (GTK_IS_TOOL_BUTTON(proxies->data)) {
                    GtkToolButton* button = GTK_TOOL_BUTTON(proxies->data);

                    GtkWidget *child = sp_get_icon_image(priv->iconId, priv->iconSize);
                    gtk_widget_set_hexpand(child, FALSE);
                    gtk_widget_set_vexpand(child, FALSE);
                    gtk_widget_show_all(child);
                    gtk_tool_button_set_icon_widget(button, child);
                }
            }

            proxies = g_slist_next(proxies);
        }
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
