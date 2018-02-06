/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "workbench/wb_context_ui.h"
#include "model/wb_model_diagram_form.h"
#include "model/wb_context_model.h"
#include "model/wb_history_tree.h"
#include "model/wb_diagram_options.h"
#include "model/wb_user_datatypes.h"
#include "model/wb_overview_physical.h"

#include "common/preferences_form.h"
#include "common/document_properties_form.h"

#include "GrtTemplates.h"
#include "WbCallbacks.h"
#include "DelegateWrapper.h"
#include "Overview.h"
#include "ModelDiagramFormWrapper.h"

namespace MySQL {
  namespace Workbench {

  public
    enum class Msg_type {
      MT_warning = grt::WarningMsg,
      MT_error = grt::ErrorMsg,
      MT_info = grt::InfoMsg,
      MT_progress = grt::ProgressMsg,
    };

    //--------------------------------------------------------------------------------------------------

  public
    ref class PageSettings {
    public:
      PageSettings()
        : paper_type(nullptr), margin_top(0), margin_bottom(0), margin_left(0), margin_right(0), orientation(nullptr) {
      }

      PageSettings(const app_PageSettingsRef& settings)
        : paper_type(CppStringToNative(settings->paperType().is_valid() ? settings->paperType()->name() : "")),
          margin_top(settings->marginTop()),
          margin_bottom(settings->marginBottom()),
          margin_left(settings->marginLeft()),
          margin_right(settings->marginRight()),
          orientation(CppStringToNative(settings->orientation())) {
      }

      void update_object(grt::ListRef<app_PaperType> paperTypes, app_PageSettingsRef settings) {
        settings->paperType(grt::find_named_object_in_list(paperTypes, NativeToCppString(paper_type)));

        settings->marginTop(grt::DoubleRef(margin_top));
        settings->marginBottom(grt::DoubleRef(margin_bottom));
        settings->marginLeft(grt::DoubleRef(margin_left));
        settings->marginRight(grt::DoubleRef(margin_right));

        settings->orientation(NativeToCppString(orientation));
      }

      String ^ paper_type;
      double margin_top;
      double margin_bottom;
      double margin_left;
      double margin_right;
      String ^ orientation;
    };

  public
    ref class PaperSize {
    public:
      PaperSize(const wb::WBPaperSize& paperSize)
        : name(CppStringToNative(paperSize.name)),
          caption(CppStringToNative(paperSize.caption)),
          margins_set(paperSize.margins_set),
          margin_top(paperSize.margin_top),
          margin_bottom(paperSize.margin_bottom),
          margin_left(paperSize.margin_left),
          margin_right(paperSize.margin_right),
          width(paperSize.width),
          height(paperSize.height),
          description(CppStringToNative(paperSize.description)) {
      }

      String ^ name;
      String ^ caption;
      double width;
      double height;
      bool margins_set;
      double margin_top;
      double margin_bottom;
      double margin_left;
      double margin_right;
      String ^ description;
    };

    // ----------------------------------------------------------------------------

  public
    ref class WbOptions {
    private:
      wb::WBOptions* inner;

    public:
      WbOptions(String ^ baseDir, String ^ userDir, bool full_init);

      wb::WBOptions* get_unmanaged_object() {
        return inner;
      };
      bool parse_args(array<String ^> ^ args, String ^ app_path);

      void analyzeCommandLineArguments();

      property bool Verbose {
        bool get() {
          return !inner->verbose;
        }
      }

      property String ^ OpenAtStartup { String ^ get(); }

        property String ^
        OpenAtStartupType { String ^ get(); }
    };

    class WbContextUiHolder {
      std::shared_ptr<wb::WBContextUI> _wbCtxUi;

    public:
      WbContextUiHolder() : _wbCtxUi(wb::WBContextUI::get()) {
      }
    };

