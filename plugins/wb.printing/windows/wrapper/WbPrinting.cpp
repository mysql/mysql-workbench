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

#include "WbPrinting.h"
#include "GrtTemplates.h"

using namespace MySQL::GUI::Workbench::Plugins;

using namespace System;
using namespace System::Collections::Generic;
using namespace MySQL::Grt;

//--------------------------------------------------------------------------------------------------

int Printing::getPageCount(MySQL::Grt::GrtValue ^ view) {
  return wbprint::getPageCount(
    model_DiagramRef::cast_from(grt::BaseListRef::cast_from(view->get_unmanaged_object()).get(0)));
}

//--------------------------------------------------------------------------------------------------

int Printing::printPageHDC(MySQL::Grt::GrtValue ^ view, int page, IntPtr ^ hdc, int width, int height) {
  return wbprint::printPageHDC(
    model_DiagramRef::cast_from(grt::BaseListRef::cast_from(view->get_unmanaged_object()).get(0)), page,
    (HDC)hdc->ToPointer(), width, height);
}

//--------------------------------------------------------------------------------------------------

Dictionary<String ^, Object ^> ^ Printing::getPaperSettings(MySQL::Grt::GrtValue ^ value) {
  app_PageSettingsRef settings = wbprint::getPageSettings(
    model_DiagramRef::cast_from(grt::BaseListRef::cast_from(value->get_unmanaged_object()).get(0)));
  app_PaperTypeRef paperType = settings->paperType();

  Dictionary<String ^, Object ^> ^ result = gcnew Dictionary<String ^, Object ^>();

  result["caption"] = CppStringToNativeRaw(paperType->caption());
  result["height"] = *paperType->height(); // As double.
  result["width"] = *paperType->width();   // As double.
  result["marginTop"] = (int)*paperType->marginTop();
  result["marginBottom"] = (int)*paperType->marginBottom();
  result["marginLeft"] = (int)*paperType->marginLeft();
  result["marginRight"] = (int)*paperType->marginRight();
  result["marginsSet"] = *paperType->marginsSet() != 0;
  result["orientation"] = CppStringToNativeRaw(settings->orientation());

  return result;
}

//--------------------------------------------------------------------------------------------------
