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
#ifndef _WIZARD_OBJECT_FILTER_PAGE_H_
#define _WIZARD_OBJECT_FILTER_PAGE_H_

#include "grt_wizard_form.h"
#include "grtdb/db_object_master_filter.h"
#include "grtui/grtdb_object_filter.h"

#include "mforms/scrollpanel.h"
#include "mforms/box.h"
#include "mforms/label.h"

namespace grtui {
  class WBPUBLICBACKEND_PUBLIC_FUNC WizardObjectFilterPage : public WizardPage {
  public:
    WizardObjectFilterPage(WizardForm *form, const char *name);
    virtual ~WizardObjectFilterPage();

    grtui::DBObjectFilterFrame *add_filter(const std::string &class_name, const std::string &caption_fmt,
                                           bec::GrtStringListModel *model, bec::GrtStringListModel *excl_model,
                                           bool *enabled_flag);

    void reset();

    // void reload_stored_filte_set();

    //

  protected:
    mforms::ScrollPanel _scroll_panel;
    mforms::Box _box;
    mforms::Label _top_label;
    bec::DBObjectMasterFilterBE _filter_be;

    std::vector<grtui::DBObjectFilterFrame *> _filters;
  };
};

#endif /* _WIZARD_OBJECT_FILTER_PAGE_H_ */
