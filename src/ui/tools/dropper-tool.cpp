// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Tool for picking colors from drawing
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <2geom/transforms.h>
#include <2geom/circle.h>

#include "color-rgba.h"
#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "message-context.h"
#include "preferences.h"
#include "selection.h"
#include "sp-cursor.h"
#include "style.h"
#include "verbs.h"

#include "display/curve.h"
#include "display/drawing.h"
#include "display/control/canvas-item-bpath.h"
#include "display/control/canvas-item-drawing.h"

#include "include/macros.h"

#include "object/sp-namedview.h"

#include "ui/pixmaps/cursor-dropper-f.xpm"
#include "ui/pixmaps/cursor-dropper-s.xpm"
#include "ui/pixmaps/cursor-dropping-f.xpm"
#include "ui/pixmaps/cursor-dropping-s.xpm"

#include "svg/svg-color.h"

#include "ui/tools/dropper-tool.h"
#include "ui/widget/canvas.h"

using Inkscape::DocumentUndo;

namespace Inkscape {
namespace UI {
namespace Tools {

const std::string& DropperTool::getPrefsPath() {
	return DropperTool::prefsPath;
}

const std::string DropperTool::prefsPath = "/tools/dropper";

DropperTool::DropperTool()
    : ToolBase(cursor_dropper_f_xpm)
	, R(0)
	, G(0)
	, B(0)
	, alpha(0)
	, radius(0)
	, invert(false)
	, stroke(false)
	, dropping(false)
	, dragging(false)
	, area(nullptr)
	, centre(0, 0)
{
}

DropperTool::~DropperTool() = default;

void DropperTool::setup() {
    ToolBase::setup();

    area = new Inkscape::CanvasItemBpath(desktop->getCanvasControls());
    area->set_stroke(0x0000007f);
    area->set_fill(0x0, SP_WIND_RULE_EVENODD);
    area->hide();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    
    if (prefs->getBool("/tools/dropper/selcue")) {
        this->enableSelectionCue();
    }

    if (prefs->getBool("/tools/dropper/gradientdrag")) {
        this->enableGrDrag();
    }
}

void DropperTool::finish() {
    this->enableGrDrag(false);

    ungrabCanvasEvents();

    if (this->area) {
        delete this->area;
        this->area = nullptr;
    }

    ToolBase::finish();
}

/**
 * Returns the current dropper context color.
 */
guint32 DropperTool::get_color(bool invert) {
    Inkscape::Preferences   *prefs = Inkscape::Preferences::get();

    int pick = prefs->getInt("/tools/dropper/pick", SP_DROPPER_PICK_VISIBLE);
    bool setalpha = prefs->getBool("/tools/dropper/setalpha", true);

    return SP_RGBA32_F_COMPOSE(
        fabs(invert - this->R),
        fabs(invert - this->G),
        fabs(invert - this->B),
       (pick == SP_DROPPER_PICK_ACTUAL && setalpha) ? this->alpha : 1.0);
}

bool DropperTool::root_handler(GdkEvent* event) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    int ret = FALSE;
    int pick = prefs->getInt("/tools/dropper/pick", SP_DROPPER_PICK_VISIBLE);

    // Decide first what kind of 'mode' we're in.
    if (event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE) {
        switch (event->key.keyval) {
            case GDK_KEY_Shift_L:
            case GDK_KEY_Shift_R:
                this->stroke = event->type == GDK_KEY_PRESS;
                break;
            case GDK_KEY_Control_L:
            case GDK_KEY_Control_R:
                this->dropping = event->type == GDK_KEY_PRESS;
                break;
            case GDK_KEY_Alt_L:
            case GDK_KEY_Alt_R:
                this->invert = event->type == GDK_KEY_PRESS;
                break;
        }
    }

