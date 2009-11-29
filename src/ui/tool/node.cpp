/** @file
 * Editable node - implementation
 */
/* Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <iostream>
#include <stdexcept>
#include <boost/utility.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <2geom/transforms.h>
#include "ui/tool/event-utils.h"
#include "ui/tool/node.h"
#include "display/sp-ctrlline.h"
#include "display/sp-canvas.h"
#include "display/sp-canvas-util.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "preferences.h"
#include "sp-metrics.h"
#include "sp-namedview.h"

namespace Inkscape {
namespace UI {   

static SelectableControlPoint::ColorSet node_colors = {
    {
        {0xbfbfbf00, 0x000000ff}, // normal fill, stroke
        {0xff000000, 0x000000ff}, // mouseover fill, stroke
        {0xff000000, 0x000000ff}  // clicked fill, stroke
    },
    {0x0000ffff, 0x000000ff}, // normal fill, stroke when selected
    {0xff000000, 0x000000ff}, // mouseover fill, stroke when selected
    {0xff000000, 0x000000ff}  // clicked fill, stroke when selected
};

static ControlPoint::ColorSet handle_colors = {
    {0xffffffff, 0x000000ff}, // normal fill, stroke
    {0xff000000, 0x000000ff}, // mouseover fill, stroke
    {0xff000000, 0x000000ff}  // clicked fill, stroke
};

std::ostream &operator<<(std::ostream &out, NodeType type)
{
    switch(type) {
    case NODE_CUSP: out << 'c'; break;
    case NODE_SMOOTH: out << 's'; break;
    case NODE_AUTO: out << 'a'; break;
    case NODE_SYMMETRIC: out << 'z'; break;
    default: out << 'b'; break;
    }
    return out;
}

/** Computes an unit vector of the direction from first to second control point */
static Geom::Point direction(Geom::Point const &first, Geom::Point const &second) {
    return Geom::unit_vector(second - first);
}

/**
 * @class Handle
 * Represents a control point of a cubic Bezier curve in a path.
 */

double Handle::_saved_length = 0.0;
bool Handle::_drag_out = false;

Handle::Handle(NodeSharedData const &data, Geom::Point const &initial_pos, Node *parent)
    : ControlPoint(data.desktop, initial_pos, Gtk::ANCHOR_CENTER, SP_CTRL_SHAPE_CIRCLE, 7.0,
        &handle_colors, data.handle_group)
    , _parent(parent)
    , _degenerate(true)
{
    _cset = &handle_colors;
    _handle_line = sp_canvas_item_new(data.handle_line_group, SP_TYPE_CTRLLINE, NULL);
    setVisible(false);
    signal_grabbed.connect(
        sigc::bind_return(
            sigc::hide(
                sigc::mem_fun(*this, &Handle::_grabbedHandler)),
            false));
    signal_dragged.connect(
        sigc::hide<0>(
            sigc::mem_fun(*this, &Handle::_draggedHandler)));
    signal_ungrabbed.connect(
        sigc::hide(sigc::mem_fun(*this, &Handle::_ungrabbedHandler)));
}
Handle::~Handle()
{
    sp_canvas_item_hide(_handle_line);
    gtk_object_destroy(GTK_OBJECT(_handle_line));
}

void Handle::setVisible(bool v)
{
    ControlPoint::setVisible(v);
    if (v) sp_canvas_item_show(_handle_line);
    else sp_canvas_item_hide(_handle_line);
}

