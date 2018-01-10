/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "wf_canvas.h"

#include "../CanvasViewer.h"

using namespace System::Drawing;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;

//----------------- CanvasWrapper -----------------------------------------------------------------------

CanvasWrapper::CanvasWrapper(mforms::Canvas *canvas) : ViewWrapper(canvas) {
}

//--------------------------------------------------------------------------------------------------

bool CanvasWrapper::create(mforms::Canvas *backend) {
  CanvasWrapper *wrapper = new CanvasWrapper(backend);

  MySQL::GUI::Mdc::WindowsCanvasViewer ^ canvas =
    CanvasWrapper::Create<MySQL::GUI::Mdc::WindowsCanvasViewer>(backend, wrapper);
  canvas->CreateCanvasView(nullptr, true, false, false);
  return true;
}

//-------------------------------------------------------------------------------------------------

mdc::CanvasView *CanvasWrapper::canvas(mforms::Canvas *backend) {
  CanvasWrapper *wrapper = backend->get_data<CanvasWrapper>();
  MySQL::GUI::Mdc::WindowsCanvasViewer ^ canvas = wrapper->GetManagedObject<MySQL::GUI::Mdc::WindowsCanvasViewer>();

  return canvas->Canvas->get_unmanaged_object();
}

//--------------------------------------------------------------------------------------------------

void CanvasWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_canvas_impl.create = &CanvasWrapper::create;
  f->_canvas_impl.canvas = &CanvasWrapper::canvas;
}
