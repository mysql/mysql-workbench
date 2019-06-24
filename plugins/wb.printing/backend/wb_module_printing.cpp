/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "grt.h"

#include "grts/structs.app.h"
#include "grts/structs.workbench.h"
#include "grts/structs.model.h"

#include "wbcanvas/model_diagram_impl.h"

#include "wb_module_printing.h"

#include "mdc_canvas_view_printing.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"

#ifdef _MSC_VER
#define FRONTEND_LIBNAME(obj, windows_dll, linux_so, osx_dylib) obj->moduleName(windows_dll)
#elif defined(__APPLE__)
#define FRONTEND_LIBNAME(obj, windows_dll, linux_so, osx_dylib) obj->moduleName(osx_dylib)
#else
#define FRONTEND_LIBNAME(obj, windows_dll, linux_so, osx_dylib) obj->moduleName(linux_so)
#endif

#define def_export_plugin(aName, aCaption, aDialogCaption, aExtensions) \
  {                                                                     \
    app_PluginRef plugin(grt::Initialized);                             \
    app_PluginObjectInputRef pdef(grt::Initialized);                    \
    plugin->name("wb.print." aName);                                    \
    plugin->caption(aCaption);                                          \
    plugin->moduleName("WbPrinting");                                   \
    plugin->moduleFunctionName(aName);                                  \
    plugin->pluginType("normal");                                       \
    plugin->showProgress(1);                                            \
    pdef->name("activeDiagram");                                        \
    pdef->objectStructName(model_Diagram::static_class_name());         \
    pdef->owner(plugin);                                                \
    plugin->inputValues().insert(pdef);                                 \
    app_PluginFileInputRef pdef2(grt::Initialized);                     \
    ;                                                                   \
    pdef2->owner(plugin);                                               \
    pdef2->dialogTitle(aDialogCaption);                                 \
    pdef2->dialogType("save");                                          \
    pdef2->fileExtensions(aExtensions);                                 \
    plugin->inputValues().insert(pdef2);                                \
    plugin->groups().insert("Application/Workbench");                   \
    list.insert(plugin);                                                \
  }

WbPrintingImpl::WbPrintingImpl(grt::CPPModuleLoader *ldr) : super(ldr) {
}

grt::ListRef<app_Plugin> WbPrintingImpl::getPluginInfo() {
  grt::ListRef<app_Plugin> list(true);

  def_export_plugin("printToPDFFile", "Print Diagram to a PDF File", "Print to PDF", "PDF Files (*.pdf)|*.pdf");
  def_export_plugin("printToPSFile", "Print Diagram to a PS File", "Print to PS", "PostScript Files (*.ps)|*.ps");

  {
    app_PluginRef plugin(grt::Initialized);

    FRONTEND_LIBNAME(plugin, ".\\wb.printing.wbp.fe.dll", "wb.printing.wbp.so", "wb.printing.mwbplugin");
    plugin->pluginType("gui");
    plugin->moduleFunctionName("PrintDialog");
    plugin->rating(100);
    plugin->name("wb.print.print");
    plugin->caption(_("Print Diagram"));
    app_PluginObjectInputRef pdef(grt::Initialized);
    pdef->name("activeDiagram");
    pdef->objectStructName(model_Diagram::static_class_name());
    pdef->owner(plugin);
    plugin->inputValues().insert(pdef);
    plugin->groups().insert("Model/Printing");
    plugin->showProgress(2);

    list.insert(plugin);
  }

  {
    app_PluginRef plugin(grt::Initialized);

    FRONTEND_LIBNAME(plugin, ".\\wb.printing.wbp.fe.dll", "wb.printing.wbp.so", "");
    plugin->pluginType("gui");
    plugin->moduleFunctionName("PrintPreviewDialog");
    plugin->rating(100);
    plugin->name("wb.print.printPreview");
    plugin->caption(_("Print Preview"));
    app_PluginObjectInputRef pdef(grt::Initialized);
    pdef->name("activeDiagram");
    pdef->objectStructName(model_Diagram::static_class_name());
    pdef->owner(plugin);
    plugin->inputValues().insert(pdef);
    plugin->groups().insert("Model/Printing");
    plugin->showProgress(2);

    list.insert(plugin);
  }

  {
    app_PluginRef plugin(grt::Initialized);

    FRONTEND_LIBNAME(plugin, "", "wb.printing.wbp.so", "");
    plugin->pluginType("gui");
    plugin->moduleFunctionName("PrintSetupDialog");
    plugin->rating(100);
    plugin->name("wb.print.setup");
    plugin->caption(_("Page Setup"));
    plugin->groups().insert("Model/Printing");
    plugin->showProgress(2);

    list.insert(plugin);
  }

  return list;
}