void Handle::move(Geom::Point const &new_pos)
{
    Handle *other, *towards, *towards_second;
    Node *node_towards; // node in direction of this handle
    Node *node_away; // node in the opposite direction
    if (this == &_parent->_front) {
        other = &_parent->_back;
        node_towards = _parent->_next();
        node_away = _parent->_prev();
        towards = node_towards ? &node_towards->_back : 0;
        towards_second = node_towards ? &node_towards->_front : 0;
    } else {
        other = &_parent->_front;
        node_towards = _parent->_prev();
        node_away = _parent->_next();
        towards = node_towards ? &node_towards->_front : 0;
        towards_second = node_towards ? &node_towards->_back : 0;
    }

    if (Geom::are_near(new_pos, _parent->position())) {
        // The handle becomes degenerate. If the segment between it and the node
        // in its direction becomes linear and there are smooth nodes
        // at its ends, make their handles colinear with the segment
        if (towards && towards->isDegenerate()) {
            if (node_towards->type() == NODE_SMOOTH) {
                towards_second->setDirection(*_parent, *node_towards);
            }
            if (_parent->type() == NODE_SMOOTH) {
                other->setDirection(*node_towards, *_parent);
            }
        }
        setPosition(new_pos);
        return;
    }

    if (_parent->type() == NODE_SMOOTH && Node::_is_line_segment(_parent, node_away)) {
        // restrict movement to the line joining the nodes
        Geom::Point direction = _parent->position() - node_away->position();
        Geom::Point delta = new_pos - _parent->position();
        // project the relative position on the direction line
        Geom::Point new_delta = (Geom::dot(delta, direction)
            / Geom::L2sq(direction)) * direction;
        setRelativePos(new_delta);
        return;
    }

    switch (_parent->type()) {
    case NODE_AUTO:
        _parent->setType(NODE_SMOOTH, false);
        // fall through - auto nodes degrade into smooth nodes
    case NODE_SMOOTH: {
        /* for smooth nodes, we need to rotate the other handle so that it's colinear
         * with the dragged one while conserving length. */
        other->setDirection(new_pos, *_parent);
        } break;
    case NODE_SYMMETRIC:
        // for symmetric nodes, place the other handle on the opposite side
        other->setRelativePos(-(new_pos - _parent->position()));
        break;
    default: break;
    }

    setPosition(new_pos);
}

void Handle::setPosition(Geom::Point const &p)
{
    ControlPoint::setPosition(p);
    sp_ctrlline_set_coords(SP_CTRLLINE(_handle_line), _parent->position(), position());

    // update degeneration info and visibility
    if (Geom::are_near(position(), _parent->position()))
        _degenerate = true;
    else _degenerate = false;
    if (_parent->_handles_shown && _parent->visible() && !_degenerate) {
        setVisible(true);
    } else {
        setVisible(false);
    }
    // If both handles become degenerate, convert to parent cusp node
    if (_parent->isDegenerate()) {
        _parent->setType(NODE_CUSP, false);
    }
}

void Handle::setLength(double len)
{
    if (isDegenerate()) return;
    Geom::Point dir = Geom::unit_vector(relativePos());
    setRelativePos(dir * len);
}

void Handle::retract()
{
    setPosition(_parent->position());
}

void Handle::setDirection(Geom::Point const &from, Geom::Point const &to)
{
    setDirection(to - from);
}

void Handle::setDirection(Geom::Point const &dir)
{
    Geom::Point unitdir = Geom::unit_vector(dir);
    setRelativePos(unitdir * length());
}

char const *Handle::handle_type_to_localized_string(NodeType type)
{
    switch(type) {
    case NODE_CUSP: return _("Cusp node handle");
    case NODE_SMOOTH: return _("Smooth node handle");
    case NODE_SYMMETRIC: return _("Symmetric node handle");
    case NODE_AUTO: return _("Auto-smooth node handle");
    default: return "";
    }
}

void Handle::_grabbedHandler()
{
    _saved_length = _drag_out ? 0 : length();
}

void Handle::_draggedHandler(Geom::Point &new_pos, GdkEventMotion *event)
{
    Geom::Point parent_pos = _parent->position();
    // with Alt, preserve length
    if (held_alt(*event)) {
        new_pos = parent_pos + Geom::unit_vector(new_pos - parent_pos) * _saved_length;
    }
    // with Ctrl, constrain to M_PI/rotationsnapsperpi increments.
    if (held_control(*event)) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        int snaps = 2 * prefs->getIntLimited("/options/rotationsnapsperpi/value", 12, 1, 1000);
        Geom::Point origin = _last_drag_origin();
        Geom::Point rel_origin = origin - parent_pos;
        new_pos = parent_pos + Geom::constrain_angle(Geom::Point(0,0), new_pos - parent_pos, snaps,
            _drag_out ? Geom::Point(1,0) : Geom::unit_vector(rel_origin));
    }
    signal_update.emit();
}

