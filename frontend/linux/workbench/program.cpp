#include "grtpp_module_python.h"

#include "gtk/lf_mforms.h"

// linux ui includes
#include "gtk_helpers.h"
#include "program.h"
#include "main_form.h"
#include "wbdbg.h"

#include <gtkmm/main.h>
#include <gtkmm/stock.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/accelgroup.h>
#include <gtkmm/builder.h>

// the rest, backend, etc ...
#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"
#include "workbench/wb_module.h"

#include "grtpp_util.h" // for strfmt

#include "sqlide_main.h"

#include "base/string_utilities.h"

using base::strfmt;

#define REFRESH_TIMER 250

#include <gtkmm/messagedialog.h>

Program* Program::_instance = 0;

static void flush_main_thread()
{
  while (Gtk::Main::events_pending())
  {
    if (Gtk::Main::iteration(false))
    {
      // return value of true means quit() was called 
      Gtk::Main::quit();
      break;
    }
  }
}

//------------------------------------------------------------------------------
Program::Program(wb::WBOptions wboptions)
{
  _instance = this;
  // Setup backend stuff
  _wb_context_ui = new wb::WBContextUI(wboptions.verbose);
  _wb_context = _wb_context_ui->get_wb();

  _grt_manager = _wb_context->get_grt_manager();

#ifdef ENBLE_DEBUG
  if ( !getenv("MWB_DATA_DIR") )
  {
    const char *path = "../share/mysql-workbench";
    g_message("MWB_DATA_DIR is unset! Setting MWB_DATA_DIR to predifined value '%s'", path);
    setenv("MWB_DATA_DIR", path, 1);
  }

  if ( !getenv("MWB_MODULE_DIR") )
  {
    const char *path = "../lib/mysql-workbench/modules";
    g_message("MWB_MODULE_DIR is unset! Setting MWB_MODULE_DIR to predifined value '%s'", path);
    setenv("MWB_MODULE_DIR", path, 1);
  }

  if ( !getenv("MWB_LIBRARY_DIR") )
  {
    const char *path = "../share/mysql-workbench/libraries";
    g_message("MWB_LIBRARY_DIR is unset! Setting MWB_LIBRARY_DIR to predifined value '%s'", path);
    setenv("MWB_LIBRARY_DIR", path, 1);
  }

  if ( !getenv("MWB_PLUGIN_DIR") )
  {
    const char *path = "../lib/mysql-workbench";
    g_message("MWB_PLUGIN_DIR is unset! Setting MWB_PLUGIN_DIR to predifined value '%s'", path);
    setenv("MWB_PLUGIN_DIR", path, 1);
  }
#endif
  if ( !getenv("MWB_DATA_DIR") || (!getenv("MWB_MODULE_DIR")))
  {
    g_print("Please start Workbench through mysql-workbench instead of calling mysql-workbench-bin directly\n");
    exit(1);
  }

  _grt_manager->set_datadir(getenv("MWB_DATA_DIR"));

  // Main form holds UI code, Glade wrapper, etc ...
  _main_form = new MainForm(_wb_context_ui);

  // Define a set of methods which backend can call to interact with user and frontend
  wb::WBFrontendCallbacks wbcallbacks;
  
  // Assign those callback methods
  wbcallbacks.show_file_dialog= sigc::mem_fun(this, &Program::show_file_dialog_becb);
  wbcallbacks.show_status_text= sigc::mem_fun(_main_form, &MainForm::show_status_text_becb);
  wbcallbacks.request_input= sigc::mem_fun(this, &Program::request_input_becb);
  wbcallbacks.open_editor= sigc::mem_fun(_main_form, &MainForm::open_plugin_becb);
  wbcallbacks.show_editor= sigc::mem_fun(_main_form, &MainForm::show_plugin_becb);
  wbcallbacks.hide_editor= sigc::mem_fun(_main_form, &MainForm::hide_plugin_becb);
  wbcallbacks.perform_command= sigc::mem_fun(_main_form, &MainForm::perform_command_becb);
  wbcallbacks.create_diagram= sigc::mem_fun(_main_form, &MainForm::create_view_becb);
  wbcallbacks.destroy_view= sigc::mem_fun(_main_form, &MainForm::destroy_view_becb);
  wbcallbacks.switched_view= sigc::mem_fun(_main_form, &MainForm::switched_view_becb);
  wbcallbacks.create_main_form_view= sigc::mem_fun(_main_form, &MainForm::create_main_form_view_becb);
  wbcallbacks.destroy_main_form_view= sigc::mem_fun(_main_form, &MainForm::destroy_main_form_view_becb);
  wbcallbacks.tool_changed= sigc::mem_fun(_main_form, &MainForm::tool_changed_becb);
  wbcallbacks.refresh_gui= sigc::mem_fun(_main_form, &MainForm::refresh_gui_becb);
  wbcallbacks.lock_gui= sigc::mem_fun(_main_form, &MainForm::lock_gui_becb);
  wbcallbacks.quit_application= sigc::mem_fun(_main_form, &MainForm::quit_app_becb);

  wboptions.basedir = getenv("MWB_DATA_DIR");
  wboptions.plugin_search_path = getenv("MWB_PLUGIN_DIR");
  wboptions.struct_search_path = wboptions.basedir + "/grt";
  wboptions.module_search_path = getenv("MWB_MODULE_DIR");
  wboptions.library_search_path = getenv("MWB_LIBRARY_DIR");
  wboptions.cdbc_driver_search_path = getenv("DBC_DRIVER_PATH")?:"";
  if (wboptions.cdbc_driver_search_path.empty())
    wboptions.cdbc_driver_search_path= wboptions.library_search_path;
  wboptions.user_data_dir = std::string(g_get_home_dir()).append("/.mysql/workbench");


  _wb_context_ui->init(&wbcallbacks, &wboptions);

  _wb_context_ui->get_wb()->get_grt_manager()->get_dispatcher()->set_main_thread_flush_and_wait(flush_main_thread);

  {
    std::string form_name;
    sigc::slot<FormViewBase*, boost::shared_ptr<bec::UIForm> > form_creator;

    setup_sqlide(_wb_context_ui, form_name, form_creator);

    _main_form->register_form_view_factory(form_name, form_creator);
  }

  _main_form->setup_ui();

  // show the window only when everything is done
  _sig_finalize_initialization = Glib::signal_idle().connect(sigc::bind_return(sigc::bind(sigc::mem_fun(this, &Program::finalize_initialization), &wboptions), false));
//  _main_form->show();
  
  _idle_signal_conn = Glib::signal_timeout().connect(sigc::mem_fun(this, &Program::idle_stuff), REFRESH_TIMER);
}

