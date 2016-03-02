/*
 * Copyright (c) 2013, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include <grts/structs.ui.h>

#include <python_context.h>

GRT_STRUCTS_UI_PUBLIC grt::AutoPyObject pyobject_from_grt(grt_PyObjectRef object);
GRT_STRUCTS_UI_PUBLIC grt_PyObjectRef pyobject_to_grt(grt::AutoPyObject object);
GRT_STRUCTS_UI_PUBLIC grt_PyObjectRef pyobject_to_grt(PyObject *object);

void pyobject_initialize();
