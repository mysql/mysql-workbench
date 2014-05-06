//!
//! \addtogroup linuxui Linux UI
//! @{
//! 

#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include <string>
#include <sigc++/sigc++.h>

#include "workbench/wb_context.h"

namespace bec
{
class GRTManager;
}

namespace Gtk
{
  class Window;
}

//==============================================================================


class MainForm;

//==============================================================================
//
//==============================================================================
//! Class Program mostly gathers together top level UI classes and top level 
//! backend classes like wb::WBContextUI under one hood, plus it
//! defines some callbacks-methods to pass to backend's wb::WBContext.
//! Also it has a timer-triggered idle tasks runner, see Program::idle_stuff() 
class Program
{
  public:
    Program(wb::WBOptions options);
    ~Program();
  
    void shutdown();

    static Program* get_instance() { return _instance; }    
    Gtk::Window* get_mainwindow() const;

  private: // Callbacks for backend
    int confirm_action_becb(const std::string& title, const std::string& msg, const std::string& default_btn, const std::string& alt_btn, const std::string& other_btn);
    std::string show_file_dialog_becb(const std::string& type, const std::string& title, const std::string& extensions);
    bool request_input_becb( const std::string& title, int flags, std::string& text);

    void finalize_initialization(wb::WBOptions *options);

    bool idle_stuff();
  private:
    wb::WBContextUI     *_wb_context_ui; //!< 
    wb::WBContext       *_wb_context;
    bec::GRTManager     *_grt_manager;
    MainForm            *_main_form;

    //sigc::signal<void>::iterator idle_signal_conn;
    sigc::connection     _sig_finalize_initialization;
    sigc::connection     _idle_signal_conn;
    Program(const Program&) {} // Forbid copy
    Program& operator=(const Program&) {return *this;} // Forbid copy
    static Program* _instance;
};

#endif

//!                                                                                                                                     
//! @}                                                                                                                                  
//!
