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

//#include "config.h"
#if defined(HAVE_LIBSECRET_KEYRING)
#include <libsecret/secret.h>
#include <giomm.h>
#endif

#include <glib/gstdio.h>
#ifdef HAVE_PRCTL_H
#include <sys/prctl.h>
#endif

// this is broken
//#undef USE_KEYRING
#include "../lf_mforms.h"

#include "base/file_functions.h"
#include "base/file_utilities.h"
#include "base/log.h"
#include "base/string_utilities.h"
#include "../lf_utilities.h"
#include "gtk_helpers.h"
#include "mforms.h"
#include "main_app.h"
#include "grt.h"

namespace mforms {
  namespace gtk {

    /**
      * We need to use our implemenation of g_environ_unsetenv function because on OL6 that function is not avaiable.
      * TODO: replace that function with g_environ_unsetenv when OL6 support will be dropped
      */
    static gchar **wb_environ_unsetenv_internal(gchar **envp, const gchar *variable) {
      g_return_val_if_fail(variable != NULL, NULL);
      g_return_val_if_fail(strchr(variable, '=') == NULL, NULL);

      if (envp == NULL)
        return NULL;

      gint len;
      gchar **_envp, **envp_tmp;

      len = strlen(variable);

      /* Note that we remove *all* environment entries for
       * the variable name, not just the first.*/
      _envp = envp_tmp = envp;
      while (*_envp != NULL) {
        if (strncmp(*_envp, variable, len) != 0 || (*_envp)[len] != '=') {
          *envp_tmp = *_envp;
          envp_tmp++;
        } else
          g_free(*_envp);

        _envp++;
      }
      *envp_tmp = NULL;

      return envp;
    }

    /**
      * Get the current active window for this application
      */
    GtkWindow *get_current_window() {
      GList *window_list = gtk_window_list_toplevels();
      do {
        GtkWindow *wnd = (GtkWindow *)window_list->data;

        if (gtk_window_is_active(wnd)) {
          g_list_free(window_list);
          return wnd;
        }
      } while ((window_list = g_list_next(window_list)) != NULL);
      g_list_free(window_list);
      return NULL;
    }

    /**
     *  Try to set this dialog as transcient to the parent window. Setting this
     *  to the main window could stick this dialog to the wrong window (leaving
     *  a window in between them in the cases like popping a dialog in the
     *  scripting shell.
     *  To find the proper parent window, we're searching for the current active window
     *  on this application.
     */
    void set_dialog_transcient(Gtk::MessageDialog &dialog) {
      GtkWindow *parent_window = get_current_window();

      //  Check if a parent was found and only set transcient if it was. Passing
      //  a NULL parent would remove the transcient flag.
      if (parent_window != NULL)
        gtk_window_set_transient_for(((Gtk::Window *)&dialog)->gobj(), parent_window);
      else // If there's no parent window, then use main window as a parent.
        gtk_window_set_transient_for(((Gtk::Window *)&dialog)->gobj(), get_mainwindow()->gobj());
    }
    //--------------------------------------------------------------------------------

    static int gtkDialog(Gtk::MessageType type, const std::string &title, const std::string &text,
                         const std::string &ok, const std::string &cancel, const std::string &other) {
      Gtk::MessageDialog dlg("<b>" + title + "</b>", true, type, Gtk::BUTTONS_NONE, true);
      dlg.set_secondary_text(text);
      dlg.add_button(ok, mforms::ResultOk);
      if (!cancel.empty())
        dlg.add_button(cancel, mforms::ResultCancel);
      if (!other.empty())
        dlg.add_button(other, mforms::ResultOther);

      set_dialog_transcient(dlg);

      int r = dlg.run();
      if (r == Gtk::RESPONSE_DELETE_EVENT)
        return mforms::ResultCancel;
      return r;
    }

    int UtilitiesImpl::show_message(const std::string &title, const std::string &text, const std::string &ok,
                                    const std::string &cancel, const std::string &other) {
      return gtkDialog(Gtk::MESSAGE_INFO, title, text, ok, cancel, other);
    }