void Handle::_ungrabbedHandler()
{
    // hide the handle if it's less than dragtolerance away from the node
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int drag_tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);
    
    Geom::Point dist = _desktop->d2w(_parent->position()) - _desktop->d2w(position());
    if (dist.length() <= drag_tolerance) {
        move(_parent->position());
    }
    _drag_out = false;
}

static double snap_increment_degrees() {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int snaps = prefs->getIntLimited("/options/rotationsnapsperpi/value", 12, 1, 1000);
    return 180.0 / snaps;
}

Glib::ustring Handle::_getTip(unsigned state)
{
    if (state_held_alt(state)) {
        if (state_held_control(state)) {
            return format_tip(C_("Path handle tip",
                "<b>Ctrl+Alt</b>: preserve length and snap rotation angle to %f° increments"),
                snap_increment_degrees());
        } else {
            return C_("Path handle tip",
                "<b>Alt:</b> preserve handle length while dragging");
        }
    } else {
        if (state_held_control(state)) {
            return format_tip(C_("Path handle tip",
                "<b>Ctrl:</b> snap rotation angle to %f° increments, click to retract"),
                snap_increment_degrees());
        }
    }
    switch (_parent->type()) {
    case NODE_AUTO:
        return C_("Path handle tip",
            "<b>Auto node handle:</b> drag to convert to smooth node");
    default:
        return format_tip(C_("Path handle tip", "<b>%s:</b> drag to shape the curve"),
            handle_type_to_localized_string(_parent->type()));
    }
}

Glib::ustring Handle::_getDragTip(GdkEventMotion *event)
{
    Geom::Point dist = position() - _last_drag_origin();
    // report angle in mathematical convention
    double angle = Geom::angle_between(Geom::Point(-1,0), position() - _parent->position());
    angle += M_PI; // angle is (-M_PI...M_PI] - offset by +pi and scale to 0...360
    angle *= 360.0 / (2 * M_PI);
    GString *x = SP_PX_TO_METRIC_STRING(dist[Geom::X], _desktop->namedview->getDefaultMetric());
    GString *y = SP_PX_TO_METRIC_STRING(dist[Geom::Y], _desktop->namedview->getDefaultMetric());
    GString *len = SP_PX_TO_METRIC_STRING(length(), _desktop->namedview->getDefaultMetric());
    Glib::ustring ret = format_tip(C_("Path handle tip",
        "Move by %s, %s; angle %.2f°, length %s"), x->str, y->str, angle, len->str);
    g_string_free(x, TRUE);
    g_string_free(y, TRUE);
    g_string_free(len, TRUE);
    return ret;
}

/**
 * @class Node
 * Represents a curve endpoint in an editable path.
 */

Node::Node(NodeSharedData const &data, Geom::Point const &initial_pos)
    : SelectableControlPoint(data.desktop, initial_pos, Gtk::ANCHOR_CENTER,
        SP_CTRL_SHAPE_DIAMOND, 9.0, *data.selection, &node_colors, data.node_group)
    , _front(data, initial_pos, this)
    , _back(data, initial_pos, this)
    , _type(NODE_CUSP)
    , _handles_shown(false)
{
    // NOTE we do not set type here, because the handles are still degenerate
    // connect to own grabbed signal - dragging out handles
    signal_grabbed.connect(
        sigc::mem_fun(*this, &Node::_grabbedHandler));
    signal_dragged.connect( sigc::hide<0>(
        sigc::mem_fun(*this, &Node::_draggedHandler)));
}

// NOTE: not using iterators won't make this much quicker because iterators can be 100% inlined.
Node *Node::_next()
{
    NodeList::iterator n = NodeList::get_iterator(this).next();
    if (n) return n.ptr();
    return NULL;
}
Node *Node::_prev()
{
    NodeList::iterator p = NodeList::get_iterator(this).prev();
    if (p) return p.ptr();
    return NULL;
}

