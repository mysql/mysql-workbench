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

#ifndef _WB_PRINTING_H_
#define _WB_PRINTING_H_

#include "grts/structs.model.h"

#ifdef _WIN32
#include <windows.h>

#ifdef WBPLUGINPRINTINGBE_EXPORTS
#define WBPRINTINGBE_PUBLIC_FUNC __declspec(dllexport)
#else
#define WBPRINTINGBE_PUBLIC_FUNC __declspec(dllimport)
#endif

#else // !_WIN32

#define WBPRINTINGBE_PUBLIC_FUNC
#endif

namespace wbprint {

#ifdef _WIN32

  int WBPRINTINGBE_PUBLIC_FUNC printPageHDC(model_DiagramRef view, int page, HDC hdc, int width, int height);

#endif

  int WBPRINTINGBE_PUBLIC_FUNC getPageCount(model_DiagramRef view);
  void WBPRINTINGBE_PUBLIC_FUNC getPageLayout(model_DiagramRef view, int &xpages, int &ypages);

  app_PageSettingsRef WBPRINTINGBE_PUBLIC_FUNC getPageSettings(model_DiagramRef diagram);
};

#endif
