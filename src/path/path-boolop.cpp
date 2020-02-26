// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Boolean operations.
 *//*
 * Authors:
 * see git history
 *  Created by fred on Fri Dec 05 2003.
 *  tweaked endlessly by bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>

#include <glibmm/i18n.h>

#include <2geom/svg-path-parser.h> // to get from SVG on boolean to Geom::Path

#include "path-boolop.h"
#include "path-util.h"

#include "message-stack.h"
#include "path-chemistry.h"     // copy_object_properties()
#include "verbs.h"

#include "display/sp-canvas.h"  // Disable drawing during op

#include "helper/geom.h"        // pathv_to_linear_and_cubic_beziers()

#include "livarot/Path.h"
#include "livarot/Shape.h"

#include "object/sp-flowtext.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"

#include "svg/svg.h"

#include "xml/repr-sorting.h"

using Inkscape::DocumentUndo;

// fonctions utilitaires
bool
Ancetre(Inkscape::XML::Node *a, Inkscape::XML::Node *who)
{
    if (who == nullptr || a == nullptr)
        return false;
    if (who == a)
        return true;
    return Ancetre(a->parent(), who);
}


bool Inkscape::ObjectSet::pathUnion(const bool skip_undo) {
    BoolOpErrors result = pathBoolOp(bool_op_union, skip_undo, false, SP_VERB_SELECTION_UNION, _("Union"));
    return DONE == result;
}

bool
Inkscape::ObjectSet::pathIntersect(const bool skip_undo)
{
    BoolOpErrors result = pathBoolOp(bool_op_inters, skip_undo, false, SP_VERB_SELECTION_INTERSECT, _("Intersection"));
    return DONE == result;
}

bool
Inkscape::ObjectSet::pathDiff(const bool skip_undo)
{
    BoolOpErrors result = pathBoolOp(bool_op_diff, skip_undo, false, SP_VERB_SELECTION_DIFF, _("Difference"));
    return DONE == result;
}

bool
Inkscape::ObjectSet::pathSymDiff(const bool skip_undo)
{
    BoolOpErrors result = pathBoolOp(bool_op_symdiff, skip_undo, false, SP_VERB_SELECTION_SYMDIFF, _("Exclusion"));
    return DONE == result;
}

bool
Inkscape::ObjectSet::pathCut(const bool skip_undo)
{
    BoolOpErrors result = pathBoolOp(bool_op_cut, skip_undo, false, SP_VERB_SELECTION_CUT, _("Division"));
    return DONE == result;
}

bool
Inkscape::ObjectSet::pathSlice(const bool skip_undo)
{
    BoolOpErrors result = pathBoolOp(bool_op_slice, skip_undo, false, SP_VERB_SELECTION_SLICE, _("Cut path"));
    return DONE == result;
}

// helper for printing error messages, regardless of whether we have a GUI or not
// If desktop == NULL, errors will be shown on stderr
static void
boolop_display_error_message(SPDesktop *desktop, Glib::ustring const &msg)
{
    if (desktop) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, msg);
    } else {
        g_printerr("%s\n", msg.c_str());
    }
}

