/* 
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include <string>

#include "../stub_base.h"

using namespace mforms;
using namespace mforms::stub;

@interface ReferenceWrapper: NSObject
@property ObjectWrapper *reference;
@end

@implementation ReferenceWrapper
@end

//--------------------------------------------------------------------------------------------------

ObjectWrapper::ObjectWrapper(mforms::Object *object)
  : owner(object)
{
  ReferenceWrapper *wrapper = [ReferenceWrapper new];
  wrapper.reference = this;
  object->set_data(wrapper);
}

//--------------------------------------------------------------------------------------------------

ObjectWrapper::~ObjectWrapper()
{
  // TODO: Check if we need to free memory here?
}

//--------------------------------------------------------------------------------------------------

ObjectWrapper* ObjectWrapper::getData(mforms::Object *backend)
{
  ReferenceWrapper *wrapper = backend->get_data();
  return wrapper.reference;
}