    int UtilitiesImpl::show_error(const std::string &title, const std::string &text, const std::string &ok,
                                  const std::string &cancel, const std::string &other) {
      return gtkDialog(Gtk::MESSAGE_ERROR, title, text, ok, cancel, other);
    }

    int UtilitiesImpl::show_warning(const std::string &title, const std::string &text, const std::string &ok,
                                    const std::string &cancel, const std::string &other) {
      return gtkDialog(Gtk::MESSAGE_WARNING, title, text, ok, cancel, other);
    }

    static void handle_click(Gtk::CheckButton *btn, bool *state) {
      *state = btn->get_active();
    }

    int UtilitiesImpl::show_message_with_checkbox(
      const std::string &title, const std::string &text, const std::string &ok, const std::string &cancel,
      const std::string &other,
      const std::string &checkbox_text, // empty text = default "Don't show this message again" text
      bool &remember_checked) {
      Gtk::MessageDialog dlg("<b>" + title + "</b>", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
      dlg.set_secondary_text(text);

      Gtk::CheckButton *btn =
        Gtk::manage(new Gtk::CheckButton(checkbox_text.empty() ? "Don't show this message again" : checkbox_text));
      dlg.get_vbox()->pack_start(*btn, false, true);
      btn->set_active(remember_checked);
      btn->signal_clicked().connect(sigc::bind(sigc::ptr_fun(handle_click), btn, &remember_checked));

      Gtk::Button *ok_btn = Gtk::manage(new Gtk::Button(ok));
      dlg.add_action_widget(*ok_btn, mforms::ResultOk);

      if (!cancel.empty())
        dlg.add_action_widget(*Gtk::manage(new Gtk::Button(cancel)), mforms::ResultCancel);
      if (!other.empty())
        dlg.add_action_widget(*Gtk::manage(new Gtk::Button(other)), mforms::ResultOther);
      dlg.show_all();

      set_dialog_transcient(dlg);

      int r = dlg.run();
      if (r == Gtk::RESPONSE_DELETE_EVENT)
        return mforms::ResultCancel;
      return r;
    }

    void UtilitiesImpl::set_clipboard_text(const std::string &text) {
      Gtk::Clipboard::get()->set_text(text);
    }

    std::string UtilitiesImpl::get_clipboard_text() {
      return Gtk::Clipboard::get()->wait_for_text();
    }

    void UtilitiesImpl::open_url(const std::string &url) {
      char *quoted_url = g_uri_escape_string(
        url.c_str(), G_URI_RESERVED_CHARS_GENERIC_DELIMITERS G_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS, FALSE);

      const gchar *argv[] = {"xdg-open", quoted_url, NULL};
      char **envp = g_get_environ();
      envp = wb_environ_unsetenv_internal(envp, "LD_PRELOAD");

      GError *error = NULL;
      gboolean result = g_spawn_async(NULL, (gchar **)argv, envp, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);

      free(quoted_url);
      g_strfreev(envp);

      if (!result) {
        char *err = g_strdup_printf("Error opening url with xdg-open: %s", error->message);
        g_error_free(error);
        std::runtime_error exc(err);
        g_free(err);
        throw exc;
      }
    }

    std::string UtilitiesImpl::get_special_folder(mforms::FolderType type) {
      std::string path;
      const char *t;

      switch (type) {
        case mforms::Documents:
#if GTK_VERSION_GT(2, 10)
          t = g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS);
          if (t)
            path = t;
#endif
          break;
        case mforms::Desktop:
#if GTK_VERSION_GT(2, 10)
          t = g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP);
          if (t)
            path = t;
#else
          t = g_strdup_printf("%s/Desktop", g_get_home_dir());
          path = t;
          g_free((void *)t);
#endif
          break;
        case mforms::ApplicationData:
          path = g_get_home_dir();
          break;
        case mforms::ApplicationSettings:
          path = g_get_home_dir();
          path.append("/.mysql/workbench");
          break;
        case WinProgramFiles:
        case WinProgramFilesX86:
          path = "/";
          break;
      }
      if (path.empty()) {
        t = g_get_home_dir();
        if (t)
          path = t;
      }
      return path.empty() ? "~" : path;
    }

