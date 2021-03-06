// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SVG <pattern> implementation
 *//*
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SP_PATTERN_H
#define SEEN_SP_PATTERN_H

#include <list>
#include <cstddef>
#include <glibmm/ustring.h>
#include <sigc++/connection.h>

#include "svg/svg-length.h"
#include "sp-paint-server.h"
#include "uri-references.h"
#include "viewbox.h"

class SPPatternReference;
class SPItem;

namespace Inkscape {
namespace XML {

class Node;

} // namespace XML
} // namespace Inkscape

#define SP_PATTERN(obj) (dynamic_cast<SPPattern *>((SPObject *)obj))
#define SP_IS_PATTERN(obj) (dynamic_cast<const SPPattern *>((SPObject *)obj) != NULL)

class SPPattern : public SPPaintServer, public SPViewBox
{
public:
    enum PatternUnits { UNITS_USERSPACEONUSE, UNITS_OBJECTBOUNDINGBOX };

    SPPattern();
    ~SPPattern() override;

    /* Reference (href) */
    Glib::ustring href;
    SPPatternReference *ref;

    gdouble x() const;
    gdouble y() const;
    gdouble width() const;
    gdouble height() const;
    Geom::OptRect viewbox() const;
    SPPattern::PatternUnits patternUnits() const;
    SPPattern::PatternUnits patternContentUnits() const;
    Geom::Affine const &getTransform() const;
    SPPattern *rootPattern(); // TODO: const

    SPPattern *clone_if_necessary(SPItem *item, const gchar *property);
    void transform_multiply(Geom::Affine postmul, bool set);

    /**
     * @brief create a new pattern in XML tree
     * @return created pattern id
     */
    static const gchar *produce(const std::vector<Inkscape::XML::Node *> &reprs, Geom::Rect bounds,
                                SPDocument *document, Geom::Affine transform, Geom::Affine move);

    bool isValid() const override;

    cairo_pattern_t *pattern_new(cairo_t *ct, Geom::OptRect const &bbox, double opacity) override;

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void release() override;
    void set(SPAttr key, const gchar *value) override;
    void update(SPCtx *ctx, unsigned flags) override;
    void modified(unsigned flags) override;

private:
    bool _hasItemChildren() const;
    void _getChildren(std::list<SPObject *> &l);
    SPPattern *_chain() const;

    /**
    Count how many times pattern is used by the styles of o and its descendants
    */
    guint _countHrefs(SPObject *o) const;

    /**
    Gets called when the pattern is reattached to another <pattern>
    */
    void _onRefChanged(SPObject *old_ref, SPObject *ref);

    /**
    Gets called when the referenced <pattern> is changed
    */
    void _onRefModified(SPObject *ref, guint flags);

    /* patternUnits and patternContentUnits attribute */
    PatternUnits _pattern_units : 1;
    bool _pattern_units_set : 1;
    PatternUnits _pattern_content_units : 1;
    bool _pattern_content_units_set : 1;
    /* patternTransform attribute */
    Geom::Affine _pattern_transform;
    bool _pattern_transform_set : 1;
    /* Tile rectangle */
    SVGLength _x;
    SVGLength _y;
    SVGLength _width;
    SVGLength _height;

    sigc::connection _modified_connection;
};

class SPPatternReference : public Inkscape::URIReference
{
public:
    SPPatternReference(SPPattern *obj)
        : URIReference(obj)
    {
    }

    SPPattern *getObject() const
    {
        return static_cast<SPPattern*>(URIReference::getObject());
    }

protected:
    bool _acceptObject(SPObject *obj) const override
    {
        return SP_IS_PATTERN(obj) && URIReference::_acceptObject(obj);
    }
};

#endif // SEEN_SP_PATTERN_H

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