  public
    ref class WbContext {
      WbContextUiHolder* _wbContextUi;

      MySQL::Grt::GrtManager ^ manager;
      MySQL::Workbench::Overview ^ physical_overview;

      System::Collections::ArrayList open_editor_slot_wrappers;

      DelegateSlot0<void, void> ^ undo_delegate;
      DelegateSlot0<bool, bool> ^ can_undo_delegate;
      DelegateSlot0<void, void> ^ redo_delegate;
      DelegateSlot0<bool, bool> ^ can_redo_delegate;
      DelegateSlot0<void, void> ^ copy_delegate;
      DelegateSlot0<bool, bool> ^ can_copy_delegate;
      DelegateSlot0<void, void> ^ cut_delegate;
      DelegateSlot0<bool, bool> ^ can_cut_delegate;
      DelegateSlot0<void, void> ^ paste_delegate;
      DelegateSlot0<bool, bool> ^ can_paste_delegate;
      DelegateSlot0<void, void> ^ select_all_delegate;
      DelegateSlot0<bool, bool> ^ can_select_all_delegate;
      DelegateSlot0<void, void> ^ delete_delegate;
      DelegateSlot0<bool, bool> ^ can_delete_delegate;
      DelegateSlot0<void, void> ^ find_delegate;
      DelegateSlot0<bool, bool> ^ can_find_delegate;
      DelegateSlot0<void, void> ^ find_replace_delegate;
      DelegateSlot0<bool, bool> ^ can_find_replace_delegate;

    public:
      delegate void VoidStrUIFormDelegate(String ^ str1, MySQL::Base::UIForm ^ form);

    private:
      // void (string, bec::UIFrom)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate void VoidStrUIFormWrapperDelegate(
        const std::string& str1, std::shared_ptr<bec::UIForm> form);
      typedef void (*WbContext::VOID_STR_UIFORM_CB)(const std::string& str1, std::shared_ptr<bec::UIForm> form);

      // TODO: implement differently, simpler!
      // Creating these views needs a lot of specialized knowledge, which requires to include many heavy-weight
      // classes (e.g. SQL editor). This should not be handled by a general callback but by the classes
      // that need to create a view (can't the wrapper create the frontend view class?).
      VoidStrUIFormDelegate ^ create_main_form_view_delegate;
      VoidStrUIFormWrapperDelegate ^ create_main_form_view_wrapper_delegate;
      void set_create_main_form_view(MySQL::Workbench::WbFrontendCallbacks ^ cbacks, VoidStrUIFormDelegate ^ dt);
      void create_main_form_view_wrapper(const std::string& view_name, std::shared_ptr<bec::UIForm> form_be);

    public:
      WbContext(bool verbose);
      ~WbContext();

      bool is_commercial() {
        return wb::WBContextUI::get()->get_wb()->is_commercial();
      };

      bool init(MySQL::Workbench::WbFrontendCallbacks ^ callbacks, WbOptions ^ options,
                VoidStrUIFormDelegate ^ create_main_form_view);

      bool opengl_rendering_enforced() {
        return wb::WBContextUI::get()->get_wb()->opengl_rendering_enforced();
      }
      bool software_rendering_enforced() {
        return wb::WBContextUI::get()->get_wb()->software_rendering_enforced();
      }
      bool is_busy() {
        return bec::GRTManager::get()->get_dispatcher()->get_busy();
      }
      bool request_quit() {
        return wb::WBContextUI::get()->request_quit();
      }
      void perform_quit() {
        wb::WBContextUI::get()->perform_quit();
      }
      bool is_quitting() {
        return wb::WBContextUI::get()->is_quitting();
      }
      void finalize() {
        wb::WBContextUI::get()->finalize();
      }

      GrtManager ^ get_grt_manager();

      System::Windows::Forms::MenuStrip ^ menu_for_form(MySQL::Base::UIForm ^ form);
      System::Windows::Forms::MenuStrip ^ menu_for_appview(MySQL::Forms::AppViewDockContent ^ content);
      void validate_menu_for_form(MySQL::Base::UIForm ^ form);
      System::Windows::Forms::ToolStrip ^ toolbar_for_form(MySQL::Base::UIForm ^ form);
      System::Windows::Forms::Control ^ shared_secondary_sidebar();

      void focus_search_box(MySQL::Base::UIForm ^ form);
      String ^ get_search_string(MySQL::Base::UIForm ^ form);
      String ^ get_title();
      bool has_unsaved_changes();
      void open_document(String ^ file);
      bool save_changes();
      void flush_idle_tasks(bool force);
      double delay_for_next_timer();
      void flush_timers();

      // ----- Edit menu handling
      void validate_edit_menu() {
        wb::WBContextUI::get()->get_command_ui()->revalidate_edit_menu_items();
      }