    //------------------------------------------------------------------------------
    static std::map<TimeoutHandle, sigc::connection> timeouts;
    static TimeoutHandle last_timeout_handle = 0;
    static base::Mutex timeout_mutex;

    inline bool run_slot(const std::function<bool()> slot, TimeoutHandle handle) {
      if (!slot()) {
        base::MutexLock lock(timeout_mutex);
        std::map<TimeoutHandle, sigc::connection>::iterator it;
        if ((it = timeouts.find(handle)) != timeouts.end())
          timeouts.erase(it);
        return false;
      }
      return true;
    }

    //------------------------------------------------------------------------------
    TimeoutHandle UtilitiesImpl::add_timeout(float interval, const std::function<bool()> &slot) {
      try {
        if (slot) {
          base::MutexLock lock(timeout_mutex);
          ++last_timeout_handle;
          timeouts[last_timeout_handle] = Glib::signal_timeout().connect(
            sigc::bind(sigc::ptr_fun(run_slot), slot, last_timeout_handle), interval * 1000);
          return last_timeout_handle;
        }
      } catch (std::exception &exc) {
        static const char *const default_log_domain = "Utilities";
        logException("Utilities: exception in add timeout function", exc);
      }
      return 0;
    }

    void UtilitiesImpl::cancel_timeout(TimeoutHandle h) {
      base::MutexLock lock(timeout_mutex);
      std::map<TimeoutHandle, sigc::connection>::iterator it;
      if ((it = timeouts.find(h)) != timeouts.end()) {
        it->second.disconnect();
        timeouts.erase(it);
      }
    }

//-----------------------------------------------------------------------------------------------------------------------

#ifdef HAVE_LIBSECRET_KEYRING

    static std::string convertAndFreeString(gchar *txt) {
      std::string ret;
      if (txt) {
        ret = txt;
        g_free(txt);
      }

      return ret;
    }

    //-----------------------------------------------------------------------------------------------------------------------

    const SecretSchema* getWbSecretSchema() {
      static const SecretSchema wbSchema = {
          .name = "org.mysql.Workbench.Password",
          .flags = SECRET_SCHEMA_NONE,
          .attributes = {
              { "service", SECRET_SCHEMA_ATTRIBUTE_STRING },
              { "account", SECRET_SCHEMA_ATTRIBUTE_STRING },
              { nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING }
          },
          .reserved = 0,
          .reserved1 = 0,
          .reserved2 = 0,
          .reserved3 = 0,
          .reserved4 = 0,
          .reserved5 = 0,
          .reserved6 = 0,
          .reserved7 = 0,
      };
      return &wbSchema;
    };

    //-----------------------------------------------------------------------------------------------------------------------

    void UtilitiesImpl::store_password(const std::string &service, const std::string &account,
                                       const std::string &password) {
      if (getenv("WB_NO_KEYRING")) {
        return;
      }

      GError *error = nullptr;

      if (!secret_password_store_sync(getWbSecretSchema(), SECRET_COLLECTION_DEFAULT, service.c_str(), password.c_str(),
                                      nullptr, &error, "service", service.c_str(), "account", account.c_str(), nullptr))
        throw std::runtime_error(error->message);

    }

    //-----------------------------------------------------------------------------------------------------------------------

    bool UtilitiesImpl::find_password(const std::string &service, const std::string &account, std::string &password) {
      if (getenv("WB_NO_KEYRING")) {
        return false;
      }

      GError *error = nullptr;
      auto cancel = Gio::Cancellable::create();
      auto userPw = convertAndFreeString(secret_password_lookup_sync(getWbSecretSchema(), cancel->gobj(), &error, "service", service.c_str(), "account", account.c_str(), nullptr));

      if (error != nullptr) {
        throw std::runtime_error(error->message);
      }

      if (cancel->is_cancelled()) {
        throw grt::user_cancelled("User cancelled password lookup.");
      }

      if (!userPw.empty()) {
        password = userPw;
        return true;
      }
      return false;
    }