//------------------------------------------------------------------------------
Program::~Program()
{ 
}


void Program::finalize_initialization(wb::WBOptions *options)
{
  _main_form->show();
  
  _wb_context_ui->init_finish(options);
}



bool Program::idle_stuff()
{
  // if there are tasks to be executed, schedule it to be done when idle so that the timer
  // doesn't get blocked during its execution
  Glib::signal_idle().connect(sigc::bind_return(sigc::mem_fun(_wb_context, &wb::WBContext::flush_idle_tasks), false));

  //_wb_context->flush_idle_tasks();
  return true;
}


void Program::shutdown()
{
  _main_form->exiting();
  
  _wb_context_ui->finalize();
  
  //sigc::connection conn(idle_signal_conn);
  _sig_finalize_initialization.disconnect();
  _idle_signal_conn.disconnect();
  
  _wb_context->get_grt_manager()->terminate();

  _grt_manager->get_dispatcher()->shutdown();

  delete _main_form;
  _main_form= 0;

// is not working well
 // delete _wb_context_ui;
  _wb_context_ui= 0;
}


//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

struct GtkAutoLock
{
  GtkAutoLock() {
    if (getenv("WB_ADD_LOCKS"))
      gdk_threads_enter();
  }
  ~GtkAutoLock() {
    if (getenv("WB_ADD_LOCKS"))
      gdk_threads_leave();
  }
};

int Program::confirm_action_becb(const std::string& title, const std::string& msg, const std::string& default_btn, const std::string& alt_btn, const std::string& other_btn)
{

  GtkAutoLock lock;

  Gtk::MessageDialog dlg(strfmt("<b>%s</b>\n%s", title.c_str(), msg.c_str()), true, Gtk::MESSAGE_QUESTION,
                         Gtk::BUTTONS_NONE, true);
  dlg.set_title(title);
  
  dlg.add_button(default_btn, 1);
  if (!other_btn.empty())
    dlg.add_button(other_btn, 3);
  dlg.add_button(alt_btn, 2);

  dlg.set_transient_for(*get_mainwindow());

  int response= dlg.run();
  
  switch (response)
  {
  case 1:
    return 1;
  case 2:
    return 0;
  case 3:
    return -1;
  default: // Escape
    if (default_btn == _("Cancel"))
      return 1;
    else if (alt_btn == _("Cancel"))
      return 0;
    else if (other_btn == _("Cancel"))
      return -1;
    return 0;
}
}

