// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Base class for gradients and patterns
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2010 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-paint-server-reference.h"
#include "sp-paint-server.h"

#include "sp-gradient.h"
#include "xml/node.h"

SPPaintServer *SPPaintServerReference::getObject() const
{
    return static_cast<SPPaintServer *>(URIReference::getObject());
}

bool SPPaintServerReference::_acceptObject(SPObject *obj) const
{
    return SP_IS_PAINT_SERVER(obj) && URIReference::_acceptObject(obj);
}

SPPaintServer::SPPaintServer()
{
    swatch = false;
}

SPPaintServer::~SPPaintServer() = default;

bool SPPaintServer::isSwatch() const
{
    return swatch;
}

bool SPPaintServer::isValid() const
{
    return true;
}

Inkscape::DrawingPattern *SPPaintServer::show(Inkscape::Drawing &/*drawing*/, unsigned int /*key*/, Geom::OptRect /*bbox*/)
{
    return nullptr;
}

void SPPaintServer::hide(unsigned int /*key*/)
{
}

void SPPaintServer::setBBox(unsigned int /*key*/, Geom::OptRect const &/*bbox*/)
{
}

cairo_pattern_t* SPPaintServer::pattern_new(cairo_t * /*ct*/, Geom::OptRect const &/*bbox*/, double /*opacity*/)
{
    return nullptr;
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