    //-----------------------------------------------------------------------------------------------------------------------

    void UtilitiesImpl::forget_password(const std::string &service, const std::string &account) {
      if (getenv("WB_NO_KEYRING")) {
        return;
      }

      GError *error = nullptr;
      auto cancel = Gio::Cancellable::create();
      secret_password_clear_sync(getWbSecretSchema(), cancel->gobj(), &error, "service", service.c_str(), "account", account.c_str(), nullptr);

      if(cancel->is_cancelled()) {
        throw grt::user_cancelled("User cancelled password lookup.");
      }

      if (error != nullptr)
        throw std::runtime_error(std::string("forget_password ") + error->message);

    }
#else

    void UtilitiesImpl::store_password(const std::string &service, const std::string &account,
                                       const std::string &password) {
    }

    bool UtilitiesImpl::find_password(const std::string &service, const std::string &account, std::string &password) {
      g_message("no gnome keyring support");
      return false;
    }

    void UtilitiesImpl::forget_password(const std::string &service, const std::string &account) {
    }

#endif // !USE_KEYRING

// Disabled in linux, as it useless here due to weird popup times
// It may not appear at all, or may appear at the end of WBA start
// or it may be displayed at the beginning of WBA start and get overlapped
// by password request dialog
#define USE_TRANSPARENT_MESSAGE

#define MESSAGE_WINDOW_WIDTH 450
#define MESSAGE_WINDOW_HEIGHT 220

#ifdef USE_TRANSPARENT_MESSAGE
    //==============================================================================
    //
    //==============================================================================
    class TransparentMessage : public Gtk::Window {
    public:
      TransparentMessage();
      void show_message(const std::string &title, const std::string &text,
                        const sigc::slot<bool> &cancel_slot = sigc::slot<bool>());
      void run();
      void stop();
      bool response;
      bool running_modal;

    private:
      Gtk::Button *cancel_button;
      sigc::slot<bool> cancel_slot;
      Glib::Mutex mutex;
      bool _is_runing;
      runtime::loop _loop;
      std::string _title;
      std::string _description;
      virtual bool on_button_release_event(GdkEventButton *ev);
      bool on_signal_draw(const ::Cairo::RefPtr< ::Cairo::Context> &ctx);
      void cancel_clicked();
    };

    //------------------------------------------------------------------------------
    TransparentMessage::TransparentMessage() : response(false), _is_runing(false) {
      Gtk::Window *owner = get_mainwindow();
      if (owner) {
        set_transient_for(*owner);
        set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
      } else
        set_position(Gtk::WIN_POS_CENTER);

      property_skip_taskbar_hint() = true;
      property_skip_pager_hint() = true;
      property_decorated() = false;

      set_size_request(MESSAGE_WINDOW_WIDTH, MESSAGE_WINDOW_HEIGHT);

      {
        Gtk::Box *vbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
        vbox->set_border_width(20);
        add(*vbox);

        Gtk::Box *hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 12));
        vbox->pack_end(*hbox, false, false);

        cancel_button = Gtk::manage(new Gtk::Button("Cancel"));
        hbox->pack_end(*cancel_button, false, true);
        vbox->show_all();

