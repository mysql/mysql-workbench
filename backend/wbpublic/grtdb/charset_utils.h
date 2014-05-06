/* 
 * Copyright (c) 2007, 2010, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _CHARSET_UTILS_H_
#define _CHARSET_UTILS_H_

#include <string>
#include "wbpublic_public_interface.h"

WBPUBLICBACKEND_PUBLIC_FUNC const std::string& get_cs_def_collation(std::string cs_name);
WBPUBLICBACKEND_PUBLIC_FUNC const std::string& get_collation_cs(std::string collation_name);

#endif // _CHARSET_UTILS_H_
