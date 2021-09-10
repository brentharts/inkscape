// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __SP_SPRAY_CONTEXT_H__
#define __SP_SPRAY_CONTEXT_H__

/*
 * Spray Tool
 *
 * Authors:
 *   Pierre-Antoine MARC
 *   Pierre CACLIN
 *   Aurel-Aimé MARMION
 *   Julien LERAY
 *   Benoît LAVORATA
 *   Vincent MONTAGNE
 *   Pierre BARBRY-BLOT
 *   Jabiertxo ARRAIZA
 *   Adrian Boguszewski
 *
 * Copyright (C) 2009 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/point.h>
#include "ui/tools/tool-base.h"
#include "object/object-set.h"

#define SP_SPRAY_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::SprayTool*>((Inkscape::UI::Tools::ToolBase*)obj))
#define SP_IS_SPRAY_CONTEXT(obj) (dynamic_cast<const Inkscape::UI::Tools::SprayTool*>((const Inkscape::UI::Tools::ToolBase*)obj) != NULL)

namespace Inkscape {
  class CanvasItemBpath;
  namespace UI {
      namespace Dialog {
          class Dialog;
      }
  }
}


#define SAMPLING_SIZE 8        /* fixme: ?? */

#define TC_MIN_PRESSURE      0.0
#define TC_MAX_PRESSURE      1.0
#define TC_DEFAULT_PRESSURE  0.35

namespace Inkscape {
namespace UI {
namespace Tools {

enum {
    SPRAY_MODE_COPY,
    SPRAY_MODE_CLONE,
    SPRAY_MODE_SINGLE_PATH,
    SPRAY_MODE_ERASER,
    SPRAY_OPTION,
};

class SprayTool : public ToolBase {
public:
    SprayTool();
    ~SprayTool() override;

    //ToolBase event_context;
    /* extended input data */
    gdouble pressure;

    /* attributes */
    bool dragging;           /* mouse state: mouse is dragging */
    bool usepressurewidth;
    bool usepressurepopulation;
    bool usepressurescale;
    bool usetilt;
    bool usetext;

    double width;
    double ratio;
    double tilt;
    double rotation_variation;
    double population;
    double scale_variation;
    double scale;
    double mean;
    double standard_deviation;

    gint distrib;

    gint mode;

    bool is_drawing;

    bool is_dilating;
    bool has_dilated;
    Geom::Point last_push;
    Inkscape::CanvasItemBpath *dilate_area;
    bool no_overlap;
    bool picker;
    bool pick_center;
    bool pick_inverse_value;
    bool pick_fill;
    bool pick_stroke;
    bool pick_no_overlap;
    bool over_transparent;
    bool over_no_transparent;
    double offset;
    int pick;
    bool do_trace;
    bool pick_to_size;
    bool pick_to_presence;
    bool pick_to_color;
    bool pick_to_opacity;
    bool invert_picked;
    double gamma_picked;
    double rand_picked;
    sigc::connection style_set_connection;

    static const std::string prefsPath;

    void setup() override;
    void set(const Inkscape::Preferences::Entry& val) override;
    virtual void setCloneTilerPrefs();
    bool root_handler(GdkEvent* event) override;

    const std::string& getPrefsPath() override;

    void update_cursor(bool /*with_shift*/);

    ObjectSet* objectSet() {
        return &object_set;
    }
    SPItem* single_path_output = nullptr;

private:
    ObjectSet object_set;
};

}
}
}

#endif

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