        cancel_button->signal_clicked().connect(sigc::mem_fun(this, &TransparentMessage::cancel_clicked));
        set_app_paintable(false);
        set_opacity(0.75);
        signal_draw().connect(sigc::mem_fun(this, &TransparentMessage::on_signal_draw));
      }

      override_background_color(Gdk::RGBA("Black"), Gtk::STATE_FLAG_NORMAL);
    }
    bool TransparentMessage::on_signal_draw(const ::Cairo::RefPtr< ::Cairo::Context> &ctx) {
      cairo_surface_t *mask =
        cairo_image_surface_create(CAIRO_FORMAT_A1, this->get_window()->get_width(), this->get_window()->get_height());
      cairo_t *cr = cairo_create(mask);
      if (cr) {
        double W = get_width();
        double H = get_height();
        double R = 45, x = 0, y = 0;

        cairo_save(cr);
        cairo_rectangle(cr, 0, 0, W, H);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_set_line_width(cr, 2);

        cairo_new_path(cr);
        cairo_move_to(cr, x + R, y);                                        // 1
        cairo_line_to(cr, x + (W - R), y);                                  // 2
        cairo_curve_to(cr, x + W, y, x + W, y, x + W, y + R);               // 3
        cairo_line_to(cr, x + W, y + (H - R));                              // 4
        cairo_curve_to(cr, x + W, y + H, x + W, y + H, x + (W - R), y + H); // 5
        cairo_line_to(cr, x + R, y + H);                                    // 6
        cairo_curve_to(cr, x, y + H, x, y + H, x, y + (H - R));             // 7
        cairo_line_to(cr, x, y + R);                                        // 8
        cairo_curve_to(cr, x, y, x, y, x + R, y);                           // 9
        cairo_close_path(cr);

        cairo_fill_preserve(cr);

        cairo_region_t *mask_region = gdk_cairo_region_create_from_surface(mask);

        gtk_widget_shape_combine_region(GTK_WIDGET(gobj()), mask_region);
        cairo_surface_destroy(mask);
        cairo_destroy(cr);

        // draw frame
        W -= 3;
        H -= 3;
        x = 1.5;
        y = 1.5;
        R -= 2;

        ctx->save();
        ctx->set_source_rgb(1.0, 1.0, 1.0);
        ctx->set_line_width(2.5);
        ctx->begin_new_path();
        ctx->move_to(x + R, y);                                        // 1
        ctx->line_to(x + (W - R), y);                                  // 2
        ctx->curve_to(x + W, y, x + W, y, x + W, y + R);               // 3
        ctx->line_to(x + W, y + (H - R));                              // 4
        ctx->curve_to(x + W, y + H, x + W, y + H, x + (W - R), y + H); // 5
        ctx->line_to(x + R, y + H);                                    // 6
        ctx->curve_to(x, y + H, x, y + H, x, y + (H - R));             // 7
        ctx->line_to(x, y + R);                                        // 8
        ctx->curve_to(x, y, x, y, x + R, y);                           // 9
        ctx->close_path();
        ctx->stroke_preserve();
        ctx->restore();

        // draw icon
        Glib::RefPtr<Gdk::Pixbuf> icon =
          Gdk::Pixbuf::create_from_file(App::get()->get_resource_path("message_wb_wait.png"));
        ctx->save();
        Gdk::Cairo::set_source_pixbuf(ctx, icon, 30, 30);
        ctx->rectangle(0, 0, icon->get_width(), icon->get_height());
        ctx->fill();
        ctx->restore();

        // draw text title
        ctx->save();
        ctx->set_source_rgb(1.0, 1.0, 1.0);
        ctx->move_to(40 + icon->get_width(), 50);
        Glib::RefPtr<Pango::Layout> layout_title = create_pango_layout(_title.c_str());
        layout_title->set_font_description(Pango::FontDescription("Bitstream Vera Sans,Helvetica, bold 14"));
        layout_title->set_width((MESSAGE_WINDOW_WIDTH - icon->get_width() - 30 - 20) * Pango::SCALE);
        layout_title->show_in_cairo_context(ctx);
        ctx->restore();

        // draw text description
        ctx->save();
        ctx->set_source_rgb(1.0, 1.0, 1.0);
        ctx->move_to(40 + icon->get_width(), 90);
        Glib::RefPtr<Pango::Layout> layout_desc = create_pango_layout(_description.c_str());
        layout_desc->set_font_description(Pango::FontDescription("Bitstream Vera Sans,Helvetica, 9"));
        layout_desc->set_width((MESSAGE_WINDOW_WIDTH - icon->get_width() - 30 - 20) * Pango::SCALE);
        layout_desc->show_in_cairo_context(ctx);
        ctx->restore();
      }
      return false;
    }
    //------------------------------------------------------------------------------
    void TransparentMessage::show_message(const std::string &title, const std::string &text,
                                          const sigc::slot<bool> &cancel_slot) {
      this->cancel_slot = cancel_slot;
      if (cancel_slot)
        cancel_button->show();
      else {
        cancel_button->hide();
        add_events(Gdk::BUTTON_RELEASE_MASK);
      }

      // We need to set text before we call realize.
      _title = title;
      _description = text;
      if (get_realized()) // Then we need to just force redraw instead of realizing the widget
        queue_draw();
      else
        realize();

      Glib::RefPtr<Gdk::Window> window = get_window();
      show_all();
      window->process_updates(true);
    }

    void TransparentMessage::run() {
      _is_runing = true;
      _loop.run();
    }

    //------------------------------------------------------------------------------
    void TransparentMessage::stop() {
      Glib::Mutex::Lock lock(mutex);
      if (running_modal) {
        unrealize();
        running_modal = false;
        if (_is_runing) {
          _loop.quit();
          _is_runing = false;
        }
      }
      response = true;
    }

    void TransparentMessage::cancel_clicked() {
      Glib::Mutex::Lock lock(mutex);
      if (cancel_slot && cancel_slot()) {
        if (running_modal) {
          unrealize();
          running_modal = false;
          if (_is_runing) {
            _loop.quit();
            hide();
            _is_runing = false;
          }
        }
        response = false;
      }
    }

    //------------------------------------------------------------------------------
    bool TransparentMessage::on_button_release_event(GdkEventButton *ev) {
      hide();
      return false;
    }

    static TransparentMessage *tm = 0;
    static TransparentMessage *tmc = 0;
