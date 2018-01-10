/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grts/structs.ui.h"

#include "grtpp_util.h"
#include "mforms_ObjectReference_impl.h"

#include <typeinfo>

#include "mforms/base.h"

#include "mforms/menubar.h"
#include "mforms/dockingpoint.h"

//================================================================================
// mforms_ObjectReference

grt::IntegerRef mforms_ObjectReference::valid() const {
  if (_data)
    return grt::IntegerRef(1);
  else
    return grt::IntegerRef(0);
}

grt::IntegerRef mforms_ObjectReference::isEqualTo(const grt::Ref<mforms_ObjectReference> &other) {
  if (_data == NULL && other->get_data() == NULL)
    return grt::IntegerRef(1);

  if (_data && other->get_data())
    return grt::IntegerRef(_data == other->get_data() ? 1 : 0);
  return grt::IntegerRef(1);
}

mforms::Object *mforms_from_grt(mforms_ObjectReferenceRef object) {
  if (!object.is_valid() || !*object->valid())
    return 0;
  return object->get_data();
}

static void release_object(mforms::Object *object) {
  if (object)
    object->release();
}

mforms_ObjectReferenceRef mforms_to_grt(mforms::Object *object, const std::string &type_name) {
  if (object) {
    // view is not necessarily managed, in some cases the view must be deleted by the caller
    // assert(object->is_managed());

    mforms_ObjectReferenceRef ref(grt::Initialized);
    object->retain();
    ref->set_data(object, object->is_managed() ? release_object : NULL);
    ref->type(grt::StringRef(type_name.empty() ? grt::get_type_name(typeid(*object)) : type_name));
    return ref;
  }
  return mforms_ObjectReferenceRef();
}

mforms_ObjectReferenceRef mforms_to_grt(mforms::ContextMenu *menu) {
  return mforms_to_grt(menu, "ContextMenu");
}

mforms_ObjectReferenceRef mforms_to_grt(mforms::DockingPoint *dpoint) {
  return mforms_to_grt(dpoint, "DockingPoint");
}
