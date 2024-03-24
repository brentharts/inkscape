// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * PathVectorNodeSatellites a class to manage nodesatellites -per node extra data- in a pathvector
 *//*
 * Authors: see git history
 * Jabiertxof
 * Nathan Hurst
 * Johan Engelen
 * Josh Andler
 * suv
 * Mc-
 * Liam P. White
 * Krzysztof Kosiński
 * This code is in public domain
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <helper/geom-pathvector_nodesatellites.h>
#include <helper/geom.h>

#include "util/units.h"

Geom::PathVector PathVectorNodeSatellites::getPathVector() const
{
    return _pathvector;
}

void PathVectorNodeSatellites::setPathVector(Geom::PathVector pathv)
{
    _pathvector = pathv;
}

NodeSatellites PathVectorNodeSatellites::getNodeSatellites()
{
    return _nodesatellites;
}

void PathVectorNodeSatellites::setNodeSatellites(NodeSatellites nodesatellites)
{
    _nodesatellites = nodesatellites;
}

size_t PathVectorNodeSatellites::getTotalNodeSatellites()
{
    size_t counter = 0;
    for (auto &_nodesatellite : _nodesatellites) {
        counter += _nodesatellite.size();
    }
    return counter;
}

std::pair<size_t, size_t> PathVectorNodeSatellites::getIndexData(size_t index)
{
    size_t counter = 0;
    for (size_t i = 0; i < _nodesatellites.size(); ++i) {
        for (size_t j = 0; j < _nodesatellites[i].size(); ++j) {
            if (index == counter) {
                return std::make_pair(i,j);
            }
            counter++;
        }
    }
    return std::make_pair(0,0);
}

void PathVectorNodeSatellites::setSelected(std::vector<size_t> selected)
{
    size_t counter = 0;
    for (auto &_nodesatellite : _nodesatellites) {
        for (auto &j : _nodesatellite) {
            if (find (selected.begin(), selected.end(), counter) != selected.end()) {
                j.setSelected(true);
            } else {
                j.setSelected(false);
            }
            counter++;
        }
    }
}

void PathVectorNodeSatellites::updateSteps(size_t steps, bool apply_no_radius, bool apply_with_radius,
                                           bool only_selected)
{
    for (auto &_nodesatellite : _nodesatellites) {
        for (auto &j : _nodesatellite) {
            if ((!apply_no_radius && j.amount == 0) ||
                (!apply_with_radius && j.amount != 0)) 
            {
                continue;
            }
            if (only_selected) {
                if (j.selected) {
                    j.steps = steps;
                }
            } else {
                j.steps = steps;
            }
        }
    }
}

void PathVectorNodeSatellites::updateAmount(double radius, bool apply_no_radius, bool apply_with_radius,
                                            bool only_selected, bool use_knot_distance, bool flexible)
{
    double power = 0;
    if (!flexible) {
        power = radius;
    } else {
        power = radius / 100;
    }
    for (size_t i = 0; i < _nodesatellites.size(); ++i) {
        for (size_t j = 0; j < _nodesatellites[i].size(); ++j) {
            std::optional<size_t> previous_index = std::nullopt;
            if (j == 0 && _pathvector[i].closed()) {
                previous_index = count_path_nodes(_pathvector[i]) - 1;
            } else if (!_pathvector[i].closed() || j != 0) {
                previous_index = j - 1;
            }
            if (!_pathvector[i].closed() && j == 0) {
                _nodesatellites[i][j].amount = 0;
                continue;
            }
            if (count_path_nodes(_pathvector[i]) == j) {
                continue;
            }
            if ((!apply_no_radius && _nodesatellites[i][j].amount == 0) ||
                (!apply_with_radius && _nodesatellites[i][j].amount != 0)) {
                continue;
            }

            if (_nodesatellites[i][j].selected || !only_selected) {
                if (!use_knot_distance && !flexible) {
                    if (previous_index) {
                        _nodesatellites[i][j].amount =
                            _nodesatellites[i][j].radToLen(power, _pathvector[i][*previous_index], _pathvector[i][j]);
                        if (power && !_nodesatellites[i][j].amount) {
                            g_warning("Seems a too high radius value");
                        }
                    } else {
                        _nodesatellites[i][j].amount = 0.0;
                    }
                } else {
                    _nodesatellites[i][j].amount = power;
                }
            }
        }
    }
}

