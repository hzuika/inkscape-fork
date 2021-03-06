// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_LPEOBJECT_REFERENCE_H
#define SEEN_LPEOBJECT_REFERENCE_H

/*
 * The reference corresponding to the inkscape:live-effect attribute
 *
 * Copyright (C) 2007 Johan Engelen
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <sigc++/sigc++.h>

#include "object/uri-references.h"

namespace Inkscape {

namespace XML {
class Node;
}
}

class LivePathEffectObject;

namespace Inkscape {

namespace LivePathEffect {

class LPEObjectReference : public Inkscape::URIReference {
public:
    LPEObjectReference(SPObject *owner);
    ~LPEObjectReference() override;

    SPObject       *owner;

    // concerning the LPEObject that is referred to:
    char                 *lpeobject_href;
    Inkscape::XML::Node  *lpeobject_repr;
    LivePathEffectObject *lpeobject;

    sigc::connection _modified_connection;
    sigc::connection _release_connection;
    sigc::connection _changed_connection;

    void            link(const char* to);
    void            unlink();
    void            start_listening(LivePathEffectObject* to);
    void            quit_listening();

    void (*user_unlink) (LPEObjectReference *me, SPObject *user);

protected:
    bool _acceptObject(SPObject * const obj) const override;

};

} //namespace LivePathEffect

} // namespace inkscape

#endif /* !SEEN_LPEOBJECT_REFERENCE_H */

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
