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

#include "grts/structs.model.h"

#ifdef _MSC_VER
#include <windows.h>

#ifdef WBPLUGINPRINTINGBE_EXPORTS
#define WBPRINTINGBE_PUBLIC_FUNC __declspec(dllexport)
#else
#define WBPRINTINGBE_PUBLIC_FUNC __declspec(dllimport)
#endif

#else // !_MSC_VER

#define WBPRINTINGBE_PUBLIC_FUNC
#endif

namespace wbprint {

#ifdef _MSC_VER

  int WBPRINTINGBE_PUBLIC_FUNC printPageHDC(model_DiagramRef view, int page, HDC hdc, int width, int height);

#endif

  int WBPRINTINGBE_PUBLIC_FUNC getPageCount(model_DiagramRef view);
  void WBPRINTINGBE_PUBLIC_FUNC getPageLayout(model_DiagramRef view, int &xpages, int &ypages);

  app_PageSettingsRef WBPRINTINGBE_PUBLIC_FUNC getPageSettings(model_DiagramRef diagram);
};