int WbPrintingImpl::printToPDFFile(model_DiagramRef view, const std::string &path) {
  mdc::CanvasViewExtras extras(view->get_data()->get_canvas_view());

  app_PageSettingsRef page(workbench_DocumentRef::cast_from(grt::GRT::get()->get("/wb/doc"))->pageSettings());

  extras.set_page_margins(page->marginTop(), page->marginLeft(), page->marginBottom(), page->marginRight());
  extras.set_paper_size(page->paperType()->width(), page->paperType()->height());
  extras.set_orientation(page->orientation() == "landscape" ? mdc::Landscape : mdc::Portrait);
  extras.set_scale(page->scale());

  int pages = extras.print_to_pdf(path);

  return pages;
}

int WbPrintingImpl::printDiagramsToFile(grt::ListRef<model_Diagram> views, const std::string &path,
                                        const std::string &format, grt::DictRef options) {
  int pages = 0;
  base::FileHandle fh(path.c_str(), "wb");
  app_PageSettingsRef page(workbench_DocumentRef::cast_from(grt::GRT::get()->get("/wb/doc"))->pageSettings());
  int total_pages = 0;

  GRTLIST_FOREACH(model_Diagram, views, view) {
    mdc::CanvasViewExtras extras((*view)->get_data()->get_canvas_view());

    extras.set_page_margins(page->marginTop(), page->marginLeft(), page->marginBottom(), page->marginRight());
    extras.set_paper_size(page->paperType()->width(), page->paperType()->height());
    extras.set_orientation(page->orientation() == "landscape" ? mdc::Landscape : mdc::Portrait);
    extras.set_scale(page->scale());

    mdc::Count xc, yc;
    extras.get_page_counts(xc, yc);
    total_pages += xc * yc;
  }

  {
    std::unique_ptr<mdc::Surface> surf;

    GRTLIST_FOREACH(model_Diagram, views, view) {
      mdc::CanvasViewExtras extras((*view)->get_data()->get_canvas_view());

      extras.set_page_margins(page->marginTop(), page->marginLeft(), page->marginBottom(), page->marginRight());
      extras.set_paper_size(page->paperType()->width(), page->paperType()->height());
      extras.set_orientation(page->orientation() == "landscape" ? mdc::Landscape : mdc::Portrait);
      extras.set_scale(page->scale());

      if (!surf.get()) {
        if (format == "pdf")
          surf = std::unique_ptr<mdc::Surface>(extras.create_pdf_surface(fh));
        else if (format == "ps")
          surf = std::unique_ptr<mdc::Surface>(extras.create_ps_surface(fh));
        else
          throw std::invalid_argument("Invalid file format " + format);
      }

      std::string htext = options.get_string("header_text");
      base::replaceStringInplace(htext, "$diagram", (*view)->name());

      std::string ftext = options.get_string("footer_text");
      base::replaceStringInplace(ftext, "$diagram", (*view)->name());

      pages += extras.print_to_surface(surf.get(), htext, ftext, pages, total_pages);
    }
  }
  return pages;
}

int WbPrintingImpl::printToPSFile(model_DiagramRef view, const std::string &path) {
  mdc::CanvasViewExtras extras(view->get_data()->get_canvas_view());

  app_PageSettingsRef page(workbench_DocumentRef::cast_from(grt::GRT::get()->get("/wb/doc"))->pageSettings());

  extras.set_page_margins(page->marginTop(), page->marginLeft(), page->marginBottom(), page->marginRight());
  extras.set_paper_size(page->paperType()->width(), page->paperType()->height());
  extras.set_orientation(page->orientation() == "landscape" ? mdc::Landscape : mdc::Portrait);
  extras.set_scale(page->scale());

  int pages = extras.print_to_ps(path);

  return pages;
}

int WbPrintingImpl::printToPrinter(model_DiagramRef view, const std::string &printer) {
  return 0;
}

GRT_MODULE_ENTRY_POINT(WbPrintingImpl);
