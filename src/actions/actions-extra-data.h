// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Extra data associated with actions: Label, Section, Tooltip.
 *
 * Extra data is indexed by "detailed action names", that is an action
 * with prefix and value (if statefull). For example:
 *   "win.canvas-display-mode(1)"
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_ACTIONS_EXTRA_DATA_H
#define INK_ACTIONS_EXTRA_DATA_H

#include <glibmm/ustring.h>
#include <glibmm/varianttype.h>
#include <map>
#include <utility>
#include <vector>

enum class ParamType
{
    INTEGER,
    DOUBLE,
    STRING,
};

<<<<<<< HEAD
class InkActionExtraDatum {
public:
    InkActionExtraDatum(Glib::ustring& label, Glib::ustring& section, Glib::ustring& tooltip)
        : action_label(label)
        , action_section(section)
        , action_tooltip(tooltip)
    {
    }

    Glib::ustring get_label()   { return action_label; }
    Glib::ustring get_section() { return action_section; }
    Glib::ustring get_tooltip() { return action_tooltip; }

private:
    Glib::ustring action_label;
    Glib::ustring action_section;
    Glib::ustring action_tooltip;
};
=======
struct ParamDetails
{
    Glib::ustring name;
    Glib::ustring description;
    ParamType type;
};

using Parameters = std::vector<ParamDetails>;
class InkActionExtraDatum
{
public:
    InkActionExtraDatum(Glib::ustring &&label, Glib::ustring &&section, Glib::ustring &&tooltip,
                        Parameters &&parameters = {})
        : _action_label(label)
        , _action_section(section)
        , _action_tooltip(tooltip)
        , _parameters(parameters)
    {}

    Glib::ustring get_label() { return _action_label; }
    Glib::ustring get_section() { return _action_section; }
    Glib::ustring get_tooltip() { return _action_tooltip; }

private:
    Glib::ustring _action_label;
    Glib::ustring _action_section;
    Glib::ustring _action_tooltip;
>>>>>>> 4dfbce63a3...  Added feature to add parameter descriptions and modified existing action extra data to comply

    Parameters _parameters;
};

<<<<<<< HEAD
public:
    InkActionExtraData() = default;

    std::vector<Glib::ustring> get_actions();

    void add_data(std::vector<std::vector<Glib::ustring>> &raw_data);

    Glib::ustring get_label_for_action(Glib::ustring const &action_name);
    Glib::ustring get_section_for_action(Glib::ustring const &action_name);
    Glib::ustring get_tooltip_for_action(Glib::ustring const &action_name);
=======
class InkActionExtraData
{
public:
    InkActionExtraData() = default;

    void add_data(std::vector<std::pair<Glib::ustring, InkActionExtraDatum>> &raw_data);

    Glib::ustring get_label_for_action(Glib::ustring &action_name);
    Glib::ustring get_section_for_action(Glib::ustring &action_name);
    Glib::ustring get_tooltip_for_action(Glib::ustring &action_name);
    ParamDetails get_parameters_for_action(Glib::ustring &action_name);
>>>>>>> 4dfbce63a3...  Added feature to add parameter descriptions and modified existing action extra data to comply

private:
    std::map<Glib::ustring, InkActionExtraDatum> data;
};

#endif // INK_ACTIONS_EXTRA_DATA_H

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
