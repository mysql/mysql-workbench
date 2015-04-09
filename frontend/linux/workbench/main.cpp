#include "gtk/lf_mforms.h"

#include "lf_wizard.h"
#include "lf_utilities.h"
#include <mforms/mforms.h>
#include <gtkmm.h>
#include <stdio.h>
#include <sys/wait.h>
#include "program.h"
#include "gtk_helpers.h"
#include "base/string_utilities.h"
#include "base/threading.h"
#include "base/log.h"
#include <gdk/gdkx.h>
#include <iostream>
DEFAULT_LOG_DOMAIN("main")

#if defined(HAVE_GNOME_KEYRING) || defined(HAVE_OLD_GNOME_KEYRING)
extern "C" {
// gnome-keyring has been deprecated in favor of libsecret
// More informations can be found here  https://mail.gnome.org/archives/commits-list/2013-October/msg08876.html
// Below defines will turn off deprecations and allow build with never Gnome until we will not move to libsecret.
  #define GNOME_KEYRING_DEPRECATED
  #define GNOME_KEYRING_DEPRECATED_FOR(x)
  #include <gnome-keyring.h>
};
#endif
#include <X11/Xlib.h>

using base::strfmt;

//==============================================================================
// We need recursive mutex to lock gtk main loop. For that we supply enter/leave
// callbacks to glib via gdk_threads_set_lock_functions(). Out lock/unlock
// functions operate on static rec mutex.
static base::RecMutex custom_gdk_rec_mutex;

static void custom_gdk_threads_enter()
{
  custom_gdk_rec_mutex.lock();
}

static void custom_gdk_threads_leave()
{
  custom_gdk_rec_mutex.unlock();
}

inline void init_gdk_thread_callbacks()
{
  gdk_threads_set_lock_functions(G_CALLBACK(&custom_gdk_threads_enter), G_CALLBACK(&custom_gdk_threads_leave));
}

//==============================================================================

extern  void lf_record_grid_init();

