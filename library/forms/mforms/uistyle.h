/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

// Various constants for platform specific look & feel

// See related utility functions in utilities.h (such as add_ok_cancel_buttons())

#ifdef _MSC_VER

#define MF_WINDOW_PADDING 12
#define MF_PANEL_PADDING 12
#define MF_BUTTON_SPACING 8
#define MF_TABLE_COLUMN_SPACING 8
#define MF_TABLE_ROW_SPACING 12

#define MF_TEXT_SECTION_PADDING 4
#define MF_PANEL_SMALL_PADDING 6

#elif defined(__APPLE__)

#define MF_WINDOW_PADDING 20
#define MF_PANEL_PADDING 12
#define MF_BUTTON_SPACING 12
#define MF_TABLE_COLUMN_SPACING 8
#define MF_TABLE_ROW_SPACING 12

#define MF_TEXT_SECTION_PADDING 4
#define MF_PANEL_SMALL_PADDING 6

#else

#define MF_WINDOW_PADDING 12
#define MF_PANEL_PADDING 12
#define MF_BUTTON_SPACING 12
#define MF_TABLE_COLUMN_SPACING 8
#define MF_TABLE_ROW_SPACING 12

#define MF_TEXT_SECTION_PADDING 4
#define MF_PANEL_SMALL_PADDING 6

#endif