void Node::move(Geom::Point const &new_pos)
{
    // move handles when the node moves.
    Geom::Point old_pos = position();
    Geom::Point delta = new_pos - position();
    setPosition(new_pos);
    _front.setPosition(_front.position() + delta);
    _back.setPosition(_back.position() + delta);

    // if the node has a smooth handle after a line segment, it should be kept colinear
    // with the segment
    _fixNeighbors(old_pos, new_pos);
}

void Node::transform(Geom::Matrix const &m)
{
    Geom::Point old_pos = position();
    setPosition(position() * m);
    _front.setPosition(_front.position() * m);
    _back.setPosition(_back.position() * m);

    /* Affine transforms keep handle invariants for smooth and symmetric nodes,
     * but smooth nodes at ends of linear segments and auto nodes need special treatment */
    _fixNeighbors(old_pos, position());
}

Geom::Rect Node::bounds()
{
    Geom::Rect b(position(), position());
    b.expandTo(_front.position());
    b.expandTo(_back.position());
    return b;
}

void Node::_fixNeighbors(Geom::Point const &old_pos, Geom::Point const &new_pos)
{
    /* This method restores handle invariants for neighboring nodes,
     * and invariants that are based on positions of those nodes for this one. */
    
    /* Fix auto handles */
    if (_type == NODE_AUTO) _updateAutoHandles();
    if (old_pos != new_pos) {
        if (_next() && _next()->_type == NODE_AUTO) _next()->_updateAutoHandles();
        if (_prev() && _prev()->_type == NODE_AUTO) _prev()->_updateAutoHandles();
    }
    
    /* Fix smooth handles at the ends of linear segments.
     * Rotate the appropriate handle to be colinear with the segment.
     * If there is a smooth node at the other end of the segment, rotate it too. */
    Handle *handle, *other_handle;
    Node *other;
    if (_is_line_segment(this, _next())) {
        handle = &_back;
        other = _next();
        other_handle = &_next()->_front;
    } else if (_is_line_segment(_prev(), this)) {
        handle = &_front;
        other = _prev();
        other_handle = &_prev()->_back;
    } else return;

    if (_type == NODE_SMOOTH && !handle->isDegenerate()) {
        handle->setDirection(other->position(), new_pos);
        /*Geom::Point handle_delta = handle->position() - position();
        Geom::Point new_delta = Geom::unit_vector(new_direction) * handle_delta.length();
        handle->setPosition(position() + new_delta);*/
    }
    // also update the handle on the other end of the segment
    if (other->_type == NODE_SMOOTH && !other_handle->isDegenerate()) {
        other_handle->setDirection(new_pos, other->position());
        /*
        Geom::Point handle_delta2 = other_handle->position() - other->position();
        Geom::Point new_delta2 = Geom::unit_vector(new_direction) * handle_delta2.length();
        other_handle->setPosition(other->position() + new_delta2);*/
    }
}

void Node::_updateAutoHandles()
{
    // Recompute the position of automatic handles.
    if (!_prev() || !_next()) {
        _front.retract();
        _back.retract();
        return;
    }
    // TODO describe in detail what the code below does
    Geom::Point vec_next = _next()->position() - position();
    Geom::Point vec_prev = _prev()->position() - position();
    double len_next = vec_next.length(), len_prev = vec_prev.length();
    if (len_next > 0 && len_prev > 0) {
        Geom::Point dir = Geom::unit_vector((len_prev / len_next) * vec_next - vec_prev);
        _back.setRelativePos(-dir * (len_prev / 3));
        _front.setRelativePos(dir * (len_next / 3));
    } else {
        _front.retract();
        _back.retract();
    }
}

void Node::showHandles(bool v)
{
    _handles_shown = v;
    if (!_front.isDegenerate()) _front.setVisible(v);
    if (!_back.isDegenerate()) _back.setVisible(v);
}

/** Sets the node type and optionally restores the invariants associated with the given type.
 * @param type The type to set
 * @param update_handles Whether to restore invariants associated with the given type.
 *                       Passing false is useful e.g. wen initially creating the path,
 *                       and when making cusp nodes during some node algorithms.
 *                       Pass true when used in response to an UI node type button.
 */
