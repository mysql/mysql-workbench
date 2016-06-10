/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "main_app.h"

runtime::loop::loop() : _loop(nullptr) {}


runtime::loop::~loop()
{
  if (_loop != nullptr)
    throw std::runtime_error("Loop not stopped");
}

void runtime::loop::run()
{
  if (!runtime::app::get().isMainThread())
    throw std::runtime_error("Loops are only supported from inside of main thread");

  if (_loop != nullptr)
    throw std::runtime_error("Loop already started");

  _loop = g_main_loop_new(NULL, FALSE);
  gdk_threads_leave();
  g_main_loop_run(_loop);
  gdk_threads_enter();
  g_main_loop_unref(_loop);
  _loop = nullptr;
}

void runtime::loop::quit()
{
  if (_loop && g_main_loop_is_running (_loop))
     g_main_loop_quit (_loop);
}

runtime::app::app()
{
  _mainThread = g_thread_self();
}

runtime::app& runtime::app::get()
{
  static app app;
  return app;
}

runtime::app::~app()
{

}

void runtime::app::init(const std::string &name, int argc, char **argv)
{

  _app = Gtk::Application::create(argc, argv, name, Gio::APPLICATION_HANDLES_COMMAND_LINE);
  _app->signal_command_line().connect(sigc::mem_fun(this, &app::onCommand), false);

  // We need to add separate workbench help option entry,
  // so we can handle it in our parse
  #if GLIB_CHECK_VERSION(2, 48, 0)
  #else
  GOptionEntry entries[] = {
      { "help-wb", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, nullptr, "Show MySQL Workbench Help", nullptr },
      { NULL, 0, 0, G_OPTION_ARG_NONE, nullptr, NULL, nullptr} // Last entry must be null, but gcc will complain, make it happy :)
  };

  g_application_add_main_option_entries((GApplication*)_app->gobj(), entries);
  #endif

  _app->signal_activate().connect([&](){
    bool activate = true;
    if (onBeforeActivate)
      activate = onBeforeActivate();

    if (!activate)
      return;

    _app->hold(); // we need to add additional app reference
               // without it, the window will not be shown
    if (onActivate)
      onActivate();
  });
}

int runtime::app::onCommand(const Glib::RefPtr<Gio::ApplicationCommandLine> &appCmdLine)
{
  int argc = 0, retval;
  char** argv = appCmdLine->get_arguments(argc);
  #if GLIB_CHECK_VERSION(2, 48, 0)
  #else

  const auto options = appCmdLine->get_options_dict();

  // We need to handle help different way in gtk
  bool showHelp = false;
  options->lookup_value("help-wb", showHelp);

  if (showHelp && showWbHelpCb)
  {
    showWbHelpCb(argv[0]);
    return EXIT_SUCCESS;
  }

  #endif
  if (parseParams && !parseParams(argc, argv, &retval))
    return retval;

  _app->activate();
  return EXIT_SUCCESS;
}

int runtime::app::run()
{
  if (_app)
    return _app->run();
  return EXIT_FAILURE;
}

void runtime::app::quit()
{
  if (_app)
    _app->quit();
}

bool runtime::app::isMainThread()
{
  return _mainThread == g_thread_self();
}

