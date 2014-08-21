/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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
 
//#include "config.h"
#if defined(HAVE_GNOME_KEYRING) || defined(HAVE_OLD_GNOME_KEYRING)
extern "C" {
  #include <gnome-keyring.h>
};
  #include <string.h>
  #define USE_KEYRING 1
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

namespace mforms {
namespace gtk {

/**
  * Get the current active window for this application
  */
GtkWindow *get_current_window()
{
  GList *window_list = gtk_window_list_toplevels();
  do
  {
    GtkWindow *wnd = (GtkWindow *)window_list->data;
    
    if (gtk_window_is_active(wnd))
      return wnd;
  } while((window_list = g_list_next(window_list)) != NULL);

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
void set_dialog_transcient(Gtk::MessageDialog &dialog)
{
  GtkWindow *parent_window = get_current_window();
  
  //  Check if a parent was found and only set transcient if it was. Passing
  //  a NULL parent would remove the transcient flag.
  if (get_current_window() != NULL)
    gtk_window_set_transient_for(((Gtk::Window *)&dialog)->gobj(), parent_window);
}
//--------------------------------------------------------------------------------
  

int UtilitiesImpl::show_message(const std::string &title, const std::string &text,
                        const std::string &ok, const std::string &cancel,
                        const std::string &other)
{
  Gtk::MessageDialog dlg("<b>"+title+"</b>", true,
                         Gtk::MESSAGE_INFO, Gtk::BUTTONS_NONE, true);
  dlg.set_secondary_text(text);
  dlg.add_button(ok, mforms::ResultOk);
  if (!cancel.empty())
    dlg.add_button(cancel, mforms::ResultCancel);
  if (!other.empty())
    dlg.add_button(other, mforms::ResultOther);

  set_dialog_transcient(dlg);

  int r= dlg.run();
  if (r == Gtk::RESPONSE_DELETE_EVENT)
    return mforms::ResultCancel;
  return r;
}


int UtilitiesImpl::show_error(const std::string &title, const std::string &text,
                      const std::string &ok, const std::string &cancel,
                      const std::string &other)
{
  Gtk::MessageDialog dlg("<b>"+title+"</b>", true,
                         Gtk::MESSAGE_ERROR, Gtk::BUTTONS_NONE, true);
  dlg.set_secondary_text(text);
  dlg.add_button(ok, mforms::ResultOk);
  if (!cancel.empty())
    dlg.add_button(cancel, mforms::ResultCancel);
  if (!other.empty())
    dlg.add_button(other, mforms::ResultOther);

  set_dialog_transcient(dlg);

  int r= dlg.run();
  if (r == Gtk::RESPONSE_DELETE_EVENT)
    return mforms::ResultCancel;
  return r;
}

int UtilitiesImpl::show_warning(const std::string &title, const std::string &text,
                      const std::string &ok, const std::string &cancel,
                      const std::string &other)
{
  Gtk::MessageDialog dlg("<b>"+title+"</b>", true,
                         Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true);
  dlg.set_secondary_text(text);
  dlg.add_button(ok, mforms::ResultOk);
  if (!cancel.empty())
    dlg.add_button(cancel, mforms::ResultCancel);
  if (!other.empty())
    dlg.add_button(other, mforms::ResultOther);

  set_dialog_transcient(dlg);
  
  int r= dlg.run();
  if (r == Gtk::RESPONSE_DELETE_EVENT)
    return mforms::ResultCancel;
  return r;
}

static void handle_click(Gtk::CheckButton* btn, bool* state)
{
  *state = btn->get_active();
}

int UtilitiesImpl::show_message_with_checkbox(const std::string &title, const std::string &text,
                               const std::string &ok, const std::string &cancel,
                               const std::string &other,
                               const std::string &checkbox_text, // empty text = default "Don't show this message again" text
                               bool &remember_checked)
{
  Gtk::MessageDialog dlg("<b>"+title+"</b>", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
  dlg.set_secondary_text(text);

  Gtk::CheckButton* btn = Gtk::manage(new Gtk::CheckButton(checkbox_text.empty() ? "Don't show this message again" : checkbox_text));
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

  int r= dlg.run();
  if (r == Gtk::RESPONSE_DELETE_EVENT)
    return mforms::ResultCancel;
  return r;
}

void UtilitiesImpl::set_clipboard_text(const std::string &text)
{
  Gtk::Clipboard::get()->set_text(text);
}

std::string UtilitiesImpl::get_clipboard_text()
{
  return Gtk::Clipboard::get()->wait_for_text();
}

void UtilitiesImpl::open_url(const std::string &url)
{
  char *quoted_url = g_uri_escape_string(url.c_str(), 
                                         G_URI_RESERVED_CHARS_GENERIC_DELIMITERS 
                                         G_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS, 
                                         FALSE);

  const gchar *argv[] = {
    "xdg-open",
    quoted_url,
    NULL 
  };
  GError *error = NULL;
  gboolean result = g_spawn_async(NULL, (gchar**)argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);
  
  free(quoted_url);
  
  if (!result)
  {
    char *err = g_strdup_printf("Error opening url with xdg-open: %s", error->message);
    g_error_free(error);
    std::runtime_error exc(err);
    g_free(err);
    throw exc;
  }
}

std::string UtilitiesImpl::get_special_folder(mforms::FolderType type)
{
  std::string path;
  const char *t;

  switch (type)
  {
  case mforms::Documents:
#if GTK_VERSION_GT(2,10)
    t = g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS);
    if (t) path= t;
#endif
    break;
  case mforms::Desktop:
#if GTK_VERSION_GT(2,10)
    t = g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP);
    if (t) path= t;
#else
    t = g_strdup_printf("%s/Desktop", g_get_home_dir());
    path= t;
    g_free((void*)t);
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
  if (path.empty())
  {
    t = g_get_home_dir();
    if (t) path= t;
  }
  return path.empty() ? "~" : path;
}

//------------------------------------------------------------------------------
static std::map<TimeoutHandle, sigc::connection> timeouts;
static TimeoutHandle last_timeout_handle = 0;
static base::Mutex timeout_mutex;

inline bool run_slot(const boost::function<bool ()> slot, TimeoutHandle handle)
{
  if (!slot())
  {
    base::MutexLock lock(timeout_mutex);
    std::map<TimeoutHandle, sigc::connection>::iterator it;
    if ((it = timeouts.find(handle)) != timeouts.end())
      timeouts.erase(it);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
TimeoutHandle UtilitiesImpl::add_timeout(float interval, const boost::function<bool ()> &slot)
{
  try
  {
    if (slot)
    {
      base::MutexLock lock(timeout_mutex);
      ++last_timeout_handle;
      timeouts[last_timeout_handle] = Glib::signal_timeout().connect(sigc::bind(sigc::ptr_fun(run_slot), slot, last_timeout_handle), interval*1000);
      return last_timeout_handle;
    }
  }
  catch (std::exception &exc)
  {
    static const char* const default_log_domain = "Utilities";
    log_exception("Utilities: exception in add timeout function", exc);
  }
  return 0;
}


void UtilitiesImpl::cancel_timeout(TimeoutHandle h)
{
  base::MutexLock lock(timeout_mutex);
  std::map<TimeoutHandle, sigc::connection>::iterator it;
  if ((it = timeouts.find(h)) != timeouts.end())
  {
    it->second.disconnect();
    timeouts.erase(it);
  }
}


//------------------------------------------------------------------------------
// GNOME KEYRING passwords section
#ifdef HAVE_GNOME_KEYRING
//------------------------------------------------------------------------------
void UtilitiesImpl::store_password(const std::string &service, const std::string &account, const std::string &password)
{
  if (getenv("WB_NO_GNOME_KEYRING"))
  {
    return;
  }

  GnomeKeyringPasswordSchema wb_pwd_schema;
  memset(&wb_pwd_schema, 0, sizeof(wb_pwd_schema));

  wb_pwd_schema.item_type = GNOME_KEYRING_ITEM_GENERIC_SECRET;
  wb_pwd_schema.attributes[0].name = "service";
  wb_pwd_schema.attributes[0].type = GNOME_KEYRING_ATTRIBUTE_TYPE_STRING;
  wb_pwd_schema.attributes[1].name = "account";
  wb_pwd_schema.attributes[1].type = GNOME_KEYRING_ATTRIBUTE_TYPE_STRING;
  // attributes list must be terminated by a pair {0,0}, since we zeroed the whole struct we're safe here

  GnomeKeyringResult res = gnome_keyring_store_password_sync(&wb_pwd_schema,
                                                             NULL, // using default keyring
                                                             account.c_str(),
                                                             password.c_str(),
                                                             "service", service.c_str(),
                                                             "account", account.c_str(),
                                                             NULL
                                                             );

  if ( res != GNOME_KEYRING_RESULT_OK )
    throw std::runtime_error(gnome_keyring_result_to_message(res));
}

//------------------------------------------------------------------------------
bool UtilitiesImpl::find_password(const std::string &service, const std::string &account, std::string &password)
{
  if (getenv("WB_NO_GNOME_KEYRING"))
  {
    return false;
  }

  bool ret = false;

  GnomeKeyringPasswordSchema wb_pwd_schema;
  memset(&wb_pwd_schema, 0, sizeof(wb_pwd_schema));

  wb_pwd_schema.item_type = GNOME_KEYRING_ITEM_GENERIC_SECRET;
  wb_pwd_schema.attributes[0].name = "service";
  wb_pwd_schema.attributes[0].type = GNOME_KEYRING_ATTRIBUTE_TYPE_STRING;
  wb_pwd_schema.attributes[1].name = "account";
  wb_pwd_schema.attributes[1].type = GNOME_KEYRING_ATTRIBUTE_TYPE_STRING;
  // attributes list must be terminated by a pair {0,0}, since we zeroed the whole struct we're safe here

  gchar *kpwd = 0;

  GnomeKeyringResult res = gnome_keyring_find_password_sync(&wb_pwd_schema,
                                                            &kpwd,
                                                            "service", service.c_str(),
                                                            "account", account.c_str(),
                                                            NULL);

  if ( res != GNOME_KEYRING_RESULT_OK && res != GNOME_KEYRING_RESULT_NO_MATCH )
  {
    if (kpwd)
      gnome_keyring_free_password(kpwd);
      kpwd = 0;
    throw std::runtime_error(gnome_keyring_result_to_message(res));
  }

  if (kpwd && res == GNOME_KEYRING_RESULT_OK)
  {
    ret = true;
    password = kpwd;
    gnome_keyring_free_password(kpwd);
    kpwd = 0;
  }

  return ret;
}

//------------------------------------------------------------------------------
void UtilitiesImpl::forget_password(const std::string &service, const std::string &account)
{
  if (getenv("WB_NO_GNOME_KEYRING"))
  {
    return;
  }

  GnomeKeyringPasswordSchema wb_pwd_schema;
  memset(&wb_pwd_schema, 0, sizeof(wb_pwd_schema));

  wb_pwd_schema.item_type = GNOME_KEYRING_ITEM_GENERIC_SECRET;
  wb_pwd_schema.attributes[0].name = "service";
  wb_pwd_schema.attributes[0].type = GNOME_KEYRING_ATTRIBUTE_TYPE_STRING;
  wb_pwd_schema.attributes[1].name = "account";
  wb_pwd_schema.attributes[1].type = GNOME_KEYRING_ATTRIBUTE_TYPE_STRING;
  // attributes list must be terminated by a pair {0,0}, since we zeroed the whole struct we're safe here

  GnomeKeyringResult res = gnome_keyring_delete_password_sync(&wb_pwd_schema,
                                                            "service", service.c_str(),
                                                            "account", account.c_str(),
                                                            NULL);

  if ( res != GNOME_KEYRING_RESULT_OK && res != GNOME_KEYRING_RESULT_NO_MATCH)
    throw std::runtime_error(std::string("forget_password ") + gnome_keyring_result_to_message(res));
}

#elif defined(HAVE_OLD_GNOME_KEYRING)

//------------------------------------------------------------------------------
enum {Gnome_keyring_results_size = 10};
static const char* gnome_keyring_results[Gnome_keyring_results_size] = {"OK",
                                             "GNOME_KEYRING_RESULT_DENIED",
                                             "GNOME_KEYRING_RESULT_NO_KEYRING_DAEMON",
                                             "GNOME_KEYRING_RESULT_ALREADY_UNLOCKED",
                                             "GNOME_KEYRING_RESULT_NO_SUCH_KEYRING",
                                             "GNOME_KEYRING_RESULT_BAD_ARGUMENTS",
                                             "GNOME_KEYRING_RESULT_IO_ERROR",
                                             "GNOME_KEYRING_RESULT_CANCELLED",
                                             "GNOME_KEYRING_RESULT_ALREADY_EXISTS",
                                             ""
                                            };

//------------------------------------------------------------------------------
static const char* gnome_keyring_result_to_message(const GnomeKeyringResult result)
{
  const char* message = gnome_keyring_results[Gnome_keyring_results_size - 1];

  if (result >= 0 && result < (GnomeKeyringResult)Gnome_keyring_results_size)
    message = gnome_keyring_results[result];

  return message;
}

//------------------------------------------------------------------------------
void UtilitiesImpl::store_password(const std::string &service, const std::string &account, const std::string &password)
{
  if (getenv("WB_NO_GNOME_KEYRING"))
  {
    return;
  }

  GnomeKeyringAttributeList *attrs = gnome_keyring_attribute_list_new();
  guint32 item_id = 0;

  gnome_keyring_attribute_list_append_string(attrs, "service", service.c_str());
  gnome_keyring_attribute_list_append_string(attrs, "account", account.c_str());

  const GnomeKeyringResult result = gnome_keyring_item_create_sync(NULL,
                                                                  GNOME_KEYRING_ITEM_GENERIC_SECRET,
                                                                  account.c_str(),
                                                                  attrs,
                                                                  password.c_str(),
                                                                  true,
                                                                  &item_id);

  gnome_keyring_attribute_list_free(attrs);

  if ( result != GNOME_KEYRING_RESULT_OK )
    throw std::runtime_error(gnome_keyring_result_to_message(result));
}

//------------------------------------------------------------------------------
static guint32 find_password_and_id(const std::string &service, const std::string &account, std::string &password)
{
  guint32 ret = 0;

  GnomeKeyringAttributeList *attrs = gnome_keyring_attribute_list_new();

  gnome_keyring_attribute_list_append_string(attrs, "service", service.c_str());
  gnome_keyring_attribute_list_append_string(attrs, "account", account.c_str());

  GList* found_items = 0;

  const GnomeKeyringResult result = gnome_keyring_find_items_sync(GNOME_KEYRING_ITEM_GENERIC_SECRET,
                                                                  attrs,
                                                                  &found_items);

  if (result != GNOME_KEYRING_RESULT_OK)
  {
    if (found_items)
      gnome_keyring_found_list_free(found_items);
    throw std::runtime_error(gnome_keyring_result_to_message(result));
  }

  if (g_list_length(found_items) > 0)
  {
    GnomeKeyringFound* item = (GnomeKeyringFound*)g_list_first(found_items);
    password = item->secret;
    ret = item->item_id;
  }

  if (found_items)
    gnome_keyring_found_list_free(found_items);

  return ret;
}

//------------------------------------------------------------------------------
bool UtilitiesImpl::find_password(const std::string &service, const std::string &account, std::string &password)
{
  if (getenv("WB_NO_GNOME_KEYRING"))
  {
    return false;
  }

  return find_password_and_id(service, account, password);
}

//------------------------------------------------------------------------------
void UtilitiesImpl::forget_password(const std::string &service, const std::string &account)
{
  if (getenv("WB_NO_GNOME_KEYRING"))
  {
    return;
  }

  std::string password;
  const guint32 item_id = find_password_and_id(service, account, password);

  GnomeKeyringResult res = GNOME_KEYRING_RESULT_BAD_ARGUMENTS;
  if (item_id)
    res = gnome_keyring_item_delete_sync(NULL, item_id);

  if ( res != GNOME_KEYRING_RESULT_OK )
    throw std::runtime_error(gnome_keyring_result_to_message(res));
}
#else

void UtilitiesImpl::store_password(const std::string &service, const std::string &account, const std::string &password)
{
}

bool UtilitiesImpl::find_password(const std::string &service, const std::string &account, std::string &password)
{
  g_message("no gnome keyring support");
  return false;
}

void UtilitiesImpl::forget_password(const std::string &service, const std::string &account)
{
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
class TransparentMessage : public Gtk::Window
{
  public:
    TransparentMessage();
    void show_message(const std::string& title, const std::string& text, const sigc::slot<bool> &cancel_slot=sigc::slot<bool>());
    void run();
    void stop();
    bool response;
    bool running_modal;
  private:
    Gtk::Button *cancel_button;
    sigc::slot<bool> cancel_slot;
    Glib::Mutex mutex;
    bool _is_runing;

    virtual bool on_button_release_event(GdkEventButton* ev);
    void cancel_clicked();
};

//------------------------------------------------------------------------------
TransparentMessage::TransparentMessage()
  : response(false), _is_runing(false)
{
  Gtk::Window *owner = get_mainwindow();
  if (owner)
  {
    set_transient_for(*owner);
    set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
  }
  else
    set_position(Gtk::WIN_POS_CENTER);


  property_skip_taskbar_hint() = true;
  property_skip_pager_hint() = true;
  property_decorated() = false;

  set_size_request(MESSAGE_WINDOW_WIDTH, MESSAGE_WINDOW_HEIGHT);
  
  set_style(get_style()->copy());

  {
    Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox(false, 0));
    vbox->set_border_width(20);
    add(*vbox);

    Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox(false, 12));
    vbox->pack_end(*hbox, false, false);

    cancel_button = Gtk::manage(new Gtk::Button("Cancel"));
    hbox->pack_end(*cancel_button, false, true);
    vbox->show_all();

    cancel_button->signal_clicked().connect(sigc::mem_fun(this, &TransparentMessage::cancel_clicked));
  }
}

//------------------------------------------------------------------------------
void TransparentMessage::show_message(const std::string& title, const std::string& text, const sigc::slot<bool> &cancel_slot)
{
  this->cancel_slot = cancel_slot;
  if (cancel_slot)
    cancel_button->show();
  else
  {
    cancel_button->hide();
    add_events(Gdk::BUTTON_RELEASE_MASK);
  }

  realize();

  Gdk::Color black("black"), white("white");
  black.rgb_find_color(get_colormap());
  white.rgb_find_color(get_colormap());
  
  Glib::RefPtr<Gdk::Pixmap> pixmap = Gdk::Pixmap::create(get_window(),
                                                         MESSAGE_WINDOW_WIDTH, MESSAGE_WINDOW_HEIGHT,
                                                         get_window()->get_depth());

  Glib::RefPtr<Gdk::GC> gc = Gdk::GC::create(pixmap);
    
  gc->set_foreground(white);
  pixmap->draw_rectangle(gc, false, 0, 0, MESSAGE_WINDOW_WIDTH-1, MESSAGE_WINDOW_HEIGHT-1);

  gc->set_foreground(black);
  pixmap->draw_rectangle(gc, true, 2, 2, MESSAGE_WINDOW_WIDTH-3, MESSAGE_WINDOW_HEIGHT-3);

  
  Glib::RefPtr<Gdk::Pixbuf> icon = Gdk::Pixbuf::create_from_file(App::get()->get_resource_path("message_wb_wait.png"));
    
  pixmap->draw_pixbuf(gc, icon, 0, 0, 20, 20, icon->get_width(), icon->get_height(),
                      Gdk::RGB_DITHER_NORMAL, 0, 0);

  Glib::RefPtr<Pango::Layout> layout = create_pango_layout(title);
  
  gc->set_foreground(white);

  layout->set_font_description(Pango::FontDescription("Bitstream Vera Sans,Helvetica, bold 14"));
  layout->set_width((MESSAGE_WINDOW_WIDTH-icon->get_width()-30-20)*Pango::SCALE);
  pixmap->draw_layout(gc, icon->get_width()+30, 40, layout);

  
  layout = create_pango_layout(text);
  layout->set_font_description(Pango::FontDescription("Bitstream Vera Sans,Helvetica, 9"));
  layout->set_width((MESSAGE_WINDOW_WIDTH-icon->get_width()-30-20)*Pango::SCALE);
  pixmap->draw_layout(gc, icon->get_width()+30, 90, layout);

  get_style()->set_bg_pixmap(Gtk::STATE_NORMAL, pixmap);
  Glib::RefPtr<Gdk::Window> window = get_window();
#if GTK_VERSION_GE(2,12)
  window->set_opacity(0.85);
#endif
  window->process_updates(true);
  show_all();
}

void TransparentMessage::run()
{
  _is_runing = true;
  Gtk::Main::run();
}

//------------------------------------------------------------------------------
void TransparentMessage::stop()
{
  Glib::Mutex::Lock lock(mutex);
  if (running_modal)
  {
    unrealize();
    running_modal = false;
    if (_is_runing)
    {
      Gtk::Main::quit();
      _is_runing = false;
    }
  }
  response= true;
}


void TransparentMessage::cancel_clicked()
{
  Glib::Mutex::Lock lock(mutex);
  if (cancel_slot && cancel_slot())
  {
    if (running_modal)
    {
      unrealize();
      running_modal = false;
      if (_is_runing)
      {
        Gtk::Main::quit();
        hide();
        _is_runing = false;
      }
    }
    response = false;
  }
}

//------------------------------------------------------------------------------
bool TransparentMessage::on_button_release_event(GdkEventButton* ev)
{
  hide();
  return false;
}

static TransparentMessage *tm = 0;
static TransparentMessage *tmc = 0;
#endif

//------------------------------------------------------------------------------
void UtilitiesImpl::show_wait_message(const std::string &title, const std::string &text)
{
  #ifdef USE_TRANSPARENT_MESSAGE
  if (!tm)
    tm = new TransparentMessage();

  if (tm)
    tm->show_message(title, text);
  #endif
}

//------------------------------------------------------------------------------
bool UtilitiesImpl::hide_wait_message()
{
  #ifdef USE_TRANSPARENT_MESSAGE
  if (tm)
  {
    tm->hide();
    delete tm;
    tm = 0;
  }
  #endif
  return false;
}


//-------------------------------------------------------------------------------

bool UtilitiesImpl::move_to_trash(const std::string &path)
{
  // trash in linux is chaos, just delete it
  if (g_file_test(path.c_str(), G_FILE_TEST_IS_DIR))
    base_rmdir_recursively(path.c_str());
  else
    g_remove(path.c_str());
  return true;
}


//------------------------------------------------------------------------------
void UtilitiesImpl::reveal_file(const std::string &path)
{
  const gchar *argv[] = {
    "xdg-open",
    base::dirname(path).c_str(),
    NULL 
  };
  GError *error = NULL;
  if (!g_spawn_async(NULL, (gchar**)argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error))
  {
    char *err = g_strdup_printf("Error opening folder with xdg-open: %s", error->message);
    g_error_free(error);
    std::runtime_error exc(err);
    g_free(err);
    throw exc;
  }

}


void UtilitiesImpl::set_thread_name(const std::string &name)
{
#ifdef HAVE_PRCTL_H 
  if (!name.empty())
    prctl(PR_SET_NAME, name.c_str(),0,0,0);
#endif
}

void UtilitiesImpl::beep()
{
  get_mainwindow()->get_window()->beep();
}

//------------------------------------------------------------------------------


bool UtilitiesImpl::run_cancelable_wait_message(const std::string &title, const std::string &text,
                                                 const boost::function<void ()> &start_task, const boost::function<bool ()> &cancel_task)
{
  if (!start_task)
    throw std::invalid_argument("start_task param cannot be empty");

  if (!tmc)
    tmc = new TransparentMessage();
  if (tmc)
  {
    tmc->show_message(title, text, sigc::mem_fun(&cancel_task,&boost::function<bool ()>::operator()));
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


void UtilitiesImpl::stop_cancelable_wait_message()
{
  if (tmc)
  {
    if (Utilities::in_main_thread())
      tmc->stop();
    else
      Utilities::perform_from_main_thread(sigc::bind_return(sigc::mem_fun(tmc, &TransparentMessage::stop), (void*)NULL));
  }
}

   
Glib::RefPtr<Gdk::Pixbuf> UtilitiesImpl::get_cached_icon(const std::string &icon)
{
  std::map<std::string, Glib::RefPtr<Gdk::Pixbuf> > cache;

  if (cache.find(icon) != cache.end())
    return cache[icon];

  if (icon == "folder")
  {
    Glib::RefPtr<Gdk::Pixbuf> pix = get_mainwindow()->render_icon(Gtk::Stock::DIRECTORY, Gtk::ICON_SIZE_MENU);
    cache[icon] = pix;
    return pix;
  }
  else
  {
    std::string path = mforms::App::get()->get_resource_path(icon);
    if (!path.empty() && g_file_test(path.c_str(), G_FILE_TEST_IS_REGULAR))
    {
      cache[icon] = Gdk::Pixbuf::create_from_file(path);
      return cache[icon];
    }
    else
      g_warning("Can't find icon %s", icon.c_str());
  }
  return Glib::RefPtr<Gdk::Pixbuf>();
}


//------------------------------------------------------------------------------

void UtilitiesImpl::init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_utilities_impl.show_message= &UtilitiesImpl::show_message;
  f->_utilities_impl.show_error= &UtilitiesImpl::show_error;
  f->_utilities_impl.show_warning= &UtilitiesImpl::show_warning;
  f->_utilities_impl.show_message_with_checkbox= &UtilitiesImpl::show_message_with_checkbox;
  f->_utilities_impl.show_wait_message= &UtilitiesImpl::show_wait_message;
  f->_utilities_impl.hide_wait_message= &UtilitiesImpl::hide_wait_message;
  f->_utilities_impl.run_cancelable_wait_message= &UtilitiesImpl::run_cancelable_wait_message;
  f->_utilities_impl.stop_cancelable_wait_message= &UtilitiesImpl::stop_cancelable_wait_message;

  f->_utilities_impl.set_clipboard_text= &UtilitiesImpl::set_clipboard_text;
  f->_utilities_impl.get_clipboard_text= &UtilitiesImpl::get_clipboard_text;
  f->_utilities_impl.open_url= &UtilitiesImpl::open_url;
  f->_utilities_impl.add_timeout= &UtilitiesImpl::add_timeout;
  f->_utilities_impl.cancel_timeout= &UtilitiesImpl::cancel_timeout;
  f->_utilities_impl.get_special_folder= &UtilitiesImpl::get_special_folder;
  f->_utilities_impl.store_password= &UtilitiesImpl::store_password;
  f->_utilities_impl.find_password= &UtilitiesImpl::find_password;
  f->_utilities_impl.forget_password= &UtilitiesImpl::forget_password;
  f->_utilities_impl.move_to_trash= &UtilitiesImpl::move_to_trash;
  f->_utilities_impl.reveal_file= &UtilitiesImpl::reveal_file;
  f->_utilities_impl.perform_from_main_thread = &MainThreadRequestQueue::perform;
  f->_utilities_impl.set_thread_name = &UtilitiesImpl::set_thread_name;
  f->_utilities_impl.beep = &UtilitiesImpl::beep;

  MainThreadRequestQueue::get(); // init from main thread
}


//---------------------------------------------------------------------------------

void MainThreadRequestQueue::from_main_thread()
{
  boost::shared_ptr<Request> req;
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


MainThreadRequestQueue::MainThreadRequestQueue()
{
  _disp.connect(sigc::mem_fun(this, &MainThreadRequestQueue::from_main_thread));
}


MainThreadRequestQueue *MainThreadRequestQueue::get()
{
  static MainThreadRequestQueue *q = new MainThreadRequestQueue();
  return q;
}

void *MainThreadRequestQueue::perform(const boost::function<void* ()> &slot, bool wait)
{
  if (Utilities::in_main_thread())
    return slot();
  else
  {
    MainThreadRequestQueue *self = MainThreadRequestQueue::get();

    boost::shared_ptr<Request> req(new Request());
    req->slot = slot;
    req->done = false;
    req->result = 0;
    {
      Glib::Mutex::Lock lock(self->_mutex);
      self->_queue.push_back(req);
    }
    self->_disp.emit();

    if (wait)
    {
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