int main(int argc, char **argv)
{



  if (!getenv("MWB_DATA_DIR"))
  {
    std::string script_name = argv[0];
    std::string termination = "-bin";
    
    if (base::ends_with(script_name, termination))
      script_name = base::left(script_name, script_name.length() - termination.length());
    
    std::cout << "To start MySQL Workbench, use " << script_name << " instead of " << argv[0] << std::endl;
    exit(1);
  }

#ifdef ENABLE_DEBUG
  if (getenv("DEBUG_CRITICAL"))
    g_log_set_always_fatal((GLogLevelFlags)(G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_ERROR));
#endif

  init_gdk_thread_callbacks(); // This call MUST be before g_threads_init is called
  base::threading_init();
  gdk_threads_init();
  // process cmdline options
  std::string user_data_dir = std::string(g_get_home_dir()).append("/.mysql/workbench");
  base::Logger log(user_data_dir, getenv("MWB_LOG_TO_STDERR")!=NULL);

  #if defined(HAVE_GNOME_KEYRING) || defined(HAVE_OLD_GNOME_KEYRING)
  if (getenv("WB_NO_GNOME_KEYRING"))
    log_info("WB_NO_GNOME_KEYRING environment variable has been set. Stored passwords will be lost once quit.\n");
  else
  {
    if (!gnome_keyring_is_available())
    {
      setenv("WB_NO_GNOME_KEYRING", "1", 1);
      log_error("Can't communicate with gnome-keyring, it's probably not running. Stored passwords will be lost once quit.\n");
    }
  }
  #endif

  wb::WBOptions wboptions;
  wboptions.user_data_dir = user_data_dir;


  wboptions.basedir = getenv("MWB_DATA_DIR");
  wboptions.plugin_search_path = getenv("MWB_PLUGIN_DIR");
  wboptions.struct_search_path = wboptions.basedir + "/grt";
  wboptions.module_search_path = getenv("MWB_MODULE_DIR");

  g_set_application_name("MySQL Workbench");

  int retval = 0;
  if (!wboptions.parse_args(argv, argc, &retval))
    return retval;

  if (getenv("WB_VERBOSE"))
    wboptions.verbose = true;

  mforms::gtk::init(getenv("WB_FORCE_SYSTEM_COLORS") != NULL);
  mforms::gtk::WizardImpl::set_icon_path(wboptions.basedir+"/images");
  {
    lf_record_grid_init();
  }



  gdk_threads_enter();
  Gtk::Main app(argc, argv);

//  Gtk::CssProvider::get_default()->load_from_path(wboptions.basedir+"/workbench.rc");

  if (getenv("XSYNC"))
  {
    g_message("Enabling XSynchronize()");
    XSynchronize(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), 1);
  }

  Program program(wboptions);
  
  mforms::gtk::check();
  
  for (;;)
  {
    try
    {
      app.run();
      break;
    }
    catch (const std::exception &exc)
    {
      g_warning("ERROR: unhandled exception %s", exc.what());

      Gtk::MessageDialog dlg(strfmt("<b>Unhandled Exception</b>\nAn unhandled exception has occurred (%s).\nInternal state may be inconsystent, please save your work to a temporary file and restart Workbench.\nPlease report this with details on how to repeat at http://bugs.mysql.com", exc.what()),
                             true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
      dlg.set_title(_("Error"));

      dlg.set_transient_for(*get_mainwindow());
      dlg.run();
    }
    catch (const Glib::Exception &exc)
    {
      g_warning("ERROR: unhandled exception %s", exc.what().c_str());

      Gtk::MessageDialog dlg(strfmt("<b>Unhandled Exception</b>\nAn unhandled exception has occurred (%s).\nInternal state may be inconsystent, please save your work to a temporary file and restart Workbench.\nPlease report this with details on how to repeat at http://bugs.mysql.com", exc.what().c_str()),
                             true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
      dlg.set_title(_("Error"));
      dlg.set_transient_for(*get_mainwindow());
      dlg.run();
    }
    catch (...)
    {
      g_warning("ERROR: unhandled exception");

      Gtk::MessageDialog dlg(strfmt("<b>Unhandled Exception</b>\nAn unhandled exception has occurred.\nInternal state may be inconsystent, please save your work to a temporary file and restart Workbench.\nPlease report this with details on how to repeat at http://bugs.mysql.com"),
                             true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
      dlg.set_title(_("Error"));

      dlg.set_transient_for(*get_mainwindow());
      dlg.run();
    }
  }

  program.shutdown();
  gdk_threads_leave();

  return 0;
}








#ifdef ENABLE_DEBUG
#ifdef __GNUC__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <cxxabi.h>
#include <sys/time.h>

static struct timeval start_time;
static int trace_depth= 0;
static FILE *trace_file= 0;
bool trace_on= true;

extern "C" {

// instrumentation code for tracing function calls. must compile with -finstrument-functions
void __cyg_profile_func_enter(void *func_address, void *call_site) __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *func_address, void *call_site) __attribute__((no_instrument_function));
static char *resolve_function(void *addr) __attribute__((no_instrument_function));

static char *resolve_function(void *addr)
{
  Dl_info info;
  int s;

  dladdr(addr, &info);

  return __cxxabiv1::__cxa_demangle(info.dli_sname, NULL, NULL, &s);
}

void __cyg_profile_func_enter(void *func_address, void *call_site)
{
  if (!trace_file)
  {
    gettimeofday(&start_time, NULL);
    trace_file= fopen("trace.txt", "w+");
  }
  
  if (trace_on)
  {
    char *s= resolve_function(func_address);
    struct timeval t;
    gettimeofday(&t, NULL);
    

    ++trace_depth;

    fprintf(trace_file ?: stdout, "TRACE:%.4f:%*senter %s\n",
            (t.tv_sec + t.tv_usec / 1000000.0) - (start_time.tv_sec + start_time.tv_usec / 1000000.0),
            trace_depth, "", s);
    free(s);
  }
}


void __cyg_profile_func_exit(void *func_address, void *call_site)
{
  if (trace_on)
  {
    char *s= resolve_function(func_address);
    struct timeval t;
    gettimeofday(&t, NULL);

    fprintf(trace_file ?: stdout, "TRACE:%.4f:%*sleave %s\n",
            (t.tv_sec + t.tv_usec / 1000000.0) - (start_time.tv_sec + start_time.tv_usec / 1000000.0),
            trace_depth, "", s);

    --trace_depth;

    free(s);
  }
}

};

#endif
#endif // ENABLE_DEBUG
