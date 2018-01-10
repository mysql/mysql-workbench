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


#include "grtui/gui_plugin_base.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <gtkmm.h>
#pragma GCC diagnostic pop
#include "wb_printing.h"
#include "mdc_canvas_view_printing.h"
#include "wbcanvas/model_diagram_impl.h"
#include "grts/structs.workbench.h"
#include "gtk_helpers.h"
#include <stdio.h>
#include <gtk/gtk.h>
#include "base/string_utilities.h"

using base::strfmt;

namespace linux_printing {

  static void update_gtk_page_setup_from_grt(Glib::RefPtr<Gtk::PageSetup> &setup, const app_PageSettingsRef &settings,
                                             bool skip_margins) {
    Gtk::PaperSize paper_size(base::replaceString(settings->paperType()->name().c_str(), "-", "_"));

    setup->set_bottom_margin((skip_margins ? 0 : 1) * settings->marginBottom(), Gtk::UNIT_MM);
    setup->set_left_margin((skip_margins ? 0 : 1) * settings->marginLeft(), Gtk::UNIT_MM);
    setup->set_right_margin((skip_margins ? 0 : 1) * settings->marginRight(), Gtk::UNIT_MM);
    setup->set_top_margin((skip_margins ? 0 : 1) * settings->marginTop(), Gtk::UNIT_MM);

    if (settings->paperType().is_valid())
      setup->set_paper_size(paper_size);

    if (*settings->orientation() == "landscape")
      setup->set_orientation(Gtk::PAGE_ORIENTATION_LANDSCAPE);
    else
      setup->set_orientation(Gtk::PAGE_ORIENTATION_PORTRAIT);
  }

  //==============================================================================
  class WBPageSetup {
  public:
    WBPageSetup(const app_PageSettingsRef &ps);

    void propagate_print_settings_to_grt_tree();
    virtual void run_setup();

    Glib::RefPtr<Gtk::PageSetup> _page_setup;
    Glib::RefPtr<Gtk::PrintSettings> _print_settings;

  private:
    static app_PageSettingsRef _app_page_settings;
  };

  app_PageSettingsRef WBPageSetup::_app_page_settings;

  //------------------------------------------------------------------------------
  WBPageSetup::WBPageSetup(const app_PageSettingsRef &ps) {
    _app_page_settings = ps;
    if (!_page_setup)
      _page_setup = Gtk::PageSetup::create();

    if (!_print_settings)
      _print_settings = Gtk::PrintSettings::create();
  }

  //------------------------------------------------------------------------------
  void WBPageSetup::run_setup() {
    if (_app_page_settings.is_valid()) {
      update_gtk_page_setup_from_grt(_page_setup, _app_page_settings, false);
    }

    if (get_mainwindow() == nullptr)
      throw std::runtime_error("Need main window to continue.");

    Glib::RefPtr<Gtk::PageSetup> new_page_setup =
      Gtk::run_page_setup_dialog(*get_mainwindow(), _page_setup, _print_settings);
    _page_setup = new_page_setup;

    propagate_print_settings_to_grt_tree();
  }

  //------------------------------------------------------------------------------
  void WBPageSetup::propagate_print_settings_to_grt_tree() {
    std::string page_orientation_as_str;

    // Set orientation
    Gtk::PageOrientation page_orient = _page_setup->get_orientation();
    if (page_orient == Gtk::PAGE_ORIENTATION_PORTRAIT)
      page_orientation_as_str = "portrait";
    else if (page_orient == Gtk::PAGE_ORIENTATION_LANDSCAPE)
      page_orientation_as_str = "landscape";
    else {
      g_message("Unsupported page orientation. Setting page orientation to portrait");
      page_orientation_as_str = "portrait";
    }
    _app_page_settings->orientation(page_orientation_as_str);

    // Set paper type
    Gtk::PaperSize gtk_paper_size = _page_setup->get_paper_size();
    app_PaperTypeRef paper_type = _app_page_settings->paperType();

    const std::string paper_name = base::replaceString(gtk_paper_size_get_name(gtk_paper_size.gobj()), "_", "-");

    grt::ListRef<app_PaperType> paper_types(
      grt::ListRef<app_PaperType>::cast_from(grt::GRT::get()->get("/wb/options/paperTypes")));

    app_PaperTypeRef ptype(app_PaperTypeRef::cast_from(grt::find_named_object_in_list(paper_types, paper_name)));

    _app_page_settings->marginBottom(gtk_paper_size.get_default_bottom_margin(Gtk::UNIT_MM));
    _app_page_settings->marginLeft(gtk_paper_size.get_default_left_margin(Gtk::UNIT_MM));
    _app_page_settings->marginRight(gtk_paper_size.get_default_right_margin(Gtk::UNIT_MM));
    _app_page_settings->marginTop(gtk_paper_size.get_default_top_margin(Gtk::UNIT_MM));

    if (ptype.is_valid())
      _app_page_settings->paperType(ptype);
    else
      g_warning("Unknown paper size selected in GTK Page Setup dialog: %s", paper_name.c_str());
  }

