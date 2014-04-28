/* 
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _LF_PROGRESSBAR_H_
#define _LF_PROGRESSBAR_H_


#include "mforms/progressbar.h"

#include "lf_view.h"

namespace mforms {
namespace gtk {

class ProgressBarImpl : public ViewImpl
{
 protected:
  Gtk::ProgressBar *_pbar;
  sigc::connection     _pbar_pulser;
  virtual Gtk::Widget *get_outer() const { return _pbar; }
  

  ProgressBarImpl(::mforms::ProgressBar *self)
    : ViewImpl(self)
  {
    _pbar= Gtk::manage(new Gtk::ProgressBar());
    _pbar->show();
    setup();
  }

  static bool create(::mforms::ProgressBar *self)
  {
    return new ProgressBarImpl(self) != 0;
  }
  
  static void set_value(::mforms::ProgressBar *self, float pct)
  {
    ProgressBarImpl *progressbar= self->get_data<ProgressBarImpl>();
    
    if (progressbar)
      progressbar->_pbar->set_fraction(pct);
  }

  bool pulse()
  {
    if (_pbar)
      this->_pbar->pulse();

    return true;
  }

  void start()
  {
    if (_pbar != NULL && _pbar_pulser.empty())
      _pbar_pulser = Glib::signal_timeout().connect(sigc::mem_fun(this, &ProgressBarImpl::pulse), 125);
  }

  void stop()
  {
    if (!_pbar_pulser.empty())
      _pbar_pulser.disconnect();

    if (_pbar)
      _pbar->set_fraction(0.0);
  }

  ~ProgressBarImpl()
  {
    if (_pbar && !_pbar_pulser.empty())
    {
      _pbar_pulser.disconnect();
    }
  }


  static void set_started(::mforms::ProgressBar *self, bool flag)
  {
    ProgressBarImpl *progressbar= self->get_data<ProgressBarImpl>();
    if (progressbar)
    {
      if (flag)
        progressbar->start();
      else
        progressbar->stop();
    }
  }

  static void set_indeterminate(::mforms::ProgressBar *self, bool flag)
  {
  }

 public:
  static void init()
  {
    ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

    f->_progressbar_impl.create         = &ProgressBarImpl::create;
    f->_progressbar_impl.set_value      = &ProgressBarImpl::set_value;
    f->_progressbar_impl.set_started    = &ProgressBarImpl::set_started;
    f->_progressbar_impl.set_indeterminate= &ProgressBarImpl::set_indeterminate;
  }
};

};
};


#endif /* _LF_PROGRESSBAR_H_ */