//------------------------------------------------------------------------------
std::string Program::show_file_dialog_becb(const std::string& type
                                          ,const std::string& title
                                          ,const std::string& extensions
                                          )
{
  Gtk::FileChooserDialog dlg(title
                             ,( type == "open" 
                                  ? Gtk::FILE_CHOOSER_ACTION_OPEN 
                                  : Gtk::FILE_CHOOSER_ACTION_SAVE
                              )
                            );

  dlg.set_transient_for(*(_main_form->get_mainwindow()));
  
  dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  if (type == "open")
    dlg.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
  else
    dlg.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

  
  // extensions format:
  // AAA Files (*.aaa)|*.aaa,BBB Files (*.bbb)

  std::vector<std::string> exts(base::split(extensions,","));
  std::string default_ext;

  for (std::vector<std::string>::const_iterator iter= exts.begin();
       iter != exts.end(); ++iter)
  {
    Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();

    if (iter->find('|') != std::string::npos)
    {
      std::string descr, ext;
      
      descr= iter->substr(0, iter->find('|'));
      ext= iter->substr(iter->find('|')+1);

      if (default_ext.empty())
      {
        default_ext = ext;
        if (default_ext[0] == '*')
          default_ext = default_ext.substr(1);
        if (default_ext[0] == '.')
          default_ext = default_ext.substr(1);
      }
      filter->add_pattern(ext);
      filter->set_name(descr);
    }
    else if (*iter == "mwb")
    {
      if (default_ext.empty()) default_ext = "mwb";
      filter->add_pattern("*.mwb");
      filter->set_name("MySQL Workbench Models (*.mwb)");
    }
    else if (*iter == "sql")
    {
      if (default_ext.empty()) default_ext = "sql";
      filter->add_pattern("*.sql");
      filter->set_name("SQL Script Files");
    }
    else
    {
      if (default_ext.empty()) default_ext = *iter;
      filter->add_pattern("*."+*iter);
      filter->set_name(strfmt("%s files (*.%s)", iter->c_str(), iter->c_str()));
    }
    dlg.add_filter(filter);
  }

  Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
  filter->add_pattern("*");
  filter->set_name("All Files");
  dlg.add_filter(filter);

  std::string file("");

  if (!default_ext.empty())
    default_ext = "."+default_ext;

  for (;;)
  {
    
    int result = dlg.run();

    if ( result == Gtk::RESPONSE_OK)
    {
      file = dlg.get_filename();
      if (!bec::has_suffix(file, default_ext) && type == "save")
	file = file + default_ext;

      if (type == "save" && g_file_test(file.c_str(), G_FILE_TEST_EXISTS))
      {
        if (confirm_action_becb(strfmt(_("\"%s\" Already Exists. Do you want to replace it?"), file.c_str()),
                              strfmt(_("Replacing it will overwrite its current contents.")),
                              _("Replace"), _("Cancel"), "") != 1)
          continue;
      }
    }
    break;
  }
  
  return file;
}

//------------------------------------------------------------------------------
bool Program::request_input_becb( const std::string& title, int flags, std::string& text)
{
  Glib::RefPtr<Gtk::Builder> ui= Gtk::Builder::create_from_file(_grt_manager->get_data_file_path("input_dialog.glade"));
  Gtk::Dialog *win;
  
  ui->get_widget("input_dialog", win);
  
  Gtk::Label *label;
  ui->get_widget("label", label);
  label->set_text(title);
  
  Gtk::Entry *entry;
  ui->get_widget("entry", entry);
  entry->set_text(text);
  
  Gtk::Button *btn;
  ui->get_widget("ok", btn);
  btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(win, &Gtk::Dialog::response), 1));
  
  ui->get_widget("cancel", btn);
  btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(win, &Gtk::Dialog::response), 0));
  
  if (flags & wb::InputPassword)
    entry->set_visibility(false);
  
  win->show();
  
  bool ret = false;
  if (win->run() == 1)
  {
    text = entry->get_text();
    ret = true;
  }
  
  win->hide();
  
  return ret;
}

//------------------------------------------------------------------------------
Gtk::Window* Program::get_mainwindow() const
{
  return _main_form->get_mainwindow();
}

// get_mainwindow is declared in gtk_helpers.h
//------------------------------------------------------------------------------
void* get_mainwindow_impl()
{
  return Program::get_instance()->get_mainwindow();
}