  //==============================================================================
  class WBPrintOperation : public Gtk::PrintOperation {
  public:
    WBPrintOperation(const model_DiagramRef &diagram);
    static Glib::RefPtr<WBPrintOperation> create(const model_DiagramRef &diagram);
    virtual ~WBPrintOperation();

  protected:
    virtual void on_begin_print(const Glib::RefPtr<Gtk::PrintContext> &ctx);
    virtual void on_draw_page(const Glib::RefPtr<Gtk::PrintContext> &ctx, int page_nr);
    virtual void on_done(Gtk::PrintOperationResult result);

  private:
    model_DiagramRef _diagram;
    mdc::CanvasViewExtras *_printer;
    int _xpages;
    int _ypages;

    Glib::RefPtr<Gtk::PageSetup> _page_setup;
    Glib::RefPtr<Gtk::PrintSettings> _print_settings;
  };

  //------------------------------------------------------------------------------
  WBPrintOperation::WBPrintOperation(const model_DiagramRef &diagram)
    : _diagram(diagram), _printer(0), _xpages(0), _ypages(0) {
    _page_setup = Gtk::PageSetup::create();
    _print_settings = Gtk::PrintSettings::create();
  }

  //------------------------------------------------------------------------------
  Glib::RefPtr<WBPrintOperation> WBPrintOperation::create(const model_DiagramRef &diagram) {
    return Glib::RefPtr<WBPrintOperation>(new WBPrintOperation(diagram));
  }

  //------------------------------------------------------------------------------
  WBPrintOperation::~WBPrintOperation() {
    delete _printer;
  }

  //------------------------------------------------------------------------------
  void WBPrintOperation::on_begin_print(const Glib::RefPtr<Gtk::PrintContext> &ctx) {
    app_PageSettingsRef pageSettings(workbench_DocumentRef::cast_from(grt::GRT::get()->get("/wb/doc"))->pageSettings());
    app_PaperTypeRef paperType(pageSettings->paperType());

    update_gtk_page_setup_from_grt(_page_setup, pageSettings, true);

    Gtk::PaperSize gpaperSize(_page_setup->get_paper_size());

    set_default_page_setup(_page_setup);
    set_print_settings(_print_settings);
    set_track_print_status();

    // page settings in cairo coordinates
    float paperWidth = (*paperType->width() * *pageSettings->scale());
    float paperHeight = (*paperType->height() * *pageSettings->scale());
    float marginLeft = (*pageSettings->marginLeft() * *pageSettings->scale());
    float marginRight = (*pageSettings->marginRight() * *pageSettings->scale());
    float marginTop = (*pageSettings->marginTop() * *pageSettings->scale());
    float marginBottom = (*pageSettings->marginBottom() * *pageSettings->scale());

    if (pageSettings->orientation() == "landscape") {
      std::swap(paperWidth, paperHeight);
      std::swap(marginLeft, marginTop);
      std::swap(marginRight, marginBottom);
    }

    // size of the printable area in cairo coords
    base::Size pageSize;
    pageSize.width = paperWidth - marginLeft - marginRight;
    pageSize.height = paperHeight - marginTop - marginBottom;

    _printer = new mdc::CanvasViewExtras(_diagram->get_data()->get_canvas_view());

    // margins are already added by the system
    _printer->set_page_margins(marginTop, marginLeft, marginBottom, marginRight);
    _printer->set_paper_size(paperWidth, paperHeight);
    //    _printer->set_orientation(pageSettings->orientation()=="landscape"?mdc::Landscape:mdc::Portrait);
    //

    _printer->set_print_border(true);

    set_n_pages(wbprint::getPageCount(_diagram));
    wbprint::getPageLayout(_diagram, _xpages, _ypages);
  }

