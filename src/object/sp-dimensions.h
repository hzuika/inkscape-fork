// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SP_DIMENSIONS_H__
#define SP_DIMENSIONS_H__

/*
 * dimensions helper class, common code used by root, image and others
 *
 * Authors:
 *  Shlomi Fish
 * Copyright (C) 2017 Shlomi Fish, authors
 *
 * Released under dual Expat and GNU GPL, read the file 'COPYING' for more information
 *
 */

#include "svg/svg-length.h"

namespace Inkscape::XML {
class Node;
} // namespace Inkscape::XML

class SPItemCtx;

class SPDimensions {

public:
    SVGLength x;
    SVGLength y;
    SVGLength width;
    SVGLength height;
    void calcDimsFromParentViewport(const SPItemCtx *ictx, bool assign_to_set = false,
                                    SPDimensions const *use = nullptr);
    void writeDimensions(Inkscape::XML::Node *) const;
};

#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-basic-offset:2
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=2:tabstop=8:softtabstop=2:fileencoding=utf-8:textwidth=99 :