void Node::setType(NodeType type, bool update_handles)
{
    if (type == NODE_PICK_BEST) {
        pickBestType();
        updateState(); // The size of the control might have changed
        return;
    }

    // if update_handles is true, adjust handle positions to match the node type
    // handle degenerate handles appropriately
    if (update_handles) {
        switch (type) {
        case NODE_CUSP:
            // if the existing type is also NODE_CUSP, retract handles
            if (_type == NODE_CUSP) {
                _front.retract();
                _back.retract();
            }
            break;
        case NODE_AUTO:
            // auto handles make no sense for endnodes
            if (isEndNode()) return;
            _updateAutoHandles();
            break;
        case NODE_SMOOTH: {
            // rotate handles to be colinear
            // for degenerate nodes set positions like auto handles
            bool prev_line = _is_line_segment(_prev(), this);
            bool next_line = _is_line_segment(this, _next());
            if (isDegenerate()) {
                _updateAutoHandles();
            } else if (_front.isDegenerate()) {
                // if the front handle is degenerate and this...next is a line segment,
                // make back colinear; otherwise pull out the other handle
                // to 1/3 of distance to prev
                if (next_line) {
                    _back.setDirection(*_next(), *this);
                } else if (_prev()) {
                    Geom::Point dir = direction(_back, *this);
                    _front.setRelativePos((_prev()->position() - position()).length() / 3 * dir);
                }
            } else if (_back.isDegenerate()) {
                if (prev_line) {
                    _front.setDirection(*_prev(), *this);
                } else if (_next()) {
                    Geom::Point dir = direction(_front, *this);
                    _back.setRelativePos((_next()->position() - position()).length() / 3 * dir);
                }
            } else {
                // both handles are extended. make colinear while keeping length
                // first make back colinear with the vector front ---> back,
                // then make front colinear with back ---> node
                // (not back ---> front because back's position was changed in the first call)
                _back.setDirection(_front, _back);
                _front.setDirection(_back, *this);
            }
            } break;
        case NODE_SYMMETRIC:
            if (isEndNode()) return; // symmetric handles make no sense for endnodes
            if (isDegenerate()) {
                // similar to auto handles but set the same length for both
                Geom::Point vec_next = _next()->position() - position();
                Geom::Point vec_prev = _prev()->position() - position();
                double len_next = vec_next.length(), len_prev = vec_prev.length();
                double len = (len_next + len_prev) / 6; // take 1/3 of average
                if (len == 0) return;
                
                Geom::Point dir = Geom::unit_vector((len_prev / len_next) * vec_next - vec_prev);
                _back.setRelativePos(-dir * len);
                _front.setRelativePos(dir * len);
            } else {
                // Both handles are extended. Compute average length, use direction from
                // back handle to front handle. This also works correctly for degenerates
                double len = (_front.length() + _back.length()) / 2;
                Geom::Point dir = direction(_back, _front);
                _front.setRelativePos(dir * len);
                _back.setRelativePos(-dir * len);
            }
            break;
        default: break;
        }
    }
    _type = type;
    _setShape(_node_type_to_shape(type));
    updateState();
}