// boolean operations PathVectors A,B -> PathVector result.
// This is derived from sp_selected_path_boolop
// take the source paths from the file, do the operation, delete the originals and add the results
// fra,fra are fill_rules for PathVectors a,b
Geom::PathVector 
sp_pathvector_boolop(Geom::PathVector const &pathva, Geom::PathVector const &pathvb, bool_op bop, fill_typ fra, fill_typ frb)
{        

    // extract the livarot Paths from the source objects
    // also get the winding rule specified in the style
    int nbOriginaux = 2;
    std::vector<Path *> originaux(nbOriginaux);
    std::vector<FillRule> origWind(nbOriginaux);
    origWind[0]=fra;
    origWind[1]=frb;
    Geom::PathVector patht;
    // Livarot's outline of arcs is broken. So convert the path to linear and cubics only, for which the outline is created correctly. 
    originaux[0] = Path_for_pathvector(pathv_to_linear_and_cubic_beziers( pathva));
    originaux[1] = Path_for_pathvector(pathv_to_linear_and_cubic_beziers( pathvb));

    // some temporary instances, first
    Shape *theShapeA = new Shape;
    Shape *theShapeB = new Shape;
    Shape *theShape = new Shape;
    Path *res = new Path;
    res->SetBackData(false);
    Path::cut_position  *toCut=nullptr;
    int                  nbToCut=0;

    if ( bop == bool_op_inters || bop == bool_op_union || bop == bool_op_diff || bop == bool_op_symdiff ) {
        // true boolean op
        // get the polygons of each path, with the winding rule specified, and apply the operation iteratively
        originaux[0]->ConvertWithBackData(0.1);

        originaux[0]->Fill(theShape, 0);

        theShapeA->ConvertToShape(theShape, origWind[0]);

        originaux[1]->ConvertWithBackData(0.1);

        originaux[1]->Fill(theShape, 1);

        theShapeB->ConvertToShape(theShape, origWind[1]);
        
        theShape->Booleen(theShapeB, theShapeA, bop);

    } else if ( bop == bool_op_cut ) {
        // cuts= sort of a bastard boolean operation, thus not the axact same modus operandi
        // technically, the cut path is not necessarily a polygon (thus has no winding rule)
        // it is just uncrossed, and cleaned from duplicate edges and points
        // then it's fed to Booleen() which will uncross it against the other path
        // then comes the trick: each edge of the cut path is duplicated (one in each direction),
        // thus making a polygon. the weight of the edges of the cut are all 0, but
        // the Booleen need to invert the ones inside the source polygon (for the subsequent
        // ConvertToForme)

        // the cut path needs to have the highest pathID in the back data
        // that's how the Booleen() function knows it's an edge of the cut

        // FIXME: this gives poor results, the final paths are full of extraneous nodes. Decreasing
        // ConvertWithBackData parameter below simply increases the number of nodes, so for now I
        // left it at 1.0. Investigate replacing this by a combination of difference and
        // intersection of the same two paths. -- bb
        {
            Path* swap=originaux[0];originaux[0]=originaux[1];originaux[1]=swap;
            int   swai=origWind[0];origWind[0]=origWind[1];origWind[1]=(fill_typ)swai;
        }
        originaux[0]->ConvertWithBackData(0.1);

        originaux[0]->Fill(theShape, 0);

        theShapeA->ConvertToShape(theShape, origWind[0]);

        originaux[1]->ConvertWithBackData(0.1);

        originaux[1]->Fill(theShape, 1,false,false,false); //do not closeIfNeeded

        theShapeB->ConvertToShape(theShape, fill_justDont); // fill_justDont doesn't computes winding numbers

        // les elements arrivent en ordre inverse dans la liste
        theShape->Booleen(theShapeB, theShapeA, bool_op_cut, 1);

    } else if ( bop == bool_op_slice ) {
        // slice is not really a boolean operation
        // you just put the 2 shapes in a single polygon, uncross it
        // the points where the degree is > 2 are intersections
        // just check it's an intersection on the path you want to cut, and keep it
        // the intersections you have found are then fed to ConvertPositionsToMoveTo() which will
        // make new subpath at each one of these positions
        // inversion pour l'opration
        {
            Path* swap=originaux[0];originaux[0]=originaux[1];originaux[1]=swap;
            int   swai=origWind[0];origWind[0]=origWind[1];origWind[1]=(fill_typ)swai;
        }
        originaux[0]->ConvertWithBackData(1.0);

        originaux[0]->Fill(theShapeA, 0,false,false,false); // don't closeIfNeeded

        originaux[1]->ConvertWithBackData(1.0);

        originaux[1]->Fill(theShapeA, 1,true,false,false);// don't closeIfNeeded and just dump in the shape, don't reset it

        theShape->ConvertToShape(theShapeA, fill_justDont);

        if ( theShape->hasBackData() ) {
            // should always be the case, but ya never know
            {
                for (int i = 0; i < theShape->numberOfPoints(); i++) {
                    if ( theShape->getPoint(i).totalDegree() > 2 ) {
                        // possibly an intersection
                        // we need to check that at least one edge from the source path is incident to it
                        // before we declare it's an intersection
                        int cb = theShape->getPoint(i).incidentEdge[FIRST];
                        int   nbOrig=0;
                        int   nbOther=0;
                        int   piece=-1;
                        float t=0.0;
                        while ( cb >= 0 && cb < theShape->numberOfEdges() ) {
                            if ( theShape->ebData[cb].pathID == 0 ) {
                                // the source has an edge incident to the point, get its position on the path
                                piece=theShape->ebData[cb].pieceID;
                                if ( theShape->getEdge(cb).st == i ) {
                                    t=theShape->ebData[cb].tSt;
                                } else {
                                    t=theShape->ebData[cb].tEn;
                                }
                                nbOrig++;
                            }
                            if ( theShape->ebData[cb].pathID == 1 ) nbOther++; // the cut is incident to this point
                            cb=theShape->NextAt(i, cb);
                        }
                        if ( nbOrig > 0 && nbOther > 0 ) {
                            // point incident to both path and cut: an intersection
                            // note that you only keep one position on the source; you could have degenerate
                            // cases where the source crosses itself at this point, and you wouyld miss an intersection
                            toCut=(Path::cut_position*)realloc(toCut, (nbToCut+1)*sizeof(Path::cut_position));
                            toCut[nbToCut].piece=piece;
                            toCut[nbToCut].t=t;
                            nbToCut++;
                        }
                    }
                }
            }
            {
                // i think it's useless now
                int i = theShape->numberOfEdges() - 1;
                for (;i>=0;i--) {
                    if ( theShape->ebData[i].pathID == 1 ) {
                        theShape->SubEdge(i);
                    }
                }
            }

        }
    }

    int*    nesting=nullptr;
    int*    conts=nullptr;
    int     nbNest=0;
    // pour compenser le swap juste avant
    if ( bop == bool_op_slice ) {
//    theShape->ConvertToForme(res, nbOriginaux, originaux, true);
//    res->ConvertForcedToMoveTo();
        res->Copy(originaux[0]);
        res->ConvertPositionsToMoveTo(nbToCut, toCut); // cut where you found intersections
        free(toCut);
    } else if ( bop == bool_op_cut ) {
        // il faut appeler pour desallouer PointData (pas vital, mais bon)
        // the Booleen() function did not deallocate the point_data array in theShape, because this
        // function needs it.
        // this function uses the point_data to get the winding number of each path (ie: is a hole or not)
        // for later reconstruction in objects, you also need to extract which path is parent of holes (nesting info)
        theShape->ConvertToFormeNested(res, nbOriginaux, &originaux[0], 1, nbNest, nesting, conts);
    } else {
        theShape->ConvertToForme(res, nbOriginaux, &originaux[0]);
    }

    delete theShape;
    delete theShapeA;
    delete theShapeB;
    delete originaux[0];
    delete originaux[1];

    gchar *result_str = res->svg_dump_path();
    Geom::PathVector outres =  Geom::parse_svg_path(result_str);
    g_free(result_str);

    delete res;
    return outres;
}


