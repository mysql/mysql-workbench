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

#include "../lf_mforms.h"
#include "../lf_textbox.h"

namespace mforms {
namespace gtk {

/*
Glib::RefPtr<Gtk::TextBuffer::Tag> TextBoxImpl::tag_for_text_attributes(const mforms::TextAttributes &attr)
{
  unsigned int hash = 0;
  if (attr.color.is_valid())
  {
    hash |= (unsigned int)(attr.color.red * 255) << 8;
    hash |= (unsigned int)(attr.color.green * 255) << 16;
    hash |= (unsigned int)(attr.color.blue * 255) << 24;
  }
  if (attr.bold)
    hash |= 1;
  if (attr.italic)
    hash |= 2;
  char name[32];
  snprintf(name, sizeof(name), "%x", hash);
  Glib::RefPtr<Gtk::TextBuffer::Tag> tag;
  if (!(tag = _text->get_buffer()->get_tag_table()->lookup(name)))
  {
    tag = _text->get_buffer()->create_tag(name);
    if (attr.bold)
      tag->property_weight() = Pango::WEIGHT_BOLD;
    if (attr.italic)
      tag->property_style() = Pango::STYLE_ITALIC;
    if (attr.color.is_valid())
    {
      char color[16];
      snprintf(color, sizeof(color), "#%02x%02x%02x", (int)(attr.color.red*255), (int)(attr.color.green*255), (int)(attr.color.blue*255));
      tag->property_foreground() = color;
    }
  }
  return tag;
}*/

bool TextBoxImpl::create(::mforms::TextBox *self, mforms::ScrollBars scroll_type)
{
  return new TextBoxImpl(self, scroll_type) != 0;
}

void TextBoxImpl::set_text(::mforms::TextBox *self, const std::string &text)
{
  TextBoxImpl* cb = self->get_data<TextBoxImpl>();

  if ( cb )
    cb->_text->get_buffer()->set_text(text);
}

void TextBoxImpl::append_text(::mforms::TextBox *self, const std::string &text, bool scroll_to_end)
{
  TextBoxImpl* cb = self->get_data<TextBoxImpl>();

  if ( cb )
  {
    Gtk::TextView *tv = cb->_text;
    if (tv)
    {
      Glib::RefPtr<Gtk::TextBuffer> buf = tv->get_buffer();
      buf->insert(buf->end(), text);

      if (scroll_to_end)
      {
        Gtk::TextIter it = buf->end();
        tv->scroll_to(it, 0.3);
      }
    }
  }
}

/*
void TextBoxImpl::append_text_with_attributes(::mforms::TextBox *self, const std::string &text, const ::mforms::TextAttributes &attr, bool scroll_to_end)
{
  TextBoxImpl* cb = self->get_data<TextBoxImpl>();

  if ( cb )
  {
    Gtk::TextView *tv = cb->_text;
    if (tv)
    {
      Glib::RefPtr<Gtk::TextBuffer> buf = tv->get_buffer();
      buf->insert_with_tag(buf->end(), text, cb->tag_for_text_attributes(attr));

      if (scroll_to_end)
      {
        Gtk::TextIter it = buf->end();
        tv->scroll_to(it, 0.3);
      }
    }
  }
}*/

std::string TextBoxImpl::get_text(::mforms::TextBox *self)
{
  TextBoxImpl* cb = self->get_data<TextBoxImpl>();
  std::string ret("");
  if ( cb )
  {
    ret = cb->_text->get_buffer()->get_text().raw();
  }
  return ret;
}

void TextBoxImpl::set_read_only(::mforms::TextBox *self, bool flag)
{
  TextBoxImpl* cb = self->get_data<TextBoxImpl>();
  if (cb && cb->_text)
    cb->_text->set_editable(!flag);
}

void TextBoxImpl::set_padding(::mforms::TextBox *self, int pad)
{
  TextBoxImpl* cb = self->get_data<TextBoxImpl>();
  if (cb && cb->_swin)
    cb->_swin->set_border_width(pad);
}

void TextBoxImpl::set_bordered(::mforms::TextBox *self, bool flag)
{
  TextBoxImpl* cb = self->get_data<TextBoxImpl>();
  if (cb)
    cb->_swin->set_shadow_type(flag ? Gtk::SHADOW_IN : Gtk::SHADOW_NONE);
}

void TextBoxImpl::set_monospaced(::mforms::TextBox *self, bool flag)
{
  TextBoxImpl* cb = self->get_data<TextBoxImpl>();
  if (cb)
  {
    Pango::FontDescription font(cb->_text->get_style()->get_font());
    
    if (flag)
    {
      font.set_family("Bitstream Vera Sans Mono");
      font.set_size(9 * Pango::SCALE);
    }
    cb->_text->modify_font(font);
  }
}

void TextBoxImpl::get_selected_range(::mforms::TextBox *self, int &start, int &end)
{
  TextBoxImpl* cb = self->get_data<TextBoxImpl>();
  if (cb)
  {
    Gtk::TextBuffer::iterator sbegin, send;
    if (cb->_text->get_buffer()->get_selection_bounds(sbegin, send))
    {
      start = sbegin.get_offset();
      end = send.get_offset();
    }
    else
    {
      start= 0;
      end= 0;
    }
  }
}

void TextBoxImpl::clear(::mforms::TextBox *self)
{
  TextBoxImpl* cb = self->get_data<TextBoxImpl>();
  if (cb && cb->_text)
    cb->_text->get_buffer()->set_text("");
}

void TextBoxImpl::set_front_color(const std::string &color)
{
  Gdk::Color clr(color);
  this->_text->modify_text(Gtk::STATE_NORMAL, clr);
}

TextBoxImpl::TextBoxImpl(::mforms::TextBox *self, mforms::ScrollBars scroll_type)
  : ViewImpl(self)
{
  _swin= Gtk::manage(new Gtk::ScrolledWindow());
  _text = Gtk::manage(new Gtk::TextView());
  _swin->add(*_text);
  
  Gtk::PolicyType h_scrollbar_policy = Gtk::POLICY_AUTOMATIC;
  Gtk::PolicyType v_scrollbar_policy = Gtk::POLICY_AUTOMATIC;
  
  switch (scroll_type)
  {
    case ::mforms::NoScrollBar:
    {
      h_scrollbar_policy = Gtk::POLICY_NEVER;
      v_scrollbar_policy = Gtk::POLICY_NEVER;
      _text->set_wrap_mode(Gtk::WRAP_WORD);
      break;
    }
    case ::mforms::HorizontalScrollBar:
    {
      h_scrollbar_policy = Gtk::POLICY_AUTOMATIC;
      v_scrollbar_policy = Gtk::POLICY_NEVER;
      _text->set_wrap_mode(Gtk::WRAP_NONE);
      break;
    }
    case ::mforms::VerticalScrollBar:
    {
      h_scrollbar_policy = Gtk::POLICY_NEVER;
      v_scrollbar_policy = Gtk::POLICY_AUTOMATIC;
      _text->set_wrap_mode(Gtk::WRAP_WORD);
      break;
    }
    case ::mforms::SmallScrollBars:
    case ::mforms::BothScrollBars:
    {
      h_scrollbar_policy = Gtk::POLICY_AUTOMATIC;
      v_scrollbar_policy = Gtk::POLICY_AUTOMATIC;
      _text->set_wrap_mode(Gtk::WRAP_NONE);
      break;
    }
  }
  _swin->set_policy(h_scrollbar_policy, v_scrollbar_policy);
  _swin->set_shadow_type(Gtk::SHADOW_IN);
  _text->show();
  _swin->show();

  _text->get_buffer()->signal_changed().connect(sigc::mem_fun(*self, &::mforms::TextBox::callback));
  _text->add_events(Gdk::KEY_PRESS_MASK);
  _text->signal_key_press_event().connect(sigc::bind(sigc::mem_fun(this, &TextBoxImpl::on_key_press), self), true);
}
  
static mforms::ModifierKey GetModifiers(const guint state, const guint keyval)
{
  mforms::ModifierKey modifiers = mforms::ModifierNoModifier;
  Gdk::ModifierType mod_type = Gtk::AccelGroup::get_default_mod_mask();
  if ((state & mod_type) == 0)
  {
    if ((keyval == GDK_KEY_Control_L) || (keyval == GDK_KEY_Control_R))
      modifiers = modifiers | mforms::ModifierControl;
    if ((keyval == GDK_KEY_Shift_L) || (keyval == GDK_KEY_Shift_R))
     modifiers = modifiers | mforms::ModifierShift;
    if ((keyval == GDK_KEY_Alt_L) || (keyval == GDK_KEY_Alt_R))
        modifiers = modifiers | mforms::ModifierAlt;
    if ((keyval == GDK_KEY_Super_L) || (keyval == GDK_KEY_Super_R))
      modifiers = modifiers | mforms::ModifierCommand;
  }
  return modifiers;
}

bool TextBoxImpl::on_key_press(GdkEventKey *event, mforms::TextBox *self)
{
  mforms::KeyCode code = mforms::KeyNone;
  switch(event->keyval)
  {
  case GDK_Home:
    code = mforms::KeyHome;
    break;
  case GDK_End:
    code = mforms::KeyEnd;
    break;
  case GDK_Page_Up:
    code = mforms::KeyPrevious;
    break;
  case GDK_Page_Down:
    code = mforms::KeyNext;
    break;

  case GDK_KEY_Return:
    code = mforms::KeyReturn;
    break;

  case GDK_KEY_KP_Enter:
    code = mforms::KeyEnter;
    break;
  case GDK_KEY_Shift_L:
  case GDK_KEY_Shift_R:
  case GDK_KEY_Alt_L:
  case GDK_KEY_Alt_R:
  case GDK_KEY_Control_L:
  case GDK_KEY_Control_R:
  case GDK_KEY_Super_L:
  case GDK_KEY_Super_R:
    code = mforms::KeyModifierOnly;
    break;
  }

  if (code == mforms::KeyNone && ((event->keyval >= GDK_KEY_A && event->keyval <= GDK_KEY_Z) || (event->keyval >= GDK_KEY_a && event->keyval <= GDK_KEY_z)))
    code = mforms::KeyChar;
  else
    code = mforms::KeyUnkown;


  return !self->key_event(code, GetModifiers(event->state, event->keyval), "");
}


void TextBoxImpl::init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_textbox_impl.create   = &TextBoxImpl::create;
  f->_textbox_impl.set_bordered= &TextBoxImpl::set_bordered;
  f->_textbox_impl.set_text = &TextBoxImpl::set_text;
  f->_textbox_impl.get_text = &TextBoxImpl::get_text;
  f->_textbox_impl.append_text = &TextBoxImpl::append_text;
//  f->_textbox_impl.append_text_with_attributes = &TextBoxImpl::append_text_with_attributes;
  f->_textbox_impl.set_read_only= &TextBoxImpl::set_read_only;
  f->_textbox_impl.set_padding = &TextBoxImpl::set_padding;
  f->_textbox_impl.get_selected_range = &TextBoxImpl::get_selected_range;
  f->_textbox_impl.set_monospaced = &TextBoxImpl::set_monospaced;
  f->_textbox_impl.clear = &TextBoxImpl::clear;
}


  };
};
