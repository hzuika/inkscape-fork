// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::ProfileManager - a view of a document's color profiles.
 *
 * Copyright 2007  Jon A. Cruz  <jon@joncruz.org>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_PROFILE_MANAGER_H
#define SEEN_INKSCAPE_PROFILE_MANAGER_H

#include <vector>

#include "document-subset.h"

class SPDocument;

namespace Inkscape {

class ColorProfile;

class ProfileManager : public DocumentSubset
{
public:
    ProfileManager(SPDocument *document);
    ~ProfileManager();

    ColorProfile* find(char const* name);

private:
    ProfileManager(ProfileManager const &) = delete; // no copy
    void operator=(ProfileManager const &) = delete; // no assign

    void _resourcesChanged();

    SPDocument* _doc;
    sigc::connection _resource_connection;
    std::vector<SPObject*> _knownProfiles;
};

}

#endif // SEEN_INKSCAPE_PROFILE_MANAGER_H

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