#endif

    //------------------------------------------------------------------------------
    void UtilitiesImpl::show_wait_message(const std::string &title, const std::string &text) {
#ifdef USE_TRANSPARENT_MESSAGE
      if (!tm)
        tm = new TransparentMessage();

      if (tm)
        tm->show_message(title, text);
#endif
    }

    //------------------------------------------------------------------------------
    bool UtilitiesImpl::hide_wait_message() {
#ifdef USE_TRANSPARENT_MESSAGE
      if (tm) {
        tm->hide();
        delete tm;
        tm = 0;
      }
#endif
      return false;
    }

    //-------------------------------------------------------------------------------

    bool UtilitiesImpl::move_to_trash(const std::string &path) {
      // trash in linux is chaos, just delete it
      if (g_file_test(path.c_str(), G_FILE_TEST_IS_DIR))
        base_rmdir_recursively(path.c_str());
      else
        g_remove(path.c_str());
      return true;
    }

    //------------------------------------------------------------------------------
    void UtilitiesImpl::reveal_file(const std::string &path) {
      std::string dirname = base::dirname(path);
      const gchar *argv[] = {"xdg-open", dirname.c_str(), NULL};

      GError *error = NULL;
      char **envp = g_get_environ();
      envp = wb_environ_unsetenv_internal(envp, "LD_PRELOAD");

      gboolean result = g_spawn_async(NULL, (gchar **)argv, envp, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);
      g_strfreev(envp);

      if (!result) {
        char *err = g_strdup_printf("Error opening folder with xdg-open: %s", error->message);
        g_error_free(error);
        std::runtime_error exc(err);
        g_free(err);
        throw exc;
      }
    }

    void UtilitiesImpl::set_thread_name(const std::string &name) {
#ifdef HAVE_PRCTL_H
      if (!name.empty())
        prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
#endif
    }

    void UtilitiesImpl::beep() {
      if (get_mainwindow())
        get_mainwindow()->get_window()->beep();
    }

    //------------------------------------------------------------------------------

    bool UtilitiesImpl::run_cancelable_wait_message(const std::string &title, const std::string &text,
                                                    const std::function<void()> &start_task,
                                                    const std::function<bool()> &cancel_task) {
      if (!start_task)
        throw std::invalid_argument("start_task param cannot be empty");

      if (!tmc)
        tmc = new TransparentMessage();
      if (tmc) {
        tmc->show_message(title, text, sigc::mem_fun(&cancel_task, &std::function<bool()>::operator()));
        tmc->running_modal = true;

        Glib::signal_idle().connect(sigc::bind_return(start_task, false));

        tmc->run();
        bool response = tmc->response;
        delete tmc;
        tmc = 0;
        return response;
      }

      return false;
    }

    void UtilitiesImpl::stop_cancelable_wait_message() {
      if (tmc) {
        if (Utilities::in_main_thread())
          tmc->stop();
        else
          Utilities::perform_from_main_thread([]() -> void * {
            if (tmc != nullptr)
              tmc->stop();
            return nullptr;
          });
      }
    }

    static std::map<std::string, Glib::RefPtr<Gdk::Pixbuf> > icon_cache;

    Glib::RefPtr<Gdk::Pixbuf> UtilitiesImpl::get_cached_icon(const std::string &icon) {
      if (icon_cache.find(icon) != icon_cache.end())
        return icon_cache[icon];

      if (icon == "folder") {
        Glib::RefPtr<Gdk::Pixbuf> pix = Gtk::IconTheme::get_default()->load_icon("folder", Gtk::ICON_SIZE_MENU);
        icon_cache[icon] = pix;
        return pix;
      } else {
        std::string path = mforms::App::get()->get_resource_path(icon);
        if (!path.empty() && g_file_test(path.c_str(), G_FILE_TEST_IS_REGULAR)) {
          icon_cache[icon] = Gdk::Pixbuf::create_from_file(path);
          return icon_cache[icon];
        } else
          g_warning("Can't find icon %s", icon.c_str());
      }
      return Glib::RefPtr<Gdk::Pixbuf>();
    }

