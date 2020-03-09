// SPDX-License-Identifier: GPL-2.0-or-later
/**
    \file xmpp.cpp

    A collaborative edition plugin.
*/
/*
 * Copyright (C) 2019 Marc Jeanmougin <marc.jeanmougin@telecom-paris.fr>
 * Copyright (C) 2020 Emmanuel Gil Peyrot <linkmauve@linkmauve.fr>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gloox/sxe.h>
#include <gloox/tag.h>
#include <gloox/jid.h>
#include <gloox/client.h>
#include <gloox/disco.h>

#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/spinbutton.h>

#include "desktop.h"

#include "2geom/geom.h"
#include "document.h"
#include "object/sp-object.h"
#include "selection.h"
#include "xml/attribute-record.h"

#include "svg/path-string.h"

#include "extension/effect.h"
#include "extension/system.h"

#include "util/units.h"

#include "xmpp.h"

namespace Inkscape {
namespace Extension {
namespace Internal {

using namespace gloox;

std::string get_uuid()
{
    // TODO: use a real UUID.
    char attr_rid[11];
    snprintf(attr_rid, 11, "%d", rand());
    return std::string(attr_rid);
}

InkscapeClient::InkscapeClient(JID jid, const std::string& password)
{
    client = new Client(jid, password);
    client->setSASLMechanisms(SaslMechPlain);
    client->disco()->setVersion("Inkscape", "version TODO", "Linux");
    client->registerConnectionListener(this);
    client->logInstance().registerLogHandler(LogLevelDebug, LogAreaXmlOutgoing | LogAreaXmlIncoming, this);
}

bool InkscapeClient::connect()
{
    return client->connect(false);
}

void InkscapeClient::disconnect()
{
    client->disconnect();
    connected = false;
}

bool InkscapeClient::isConnected()
{
    return connected;
}

ConnectionError InkscapeClient::recv()
{
    // Return immediately if no data was available on the socket.
    return client->recv(0);
}

int InkscapeClient::runLoop(void *data)
{
    InkscapeClient *client = static_cast<InkscapeClient*>(data);
    ConnectionError err = client->recv();
    if (err != ConnNoError) {
        printf("Error while receiving on gloox socket: %d\n", err);
        return FALSE;
    }

    return TRUE;
}

// From ConnectionListener
void InkscapeClient::onConnect()
{
    printf("connected!\n");
    connected = true;
}

void InkscapeClient::onDisconnect(ConnectionError e)
{
    printf("disconnected\n");
    connected = false;
}

bool InkscapeClient::onTLSConnect(const CertInfo& info)
{
    printf("accept cert? yes of course\n");
    return true;
}

// From LogHandler
void InkscapeClient::handleLog(LogLevel level, LogArea area, const std::string& message)
{
    printf("gloox: %s\n", message.c_str());
    fflush(stdout);
}

void XMPPObserver::notifyUndoCommitEvent(Event *ee)
{
    XML::Event *e = ee->event;
    std::cout << "UndoCommitEvent" << std::endl;

    while (e) {
        // printf("DOCUMENT %p\n", e->repr->document());
        if (e->repr)
            printf("AFFECTED %s\n", e->repr->attribute("id"));
        XML::EventAdd *eadd;
        XML::EventDel *edel;
        XML::EventChgAttr *echga;
        XML::EventChgContent *echgc;
        XML::EventChgOrder *echgo;
        XML::EventChgElementName *echgn;

        if ((eadd = dynamic_cast<XML::EventAdd *>(e))) {
            std::cout << "EventAdd" << std::endl;
            sp_repr_write_stream(eadd->child, *writer, 0, false, GQuark(0), 0, 0);
            XML::Node *node = eadd->child;

            std::string rid = get_uuid();
            std::string name = node->name();
            if (name.substr(0, 4) != "svg:") {
                printf("Wrong prefix \"%s\"!\n", name.substr(0, 4).c_str());
                abort();
            }
            name = name.substr(4);

            SxeNew new_ = {
                .rid = rid.c_str(),
                .type = "element",
                .name = name.c_str(),
                .ns = "http://www.w3.org/2000/svg",
                .parent = "",
                .chdata = "",
            };
            SxeStateChange change = {
                .type = SxeStateChangeNew,
                .new_ = new_,
            };
            std::vector<SxeStateChange> state_changes = {};
            state_changes.push_back(change);

            for (Util::List<XML::AttributeRecord const> it = node->attributeList(); it; ++it) {
                std::string attr_rid = get_uuid();

                SxeNew new_ = {
                    .rid = attr_rid.c_str(),
                    .type = "attr",
                    .name = g_quark_to_string(it->key),
                    .ns = "",
                    .parent = rid.c_str(),
                    .chdata = it->value,
                };
                SxeStateChange change = {
                    .type = SxeStateChangeNew,
                    .new_ = new_,
                };
                state_changes.push_back(change);
            }

            Sxe sxe("session", "id", SxeState, {}, state_changes);

            printf("gloox %s\n", sxe.tag()->xml().c_str());
            printf("\n");
        } else if ((edel = dynamic_cast<XML::EventDel *>(e))) {
            std::cout << "EventDel" << std::endl;
            sp_repr_write_stream(edel->child, *writer, 0, false, GQuark(0), 0, 0);

            SxeStateChange change = {
                .type = SxeStateChangeRemove,
                .remove = SxeRemove {
                    .target = "coucou",
                },
            };
            std::vector<SxeStateChange> state_changes = {};
            state_changes.push_back(change);
            Sxe sxe("session", "id", SxeState, {}, state_changes);

            printf("gloox %s\n", sxe.tag()->xml().c_str());
            printf("\n");
        } else if ((echga = dynamic_cast<XML::EventChgAttr *>(e))) {
            std::cout << "EventChgAttr" << std::endl;
            printf("%s from %s to %s", g_quark_to_string(echga->key), &*(echga->oldval), &*(echga->newval));
            printf("\n");
        } else if ((echgc = dynamic_cast<XML::EventChgContent *>(e))) {
            std::cout << "EventChgContent" << std::endl;
            printf("%s to %s", &*(echgc->oldval), &*(echgc->newval));
            printf("\n");
        } else if ((echgo = dynamic_cast<XML::EventChgOrder *>(e))) {
            std::cout << "EventChgOrder" << std::endl;
        } else if ((echgn = dynamic_cast<XML::EventChgElementName *>(e))) {
            std::cout << "EventChgElementName" << std::endl;
        } else {
            std::cout << "Unknown event" << std::endl;
        }

        e = e->next;
    }
}
void XMPPObserver::notifyUndoEvent(Event *e)
{
    std::cout << "UndoEvent" << std::endl;
    this->notifyUndoCommitEvent(e);
}
void XMPPObserver::notifyRedoEvent(Event *e)
{
    std::cout << "RedoEvent" << std::endl;
    this->notifyUndoCommitEvent(e);
}
void XMPPObserver::notifyClearUndoEvent() { std::cout << "ClearUndoEvent" << std::endl; }
void XMPPObserver::notifyClearRedoEvent() { std::cout << "ClearRedoEvent" << std::endl; }

/**
    \brief  A function to allocated anything -- just an example here
    \param  module  Unused
    \return Whether the load was successful
*/
bool XMPP::load(Inkscape::Extension::Extension * /*module*/)
{
    this->obs = new XMPPObserver();
    this->obs->writer = new IO::StdWriter();
    this->enabled = false;

    // TODO: fetch these from the preferences.
    JID jid("test@linkmauve.fr");
    const char *password = "test";

    //client = std::unique_ptr<InkscapeClient>(new InkscapeClient(jid, password));
    InkscapeClient *client = new InkscapeClient(jid, password);
    bool connected = client->connect();
    printf("just attempted to connect, should be 0: %d\n", connected);

    // TODO: find a better way to integrate gloox’s fd into the main loop.
    g_timeout_add(16, &InkscapeClient::runLoop, client);
    return TRUE;
}

/**
    \brief  This actually draws the grid.
    \param  module   The effect that was called (unused)
    \param  document What should be edited.
*/
void XMPP::effect(Inkscape::Extension::Effect *module, Inkscape::UI::View::View *document,
                  Inkscape::Extension::Implementation::ImplementationDocumentCache * /*docCache*/)
{
    std::cout << (enabled ? "disabling" : "enabling") << std::endl;
    if (!this->enabled)
        document->doc()->addUndoObserver(*obs);
    else
        document->doc()->removeUndoObserver(*obs);
    this->enabled = !this->enabled;
}



}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */


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
