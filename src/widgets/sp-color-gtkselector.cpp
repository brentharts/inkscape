#include <config.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktable.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkcolorsel.h>
#include "../color.h"
#include "../helper/sp-intl.h"
#include "../dialogs/dialog-events.h"
#include "sp-color-gtkselector.h"


static void sp_color_gtkselector_class_init (SPColorGtkselectorClass *klass);
static void sp_color_gtkselector_init (SPColorGtkselector *csel);
static void sp_color_gtkselector_destroy (GtkObject *object);

static void sp_color_gtkselector_show_all (GtkWidget *widget);
static void sp_color_gtkselector_hide_all (GtkWidget *widget);


static SPColorSelectorClass *parent_class;

#define XPAD 4
#define YPAD 1

GType
sp_color_gtkselector_get_type (void)
{
	static GType type = 0;
	if (!type) {
		static const GTypeInfo info = {
			sizeof (SPColorGtkselectorClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) sp_color_gtkselector_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (SPColorGtkselector),
			0,	  /* n_preallocs */
			(GInstanceInitFunc) sp_color_gtkselector_init,
		};

		type = g_type_register_static (SP_TYPE_COLOR_SELECTOR,
									   "SPColorGtkselector",
									   &info,
									   INK_STATIC_CAST( GTypeFlags, 0));
	}
	return type;
}

static void
sp_color_gtkselector_class_init (SPColorGtkselectorClass *klass)
{
	static const gchar* nameset[] = {N_("GTK+"), 0};
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	SPColorSelectorClass *selector_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;
	selector_class = SP_COLOR_SELECTOR_CLASS (klass);

	parent_class = SP_COLOR_SELECTOR_CLASS (g_type_class_peek_parent (klass));

	selector_class->name = nameset;
	selector_class->submode_count = 1;

	object_class->destroy = sp_color_gtkselector_destroy;

	widget_class->show_all = sp_color_gtkselector_show_all;
	widget_class->hide_all = sp_color_gtkselector_hide_all;
}

void sp_color_gtkselector_init (SPColorGtkselector *csel)
{
    SP_COLOR_SELECTOR(csel)->base = new ColorGtkselector( SP_COLOR_SELECTOR(csel) );

    if ( SP_COLOR_SELECTOR(csel)->base )
    {
        SP_COLOR_SELECTOR(csel)->base->init();
    }
}

void ColorGtkselector::init()
{
	GtkWidget *gtksel;

	gtksel = gtk_color_selection_new();
	gtk_widget_show (gtksel);
	_gtkThing = GTK_COLOR_SELECTION (gtksel);
	gtk_box_pack_start (GTK_BOX (_csel), gtksel, TRUE, TRUE, 0);

	gtk_signal_connect (GTK_OBJECT (gtksel), "color-changed", GTK_SIGNAL_FUNC (_gtkChanged), _csel);
}

static void
sp_color_gtkselector_destroy (GtkObject *object)
{
	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_color_gtkselector_show_all (GtkWidget *widget)
{
	gtk_widget_show (widget);
}

static void
sp_color_gtkselector_hide_all (GtkWidget *widget)
{
	gtk_widget_hide (widget);
}

GtkWidget *
sp_color_gtkselector_new (GType selector_type, SPColorSpaceType colorspace)
{
	SPColorGtkselector *csel;

	csel = (SPColorGtkselector*)gtk_type_new (SP_TYPE_COLOR_GTKSELECTOR);

	return GTK_WIDGET (csel);
}

ColorGtkselector::ColorGtkselector( SPColorSelector* csel )
    : ColorSelector( csel ),
      _gtkThing(0)
{
}

ColorGtkselector::~ColorGtkselector()
{
}

void ColorGtkselector::_colorChanged( const SPColor& color, gfloat alpha )
{
    GdkColor gcolor;
    float rgb[3];
    g_return_if_fail (_csel != NULL);
    g_return_if_fail (SP_IS_COLOR_GTKSELECTOR (_csel));

    sp_color_copy (&_color, &color);
    _alpha = alpha;

    sp_color_get_rgb_floatv( &color, rgb );
    gcolor.pixel = 0;
    gcolor.red = INK_STATIC_CAST (guint16, rgb[0] * 65535);
    gcolor.green = INK_STATIC_CAST (guint16, rgb[1] * 65535);
    gcolor.blue = INK_STATIC_CAST (guint16, rgb[2] * 65535);

//     g_message( "_colorChanged %04x %04x %04x", gcolor.red, gcolor.green, gcolor.blue );
    g_signal_handlers_block_by_func( _gtkThing, (gpointer)(_gtkChanged), _csel );
    gtk_color_selection_set_current_alpha (_gtkThing, (guint16)(65535 * alpha));
    gtk_color_selection_set_current_color (_gtkThing, &gcolor);
    g_signal_handlers_unblock_by_func(_gtkThing, (gpointer)(_gtkChanged), _csel );
}

void ColorGtkselector::_gtkChanged( GtkColorSelection *colorselection, SPColorGtkselector *gtksel )
{
    ColorGtkselector* gtkInst = (ColorGtkselector*)(SP_COLOR_SELECTOR(gtksel)->base);
    SPColor ourColor;
    GdkColor color;
    guint16 alpha;

    gtk_color_selection_get_current_color (colorselection, &color);
    alpha = gtk_color_selection_get_current_alpha (colorselection);

    sp_color_set_rgb_float (&ourColor, (color.red / 65535.0), (color.green / 65535.0), (color.blue / 65535.0));

//     g_message( "_gtkChanged   %04x %04x %04x", color.red, color.green, color.blue );

    gtkInst->_updateInternals( ourColor, INK_STATIC_CAST(gfloat, alpha / 65535.0), FALSE );
}
