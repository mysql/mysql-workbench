/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WBVALIDATION_IF_H_
#define _WBVALIDATION_IF_H_

#include "grtpp_module_cpp.h"
#include "grts/structs.db.h"

// database object validation interface definition header

class WbValidationInterfaceImpl : public grt::InterfaceImplBase //, public grt::Validator
{
public:
  DECLARE_REGISTER_INTERFACE(WbValidationInterfaceImpl, DECLARE_INTERFACE_FUNCTION(grt::Validator::validate),
                             DECLARE_INTERFACE_FUNCTION(WbValidationInterfaceImpl::getValidationDescription));

  //// Call all validations
  virtual std::string getValidationDescription(const grt::ObjectRef& root) = 0;
};

#endif /* _WBVALIDATION_IF_H_ */
