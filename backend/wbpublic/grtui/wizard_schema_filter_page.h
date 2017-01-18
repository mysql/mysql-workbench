/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#ifndef _WIZARD_SCHEMA_FILTER_PAGE_H_
#define _WIZARD_SCHEMA_FILTER_PAGE_H_

#include "grt_wizard_form.h"
#include "checkbox_list_control.h"

#include "mforms/imagebox.h"
#include "mforms/box.h"
#include "mforms/label.h"

namespace grtui {
  class WBPUBLICBACKEND_PUBLIC_FUNC WizardSchemaFilterPage : public WizardPage {
  public:
    WizardSchemaFilterPage(WizardForm *form, const char *name);

    virtual void enter(bool advancing);
    virtual void leave(bool advancing);

    virtual bool allow_next();

  protected:
    mforms::Box _header;
    mforms::ImageBox _image;
    mforms::Label _label;
    StringCheckBoxList _check_list;
  };
};

#endif /* _WIZARD_SCHEMA_FILTER_PAGE_H_ */
