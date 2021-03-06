// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Struct describing a single glyph in a font.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef LIBNRTYPE_FONT_GLYPH_H
#define LIBNRTYPE_FONT_GLYPH_H

#include <memory>
#include <2geom/pathvector.h>

// The info for a glyph in a font. It's totally resolution- and fontsize-independent.
struct FontGlyph
{
    double h_advance, h_width; // width != advance because of kerning adjustements
    double v_advance, v_width;
    double bbox[4];            // bbox of the path (and the artbpath), not the bbox of the glyph as the fonts sometimes contain outline as a livarot Path
    Geom::PathVector pathvector; // outline as 2geom pathvector, for text->curve stuff (should be unified with livarot)
};

#endif // LIBNRTYPE_FONT_GLYPH_H

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
