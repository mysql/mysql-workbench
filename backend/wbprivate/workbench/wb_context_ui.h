/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

// WB application functionality for UI

#include "wb_backend_public_interface.h"
#include "base/trackable.h"
#include "base/notifications.h"
#include "base/any.h"

#include "grts/structs.app.h"
#include "grts/structs.db.mgmt.h"
#include "grt/plugin_manager.h"
#include "mforms/home_screen_helpers.h"

namespace bec {
  class ValueTreeBE;
  class ValueInspectorBE;
  class BaseEditor;
  class UIForm;
};

namespace mdc {
  class CanvasView;
}

class GRTShellWindow;
class PluginInstallWindow;
class AddOnDownloadWindow;

namespace mforms {
  class HomeScreen;
  class XConnectionsSection;
  class ConnectionsSection;
  class DocumentsSection;
}

namespace wb {

  class WBContext;
  struct WBFrontendCallbacks;
  struct WBOptions;
  struct WBPaperSize;

  class CommandUI;
  class OverviewBE;
  class PhysicalOverviewBE;
  class HistoryTreeBE;
  class DiagramOptionsBE;

  class ModelDiagramForm;

  // this class contains functionality that the UI needs,
  // like menu/toolbar access, special form backends etc

  class MYSQLWBBACKEND_PUBLIC_FUNC WBContextUI : public base::trackable {
  public:
    static std::shared_ptr<WBContextUI> get(); // Singleton.
    void cleanUp();
    void reinit();
    virtual ~WBContextUI();

    bool init(WBFrontendCallbacks *callbacks, WBOptions *options);

    // must be called when the frontend window is already on screen
    void init_finish(WBOptions *options);

    void finalize();

    bool request_quit();
    void perform_quit();
    bool is_quitting() {
      return _quitting;
    };

    void reset();

    WBContext *get_wb() {
      return _wb;
    }
    CommandUI *get_command_ui() {
      return _command_ui;
    }

    // form/panel backends
    PhysicalOverviewBE *get_physical_overview();

    bec::ValueInspectorBE *create_inspector_for_selection(bec::UIForm *form, std::vector<std::string> &items);
    bec::ValueInspectorBE *create_inspector_for_selection(std::vector<std::string> &items);

    std::string get_description_for_selection(bec::UIForm *form, grt::ListRef<GrtObject> &activeObjList,
                                              std::vector<std::string> &items);
    std::string get_description_for_selection(grt::ListRef<GrtObject> &activeObjList, std::vector<std::string> &items);
    void set_description_for_selection(const grt::ListRef<GrtObject> &objList, const std::string &val);

    DiagramOptionsBE *create_diagram_options_be(mdc::CanvasView *view);

    GRTShellWindow *get_shell_window();

    std::string get_active_diagram_info();

    void activate_figure(const grt::ValueRef &value);

    // utility functions for user preferences
    void get_doc_properties(std::string &caption, std::string &version, std::string &author, std::string &project,
                            std::string &date_created, std::string &date_changed, std::string &description);
    void set_doc_properties(const std::string &caption, const std::string &version, const std::string &author,
                            const std::string &project, const std::string &date_created,
                            const std::string &date_changed, const std::string &description);

    std::list<WBPaperSize> get_paper_sizes(bool descr_in_inches);
    bool add_paper_size(const std::string &name, double width, double height, bool margins, double margin_top,
                        double margin_bottom, double margin_left, double margin_right);

    std::vector<std::string> get_wb_options_keys(const std::string &model);

    grt::DictRef get_model_options(const std::string &model_id);

    bool get_wb_options_value(const std::string &model, const std::string &key, std::string &value);
    void set_wb_options_value(const std::string &model, const std::string &key, const std::string &value,
                              const grt::Type default_type = grt::AnyType);

    void discard_wb_model_options(const std::string &model);

    app_PageSettingsRef get_page_settings();

    // form management
    void register_form(bec::UIForm *form);

    void set_active_form(bec::UIForm *form);
    bec::UIForm *get_active_form();

    bec::UIForm *get_active_main_form();

    std::string get_active_context(bool main_context = true);

    boost::signals2::signal<void(bec::UIForm *)> *signal_form_change() {
      return &_form_change_signal;
    }

    // other functionality for UI
    std::string get_title();
    std::string get_document_name();

    void refresh_home_connections(bool clear_state = true);
    void refresh_home_documents();

    bool start_plugin_install(const std::string &path);
    void start_plugin_net_install(const std::string &url);

  private:
    friend class WBContext;
    WBContextUI(); // Enforce singleton model.
    WBContextUI(const WBContextUI &) = delete;
    WBContextUI &operator=(const WBContextUI &) = delete;

    void load_app_options(bool update);

    void history_changed();

    void overview_selection_changed();
    friend class WBContextModel; // for these callbacks, remove once everythign is moved there

    static void *form_destroyed(void *data);

    void refresh_editor_cb(bec::BaseEditor *editor);

    void form_changed();
    void update_current_diagram(bec::UIForm *form);

    void add_backend_builtin_commands();

    void show_about();
    void show_home_screen();
    void show_web_page(const std::string &url, bool internal_browser);
    void show_help_index();
    void showLicense();
    void locate_log_file();
    void show_log_file();

    void handle_home_action(mforms::HomeScreenAction action, const base::any &anyObject);

    void remove_connection(const db_mgmt_ConnectionRef &connection);
    void handle_home_context_menu(const base::any &object, const std::string &action);

    void start_plugin(const std::string &title, const std::string &command, const bec::ArgumentPool &defaults,
                      bool force_external = false);

    db_mgmt_ConnectionRef getConnectionById(const std::string &id);
    mforms::anyMap connectionToMap(db_mgmt_ConnectionRef connection);

  private:
    WBContext *_wb;

    // special forms/panels
    GRTShellWindow *_shell_window;
    mforms::HomeScreen *_home_screen;
    mforms::XConnectionsSection *_xConnectionsSection;
    mforms::ConnectionsSection *_connectionsSection;
    mforms::DocumentsSection *_documentsSection;
    std::vector<db_mgmt_ConnectionRef> _oldAuthList;

    AddOnDownloadWindow *_addon_download_window;
    PluginInstallWindow *_plugin_install_window;

    CommandUI *_command_ui;

    // form management
    bec::UIForm *_active_form;
    bec::UIForm *_active_main_form;

    boost::signals2::signal<void(bec::UIForm *)> _form_change_signal;

    bool _last_unsaved_changes_state;
    bool _initializing_home_screen;
    bool _quitting;
    bool _processing_action_open_connection;
  };
};