void PathVectorNodeSatellites::convertUnit(Glib::ustring in, Glib::ustring to, bool apply_no_radius,
                                           bool apply_with_radius)
{
    for (size_t i = 0; i < _nodesatellites.size(); ++i) {
        for (size_t j = 0; j < _nodesatellites[i].size(); ++j) {
            if (!_pathvector[i].closed() && j == 0) {
                _nodesatellites[i][j].amount = 0;
                continue;
            }
            if (count_path_nodes(_pathvector[i]) == j) {
                continue;
            }
            if ((!apply_no_radius && _nodesatellites[i][j].amount == 0) ||
                (!apply_with_radius && _nodesatellites[i][j].amount != 0)) {
                continue;
            }
            _nodesatellites[i][j].amount =
                Inkscape::Util::Quantity::convert(_nodesatellites[i][j].amount, in.c_str(), to.c_str());
        }
    }
}

void PathVectorNodeSatellites::updateNodeSatelliteType(NodeSatelliteType nodesatellitetype, bool apply_no_radius,
                                                       bool apply_with_radius, bool only_selected)
{
    for (size_t i = 0; i < _nodesatellites.size(); ++i) {
        for (size_t j = 0; j < _nodesatellites[i].size(); ++j) {
            if ((!apply_no_radius && _nodesatellites[i][j].amount == 0) ||
                (!apply_with_radius && _nodesatellites[i][j].amount != 0)) {
                continue;
            }
            if (count_path_nodes(_pathvector[i]) == j) {
                if (!only_selected) {
                    _nodesatellites[i][j].nodesatellite_type = nodesatellitetype;
                }
                continue;
            }
            if (only_selected) {
                if (_nodesatellites[i][j].selected) {
                    _nodesatellites[i][j].nodesatellite_type = nodesatellitetype;
                }
            } else {
                _nodesatellites[i][j].nodesatellite_type = nodesatellitetype;
            }
        }
    }
}

std::optional<NodeSatellite> PathVectorNodeSatellites::findNearSatelite(Geom::Point point, double precission)
{
    size_t npaths = _pathvector.size();
    for (size_t i = 0; i < npaths; ++i) {
        size_t count = count_path_nodes(_pathvector[i]);
        for (size_t j = 0; j < count; ++j) {
            if (i < _nodesatellites.size() && j < _nodesatellites[i].size()) {
                if (Geom::are_near(point, _pathvector[i][j].initialPoint(), precission)) {
                    return std::make_optional(_nodesatellites[i][j]);
                }
            }
        }
    }
    return std::nullopt;
}

void PathVectorNodeSatellites::adjustForNewPath(Geom::PathVector const pathv, double precission)
{
    NodeSatellites nodesatellites;
    // TODO issue for nodes in same place
    size_t npaths = pathv.size();
    NodeSatellite ns = _nodesatellite;
    if (_nodesatellites.size() && _nodesatellites[0].size()) {
        ns.setNodeSatellitesType(_nodesatellites[0][0].getNodeSatellitesTypeGchar());
    }
    for (size_t i = 0; i < npaths; ++i) {
        std::vector<NodeSatellite> path_nodesatellites;
        size_t count = count_path_nodes(pathv[i]);
        for (size_t j = 0; j < count; ++j) {
            path_nodesatellites.push_back(findNearSatelite(pathv[i][j].initialPoint()).value_or(ns));
        }
        nodesatellites.push_back(path_nodesatellites);
    }
    setPathVector(pathv);
    setNodeSatellites(nodesatellites);
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