  //------------------------------------------------------------------------------
  void WBPrintOperation::on_draw_page(const Glib::RefPtr<Gtk::PrintContext> &ctx, int page_nr) {
    Cairo::RefPtr<Cairo::Context> context = ctx->get_cairo_context();
    mdc::CairoCtx cairoctx(context->cobj());

    // scaling to transform from cairo coordinates to Cocoa coordinates
    double pwidth, pheight;
    _printer->get_paper_size(pwidth, pheight);
    float xscale = ctx->get_width() / pwidth;
    float yscale = ctx->get_height() / pheight;

    // scale has to be set here, ctx->get_width() returns some different value in on_begin_print()
    _printer->set_scale(xscale, yscale);

    _printer->render_page(&cairoctx, page_nr % _xpages, page_nr / _xpages);
  }

  //------------------------------------------------------------------------------
  void WBPrintOperation::on_done(Gtk::PrintOperationResult result) {
    delete _printer;
    _printer = 0;
    PrintOperation::on_done(result);
  }

  //==============================================================================
  class WBPrintingLinux : public GUIPluginBase {
  public:
    WBPrintingLinux(grt::Module *m, const grt::BaseListRef &args);
    virtual void execute();
    virtual void show_plugin();

  private:
    void on_print_done(Gtk::PrintOperationResult result, Glib::RefPtr<WBPrintOperation> &op);
    model_DiagramRef _diagram; //!< TODO: use currently selected diagram!
  };

  //------------------------------------------------------------------------------
  WBPrintingLinux::WBPrintingLinux(grt::Module *m, const grt::BaseListRef &args)
    : GUIPluginBase(m), _diagram(model_DiagramRef::cast_from(args.get(0))) {
  }

  //------------------------------------------------------------------------------
  void WBPrintingLinux::execute() {
  }

  //------------------------------------------------------------------------------
  void WBPrintingLinux::show_plugin() {
    try {
      if (get_mainwindow() == nullptr)
        throw std::runtime_error("Need main window to continue");

      Glib::RefPtr<WBPrintOperation> printer = WBPrintOperation::create(_diagram);

      printer->signal_done().connect(sigc::bind(sigc::mem_fun(this, &WBPrintingLinux::on_print_done), printer));
      /*Gtk::PrintOperationResult result = */ printer->run(Gtk::PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                                           *(get_mainwindow()));
    } catch (const Gtk::PrintError &e) {
      g_message("Error while printing %s", e.what().c_str());
    }
  }

  //------------------------------------------------------------------------------
  void WBPrintingLinux::on_print_done(Gtk::PrintOperationResult result, Glib::RefPtr<WBPrintOperation> &op) {
    if (result == Gtk::PRINT_OPERATION_RESULT_ERROR) {
      if (get_mainwindow() == nullptr)
        throw std::runtime_error("Need main window to continue");

      Gtk::MessageDialog err_dlg(*get_mainwindow(), "Error printing document", false, Gtk::MESSAGE_ERROR,
                                 Gtk::BUTTONS_OK, true);
      err_dlg.run();
    } else if (result == Gtk::PRINT_OPERATION_RESULT_APPLY) {
      //_print_settings = op->get_print_settings();
    }
  }

} // end of namespace linux_printing

//------------------------------------------------------------------------------
extern "C" {
GUIPluginBase *createPrintDialog(grt::Module *m, const grt::BaseListRef &args) {
  // return new linux_printing::WBPrintingLinux(m, grtm, args);
  linux_printing::WBPrintingLinux lp(m, args);
  lp.show_plugin();
  return 0;
}
};

//------------------------------------------------------------------------------
extern "C" {
GUIPluginBase *createPrintPreviewDialog(grt::Module *m, const grt::BaseListRef &args) {
  g_message("print preview");
  //    linux_printing::WBPrintingLinux lp(m, grtm, args);
  //    lp.show_plugin();
  return 0;
}
};

//------------------------------------------------------------------------------
extern "C" {
GUIPluginBase *createPrintSetupDialog(grt::Module *m, const grt::BaseListRef &args) {
  workbench_DocumentRef doc(workbench_DocumentRef::cast_from(grt::GRT::get()->get("/wb/doc")));
  if (doc.is_valid()) {
    linux_printing::WBPageSetup ps(doc->pageSettings());

    ps.run_setup();
  }
  return 0;
}
};
