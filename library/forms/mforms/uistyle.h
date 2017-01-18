/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

// Various constants for platform specific look & feel

// See related utility functions in utilities.h (such as add_ok_cancel_buttons())

#ifdef _WIN32

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