      void edit_undo() {
        if (wb::WBContextUI::get()->get_active_main_form())
          wb::WBContextUI::get()->get_active_main_form()->undo();
      }

      bool edit_can_undo() {
        if (wb::WBContextUI::get()->get_active_main_form() &&
            wb::WBContextUI::get()->get_active_main_form()->can_undo())
          return true;
        return false;
      }

      void edit_redo() {
        if (wb::WBContextUI::get()->get_active_main_form())
          wb::WBContextUI::get()->get_active_main_form()->redo();
      }

      bool edit_can_redo() {
        if (wb::WBContextUI::get()->get_active_main_form() &&
            wb::WBContextUI::get()->get_active_main_form()->can_redo())
          return true;
        return false;
      }

      void edit_copy() {
        if (wb::WBContextUI::get()->get_active_form())
          wb::WBContextUI::get()->get_active_form()->copy();
      }

      bool edit_can_copy() {
        if (wb::WBContextUI::get()->get_active_form())
          return wb::WBContextUI::get()->get_active_form()->can_copy();
        return false;
      }

      void edit_cut() {
        if (wb::WBContextUI::get()->get_active_form())
          wb::WBContextUI::get()->get_active_form()->cut();
      }

      bool edit_can_cut() {
        if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_cut())
          return true;
        return false;
      }

      void edit_paste() {
        if (wb::WBContextUI::get()->get_active_form())
          wb::WBContextUI::get()->get_active_form()->paste();
      }

      bool edit_can_paste() {
        if (wb::WBContextUI::get()->get_active_form())
          return wb::WBContextUI::get()->get_active_form()->can_paste();
        return false;
      }

      void edit_select_all() {
        if (wb::WBContextUI::get()->get_active_form())
          wb::WBContextUI::get()->get_active_form()->select_all();
      }

