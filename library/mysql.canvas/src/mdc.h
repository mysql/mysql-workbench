/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

// MySQL Diagramming Canvas

#include "mdc_common.h"
#include "mdc_algorithms.h"

#include "mdc_box.h"
#include "mdc_image_manager.h"
#include "mdc_button.h"
#include "mdc_canvas_item.h"
#include "mdc_canvas_view.h"
#include "mdc_figure.h"
#include "mdc_image.h"
#include "mdc_item_handle.h"
#include "mdc_box_handle.h"
#include "mdc_vertex_handle.h"
#include "mdc_layer.h"
#include "mdc_back_layer.h"
#include "mdc_interaction_layer.h"
#include "mdc_layouter.h"
#include "mdc_line.h"
#include "mdc_straight_line_layouter.h"
#include "mdc_orthogonal_line_layouter.h"
#include "mdc_polygon.h"
#include "mdc_rectangle.h"
#include "mdc_text.h"
#include "mdc_icon_text.h"
#include "mdc_selection.h"
#include "mdc_group.h"
#include "mdc_area_group.h"
#include "mdc_events.h"
#include "mdc_magnet.h"
#include "mdc_bounds_magnet.h"
#include "mdc_box_side_magnet.h"
#include "mdc_connector.h"
