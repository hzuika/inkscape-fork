// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Anshudhar Kumar Singh <anshudhar2001@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "export-preview.h"

#include <glibmm/i18n.h>
#include <glibmm/main.h>
#include <glibmm/timer.h>
#include <gtkmm.h>

#include "display/cairo-utils.h"
#include "inkscape.h"
#include "object/sp-defs.h"
#include "object/sp-item.h"
#include "object/sp-namedview.h"
#include "object/sp-root.h"
#include "util/preview.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

void ExportPreview::resetPixels()
{
    clear();
    show();
}

ExportPreview::~ExportPreview()
{
    if (drawing && _document) {
        _document->getRoot()->invoke_hide(visionkey);
    }
    if (timer) {
        timer->stop();
    }
    if (renderTimer) {
        renderTimer->stop();
    }
    _item = nullptr;
    _document = nullptr;
}

void ExportPreview::setItem(SPItem *item)
{
    _item = item;
    _dbox = Geom::OptRect();
}
void ExportPreview::setDbox(double x0, double x1, double y0, double y1)
{
    if (!_document) {
        return;
    }
    if ((x1 - x0 == 0) || (y1 - y0) == 0) {
        return;
    }
    _item = nullptr;
    _dbox = Geom::Rect(Geom::Point(x0, y0), Geom::Point(x1, y1)) * _document->dt2doc();
}

void ExportPreview::setDocument(SPDocument *document)
{
    if (drawing) {
        if (_document) {
            _document->getRoot()->invoke_hide(visionkey);
        }
        drawing.reset();
    }
    _document = document;
    if (_document) {
        drawing = std::make_unique<Inkscape::Drawing>();
        visionkey = SPItem::display_key_new(1);
        DrawingItem *ai = _document->getRoot()->invoke_show(*drawing, visionkey, SP_ITEM_SHOW_DISPLAY);
        if (ai) {
            drawing->setRoot(ai);
        }
    }
}

void ExportPreview::refreshHide(std::vector<SPItem *> const &list)
{
    _hidden_excluded = list;
    _hidden_requested = true;
}

void ExportPreview::performHide()
{
    if (_document) {
        if (isLastHide) {
            if (drawing) {
                _document->getRoot()->invoke_hide(visionkey);
            }
            drawing = std::make_unique<Inkscape::Drawing>();
            visionkey = SPItem::display_key_new(1);
            DrawingItem *ai = _document->getRoot()->invoke_show(*drawing, visionkey, SP_ITEM_SHOW_DISPLAY);
            if (ai) {
                drawing->setRoot(ai);
            }
            isLastHide = false;
        }
        if (!_hidden_excluded.empty()) {
            _document->getRoot()->invoke_hide_except(visionkey, _hidden_excluded);
            isLastHide = true;
        }
    }
}

void ExportPreview::queueRefresh()
{
    if (drawing == nullptr) {
        return;
    }
    if (!pending) {
        pending = true;
        if (!timer) {
            timer = std::make_unique<Glib::Timer>();
        }
        Glib::signal_idle().connect(sigc::mem_fun(this, &ExportPreview::refreshCB), Glib::PRIORITY_DEFAULT_IDLE);
    }
}

bool ExportPreview::refreshCB()
{
    bool callAgain = true;
    if (!timer) {
        timer = std::make_unique<Glib::Timer>();
    }
    if (timer->elapsed() > minDelay) {
        callAgain = false;
        refreshPreview();
        pending = false;
    }
    return callAgain;
}

void ExportPreview::refreshPreview()
{
    auto document = _document;
    if (!timer) {
        timer = std::make_unique<Glib::Timer>();
    }
    if (timer->elapsed() < minDelay) {
        // Do not refresh too quickly
        queueRefresh();
    } else if (document) {
        renderPreview();
        timer->reset();
    }
}

/*
This is main function which finally render preview. Call this after setting document, item and dbox.
If dbox is given it will use it.
if item is given and not dbox then item is used
If both are not given then simply we do nothing.
*/
void ExportPreview::renderPreview()
{
    if (!renderTimer) {
        renderTimer = std::make_unique<Glib::Timer>();
    }
    renderTimer->reset();
    if (drawing == nullptr) {
        return;
    }

    if (_hidden_requested) {
        performHide();
        _hidden_requested = false;
    }
    if (_document) {
        GdkPixbuf *pb = nullptr;
        if (_item) {
            pb = Inkscape::UI::PREVIEW::render_preview(_document, *drawing, _bg_color, _item, size, size);
        } else if (_dbox) {
            pb = Inkscape::UI::PREVIEW::render_preview(_document, *drawing, _bg_color, nullptr, size, size, &_dbox);
        }
        if (pb) {
            set(Glib::wrap(pb));
            show();
        }
    }

    renderTimer->stop();
    minDelay = std::max(0.1, renderTimer->elapsed() * 3.0);
}

void ExportPreview::set_background_color(guint32 bg_color) {
    _bg_color = bg_color;
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

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