    // Get color from selected object instead.
    if(this->dropping) {
	Inkscape::Selection *selection = desktop->getSelection();
	g_assert(selection);
        guint32 apply_color;
        bool apply_set = false;
        for (auto& obj: selection->objects()) {
            if(obj->style) {
                double opacity = 1.0;
                if(!this->stroke && obj->style->fill.set) {
                    if(obj->style->fill_opacity.set) {
                        opacity = obj->style->fill_opacity.value;
                    }
                    apply_color = obj->style->fill.value.color.toRGBA32(opacity);
                    apply_set = true;
                } else if(this->stroke && obj->style->stroke.set) {
                    if(obj->style->stroke_opacity.set) {
                        opacity = obj->style->stroke_opacity.value;
                    }
                    apply_color = obj->style->stroke.value.color.toRGBA32(opacity);
                    apply_set = true;
                }
            }
        }
        if(apply_set) {
            this->R = SP_RGBA32_R_F(apply_color);
            this->G = SP_RGBA32_G_F(apply_color);
            this->B = SP_RGBA32_B_F(apply_color);
            this->alpha = SP_RGBA32_A_F(apply_color);
        } else {
            // This means that having no selection or some other error
            // we will default back to normal dropper mode.
            this->dropping = false;
        }
    }

    switch (event->type) {
	case GDK_BUTTON_PRESS:
            if (event->button.button == 1 && !this->space_panning) {
                this->centre = Geom::Point(event->button.x, event->button.y);
                this->dragging = true;
                ret = TRUE;
            }

            grabCanvasEvents(Gdk::KEY_PRESS_MASK      |
                             Gdk::KEY_RELEASE_MASK    |
                             Gdk::BUTTON_RELEASE_MASK |
                             Gdk::POINTER_MOTION_MASK |
                             Gdk::BUTTON_PRESS_MASK   );
            break;

	case GDK_MOTION_NOTIFY:
            if (event->motion.state & GDK_BUTTON2_MASK || event->motion.state & GDK_BUTTON3_MASK) {
                // pass on middle and right drag
                ret = FALSE;
                break;
            } else if (!this->space_panning) {
                // otherwise, constantly calculate color no matter is any button pressed or not

                Geom::IntRect pick_area;
                if (this->dragging) {
                    // calculate average

                    // radius
                    double rw = std::min(Geom::L2(Geom::Point(event->button.x, event->button.y) - this->centre), 400.0);
                    if (rw == 0) { // happens sometimes, little idea why...
                        break;
                    }
                    this->radius = rw;

                    Geom::Point const cd = desktop->w2d(this->centre);
                    Geom::Affine const w2dt = desktop->w2d();
                    const double scale = rw * w2dt.descrim();
                    Geom::Affine const sm( Geom::Scale(scale, scale) * Geom::Translate(cd) );

                    // Show circle on canvas
                    Geom::PathVector path = Geom::Path(Geom::Circle(0, 0, 1)); // Unit circle centered at origin.
                    path *= sm;
                    this->area->set_bpath(path);
                    this->area->show();

                    /* Get buffer */
                    Geom::Rect r(this->centre, this->centre);
                    r.expandBy(rw);
                    if (!r.hasZeroArea()) {
                        pick_area = r.roundOutwards();

                    }
                } else {
                    // pick single pixel
                    pick_area = Geom::IntRect::from_xywh(floor(event->button.x), floor(event->button.y), 1, 1);
                }

                Inkscape::CanvasItemDrawing *canvas_item_drawing = desktop->getCanvasDrawing();
                Inkscape::Drawing *drawing = canvas_item_drawing->get_drawing();

                // Ensure drawing up-to-date. (Is this really necessary?)
                drawing->update(Geom::IntRect::infinite(), canvas_item_drawing->get_context());

                // Get average color.
                double R, G, B, A;
                drawing->average_color(pick_area, R, G, B, A);

                if (pick == SP_DROPPER_PICK_VISIBLE) {
                    // compose with page color
                    guint32 bg = desktop->getNamedView()->pagecolor;
                    R = R + (SP_RGBA32_R_F(bg)) * (1 - A);
                    G = G + (SP_RGBA32_G_F(bg)) * (1 - A);
                    B = B + (SP_RGBA32_B_F(bg)) * (1 - A);
                    A = 1.0;
                } else {
                    // un-premultiply color channels
                    if (A > 0) {
                        R /= A;
                        G /= A;
                        B /= A;
                    }
                }

                if (fabs(A) < 1e-4) {
                    A = 0; // suppress exponentials, CSS does not allow that
                }

                // remember color
                if(!this->dropping && (R != this->R || G != this->G || B != this->B || A != this->alpha)) {
                    this->R = R;
                    this->G = G;
                    this->B = B;
                    this->alpha = A;
                }
                ret = TRUE;
            }
            break;

	case GDK_BUTTON_RELEASE:
            if (event->button.button == 1 && !this->space_panning) {
                this->area->hide();
		this->dragging = false;

                ungrabCanvasEvents();

                Inkscape::Selection *selection = desktop->getSelection();
                g_assert(selection);
                std::vector<SPItem *> old_selection(selection->items().begin(), selection->items().end());
                if(this->dropping) {
		    Geom::Point const button_w(event->button.x, event->button.y);
		    // remember clicked item, disregarding groups, honoring Alt
		    this->item_to_select = sp_event_context_find_item (desktop, button_w, event->button.state & GDK_MOD1_MASK, TRUE);

                    // Change selected object to object under cursor
                    if (this->item_to_select) {
                        std::vector<SPItem *> vec(selection->items().begin(), selection->items().end());
                        selection->set(this->item_to_select);
                    }
                } else {
                    if (prefs->getBool("/tools/dropper/onetimepick", false)) {
                        // "One time" pick from Fill/Stroke dialog stroke page, always apply fill or stroke (ignore <Shift> key)
                        stroke = (prefs->getInt("/dialogs/fillstroke/page", 0) == 0) ? false : true;
                    }
                }

                // do the actual color setting
                sp_desktop_set_color(desktop, ColorRGBA(this->get_color(this->invert)), false, !this->stroke);

                // REJON: set aux. toolbar input to hex color!
                if (!(desktop->getSelection()->isEmpty())) {
                    DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_DROPPER,
                                       _("Set picked color"));
                }
                if(this->dropping) {
                    selection->setList(old_selection);
                }

                if (prefs->getBool("/tools/dropper/onetimepick", false)) {
                    prefs->setBool("/tools/dropper/onetimepick", false);
                    sp_toggle_dropper(desktop);

                    // sp_toggle_dropper will delete ourselves.
                    // Thus, make sure we return immediately.
                    return true;
                }

                ret = TRUE;
            }
            break;

