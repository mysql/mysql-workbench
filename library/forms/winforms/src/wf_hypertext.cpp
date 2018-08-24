/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include "wf_base.h"
#include "wf_view.h"
#include "wf_hypertext.h"

#include "base/log.h"

using namespace System::Windows::Forms;

using namespace HtmlRenderer;
using namespace HtmlRenderer::Entities;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_WRAPPER)

ref class MformsHtmlPanel : public HtmlPanel {
public:
  mforms::HyperText *backend;

  virtual void OnLinkClicked(HtmlLinkClickedEventArgs ^args) override {
    __super ::OnLinkClicked(args);
    if (!args->Handled) {
      backend->handle_url_click(NativeToCppString(args->Link));
      args->Handled = true;
    }
  }

  //--------------------------------------------------------------------------------------------------
};

//----------------- Static functions ---------------------------------------------------------------

HyperTextWrapper::HyperTextWrapper(mforms::HyperText *backend) : ViewWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool HyperTextWrapper::create(mforms::HyperText *backend) {
  HyperTextWrapper *wrapper = new HyperTextWrapper(backend);

  MformsHtmlPanel ^ panel = HyperTextWrapper::Create<MformsHtmlPanel>(backend, wrapper);
  panel->backend = backend;
  panel->AutoScroll = true;

  // Default font. Can be overridden in the HTML code as usual.
  try {
    panel->Font = ControlUtilities::GetFont("Tahoma", 8.25f, Drawing::FontStyle::Regular);
  } catch (System::ArgumentException ^ e) {
    // Argument exception pops up when the system cannot find the Regular font style (corrupt font).
    logError("HyperTextWrapper, setting default font failed. %s\n", e->Message);
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

void HyperTextWrapper::set_markup_text(mforms::HyperText *backend, const std::string &text) {
  HtmlPanel ^ panel = HyperTextWrapper::GetManagedObject<HtmlPanel>(backend);
  panel->Text = CppStringToNative(text);
}

//--------------------------------------------------------------------------------------------------

void HyperTextWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_hypertext_impl.create = &HyperTextWrapper::create;
  f->_hypertext_impl.set_markup_text = &HyperTextWrapper::set_markup_text;
}

//--------------------------------------------------------------------------------------------------