void Node::pickBestType()
{
    _type = NODE_CUSP;
    bool front_degen = _front.isDegenerate();
    bool back_degen = _back.isDegenerate();
    bool both_degen = front_degen && back_degen;
    bool neither_degen = !front_degen && !back_degen;
    do {
        // if both handles are degenerate, do nothing
        if (both_degen) break;
        // if neither are degenerate, check their respective positions
        if (neither_degen) {
            Geom::Point front_delta = _front.position() - position();
            Geom::Point back_delta = _back.position() - position();
            // for now do not automatically make nodes symmetric, it can be annoying
            /*if (Geom::are_near(front_delta, -back_delta)) {
                _type = NODE_SYMMETRIC;
                break;
            }*/
            if (Geom::are_near(Geom::unit_vector(front_delta),
                Geom::unit_vector(-back_delta)))
            {
                _type = NODE_SMOOTH;
                break;
            }
        }
        // check whether the handle aligns with the previous line segment.
        // we know that if front is degenerate, back isn't, because
        // both_degen was false
        if (front_degen && _next() && _next()->_back.isDegenerate()) {
            Geom::Point segment_delta = Geom::unit_vector(_next()->position() - position());
            Geom::Point handle_delta = Geom::unit_vector(_back.position() - position());
            if (Geom::are_near(segment_delta, -handle_delta)) {
                _type = NODE_SMOOTH;
                break;
            }
        } else if (back_degen && _prev() && _prev()->_front.isDegenerate()) {
            Geom::Point segment_delta = Geom::unit_vector(_prev()->position() - position());
            Geom::Point handle_delta = Geom::unit_vector(_front.position() - position());
            if (Geom::are_near(segment_delta, -handle_delta)) {
                _type = NODE_SMOOTH;
                break;
            }
        }
    } while (false);
    _setShape(_node_type_to_shape(_type));
    updateState();
}

bool Node::isEndNode()
{
    return !_prev() || !_next();
}

/** Move the node to the bottom of its canvas group. Useful for node break, to ensure that
 * the selected nodes are above the unselected ones. */
void Node::sink()
{
    sp_canvas_item_move_to_z(_canvas_item, 0);
}

NodeType Node::parse_nodetype(char x)
{
    switch (x) {
    case 'a': return NODE_AUTO;
    case 'c': return NODE_CUSP;
    case 's': return NODE_SMOOTH;
    case 'z': return NODE_SYMMETRIC;
    default: return NODE_PICK_BEST;
    }
}

void Node::_setState(State state)
{
    // change node size to match type and selection state
    switch (_type) {
    case NODE_AUTO:
    case NODE_CUSP:
        if (selected()) _setSize(11);
        else _setSize(9);
        break;
    default:
        if(selected()) _setSize(9);
        else _setSize(7);
        break;
    }
    SelectableControlPoint::_setState(state);
}

bool Node::_grabbedHandler(GdkEventMotion *event)
{
    // dragging out handles
    if (!held_shift(*event)) return false;

    Handle *h;
    Geom::Point evp = event_point(*event);
    Geom::Point rel_evp = evp - _last_click_event_point();

    // this should work even if dragtolerance is zero and evp coincides with node position
    double angle_next = HUGE_VAL;
    double angle_prev = HUGE_VAL;
    bool has_degenerate = false;
    // determine which handle to drag out based on degeneration and the direction of drag
    if (_front.isDegenerate() && _next()) {
        Geom::Point next_relpos = _desktop->d2w(_next()->position())
            - _desktop->d2w(position());
        angle_next = fabs(Geom::angle_between(rel_evp, next_relpos));
        has_degenerate = true;
    }
    if (_back.isDegenerate() && _prev()) {
        Geom::Point prev_relpos = _desktop->d2w(_prev()->position())
            - _desktop->d2w(position());
        angle_prev = fabs(Geom::angle_between(rel_evp, prev_relpos));
        has_degenerate = true;
    }
    if (!has_degenerate) return false;
    h = angle_next < angle_prev ? &_front : &_back;

    h->setPosition(_desktop->w2d(evp));
    h->setVisible(true);
    h->transferGrab(this, event);
    Handle::_drag_out = true;
    return true;
}

void Node::_draggedHandler(Geom::Point &new_pos, GdkEventMotion *event)
{
    if (!held_control(*event)) return;
    if (held_alt(*event)) {
        // with Ctrl+Alt, constrain to handle lines
        // project the new position onto a handle line that is closer
        Geom::Point origin = _last_drag_origin();
        Geom::Line line_front(origin, origin + _front.relativePos());
        Geom::Line line_back(origin, origin + _back.relativePos());
        double dist_front, dist_back;
        dist_front = Geom::distance(new_pos, line_front);
        dist_back = Geom::distance(new_pos, line_back);
        if (dist_front < dist_back) {
            new_pos = Geom::projection(new_pos, line_front);
        } else {
            new_pos = Geom::projection(new_pos, line_back);
        }
    } else {
        // with Ctrl, constrain to axes
        // TODO this probably has to be separated into an AxisConstrainablePoint class
        // TODO maybe add diagonals when the distance from origin is large enough?
        Geom::Point origin = _last_drag_origin();
        Geom::Point delta = new_pos - origin;
        Geom::Dim2 d = (fabs(delta[Geom::X]) < fabs(delta[Geom::Y])) ? Geom::X : Geom::Y;
        new_pos[d] = origin[d];
    }
}