    case GDK_KEY_PRESS:
        switch (get_latin_keyval(&event->key)) {
          case GDK_KEY_Up:
          case GDK_KEY_Down:
          case GDK_KEY_KP_Up:
          case GDK_KEY_KP_Down:
            // prevent the zoom field from activation
            if (!MOD__CTRL_ONLY(event)) {
                ret = TRUE;
            }
            break;
          case GDK_KEY_Escape:
            desktop->getSelection()->clear();
            break;
        }
        break;
    }

    // set the status message to the right text.
    gchar c[64];
    sp_svg_write_color(c, sizeof(c), this->get_color(this->invert));

    // alpha of color under cursor, to show in the statusbar
    // locale-sensitive printf is OK, since this goes to the UI, not into SVG
    gchar *alpha = g_strdup_printf(_(" alpha %.3g"), this->alpha);
    // where the color is picked, to show in the statusbar
    gchar *where = this->dragging ? g_strdup_printf(_(", averaged with radius %d"), (int) this->radius) : g_strdup_printf("%s", _(" under cursor"));
    // message, to show in the statusbar
    const gchar *message = this->dragging ? _("<b>Release mouse</b> to set color.") : _("<b>Click</b> to set fill, <b>Shift+click</b> to set stroke; <b>drag</b> to average color in area; with <b>Alt</b> to pick inverse color; <b>Ctrl+C</b> to copy the color under mouse to clipboard");

    this->defaultMessageContext()->setF(
        Inkscape::NORMAL_MESSAGE,
        "<b>%s%s</b>%s. %s", c,
        (pick == SP_DROPPER_PICK_VISIBLE) ? "" : alpha, where, message);

    g_free(where);
    g_free(alpha);

    // Set the right cursor for the mode and apply the special Fill color
    auto xpm = (this->dropping ? (this->stroke ? cursor_dropping_s_xpm : cursor_dropping_f_xpm) :
                                 (this->stroke ? cursor_dropper_s_xpm : cursor_dropper_f_xpm));
    GdkCursor *cursor = sp_cursor_from_xpm(xpm, this->get_color(this->invert));
    auto window = desktop->getCanvas()->get_window();
    gdk_window_set_cursor(window->gobj(), cursor);
    g_object_unref(cursor);

    if (!ret) {
    	ret = ToolBase::root_handler(event);
    }

    return ret;
}

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