//------------------------------------------------------------------------------
#include <pango/pangoft2.h>

    class FontMeasurement {
    public:
      PangoLayout *_layout;
      FontMeasurement(PangoLayout *pango) : _layout(pango) {
      }

      //  FontMeasurement(const FontMeasurement &other)
      //  {
      //    if (other._layout!=NULL)
      //      g_object_ref(other._layout);
      //  }
      //
      //  void operator=(const FontMeasurement &other)
      //  {
      //    if (other._layout!=NULL)
      //      g_object_ref(other._layout);
      //  }

      ~FontMeasurement() {
        if (_layout != NULL)
          g_object_unref(_layout);
      }
    };

    std::map<std::string, FontMeasurement *> FontMeasurementDescriptors;

    double UtilitiesImpl::get_text_width(const std::string &text, const std::string &font_desc) {
      std::string font;
      float size = 0;
      bool bold = false;
      bool italic = false;

      if (!base::parse_font_description(font_desc, font, size, bold, italic))
        return 0;

      PangoLayout *layout = NULL;
      std::map<std::string, FontMeasurement *>::iterator it = FontMeasurementDescriptors.find(font_desc);
      if (it == FontMeasurementDescriptors.end()) {
        if (get_mainwindow() == nullptr)
          throw std::runtime_error("Need main window to continue.");
        PangoFontDescription *font_description = pango_font_description_new();
        FontMeasurement *font_measurement =
          new FontMeasurement(pango_layout_new(get_mainwindow()->get_pango_context()->gobj()));

        pango_font_description_set_family(font_description, font.c_str());
        pango_font_description_set_style(font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
        pango_font_description_set_variant(font_description, PANGO_VARIANT_NORMAL);
        pango_font_description_set_weight(font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
        pango_font_description_set_stretch(font_description, PANGO_STRETCH_NORMAL);
        pango_font_description_set_size(font_description, size * PANGO_SCALE);
        pango_layout_set_font_description(font_measurement->_layout, font_description);
        pango_font_description_free(font_description);

        FontMeasurementDescriptors[font_desc] = font_measurement;
        layout = font_measurement->_layout;
      } else
        layout = it->second->_layout;

      pango_layout_set_text(layout, text.c_str(), -1);

      int width = 0;
      pango_layout_get_pixel_size(layout, &width, NULL);

      return (double)width;
    }

    //------------------------------------------------------------------------------

    void UtilitiesImpl::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_utilities_impl.show_message = &UtilitiesImpl::show_message;
      f->_utilities_impl.show_error = &UtilitiesImpl::show_error;
      f->_utilities_impl.show_warning = &UtilitiesImpl::show_warning;
      f->_utilities_impl.show_message_with_checkbox = &UtilitiesImpl::show_message_with_checkbox;
      f->_utilities_impl.show_wait_message = &UtilitiesImpl::show_wait_message;
      f->_utilities_impl.hide_wait_message = &UtilitiesImpl::hide_wait_message;
      f->_utilities_impl.run_cancelable_wait_message = &UtilitiesImpl::run_cancelable_wait_message;
      f->_utilities_impl.stop_cancelable_wait_message = &UtilitiesImpl::stop_cancelable_wait_message;

      f->_utilities_impl.set_clipboard_text = &UtilitiesImpl::set_clipboard_text;
      f->_utilities_impl.get_clipboard_text = &UtilitiesImpl::get_clipboard_text;
      f->_utilities_impl.open_url = &UtilitiesImpl::open_url;
      f->_utilities_impl.add_timeout = &UtilitiesImpl::add_timeout;
      f->_utilities_impl.cancel_timeout = &UtilitiesImpl::cancel_timeout;
      f->_utilities_impl.get_special_folder = &UtilitiesImpl::get_special_folder;
      f->_utilities_impl.store_password = &UtilitiesImpl::store_password;
      f->_utilities_impl.find_password = &UtilitiesImpl::find_password;
      f->_utilities_impl.forget_password = &UtilitiesImpl::forget_password;
      f->_utilities_impl.move_to_trash = &UtilitiesImpl::move_to_trash;
      f->_utilities_impl.reveal_file = &UtilitiesImpl::reveal_file;
      f->_utilities_impl.perform_from_main_thread = &MainThreadRequestQueue::perform;
      f->_utilities_impl.set_thread_name = &UtilitiesImpl::set_thread_name;
      f->_utilities_impl.beep = &UtilitiesImpl::beep;

      f->_utilities_impl.get_text_width = &UtilitiesImpl::get_text_width;
      MainThreadRequestQueue::get(); // init from main thread
    }

    //---------------------------------------------------------------------------------

    void MainThreadRequestQueue::from_main_thread() {
      std::shared_ptr<Request> req;
      {
        Glib::Mutex::Lock lock(_mutex);
        if (_queue.empty())
          return;
        req = _queue.front();
        _queue.pop_front();
      }
      req->result = req->slot();

      Glib::Mutex::Lock lock(req->mutex);
      req->done = true;
      req->cond.signal();
    }

    MainThreadRequestQueue::MainThreadRequestQueue() {
      _disp.connect(sigc::mem_fun(this, &MainThreadRequestQueue::from_main_thread));
    }

    MainThreadRequestQueue *MainThreadRequestQueue::get() {
      static MainThreadRequestQueue *q = new MainThreadRequestQueue();
      return q;
    }

    void *MainThreadRequestQueue::perform(const std::function<void *()> &slot, bool wait) {
      if (Utilities::in_main_thread())
        return slot();
      else {
        MainThreadRequestQueue *self = MainThreadRequestQueue::get();

        std::shared_ptr<Request> req(new Request());
        req->slot = slot;
        req->done = false;
        req->result = 0;
        {
          Glib::Mutex::Lock lock(self->_mutex);
          self->_queue.push_back(req);
        }
        self->_disp.emit();

        if (wait) {
          Glib::Mutex::Lock lock(req->mutex);
          while (!req->done)
            req->cond.wait(req->mutex);

          return req->result;
        }
        return NULL;
      }
    }
  };
};
