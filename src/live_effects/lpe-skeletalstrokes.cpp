#define INKSCAPE_LPE_SKELETAL_STROKES_CPP

/*
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-skeletalstrokes.h"
#include "sp-shape.h"
#include "display/curve.h"
#include <libnr/n-art-bpath.h>
#include "live_effects/n-art-bpath-2geom.h"
#include "svg/svg.h"

#include <2geom/sbasis.h>
#include <2geom/sbasis-geometric.h>
#include <2geom/bezier-to-sbasis.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/d2.h>
#include <2geom/piecewise.h>

#include <algorithm>
using std::vector;


/* Theory in e-mail from J.F. Barraud
Let B be the skeleton path, and P the pattern (the path to be deformed).

P is a map t --> P(t) = ( x(t), y(t) ).
B is a map t --> B(t) = ( a(t), b(t) ).

The first step is to re-parametrize B by its arc length: this is the parametrization in which a point p on B is located by its distance s from start. One obtains a new map s --> U(s) = (a'(s),b'(s)), that still describes the same path B, but where the distance along B from start to
U(s) is s itself.

We also need a unit normal to the path. This can be obtained by computing a unit tangent vector, and rotate it by 90�. Call this normal vector N(s).

The basic deformation associated to B is then given by:

   (x,y) --> U(x)+y*N(x)

(i.e. we go for distance x along the path, and then for distance y along the normal)

Of course this formula needs some minor adaptations (as is it depends on the absolute position of P for instance, so a little translation is needed
first) but I think we can first forget about them.
*/

namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<SkelCopyType> SkelCopyTypeData[SSCT_END] = {
    {SSCT_SINGLE,               N_("Single"),               "single"},
    {SSCT_SINGLE_STRETCHED,     N_("Single, stretched"),    "single_stretched"},
    {SSCT_REPEATED,             N_("Repeated"),             "repeated"},
    {SSCT_REPEATED_STRETCHED,   N_("Repeated, stretched"),  "repeated_stretched"}
};
static const Util::EnumDataConverter<SkelCopyType> SkelCopyTypeConverter(SkelCopyTypeData, SSCT_END);

LPESkeletalStrokes::LPESkeletalStrokes(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    pattern(_("Pattern"), _("Path to put along path"), "pattern", &wr, this, "M0,0 L1,1"),
    origin(_("Origin"), _("Startpoint of the pattern path to put along path"), "origin", &wr, this, Geom::Point(0,0)),
    copytype(_("Copytype"), _("How to shape the pattern path along the path"), "copytype", SkelCopyTypeConverter, &wr, this, SSCT_SINGLE_STRETCHED)
{
    registerParameter( dynamic_cast<Parameter *>(&origin) );
    registerParameter( dynamic_cast<Parameter *>(&pattern) );
    registerParameter( dynamic_cast<Parameter *>(&copytype) );

    pattern.signal_path_pasted.connect(sigc::mem_fun(*this, &LPESkeletalStrokes::on_pattern_pasted));
}

LPESkeletalStrokes::~LPESkeletalStrokes()
{

}


Geom::Piecewise<Geom::D2<Geom::SBasis> >
LPESkeletalStrokes::doEffect (Geom::Piecewise<Geom::D2<Geom::SBasis> > & pwd2_in)
{
    using namespace Geom;

/* LOTS OF CODE COPIED FROM 2geom/src/toys/path-along-path.cpp
 * All credits should go to jfb and mgsloan of lib2geom development! */

    SkelCopyType type = copytype.get_value();

    Piecewise<D2<SBasis> > uskeleton = arc_length_parametrization(Piecewise<D2<SBasis> >(pwd2_in),2,.1);
    uskeleton = remove_short_cuts(uskeleton,.01);
    Piecewise<D2<SBasis> > n = rot90(derivative(uskeleton));
    n = force_continuity(remove_short_cuts(n,.1));

    D2<Piecewise<SBasis> > patternd2 = make_cuts_independant(pattern);
    Piecewise<SBasis> x=Piecewise<SBasis>(patternd2[0]-origin[0]);
    Piecewise<SBasis> y=Piecewise<SBasis>(patternd2[1]-origin[1]);
    Interval pattBnds = bounds_exact(x);

    int nbCopies = int(uskeleton.cuts.back()/pattBnds.extent());
    double scaling = 1;

    switch(type) {
        case SSCT_REPEATED:
            break;

        case SSCT_SINGLE:
            nbCopies = (nbCopies > 0) ? 1 : 0;
            break;

        case SSCT_SINGLE_STRETCHED:
            nbCopies = 1;
            scaling = uskeleton.cuts.back()/pattBnds.extent();
            break;

        case SSCT_REPEATED_STRETCHED:
             scaling = uskeleton.cuts.back()/(((double)nbCopies)*pattBnds.extent());
            break;

        default:
            return pwd2_in;
    };

    double pattWidth = pattBnds.extent() * scaling;

    x-=pattBnds.min();
    if (scaling != 1)
        x*=scaling;

    double offs = 0;
    Piecewise<D2<SBasis> > output;
    for (int i=0; i<nbCopies; i++){
        output.concat(compose(uskeleton,x+offs)+y*compose(n,x+offs));
        offs+=pattWidth;
    }
    return output;
}

void
LPESkeletalStrokes::on_pattern_pasted()
{
    // a new pattern was pasted through the dialog.  overwrite the origin thingie to the first point of the path
    origin.param_setValue(pattern.valueAt(0));
}



} // namespace LivePathEffect
} /* namespace Inkscape */

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