      bool edit_can_select_all() {
        if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_select_all())
          return true;
        return false;
      }

      void edit_delete() {
        if (wb::WBContextUI::get()->get_active_form())
          wb::WBContextUI::get()->get_active_form()->delete_selection();
      }

      bool edit_can_delete() {
        if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_delete())
          return true;
        return false;
      }

      bool try_searching_diagram(String ^ text) {
        bec::UIForm* form = wb::WBContextUI::get()->get_active_main_form();
        if (form && dynamic_cast<wb::ModelDiagramForm*>(form)) {
          dynamic_cast<wb::ModelDiagramForm*>(form)->search_and_focus_object(NativeToCppString(text));
          return true;
        }
        return false;
      }

      typedef DelegateSlot0<void, void> CommandActionDelegate;
      typedef DelegateSlot0<bool, bool> CommandValidateDelegate;

      void set_edit_menu_delegates(CommandActionDelegate::ManagedDelegate ^ undo_delegate,
                                   CommandValidateDelegate::ManagedDelegate ^ can_undo_delegate,
                                   CommandActionDelegate::ManagedDelegate ^ redo_delegate,
                                   CommandValidateDelegate::ManagedDelegate ^ can_redo_delegate,
                                   CommandActionDelegate::ManagedDelegate ^ copy_delegate,
                                   CommandValidateDelegate::ManagedDelegate ^ can_copy_delegate,
                                   CommandActionDelegate::ManagedDelegate ^ cut_delegate,
                                   CommandValidateDelegate::ManagedDelegate ^ can_cut_delegate,
                                   CommandActionDelegate::ManagedDelegate ^ paste_delegate,
                                   CommandValidateDelegate::ManagedDelegate ^ can_paste_delegate,
                                   CommandActionDelegate::ManagedDelegate ^ select_all_delegate,
                                   CommandValidateDelegate::ManagedDelegate ^ can_select_all_delegate,
                                   CommandActionDelegate::ManagedDelegate ^ delete_delegate,
                                   CommandValidateDelegate::ManagedDelegate ^ can_delete_delegate,
                                   CommandActionDelegate::ManagedDelegate ^ find_delegate,
                                   CommandValidateDelegate::ManagedDelegate ^ can_find_delegate,
                                   CommandActionDelegate::ManagedDelegate ^ find_replace_delegate,
                                   CommandValidateDelegate::ManagedDelegate ^ can_find_replace_delegate) {
        this->undo_delegate = gcnew CommandActionDelegate(undo_delegate);
        this->can_undo_delegate = gcnew CommandValidateDelegate(can_undo_delegate);
        this->redo_delegate = gcnew CommandActionDelegate(redo_delegate);
        this->can_redo_delegate = gcnew CommandValidateDelegate(can_redo_delegate);
        this->copy_delegate = gcnew CommandActionDelegate(copy_delegate);
        this->can_copy_delegate = gcnew CommandValidateDelegate(can_copy_delegate);
        this->cut_delegate = gcnew CommandActionDelegate(cut_delegate);
        this->can_cut_delegate = gcnew CommandValidateDelegate(can_cut_delegate);
        this->paste_delegate = gcnew CommandActionDelegate(paste_delegate);
        this->can_paste_delegate = gcnew CommandValidateDelegate(can_paste_delegate);
        this->select_all_delegate = gcnew CommandActionDelegate(select_all_delegate);
        this->can_select_all_delegate = gcnew CommandValidateDelegate(can_select_all_delegate);
        this->delete_delegate = gcnew CommandActionDelegate(delete_delegate);
        this->can_delete_delegate = gcnew CommandValidateDelegate(can_delete_delegate);
        this->find_delegate = gcnew CommandActionDelegate(find_delegate);
        this->can_find_delegate = gcnew CommandValidateDelegate(can_find_delegate);
        this->find_replace_delegate = gcnew CommandActionDelegate(find_replace_delegate);
        this->can_find_replace_delegate = gcnew CommandValidateDelegate(can_find_replace_delegate);

        wb::WBContextUI::get()->get_command_ui()->add_builtin_command("undo", this->undo_delegate->get_slot(),
                                                                      this->can_undo_delegate->get_slot());
        wb::WBContextUI::get()->get_command_ui()->add_builtin_command("redo", this->redo_delegate->get_slot(),
                                                                      this->can_redo_delegate->get_slot());
        wb::WBContextUI::get()->get_command_ui()->add_builtin_command("copy", this->copy_delegate->get_slot(),
                                                                      this->can_copy_delegate->get_slot());
        wb::WBContextUI::get()->get_command_ui()->add_builtin_command("cut", this->cut_delegate->get_slot(),
                                                                      this->can_cut_delegate->get_slot());
        wb::WBContextUI::get()->get_command_ui()->add_builtin_command("paste", this->paste_delegate->get_slot(),
                                                                      this->can_paste_delegate->get_slot());
        wb::WBContextUI::get()->get_command_ui()->add_builtin_command(
          "selectAll", this->select_all_delegate->get_slot(), this->can_select_all_delegate->get_slot());
        wb::WBContextUI::get()->get_command_ui()->add_builtin_command("delete", this->delete_delegate->get_slot(),
                                                                      this->can_delete_delegate->get_slot());
        wb::WBContextUI::get()->get_command_ui()->add_builtin_command("find", this->find_delegate->get_slot(),
                                                                      this->can_find_delegate->get_slot());
        wb::WBContextUI::get()->get_command_ui()->add_builtin_command(
          "find_replace", this->find_replace_delegate->get_slot(), this->can_find_replace_delegate->get_slot());
      }

      // Plugins/command handling.
      void add_frontend_commands(List<String ^> ^ commands);
      void remove_frontend_commands(List<String ^> ^ commands);
      void activate_command(String ^ name);

      // Overview.
      Overview ^ get_physical_overview();
      Aga::Controls::Tree::TreeViewAdv ^ get_history_tree();
      Aga::Controls::Tree::TreeViewAdv ^ get_usertypes_tree();
      MySQL::Grt::GrtValueInspector ^
        get_inspector_for_selection(MySQL::Base::UIForm ^ form, [Out] List<String ^> ^ % items);
      bool are_lists_equal(GrtValue ^ v1, GrtValue ^ v2);
      String ^ get_description_for_selection(MySQL::Base::UIForm ^ form, [Out] GrtValue ^ % activeObjList,
                                             [Out] List<String ^> ^ % items);
      String ^ get_description_for_selection([Out] GrtValue ^ % activeObjList, [Out] List<String ^> ^ % items);
      void set_description_for_selection(GrtValue ^ activeObjList, String ^ val);
      ::MySQL::Workbench::ModelDiagramFormWrapper ^ get_diagram_form_for_diagram(String ^ id);
      void set_active_form(MySQL::Base::UIForm ^ uiform);
      void set_active_form_from_appview(MySQL::Forms::AppViewDockContent ^ form);
      String ^ get_active_context();
      void close_gui_plugin(IntPtr handle);
      void execute_plugin(String ^ name);
      void report_bug(String ^ errorInfo);

      // State support.
      String ^ read_state(String ^ name, String ^ domain, String ^ default_value);
      int read_state(String ^ name, String ^ domain, const int default_value);
      double read_state(String ^ name, String ^ domain, const double default_value);
      bool read_state(String ^ name, String ^ domain, const bool default_value);

      void save_state(String ^ name, String ^ domain, String ^ value);
      void save_state(String ^ name, String ^ domain, const int value);
      void save_state(String ^ name, String ^ domain, const double value);
      void save_state(String ^ name, String ^ domain, const bool value);

      // Preferences support.
      String ^ read_option_value(String ^ model, String ^ key, String ^ default_value);

      // Paper.
      List<PaperSize ^> ^
        get_paper_sizes() {
          return MySQL::Grt::CppListToObjectList<::wb::WBPaperSize, PaperSize>(
            wb::WBContextUI::get()->get_paper_sizes(false));
        }

        PageSettings
        ^
        get_page_settings() {
          app_PageSettingsRef settings(wb::WBContextUI::get()->get_page_settings());
          if (settings.is_valid())
            return gcnew PageSettings(settings);
          return nullptr;
        }

        void set_page_settings(PageSettings ^ settings) {
        settings->update_object(wb::WBContextUI::get()->get_wb()->get_root()->options()->paperTypes(),
                                wb::WBContextUI::get()->get_page_settings());
        wb::WBContextUI::get()->get_wb()->get_model_context()->update_page_settings();
      }

      /**
       * To be called by the front end once the main form is ready.
       */
      void finished_loading(WbOptions ^ options) {
        wb::WBContextUI::get()->init_finish(options->get_unmanaged_object());
      }

      void close_document_finish() {
        wb::WBContextUI::get()->get_wb()->close_document_finish();

        // Explicitly delete the overview object to avoid garbage collection to kick in after
        // our internal (non-managed) objects are gone already.
        delete physical_overview;
        physical_overview = nullptr;
      }

      void new_model_finish() {
        wb::WBContextUI::get()->get_wb()->new_model_finish();
      }

      String ^ get_filename() { return CppStringToNative(wb::WBContextUI::get()->get_wb()->get_filename()); }

        void mainform_activated() {
        mforms::Form::main_form()->activated();
      }
      void mainform_deactivated() {
        mforms::Form::main_form()->deactivated();
      }
      bool mainform_active() {
        return mforms::Form::main_form()->is_active();
      }
    };

  public
    ref class DiagramOptionsBE {
      wb::DiagramOptionsBE* inner;

      MySQL::Grt::DelegateSlot0<void, void> ^ _delegate;

    public:
      DiagramOptionsBE(WindowsGDICanvasView ^ view, WbContext ^ wbContext,
                       MySQL::Grt::DelegateSlot0<void, void>::ManagedDelegate ^ deleg) {
        _delegate = gcnew MySQL::Grt::DelegateSlot0<void, void>(deleg);

        inner = new wb::DiagramOptionsBE(
          view->get_unmanaged_object(),
          wb::WBContextUI::get()->get_wb()->get_model_context()->get_active_model_diagram(true),
          wb::WBContextUI::get()->get_wb());

        inner->scoped_connect(inner->signal_changed(), _delegate->get_slot());
      }

      ~DiagramOptionsBE() {
        delete _delegate;
        delete inner;
      }

      String ^ get_name() { return CppStringToNative(inner->get_name()); }

        void set_name(String ^ name) {
        inner->set_name(NativeToCppString(name));
      }

      int get_xpages() {
        return inner->get_xpages();
      }

      int get_ypages() {
        return inner->get_ypages();
      }

      void set_xpages(int c) {
        inner->set_xpages(c);
      }

      void set_ypages(int c) {
        inner->set_ypages(c);
      }

      void commit() {
        inner->commit();
      }

      void update_size() {
        inner->update_size();
      }
    };

  } // namespace Workbench
} // namespace MySQL
