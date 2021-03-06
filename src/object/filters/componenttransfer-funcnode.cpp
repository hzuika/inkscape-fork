// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <funcR>, <funcG>, <funcB> and <funcA> implementations.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006, 2007, 2008 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>

#include "attributes.h"
#include "document.h"
#include "componenttransfer.h"
#include "componenttransfer-funcnode.h"
#include "util/numeric/converters.h"
#include "xml/repr.h"

/* FeFuncNode class */
SPFeFuncNode::SPFeFuncNode(SPFeFuncNode::Channel channel)
    : SPObject(), type(Inkscape::Filters::COMPONENTTRANSFER_TYPE_IDENTITY),
      slope(1), intercept(0), amplitude(1), exponent(1), offset(0), channel(channel) {
}

SPFeFuncNode::~SPFeFuncNode() = default;

/**
 * Reads the Inkscape::XML::Node, and initializes SPDistantLight variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void SPFeFuncNode::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPObject::build(document, repr);

    //Read values of key attributes from XML nodes into object.
    this->readAttr(SPAttr::TYPE);
    this->readAttr(SPAttr::TABLEVALUES);
    this->readAttr(SPAttr::SLOPE);
    this->readAttr(SPAttr::INTERCEPT);
    this->readAttr(SPAttr::AMPLITUDE);
    this->readAttr(SPAttr::EXPONENT);
    this->readAttr(SPAttr::OFFSET);


//is this necessary?
    document->addResource("fefuncnode", this); //maybe feFuncR, fefuncG, feFuncB and fefuncA ?
}

/**
 * Drops any allocated memory.
 */
void SPFeFuncNode::release() {
    if ( this->document ) {
        // Unregister ourselves
        this->document->removeResource("fefuncnode", this);
    }

//TODO: release resources here
}

static Inkscape::Filters::FilterComponentTransferType sp_feComponenttransfer_read_type(gchar const *value){
    if (!value) {
    	return Inkscape::Filters::COMPONENTTRANSFER_TYPE_ERROR; //type attribute is REQUIRED.
    }

    switch(value[0]){
        case 'i':
            if (strncmp(value, "identity", 8) == 0) {
            	return Inkscape::Filters::COMPONENTTRANSFER_TYPE_IDENTITY;
            }
            break;
        case 't':
            if (strncmp(value, "table", 5) == 0) {
            	return Inkscape::Filters::COMPONENTTRANSFER_TYPE_TABLE;
            }
            break;
        case 'd':
            if (strncmp(value, "discrete", 8) == 0) {
            	return Inkscape::Filters::COMPONENTTRANSFER_TYPE_DISCRETE;
            }
            break;
        case 'l':
            if (strncmp(value, "linear", 6) == 0) {
            	return Inkscape::Filters::COMPONENTTRANSFER_TYPE_LINEAR;
            }
            break;
        case 'g':
            if (strncmp(value, "gamma", 5) == 0) {
            	return Inkscape::Filters::COMPONENTTRANSFER_TYPE_GAMMA;
            }
            break;
    }

    return Inkscape::Filters::COMPONENTTRANSFER_TYPE_ERROR; //type attribute is REQUIRED.
}

/**
 * Sets a specific value in the SPFeFuncNode.
 */
void SPFeFuncNode::set(SPAttr key, gchar const *value) {
    Inkscape::Filters::FilterComponentTransferType type;
    double read_num;

    switch(key) {
        case SPAttr::TYPE:
            type = sp_feComponenttransfer_read_type(value);

            if(type != this->type) {
                this->type = type;
                this->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SPAttr::TABLEVALUES:
            if (value){
                this->tableValues = Inkscape::Util::read_vector(value);
                this->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SPAttr::SLOPE:
            read_num = value ? Inkscape::Util::read_number(value) : 1;

            if (read_num != this->slope) {
                this->slope = read_num;
                this->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SPAttr::INTERCEPT:
            read_num = value ? Inkscape::Util::read_number(value) : 0;

            if (read_num != this->intercept) {
                this->intercept = read_num;
                this->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SPAttr::AMPLITUDE:
            read_num = value ? Inkscape::Util::read_number(value) : 1;

            if (read_num != this->amplitude) {
                this->amplitude = read_num;
                this->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SPAttr::EXPONENT:
            read_num = value ? Inkscape::Util::read_number(value) : 1;

            if (read_num != this->exponent) {
                this->exponent = read_num;
                this->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SPAttr::OFFSET:
            read_num = value ? Inkscape::Util::read_number(value) : 0;

            if (read_num != this->offset) {
                this->offset = read_num;
                this->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        default:
        	SPObject::set(key, value);
            break;
    }
}

/**
 * Receives update notifications.
 */
void SPFeFuncNode::update(SPCtx *ctx, guint flags) {
    std::cout << "SPFeFuncNode::update" << std::endl;
    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        this->readAttr(SPAttr::TYPE);
        this->readAttr(SPAttr::TABLEVALUES);
        this->readAttr(SPAttr::SLOPE);
        this->readAttr(SPAttr::INTERCEPT);
        this->readAttr(SPAttr::AMPLITUDE);
        this->readAttr(SPAttr::EXPONENT);
        this->readAttr(SPAttr::OFFSET);
    }

    SPObject::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* SPFeFuncNode::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
    std::cout << "SPFeFuncNode::write" << std::endl;
    if (!repr) {
        repr = this->getRepr()->duplicate(doc);
    }

    SPObject::write(doc, repr, flags);

    return repr;
}

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
