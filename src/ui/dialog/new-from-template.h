// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief New From Template main dialog
 */
/* Authors:
 *   Jan Darowski <jan.darowski@gmail.com>, supervised by Krzysztof Kosiński
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_SEEN_UI_DIALOG_NEW_FROM_TEMPLATE_H
#define INKSCAPE_SEEN_UI_DIALOG_NEW_FROM_TEMPLATE_H

#include <gtkmm/dialog.h>
#include <gtkmm/button.h>

namespace Inkscape {
namespace UI {
namespace Widget {
class TemplateList;
}

class NewFromTemplate : public Gtk::Dialog
{

friend class TemplateLoadTab;
public:
    static void load_new_from_template();
    ~NewFromTemplate() override{};

private:
    NewFromTemplate();
    Gtk::Button _create_template_button;
    Inkscape::UI::Widget::TemplateList *templates;

    void _createFromTemplate();
    void _onClose();
};

}
}
#endif