Glib::ustring Node::_getTip(unsigned state)
{
    if (state_held_shift(state)) {
        if ((_next() && _front.isDegenerate()) || (_prev() && _back.isDegenerate())) {
            if (state_held_control(state)) {
                return format_tip(C_("Path node tip",
                    "<b>Shift+Ctrl:</b> drag out a handle and snap its angle "
                    "to %f° increments"), snap_increment_degrees());
            }
            return C_("Path node tip",
                "<b>Shift:</b> drag out a handle, click to toggle selection");
        }
        return C_("Path node statusbar tip", "<b>Shift:</b> click to toggle selection");
    }

    if (state_held_control(state)) {
        if (state_held_alt(state)) {
            return C_("Path node tip", "<b>Ctrl+Alt:</b> move along handle lines");
        }
        return C_("Path node tip",
            "<b>Ctrl:</b> move along axes, click to change node type");
    }
    
    // assemble tip from node name
    char const *nodetype = node_type_to_localized_string(_type);
    return format_tip(C_("Path node tip",
        "<b>%s:</b> drag to shape the path, click to select this node"), nodetype);
}

Glib::ustring Node::_getDragTip(GdkEventMotion *event)
{
    Geom::Point dist = position() - _last_drag_origin();
    GString *x = SP_PX_TO_METRIC_STRING(dist[Geom::X], _desktop->namedview->getDefaultMetric());
    GString *y = SP_PX_TO_METRIC_STRING(dist[Geom::Y], _desktop->namedview->getDefaultMetric());
    Glib::ustring ret = format_tip(C_("Path node statusbar tip", "Move by %s, %s"),
        x->str, y->str);
    g_string_free(x, TRUE);
    g_string_free(y, TRUE);
    return ret;
}

char const *Node::node_type_to_localized_string(NodeType type)
{
    switch (type) {
    case NODE_CUSP: return _("Cusp node");
    case NODE_SMOOTH: return _("Smooth node");
    case NODE_SYMMETRIC: return _("Symmetric node");
    case NODE_AUTO: return _("Auto-smooth node");
    default: return "";
    }
}

/** Determine whether two nodes are joined by a linear segment. */
bool Node::_is_line_segment(Node *first, Node *second)
{
    if (!first || !second) return false;
    if (first->_next() == second)
        return first->_front.isDegenerate() && second->_back.isDegenerate();
    if (second->_next() == first)
        return second->_front.isDegenerate() && first->_back.isDegenerate();
    return false;
}

SPCtrlShapeType Node::_node_type_to_shape(NodeType type)
{
    switch(type) {
    case NODE_CUSP: return SP_CTRL_SHAPE_DIAMOND;
    case NODE_SMOOTH: return SP_CTRL_SHAPE_SQUARE;
    case NODE_AUTO: return SP_CTRL_SHAPE_CIRCLE;
    case NODE_SYMMETRIC: return SP_CTRL_SHAPE_SQUARE;
    default: return SP_CTRL_SHAPE_DIAMOND;
    }
}


/**
 * @class NodeList
 * @brief An editable list of nodes representing a subpath.
 *
 * It can optionally be cyclic to represent a closed path.
 * The list has iterators that act like plain node iterators, but can also be used
 * to obtain shared pointers to nodes.
 *
 * @todo Manage geometric representation to improve speed
 */

NodeList::NodeList(SubpathList &splist)
    : _list(splist)
    , _closed(false)
{
    this->list = this;
    this->next = this;
    this->prev = this;
}

NodeList::~NodeList()
{
    clear();
}

bool NodeList::empty()
{
    return next == this;
}

NodeList::size_type NodeList::size()
{
    size_type sz = 0;
    for (ListNode *ln = next; ln != this; ln = ln->next) ++sz;
    return sz;
}

bool NodeList::closed()
{
    return _closed;
}

/** A subpath is degenerate if it has no segments - either one node in an open path
 * or no nodes in a closed path */
bool NodeList::degenerate()
{
    return closed() ? empty() : ++begin() == end();
}

NodeList::iterator NodeList::before(double t, double *fracpart)
{
    double intpart;
    *fracpart = std::modf(t, &intpart);
    int index = intpart;

    iterator ret = begin();
    std::advance(ret, index);
    return ret;
}

// insert a node before i
NodeList::iterator NodeList::insert(iterator i, Node *x)
{
    ListNode *ins = i._node;
    x->next = ins;
    x->prev = ins->prev;
    ins->prev->next = x;
    ins->prev = x;
    x->ListNode::list = this;
    _list.signal_insert_node.emit(x);
    return iterator(x);
}

void NodeList::splice(iterator pos, NodeList &list)
{
    splice(pos, list, list.begin(), list.end());
}

void NodeList::splice(iterator pos, NodeList &list, iterator i)
{
    NodeList::iterator j = i;
    ++j;
    splice(pos, list, i, j);
}

void NodeList::splice(iterator pos, NodeList &list, iterator first, iterator last)
{
    ListNode *ins_beg = first._node, *ins_end = last._node, *at = pos._node;
    for (ListNode *ln = ins_beg; ln != ins_end; ln = ln->next) {
        list._list.signal_remove_node.emit(static_cast<Node*>(ln));
        ln->list = this;
        _list.signal_insert_node.emit(static_cast<Node*>(ln));
    }
    ins_beg->prev->next = ins_end;
    ins_end->prev->next = at;
    at->prev->next = ins_beg;

    ListNode *atprev = at->prev;
    at->prev = ins_end->prev;
    ins_end->prev = ins_beg->prev;
    ins_beg->prev = atprev;
}

void NodeList::shift(int n)
{
    // 1. make the list perfectly cyclic
    next->prev = prev;
    prev->next = next;
    // 2. find new begin
    ListNode *new_begin = next;
    if (n > 0) {
        for (; n > 0; --n) new_begin = new_begin->next;
    } else {
        for (; n < 0; ++n) new_begin = new_begin->prev;
    }
    // 3. relink begin to list
    next = new_begin;
    prev = new_begin->prev;
    new_begin->prev->next = this;
    new_begin->prev = this;
}

void NodeList::reverse()
{
    for (ListNode *ln = next; ln != this; ln = ln->prev) {
        std::swap(ln->next, ln->prev);
        Node *node = static_cast<Node*>(ln);
        Geom::Point save_pos = node->front()->position();
        node->front()->setPosition(node->back()->position());
        node->back()->setPosition(save_pos);
    }
    std::swap(next, prev);
}

void NodeList::clear()
{
    for (iterator i = begin(); i != end();) erase (i++);
}

NodeList::iterator NodeList::erase(iterator i)
{
    // some acrobatics are required to ensure that the node is valid when deleted;
    // otherwise the code that updates handle visibility will break
    Node *rm = static_cast<Node*>(i._node);
    ListNode *rmnext = rm->next, *rmprev = rm->prev;
    ++i;
    _list.signal_remove_node.emit(rm);
    delete rm;
    rmprev->next = rmnext;
    rmnext->prev = rmprev;
    return i;
}

void NodeList::kill()
{
    for (SubpathList::iterator i = _list.begin(); i != _list.end(); ++i) {
        if (i->get() == this) {
            _list.erase(i);
            return;
        }
    }
}

NodeList &NodeList::get(Node *n) {
    return *(n->list());
}
NodeList &NodeList::get(iterator const &i) {
    return *(i._node->list);
}


/**
 * @class SubpathList
 * @brief Editable path composed of one or more subpaths
 */

} // namespace UI
} // namespace Inkscape

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
