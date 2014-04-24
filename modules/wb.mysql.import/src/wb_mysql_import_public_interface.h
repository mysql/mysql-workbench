/*
* Copyright (c) 2004, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WB_MYSQL_IMPORT_PUBLIC_INTERFACE_H_
#define _WB_MYSQL_IMPORT_PUBLIC_INTERFACE_H_

// STATIC_WB_MYSQL_WBM_IMPORT is used for tut tests, which
// links these classes statically

#if defined(_WIN32) && !defined(STATIC_WB_MODULE_IMPORT)

#ifdef WB_MYSQL_IMPORT_WBM_EXPORTS
#define WB_MYSQL_IMPORT_WBM_PUBLIC_FUNC __declspec(dllexport)
#else
#define WB_MYSQL_IMPORT_WBM_PUBLIC_FUNC __declspec(dllimport)
#endif

#else
#define WB_MYSQL_IMPORT_WBM_PUBLIC_FUNC
#endif

#endif // _WB_MYSQL_IMPORT_PUBLIC_INTERFACE_H_

