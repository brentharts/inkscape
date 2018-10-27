/** \file
 * Notebook and NotebookPage parameters for extensions.
 */

/*
 * Authors:
 *   Johan Engelen <johan@shouraizou.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2006 Author
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/notebook.h>

#include <glibmm/i18n.h>

#include <xml/node.h>

#include <extension/extension.h>
#include "preferences.h"

#include "notebook.h"

/**
 * The root directory in the preferences database for extension
 * related parameters.
 */
#define PREF_DIR "extensions"

namespace Inkscape {
namespace Extension {


ParamNotebook::ParamNotebookPage::ParamNotebookPage(const gchar * name,
                                     const gchar * text,
                                     const gchar * description,
                                     bool hidden,
                                     Inkscape::Extension::Extension * ext,
                                     Inkscape::XML::Node * xml)
    : Parameter(name, text, description, hidden, /*indent*/ 0, ext)
{

    // Read XML to build page
    if (xml != nullptr) {
        Inkscape::XML::Node *child_repr = xml->firstChild();
        while (child_repr != nullptr) {
            char const * chname = child_repr->name();
            if (!strncmp(chname, INKSCAPE_EXTENSION_NS_NC, strlen(INKSCAPE_EXTENSION_NS_NC))) {
                chname += strlen(INKSCAPE_EXTENSION_NS);
            }
            if (chname[0] == '_') // Allow _ for translation of tags
                chname++;
            if (!strcmp(chname, "param") || !strcmp(chname, "_param")) {
                Parameter * param;
                param = Parameter::make(child_repr, ext);
                if (param != nullptr) parameters.push_back(param);
            }
            child_repr = child_repr->next();
        }
    }
}

ParamNotebook::ParamNotebookPage::~ParamNotebookPage ()
{
    //destroy parameters
    for (auto param:parameters) {
        delete param;
    }
}

/** Return the value as a string. */
void ParamNotebook::ParamNotebookPage::paramString(std::list <std::string> &list)
{
    for (auto param:parameters) {
        param->string(list);
    }
}


/**
    \return None
    \brief  This function creates a page that can be used later.  This
            is typically done in the creation of the notebook and defined
            in the XML file describing the extension (it's private so people
            have to use the system) :)
    \param  in_repr  The XML describing the page
    \todo   the 'gui-hidden' attribute is read but not used!

    This function first grabs all of the data out of the Repr and puts
    it into local variables.  Actually, these are just pointers, and the
    data is not duplicated so we need to be careful with it.  If there
    isn't a name in the XML, then no page is created as
    the function just returns.

    From this point on, we're pretty committed as we've allocated an
    object and we're starting to fill it.  The name is set first, and
    is created with a strdup to actually allocate memory for it.  Then
    there is a case statement (roughly because strcmp requires 'ifs')
    based on what type of parameter this is.  Depending which type it
    is, the value is interpreted differently, but they are relatively
    straight forward.  In all cases the value is set to the default
    value from the XML and the type is set to the interpreted type.
*/
ParamNotebook::ParamNotebookPage *
ParamNotebook::ParamNotebookPage::makepage (Inkscape::XML::Node * in_repr, Inkscape::Extension::Extension * in_ext)
{
    const char * name;
    const char * text;
    const char * description;
    bool hidden = false;
    const char * hide;

    name = in_repr->attribute("name");
    text = in_repr->attribute("gui-text");
    if (text == nullptr)
        text = in_repr->attribute("_gui-text");
    description = in_repr->attribute("gui-description");
    if (description == nullptr)
        description = in_repr->attribute("_gui-description");
    hide = in_repr->attribute("gui-hidden");
    if (hide != nullptr) {
        if (strcmp(hide, "1") == 0 ||
            strcmp(hide, "true") == 0) {
                hidden = true;
        }
        /* else stays false */
    }

    /* In this case we just don't have enough information */
    if (name == nullptr) {
        return nullptr;
    }

    ParamNotebookPage * page = new ParamNotebookPage(name, text, description, hidden, in_ext, in_repr);

    /* Note: page could equal NULL */
    return page;
}



/**
 * Creates a notebookpage widget for a notebook.
 *
 * Builds a notebook page (a vbox) and puts parameters on it.
 */
Gtk::Widget * ParamNotebook::ParamNotebookPage::get_widget(SPDocument * doc, Inkscape::XML::Node * node, sigc::signal<void> * changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    Gtk::VBox * vbox = Gtk::manage(new Gtk::VBox);
    vbox->set_border_width(Parameter::GUI_BOX_MARGIN);
    vbox->set_spacing(Parameter::GUI_BOX_SPACING);

    // add parameters onto page (if any)
    for (auto param:parameters) {
        Gtk::Widget * widg = param->get_widget(doc, node, changeSignal);
        if (widg) {
            int indent = param->get_indent();
#if GTK_CHECK_VERSION(3,12,0)
            widg->set_margin_start(indent * Parameter::GUI_INDENTATION);
#else
            widg->set_margin_left(indent * Parameter::GUI_INDENTATION);
#endif
            vbox->pack_start(*widg, false, false, 0);

            gchar const * tip = param->get_tooltip();
            if (tip) {
                widg->set_tooltip_text(tip);
            } else {
                widg->set_tooltip_text("");
                widg->set_has_tooltip(false);
            }
        }
    }

    vbox->show();

    return dynamic_cast<Gtk::Widget *>(vbox);
}

/** Search the parameter's name in the page content. */
Parameter *ParamNotebook::ParamNotebookPage::get_param(const gchar * name)
{
    if (name == nullptr) {
        throw Extension::param_not_exist();
    }
    if (this->parameters.empty()) {
        // the list of parameters is empty
        throw Extension::param_not_exist();
    }

    for (auto param:parameters) {
        if (!strcmp(param->name(), name)) {
            return param;
        }
    }

    return nullptr;
}

/** End ParamNotebookPage **/
/** ParamNotebook **/

ParamNotebook::ParamNotebook(const gchar * name,
                             const gchar * text,
                             const gchar * description,
                             bool hidden,
                             int indent,
                             Inkscape::Extension::Extension * ext,
                             Inkscape::XML::Node * xml)
    : Parameter(name, text, description, hidden, indent, ext)
{
    // Read XML tree to add pages:
    if (xml != nullptr) {
        Inkscape::XML::Node *child_repr = xml->firstChild();
        while (child_repr != nullptr) {
            char const * chname = child_repr->name();
            if (!strncmp(chname, INKSCAPE_EXTENSION_NS_NC, strlen(INKSCAPE_EXTENSION_NS_NC))) {
                chname += strlen(INKSCAPE_EXTENSION_NS);
            }
            if (chname[0] == '_') // Allow _ for translation of tags
                chname++;
            if (!strcmp(chname, "page")) {
                ParamNotebookPage * page;
                page = ParamNotebookPage::makepage(child_repr, ext);
                if (page != nullptr) pages.push_back(page);
            }
            child_repr = child_repr->next();
        }
    }

    // Initialize _value with the current page
    const char * defaultval = nullptr;
    // set first page as default
    if (!pages.empty()) {
        defaultval = pages[0]->name();
    }

    gchar * pref_name = this->pref_name();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring paramval = prefs->getString(extension_pref_root + pref_name);
    g_free(pref_name);

    if (!paramval.empty())
        defaultval = paramval.data();
    if (defaultval != nullptr)
        _value = g_strdup(defaultval);  // allocate space for _value
}

ParamNotebook::~ParamNotebook ()
{
    //destroy pages
    for (auto page:pages) {
        delete page;
    }
    g_free(_value);
}


/**
 * A function to set the \c _value.
 *
 * This function sets the internal value, but it also sets the value
 * in the preferences structure.  To put it in the right place, \c PREF_DIR
 * and \c pref_name() are used.
 *
 * To copy the data into _value the old memory must be free'd first.
 * It is important to note that \c g_free handles \c NULL just fine.  Then
 * the passed in value is duplicated using \c g_strdup().
 *
 * @param  in   The number of the page which value must be set.
 * @param  doc  A document that should be used to set the value.
 * @param  node The node where the value may be placed.
 */
const gchar *ParamNotebook::set(const int in, SPDocument * /*doc*/, Inkscape::XML::Node * /*node*/)
{
    int i = in < pages.size() ? in : pages.size()-1;
    ParamNotebookPage * page = pages[i];

    if (page == nullptr) return _value;

    if (_value != nullptr) g_free(_value);
    _value = g_strdup(page->name());

    gchar * prefname = this->pref_name();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString(extension_pref_root + prefname, _value);
    g_free(prefname);

    return _value;
}

void ParamNotebook::string(std::list <std::string> &list) const
{
    std::string param_string;
    param_string += "--";
    param_string += name();
    param_string += "=";

    param_string += "\"";
    param_string += _value;  // the name of the current page
    param_string += "\"";
    list.insert(list.end(), param_string);

    for (auto page:pages) {
        page->paramString(list);
    }
}

/** A special category of Gtk::Notebook to handle notebook parameters. */
class ParamNotebookWdg : public Gtk::Notebook {
private:
    ParamNotebook * _pref;
    SPDocument * _doc;
    Inkscape::XML::Node * _node;
public:
    /**
     * Build a notebookpage preference for the given parameter.
     * @param  pref  Where to get the string (pagename) from, and where to put it
     *               when it changes.
     */
    ParamNotebookWdg (ParamNotebook * pref, SPDocument * doc, Inkscape::XML::Node * node) :
        Gtk::Notebook(), _pref(pref), _doc(doc), _node(node), activated(false) {
        // don't have to set the correct page: this is done in ParamNotebook::get_widget.
        // hook function
        this->signal_switch_page().connect(sigc::mem_fun(this, &ParamNotebookWdg::changed_page));
    };
    void changed_page(Gtk::Widget *page, guint pagenum);
    bool activated;
};

/**
 * Respond to the selected page of notebook changing.
 * This function responds to the changing by reporting it to
 * ParamNotebook. The change is only reported when the notebook
 * is actually visible. This to exclude 'fake' changes when the
 * notebookpages are added or removed.
 */
void ParamNotebookWdg::changed_page(Gtk::Widget * /*page*/, guint pagenum)
{
    if (get_visible()) {
        _pref->set((int)pagenum, _doc, _node);
    }
}

/** Search the parameter's name in the notebook content. */
Parameter *ParamNotebook::get_param(const gchar * name)
{
    if (name == nullptr) {
        throw Extension::param_not_exist();
    }
    for (auto page:pages) {
        Parameter * subparam = page->get_param(name);
        if (subparam) {
            return subparam;
        }
    }

    return nullptr;
}


/**
 * Creates a Notebook widget for a notebook parameter.
 *
 * Builds a notebook and puts pages in it.
 */
Gtk::Widget * ParamNotebook::get_widget(SPDocument * doc, Inkscape::XML::Node * node, sigc::signal<void> * changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    ParamNotebookWdg * nb = Gtk::manage(new ParamNotebookWdg(this, doc, node));

    // add pages (if any)
    int i = -1;
    int pagenr = i;
    for (auto page:pages) {
        i++;
        Gtk::Widget * widg = page->get_widget(doc, node, changeSignal);
        nb->append_page(*widg, _(page->get_text()));
        if (!strcmp(_value, page->name())) {
            pagenr = i; // this is the page to be displayed?
        }
    }

    nb->show();

    if (pagenr >= 0) nb->set_current_page(pagenr);

    return dynamic_cast<Gtk::Widget *>(nb);
}


}  // namespace Extension
}  // namespace Inkscape

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
