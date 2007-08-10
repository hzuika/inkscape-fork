/*
 * Copyright (C) 2007 Authors:
 *   Christopher Brown <audiere@gmail.com>
 *   Ted Gould <ted@gould.cx>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "extension/effect.h"
#include "extension/system.h"

#include "blur.h"

namespace Inkscape {
namespace Extension {
namespace Internal {
namespace Bitmap {
	
void
Blur::applyEffect(Magick::Image *image) {
	image->blur(_radius, _sigma);
}

void
Blur::refreshParameters(Inkscape::Extension::Effect *module) {	
	_radius = module->get_param_float("radius");
	_sigma = module->get_param_float("sigma");
}

#include "../clear-n_.h"

void
Blur::init(void)
{
	Inkscape::Extension::build_from_mem(
		"<inkscape-extension>\n"
			"<name>" N_("Blur") "</name>\n"
			"<id>org.inkscape.effect.bitmap.blur</id>\n"
			"<param name=\"radius\" gui-text=\"" N_("Radius") "\" type=\"float\" min=\"0.1\" max=\"20\">5.0</param>\n"
			"<param name=\"sigma\" gui-text=\"" N_("Sigma") "\" type=\"float\" min=\"0.1\" max=\"5\">1.0</param>\n"
			"<effect>\n"
				"<object-type>all</object-type>\n"
				"<effects-menu>\n"
					"<submenu name=\"" N_("Raster") "\" />\n"
				"</effects-menu>\n"
				"<menu-tip>" N_("Apply Blur Effect") "</menu-tip>\n"
			"</effect>\n"
		"</inkscape-extension>\n", new Blur());
}

}; /* namespace Bitmap */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */
