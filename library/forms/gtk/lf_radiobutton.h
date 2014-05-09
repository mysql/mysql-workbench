/* 
 * Copyright (c) 2008, 2012, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _LF_RADIOBUTTON_H_
#define _LF_RADIOBUTTON_H_

#include <mforms/radiobutton.h>


#include "lf_view.h"
#include "lf_panel.h"

namespace mforms {
namespace gtk {
  
static std::map<int, Gtk::RadioButton*> groups;

class RadioButtonImpl : public ButtonImpl
{
 protected:
  Gtk::RadioButton *_radio;
  int _group_id;

  virtual Gtk::Widget *get_outer() const { return _radio; }

  RadioButtonImpl(::mforms::RadioButton *self, int group_id)
    : ButtonImpl(self), _group_id(group_id)
  {
    _radio= Gtk::manage(new Gtk::RadioButton());
    _radio->set_use_underline(false);
    _button= _radio;
    
    if (groups.find(group_id) != groups.end())
    {
      Gtk::RadioButton::Group group(groups[group_id]->get_group());
      _radio->set_group(group);
    }
    else
    {
      groups[group_id] = _radio;
    }

    self->add_destroy_notify_callback(reinterpret_cast<void*>(group_id), &RadioButtonImpl::unregister_group);
    _radio->add_destroy_notify_callback(reinterpret_cast<void*>(group_id), &RadioButtonImpl::unregister_group);

    _radio->signal_toggled().connect(sigc::bind(sigc::ptr_fun(&RadioButtonImpl::callback), self));
    _radio->show();
  }
  
  static void* unregister_group(void *data)
  {
    int group_id = reinterpret_cast<intptr_t>(data);

    std::map<int, Gtk::RadioButton*>::iterator iter;
    
    if ((iter=groups.find(group_id)) != groups.end())
      groups.erase(iter);
    return NULL;
  }

  static void callback(::mforms::RadioButton* self)
  {
    if (!self->is_updating() && self->get_data<RadioButtonImpl>()->_radio->get_active())
      self->callback();
  }

  static bool create(::mforms::RadioButton *self, int group_id)
  {
    return new RadioButtonImpl(self, group_id);
  }

  static bool get_active(::mforms::RadioButton *self)
  {
    RadioButtonImpl* button = self->get_data<RadioButtonImpl>();
    
    if ( button )
    {
      return button->_radio->get_active();
    }
    return false;
  }
  
  
  static void set_active(::mforms::RadioButton *self, bool flag)
  {
    RadioButtonImpl* button = self->get_data<RadioButtonImpl>();
    
    if ( button )
    {
      button->_radio->set_active(flag);
    }
  }

  virtual void set_text(const std::string &text)
  {
    if (_label)
      _label->set_label(text);
    else
      _button->set_label(text);
  }

public:
  static void init()
  {
    ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

    f->_radio_impl.create= &RadioButtonImpl::create;
    f->_radio_impl.get_active= &RadioButtonImpl::get_active;
    f->_radio_impl.set_active= &RadioButtonImpl::set_active;
  }
};

}
}


#endif /* _LF_RADIOBUTTON_H_ */