// boolean operations on the desktop
// take the source paths from the file, do the operation, delete the originals and add the results
BoolOpErrors Inkscape::ObjectSet::pathBoolOp(bool_op bop, const bool skip_undo, const bool checked, const unsigned int verb, const Glib::ustring description)
{
    if (nullptr != desktop() && !checked) {
        SPDocument *doc = desktop()->getDocument();
        // don't redraw the canvas during the operation as that can remarkably slow down the progress
        desktop()->getCanvas()->_drawing_disabled = true;
        BoolOpErrors returnCode = ObjectSet::pathBoolOp(bop, true, true);
        desktop()->getCanvas()->_drawing_disabled = false;

        switch(returnCode) {
        case ERR_TOO_LESS_PATHS_1:
            boolop_display_error_message(desktop(), _("Select <b>at least 1 path</b> to perform a boolean union."));
            break;
        case ERR_TOO_LESS_PATHS_2:
            boolop_display_error_message(desktop(), _("Select <b>at least 2 paths</b> to perform a boolean operation."));
            break;
        case ERR_NO_PATHS:
            boolop_display_error_message(desktop(), _("One of the objects is <b>not a path</b>, cannot perform boolean operation."));
            break;
        case ERR_Z_ORDER:
            boolop_display_error_message(desktop(), _("Unable to determine the <b>z-order</b> of the objects selected for difference, XOR, division, or path cut."));
            break;
        case DONE_NO_PATH:
            if (!skip_undo) { 
                DocumentUndo::done(doc, SP_VERB_NONE, description);
            }
            break;
        case DONE:
            if (!skip_undo) { 
                DocumentUndo::done(doc, verb, description);
            }
            break;
        case DONE_NO_ACTION:
            // Do nothing (?)
            break;
        }
        return returnCode;
    }

    SPDocument *doc = document();
    std::vector<SPItem*> il(items().begin(), items().end());

    // allow union on a single object for the purpose of removing self overlapse (svn log, revision 13334)
    if (il.size() < 2 && bop != bool_op_union) {
        return ERR_TOO_LESS_PATHS_2;
    }
    else if (il.size() < 1) {
        return ERR_TOO_LESS_PATHS_1;
    }

    g_assert(!il.empty());

    // reverseOrderForOp marks whether the order of the list is the top->down order
    // it's only used when there are 2 objects, and for operations who need to know the
    // topmost object (differences, cuts)
    bool reverseOrderForOp = false;

    if (bop == bool_op_diff || bop == bool_op_cut || bop == bool_op_slice) {
        // check in the tree to find which element of the selection list is topmost (for 2-operand commands only)
        Inkscape::XML::Node *a = il.front()->getRepr();
        Inkscape::XML::Node *b = il.back()->getRepr();

        if (a == nullptr || b == nullptr) {
            return ERR_Z_ORDER;
        }

        if (Ancetre(a, b)) {
            // a is the parent of b, already in the proper order
        } else if (Ancetre(b, a)) {
            // reverse order
            reverseOrderForOp = true;
        } else {

            // objects are not in parent/child relationship;
            // find their lowest common ancestor
            Inkscape::XML::Node *parent = LCA(a, b);
            if (parent == nullptr) {
                return ERR_Z_ORDER;
            }

            // find the children of the LCA that lead from it to the a and b
            Inkscape::XML::Node *as = AncetreFils(a, parent);
            Inkscape::XML::Node *bs = AncetreFils(b, parent);

            // find out which comes first
            for (Inkscape::XML::Node *child = parent->firstChild(); child; child = child->next()) {
                if (child == as) {
                    /* a first, so reverse. */
                    reverseOrderForOp = true;
                    break;
                }
                if (child == bs)
                    break;
            }
        }
    }

    g_assert(!il.empty());

    // first check if all the input objects have shapes
    // otherwise bail out
    for (std::vector<SPItem*>::const_iterator l = il.begin(); l != il.end(); l++)
    {
        SPItem *item = *l;
        if (!SP_IS_SHAPE(item) && !SP_IS_TEXT(item) && !SP_IS_FLOWTEXT(item))
        {
            return ERR_NO_PATHS;
        }
    }

    // extract the livarot Paths from the source objects
    // also get the winding rule specified in the style
    int nbOriginaux = il.size();
    std::vector<Path *> originaux(nbOriginaux);
    std::vector<FillRule> origWind(nbOriginaux);
    int curOrig;
    {
        curOrig = 0;
        for (std::vector<SPItem*>::const_iterator l = il.begin(); l != il.end(); l++)
        {
            // apply live path effects prior to performing boolean operation
            if (SP_IS_LPE_ITEM(*l)) {
                SP_LPE_ITEM(*l)->removeAllPathEffects(true);
            }

            SPCSSAttr *css = sp_repr_css_attr(reinterpret_cast<SPObject *>(il[0])->getRepr(), "style");
            gchar const *val = sp_repr_css_property(css, "fill-rule", nullptr);
            if (val && strcmp(val, "nonzero") == 0) {
                origWind[curOrig]= fill_nonZero;
            } else if (val && strcmp(val, "evenodd") == 0) {
                origWind[curOrig]= fill_oddEven;
            } else {
                origWind[curOrig]= fill_nonZero;
            }

            originaux[curOrig] = Path_for_item(*l, true, true);
            if (originaux[curOrig] == nullptr || originaux[curOrig]->descr_cmd.size() <= 1)
            {
                for (int i = curOrig; i >= 0; i--) delete originaux[i];
                return DONE_NO_ACTION;
            }
            curOrig++;
        }
    }
    // reverse if needed
    // note that the selection list keeps its order
    if ( reverseOrderForOp ) {
        std::swap(originaux[0], originaux[1]);
        std::swap(origWind[0], origWind[1]);
    }

    // and work
    // some temporary instances, first
    Shape *theShapeA = new Shape;
    Shape *theShapeB = new Shape;
    Shape *theShape = new Shape;
    Path *res = new Path;
    res->SetBackData(false);
    Path::cut_position  *toCut=nullptr;
    int                  nbToCut=0;

    if ( bop == bool_op_inters || bop == bool_op_union || bop == bool_op_diff || bop == bool_op_symdiff ) {
        // true boolean op
        // get the polygons of each path, with the winding rule specified, and apply the operation iteratively
        originaux[0]->ConvertWithBackData(0.1);

        originaux[0]->Fill(theShape, 0);

        theShapeA->ConvertToShape(theShape, origWind[0]);

        curOrig = 1;
        for (std::vector<SPItem*>::const_iterator l = il.begin(); l != il.end(); l++){
            if(*l==il[0])continue;
            originaux[curOrig]->ConvertWithBackData(0.1);

            originaux[curOrig]->Fill(theShape, curOrig);

            theShapeB->ConvertToShape(theShape, origWind[curOrig]);

            /* Due to quantization of the input shape coordinates, we may end up with A or B being empty.
             * If this is a union or symdiff operation, we just use the non-empty shape as the result:
             *   A=0  =>  (0 or B) == B
             *   B=0  =>  (A or 0) == A
             *   A=0  =>  (0 xor B) == B
             *   B=0  =>  (A xor 0) == A
             * If this is an intersection operation, we just use the empty shape as the result:
             *   A=0  =>  (0 and B) == 0 == A
             *   B=0  =>  (A and 0) == 0 == B
             * If this a difference operation, and the upper shape (A) is empty, we keep B.
             * If the lower shape (B) is empty, we still keep B, as it's empty:
             *   A=0  =>  (B - 0) == B
             *   B=0  =>  (0 - A) == 0 == B
             *
             * In any case, the output from this operation is stored in shape A, so we may apply
             * the above rules simply by judicious use of swapping A and B where necessary.
             */
            bool zeroA = theShapeA->numberOfEdges() == 0;
            bool zeroB = theShapeB->numberOfEdges() == 0;
            if (zeroA || zeroB) {
                // We might need to do a swap. Apply the above rules depending on operation type.
                bool resultIsB =   ((bop == bool_op_union || bop == bool_op_symdiff) && zeroA)
                                   || ((bop == bool_op_inters) && zeroB)
                                   ||  (bop == bool_op_diff);
                if (resultIsB) {
                    // Swap A and B to use B as the result
                    Shape *swap = theShapeB;
                    theShapeB = theShapeA;
                    theShapeA = swap;
                }
            } else {
                // Just do the Boolean operation as usual
                // les elements arrivent en ordre inverse dans la liste
                theShape->Booleen(theShapeB, theShapeA, bop);
                Shape *swap = theShape;
                theShape = theShapeA;
                theShapeA = swap;
            }
            curOrig++;
        }

        {
            Shape *swap = theShape;
            theShape = theShapeA;
            theShapeA = swap;
        }

    } else if ( bop == bool_op_cut ) {
        // cuts= sort of a bastard boolean operation, thus not the axact same modus operandi
        // technically, the cut path is not necessarily a polygon (thus has no winding rule)
        // it is just uncrossed, and cleaned from duplicate edges and points
        // then it's fed to Booleen() which will uncross it against the other path
        // then comes the trick: each edge of the cut path is duplicated (one in each direction),
        // thus making a polygon. the weight of the edges of the cut are all 0, but
        // the Booleen need to invert the ones inside the source polygon (for the subsequent
        // ConvertToForme)

        // the cut path needs to have the highest pathID in the back data
        // that's how the Booleen() function knows it's an edge of the cut

        // FIXME: this gives poor results, the final paths are full of extraneous nodes. Decreasing
        // ConvertWithBackData parameter below simply increases the number of nodes, so for now I
        // left it at 1.0. Investigate replacing this by a combination of difference and
        // intersection of the same two paths. -- bb
        {
            Path* swap=originaux[0];originaux[0]=originaux[1];originaux[1]=swap;
            int   swai=origWind[0];origWind[0]=origWind[1];origWind[1]=(fill_typ)swai;
        }
        originaux[0]->ConvertWithBackData(1.0);

        originaux[0]->Fill(theShape, 0);

        theShapeA->ConvertToShape(theShape, origWind[0]);

        originaux[1]->ConvertWithBackData(1.0);

        if ((originaux[1]->pts.size() == 2) && originaux[1]->pts[0].isMoveTo && !originaux[1]->pts[1].isMoveTo)
            originaux[1]->Fill(theShape, 1,false,true,false); // see LP Bug 177956
        else
            originaux[1]->Fill(theShape, 1,false,false,false); //do not closeIfNeeded

        theShapeB->ConvertToShape(theShape, fill_justDont); // fill_justDont doesn't computes winding numbers

        // les elements arrivent en ordre inverse dans la liste
        theShape->Booleen(theShapeB, theShapeA, bool_op_cut, 1);

    } else if ( bop == bool_op_slice ) {
        // slice is not really a boolean operation
        // you just put the 2 shapes in a single polygon, uncross it
        // the points where the degree is > 2 are intersections
        // just check it's an intersection on the path you want to cut, and keep it
        // the intersections you have found are then fed to ConvertPositionsToMoveTo() which will
        // make new subpath at each one of these positions
        // inversion pour l'opration
        {
            Path* swap=originaux[0];originaux[0]=originaux[1];originaux[1]=swap;
            int   swai=origWind[0];origWind[0]=origWind[1];origWind[1]=(fill_typ)swai;
        }
        originaux[0]->ConvertWithBackData(1.0);

        originaux[0]->Fill(theShapeA, 0,false,false,false); // don't closeIfNeeded

        originaux[1]->ConvertWithBackData(1.0);

        originaux[1]->Fill(theShapeA, 1,true,false,false);// don't closeIfNeeded and just dump in the shape, don't reset it

        theShape->ConvertToShape(theShapeA, fill_justDont);

        if ( theShape->hasBackData() ) {
            // should always be the case, but ya never know
            {
                for (int i = 0; i < theShape->numberOfPoints(); i++) {
                    if ( theShape->getPoint(i).totalDegree() > 2 ) {
                        // possibly an intersection
                        // we need to check that at least one edge from the source path is incident to it
                        // before we declare it's an intersection
                        int cb = theShape->getPoint(i).incidentEdge[FIRST];
                        int   nbOrig=0;
                        int   nbOther=0;
                        int   piece=-1;
                        float t=0.0;
                        while ( cb >= 0 && cb < theShape->numberOfEdges() ) {
                            if ( theShape->ebData[cb].pathID == 0 ) {
                                // the source has an edge incident to the point, get its position on the path
                                piece=theShape->ebData[cb].pieceID;
                                if ( theShape->getEdge(cb).st == i ) {
                                    t=theShape->ebData[cb].tSt;
                                } else {
                                    t=theShape->ebData[cb].tEn;
                                }
                                nbOrig++;
                            }
                            if ( theShape->ebData[cb].pathID == 1 ) nbOther++; // the cut is incident to this point
                            cb=theShape->NextAt(i, cb);
                        }
                        if ( nbOrig > 0 && nbOther > 0 ) {
                            // point incident to both path and cut: an intersection
                            // note that you only keep one position on the source; you could have degenerate
                            // cases where the source crosses itself at this point, and you wouyld miss an intersection
                            toCut=(Path::cut_position*)realloc(toCut, (nbToCut+1)*sizeof(Path::cut_position));
                            toCut[nbToCut].piece=piece;
                            toCut[nbToCut].t=t;
                            nbToCut++;
                        }
                    }
                }
            }
            {
                // i think it's useless now
                int i = theShape->numberOfEdges() - 1;
                for (;i>=0;i--) {
                    if ( theShape->ebData[i].pathID == 1 ) {
                        theShape->SubEdge(i);
                    }
                }
            }

        }
    }

    int*    nesting=nullptr;
    int*    conts=nullptr;
    int     nbNest=0;
    // pour compenser le swap juste avant
    if ( bop == bool_op_slice ) {
//    theShape->ConvertToForme(res, nbOriginaux, originaux, true);
//    res->ConvertForcedToMoveTo();
        res->Copy(originaux[0]);
        res->ConvertPositionsToMoveTo(nbToCut, toCut); // cut where you found intersections
        free(toCut);
    } else if ( bop == bool_op_cut ) {
        // il faut appeler pour desallouer PointData (pas vital, mais bon)
        // the Booleen() function did not deallocate the point_data array in theShape, because this
        // function needs it.
        // this function uses the point_data to get the winding number of each path (ie: is a hole or not)
        // for later reconstruction in objects, you also need to extract which path is parent of holes (nesting info)
        theShape->ConvertToFormeNested(res, nbOriginaux, &originaux[0], 1, nbNest, nesting, conts);
    } else {
        theShape->ConvertToForme(res, nbOriginaux, &originaux[0]);
    }

    delete theShape;
    delete theShapeA;
    delete theShapeB;
    for (int i = 0; i < nbOriginaux; i++)  delete originaux[i];

    if (res->descr_cmd.size() <= 1)
    {
        // only one command, presumably a moveto: it isn't a path
        for (std::vector<SPItem*>::const_iterator l = il.begin(); l != il.end(); l++){
            (*l)->deleteObject();
        }
        clear();

        delete res;
        return DONE_NO_PATH;
    }

    // get the source path object
    SPObject *source;
    if ( bop == bool_op_diff || bop == bool_op_cut || bop == bool_op_slice ) {
        if (reverseOrderForOp) {
            source = il[0];
        } else {
            source = il.back();
        }
    } else {
        // find out the bottom object
        std::vector<Inkscape::XML::Node*> sorted(xmlNodes().begin(), xmlNodes().end());

        sort(sorted.begin(),sorted.end(),sp_repr_compare_position_bool);

        source = doc->getObjectByRepr(sorted.front());
    }

    // adjust style properties that depend on a possible transform in the source object in order
    // to get a correct style attribute for the new path
    SPItem* item_source = SP_ITEM(source);
    Geom::Affine i2doc(item_source->i2doc_affine());
    item_source->adjust_stroke(i2doc.descrim());
    item_source->adjust_pattern(i2doc);
    item_source->adjust_gradient(i2doc);

    Inkscape::XML::Node *repr_source = source->getRepr();

    // remember important aspects of the source path, to be restored
    gint pos = repr_source->position();
    Inkscape::XML::Node *parent = repr_source->parent();
    // remove source paths
    clear();
    for (std::vector<SPItem*>::const_iterator l = il.begin(); l != il.end(); l++){
        if ((*l) != item_source) {
            // delete the object for real, so that its clones can take appropriate action
            (*l)->deleteObject();
        }
    }

    // premultiply by the inverse of parent's repr
    SPItem *parent_item = SP_ITEM(doc->getObjectByRepr(parent));
    Geom::Affine local (parent_item->i2doc_affine());
    gchar *transform = sp_svg_transform_write(local.inverse());

    // now that we have the result, add it on the canvas
    if ( bop == bool_op_cut || bop == bool_op_slice ) {
        int    nbRP=0;
        Path** resPath;
        if ( bop == bool_op_slice ) {
            // there are moveto's at each intersection, but it's still one unique path
            // so break it down and add each subpath independently
            // we could call break_apart to do this, but while we have the description...
            resPath=res->SubPaths(nbRP, false);
        } else {
            // cut operation is a bit wicked: you need to keep holes
            // that's why you needed the nesting
            // ConvertToFormeNested() dumped all the subpath in a single Path "res", so we need
            // to get the path for each part of the polygon. that's why you need the nesting info:
            // to know in which subpath to add a subpath
            resPath=res->SubPathsWithNesting(nbRP, true, nbNest, nesting, conts);

            // cleaning
            if ( conts ) free(conts);
            if ( nesting ) free(nesting);
        }

        // add all the pieces resulting from cut or slice
        std::vector <Inkscape::XML::Node*> selection;
        for (int i=0;i<nbRP;i++) {
            gchar *d = resPath[i]->svg_dump_path();

            Inkscape::XML::Document *xml_doc = doc->getReprDoc();
            Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");

            Inkscape::copy_object_properties(repr, repr_source);

            // Delete source on last iteration (after we don't need repr_source anymore). As a consequence, the last
            // item will inherit the original's id.
            if (i + 1 == nbRP) {
                item_source->deleteObject(false);
            }

            repr->setAttribute("d", d);
            g_free(d);

            // for slice, remove fill
            if (bop == bool_op_slice) {
                SPCSSAttr *css;

                css = sp_repr_css_attr_new();
                sp_repr_css_set_property(css, "fill", "none");

                sp_repr_css_change(repr, css, "style");

                sp_repr_css_attr_unref(css);
            }

            repr->setAttribute("transform", transform);

            // add the new repr to the parent
            // move to the saved position
            parent->addChildAtPos(repr, pos);

            selection.push_back(repr);
            Inkscape::GC::release(repr);

            delete resPath[i];
        }
        setReprList(selection);
        if ( resPath ) free(resPath);

    } else {
        gchar *d = res->svg_dump_path();

        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
        Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");

        Inkscape::copy_object_properties(repr, repr_source);

        // delete it so that its clones don't get alerted; this object will be restored shortly, with the same id
        item_source->deleteObject(false);

        repr->setAttribute("d", d);
        g_free(d);

        repr->setAttribute("transform", transform);

        parent->addChildAtPos(repr, pos);

        set(repr);
        Inkscape::GC::release(repr);
    }

    g_free(transform);

    delete res;

    return DONE;
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
