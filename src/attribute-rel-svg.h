// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef __SP_ATTRIBUTE_REL_SVG_H__
#define __SP_ATTRIBUTE_REL_SVG_H__

/*
 * attribute-rel-svg.h
 *
 *  Created on: Jul 25, 2011
 *      Author: abhishek
 */

#include <map>
#include <set>
#include <string>

#include <glibmm/ustring.h>

// This data structure stores the valid (element -> set of attributes) pair
typedef std::map<Glib::ustring, std::set<Glib::ustring>> hashList;

/*
 * Utility class to check whether a combination of element and attribute
 * is valid or not.
 */
class SPAttributeRelSVG
{
public:
    static bool isSVGElement(Glib::ustring const &element);
    static bool findIfValid(Glib::ustring const &attribute, Glib::ustring const &element);

private:
    SPAttributeRelSVG();
    SPAttributeRelSVG(const SPAttributeRelSVG &) = delete;
    SPAttributeRelSVG &operator=(const SPAttributeRelSVG &) = delete;
    static SPAttributeRelSVG &getInstance();

private:
    static bool foundFile;
    hashList attributesOfElements;
};

#endif /* __SP_ATTRIBUTE_REL_SVG_H__ */

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
