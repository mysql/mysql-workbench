/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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
