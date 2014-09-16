/*
 * Copyright (c) 2010, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "active_label.h"
#include <mforms/app.h>

//--------------------------------------------------------------------------------
ActiveLabel::ActiveLabel(const Glib::ustring& text, const sigc::slot<void> &close_callback)
            : _close_callback(close_callback)
            , _closeImage(mforms::App::get()->get_resource_path("Close_16x16.png"))
            , _text_label(text)
            , _menu(NULL)
            , _delete_menu(false)
{
  _closeImage.set_size_request(16, 16);

  _evbox.set_visible_window(false);
  _evbox.add(_closeImage);

  _evbox.signal_event().connect(sigc::mem_fun(this, &ActiveLabel::handle_event));
  _evbox.show_all();

  _text_label_eventbox.set_visible_window(false);
  _text_label_eventbox.add(_text_label);
  _text_label_eventbox.show_all();

  pack_start(_text_label_eventbox);
  pack_start(_evbox);

#if GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 20
  pack_start(_spinner);
  _spinner.set_size_request(16, 16);  //  Set the same size as the _closeImage, so the tab won't resize when swaping
  _spinner.hide();
#endif

  signal_button_press_event().connect(sigc::mem_fun(this, &ActiveLabel::button_press_slot));
}

//--------------------------------------------------------------------------------
ActiveLabel::~ActiveLabel()
{
  if (_delete_menu)
    delete _menu;
}

//--------------------------------------------------------------------------------
bool ActiveLabel::handle_event(GdkEvent* e)
{
  switch (e->type)
  {
    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *evb = (GdkEventButton*)e;
        if (evb->button == 1)
          _close_callback();
        break;
      }
    default:
      break;
  }

  return false;
}
//--------------------------------------------------------------------------------
void ActiveLabel::set_menu(mforms::Menu* m, bool delete_when_done)
{
  this->_menu = m;
  _delete_menu = delete_when_done;
}

bool ActiveLabel::has_menu()
{
  return this->_menu != NULL;
}

//--------------------------------------------------------------------------------
void ActiveLabel::set_text(const std::string& lbl)
{
  _text_label.set_text(lbl);
}

//--------------------------------------------------------------------------------
bool ActiveLabel::button_press_slot(GdkEventButton* evb)
{
  if (evb->button == 3 && _menu && !_menu->empty())
    _menu->popup_at(0, evb->x, evb->y);
  return false;
}

//--------------------------------------------------------------------------------
void ActiveLabel::start_busy()
{
#if GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 20
  _evbox.hide();

  _spinner.show();
  _spinner.start();
#endif
}

//--------------------------------------------------------------------------------
void ActiveLabel::stop_busy()
{
#if GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 20
  _spinner.stop();
  _spinner.hide();
  
  _evbox.show();
#endif
}
