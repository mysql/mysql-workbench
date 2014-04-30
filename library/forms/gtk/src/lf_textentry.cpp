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

#include "../lf_mforms.h"
#include "../lf_textentry.h"

namespace mforms {
namespace gtk {

bool TextEntryImpl::create(::mforms::TextEntry *self, TextEntryType type)
{
  return new TextEntryImpl(self, type);
}

void TextEntryImpl::set_text(::mforms::TextEntry *self, const std::string &text)
{
  TextEntryImpl* cb = self->get_data<TextEntryImpl>();

  if ( cb )
  {
    cb->set_text(text);
  }
}

void TextEntryImpl::set_placeholder_text(::mforms::TextEntry *self, const std::string &text)
{
  TextEntryImpl* cb = self->get_data<TextEntryImpl>();

  if ( cb )
  {
    cb->set_placeholder_text(text);
  }
}

void TextEntryImpl::set_max_length(::mforms::TextEntry *self, int len)
{
  TextEntryImpl* cb = self->get_data<TextEntryImpl>();

  if ( cb )
  {
    cb->_entry->set_max_length(len);
  }
}

std::string TextEntryImpl::get_text(::mforms::TextEntry *self)
{
  TextEntryImpl* cb = self->get_data<TextEntryImpl>();
  std::string ret("");
  if (cb && cb->_has_real_text)
  {
    ret = cb->_entry->get_text().raw();
  }
  return ret;
}

void TextEntryImpl::set_read_only(::mforms::TextEntry *self, bool flag)
{
  TextEntryImpl* cb = self->get_data<TextEntryImpl>();
  if (cb && cb->_entry)
    cb->_entry->set_editable(!flag);
}

void TextEntryImpl::set_bordered(::mforms::TextEntry *self, bool flag)
{
  TextEntryImpl* te = self->get_data<TextEntryImpl>();
  if(te)
    te->_entry->set_has_frame(flag);
}

TextEntryImpl::TextEntryImpl(::mforms::TextEntry *self, TextEntryType type)
  : ViewImpl(self), _has_real_text(false), _changing_text(false)
{
  _entry= Gtk::manage(new Gtk::Entry());
  _type = type;
  switch (type)
  {
  case mforms::NormalEntry:
    break;
  case mforms::PasswordEntry:
    _entry->set_visibility(false);
    break;
  case mforms::SearchEntry:
#if GTK_VERSION_GE(2, 16)
    _entry->set_icon_from_stock(Gtk::Stock::FIND);
    _entry->signal_icon_press().connect(sigc::mem_fun(this, &TextEntryImpl::icon_pressed));
#endif
    break;
  }
  _entry->signal_changed().connect(sigc::bind(sigc::mem_fun(this, &TextEntryImpl::changed), self));
  _entry->signal_activate().connect(sigc::bind(sigc::mem_fun(this, &TextEntryImpl::activated), self));
  _entry->signal_key_press_event().connect(sigc::bind(sigc::mem_fun(this, &TextEntryImpl::key_press), self));
  _entry->signal_focus_in_event().connect_notify(sigc::mem_fun(this, &TextEntryImpl::focus_in));
  _entry->signal_focus_out_event().connect_notify(sigc::mem_fun(this, &TextEntryImpl::focus_out));
  _entry->show();
  _text_color = _entry->get_style()->get_text(Gtk::STATE_NORMAL);
  Gdk::Color color("#888888");
  _placeholder_color = color;
}

#if GTK_VERSION_GE(2, 16)
void TextEntryImpl::icon_pressed(Gtk::EntryIconPosition pos, const GdkEventButton *ev)
{
  if (pos == Gtk::ENTRY_ICON_SECONDARY)
    set_text("");
}
#endif

void TextEntryImpl::activated(mforms::TextEntry *self)
{
  self->action(mforms::EntryActivate);
}

bool TextEntryImpl::key_press(GdkEventKey *event, mforms::TextEntry *self)
{
  if (event->keyval == GDK_Up)
  {
    if (event->state & GDK_CONTROL_MASK)
      self->action(mforms::EntryCKeyUp);
    else
      self->action(mforms::EntryKeyUp);
    return true;
  }
  else if (event->keyval == GDK_Down)
  {
    if (event->state & GDK_CONTROL_MASK)
      self->action(mforms::EntryCKeyDown);
    else
      self->action(mforms::EntryKeyDown);
    return true;
  }
  return false;
}

void TextEntryImpl::changed(mforms::TextEntry *self)
{
  if (_changing_text)
    return;
  if (_has_real_text)
  {
#if GTK_VERSION_GT(2, 16)
    if (_type == mforms::SearchEntry)
    {
      if (_entry->get_text().empty())
        _entry->set_icon_from_pixbuf(Glib::RefPtr<Gdk::Pixbuf>(), Gtk::ENTRY_ICON_SECONDARY);
      else
        _entry->set_icon_from_stock(Gtk::Stock::CLEAR, Gtk::ENTRY_ICON_SECONDARY);
    }
#endif
    if (_entry->get_text().empty())
      _has_real_text = false;
  }
  else
    _has_real_text = !_entry->get_text().empty();
  self->callback();
}

void TextEntryImpl::set_front_color(const std::string &color)
{
  Gdk::Color gcolor(color);
  this->_text_color = gcolor;
}

void TextEntryImpl::set_text(const std::string &text)
{
  if (!text.empty())
  {
    if (!_has_real_text)
      focus_out(NULL);
    _has_real_text = true;
  }
  else
  {
    if (_has_real_text)
      focus_in(NULL);
    _has_real_text = false;
  }
  _entry->set_text(text);
}

void TextEntryImpl::set_placeholder_text(const std::string &text)
{
  _placeholder = text;
  if (!_entry->has_focus() && !_has_real_text)
    focus_out(NULL);
}

void TextEntryImpl::set_placeholder_color(::mforms::TextEntry *self, const std::string &color)
{
  TextEntryImpl* te = self->get_data<TextEntryImpl>();
  if(te)
  {
    Gdk::Color gcolor(color);
    te->_placeholder_color = gcolor;
  }
}

void TextEntryImpl::focus_in(GdkEventFocus*)
{
  if (!_has_real_text)
  {
    _entry->modify_text(Gtk::STATE_NORMAL, _text_color);
    _changing_text = true;
    _entry->set_text("");
    _changing_text = false;
  }
}

void TextEntryImpl::focus_out(GdkEventFocus*)
{
  if (!_has_real_text && !_placeholder.empty())
  {
    _entry->get_colormap()->alloc_color(_placeholder_color);
    _entry->modify_text(Gtk::STATE_NORMAL, _placeholder_color);
    _changing_text = true;
    _entry->set_text(_placeholder);
    _changing_text = false;
  }
}

void TextEntryImpl::init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_textentry_impl.create   = &TextEntryImpl::create;
  f->_textentry_impl.set_text = &TextEntryImpl::set_text;
  f->_textentry_impl.set_max_length = &TextEntryImpl::set_max_length;
  f->_textentry_impl.get_text = &TextEntryImpl::get_text;
  f->_textentry_impl.set_read_only= &TextEntryImpl::set_read_only;
  f->_textentry_impl.set_placeholder_text = &TextEntryImpl::set_placeholder_text;
  f->_textentry_impl.set_placeholder_color = &TextEntryImpl::set_placeholder_color;
  f->_textentry_impl.set_bordered = &TextEntryImpl::set_bordered;
}

};
};
