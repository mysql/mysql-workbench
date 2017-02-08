/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "../lf_scrollpanel.h"

mforms::gtk::ScrollPanelImpl::ScrollPanelImpl(::mforms::ScrollPanel *self, mforms::ScrollPanelFlags flags)
  : mforms::gtk::ViewImpl(self), mforms::gtk::BinImpl(this) {
  _swin = new Gtk::ScrolledWindow();

  _swin->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  _vertical = true;
  _horizontal = true;
  _autohide = true;

  if (flags & mforms::ScrollPanelBordered)
    _swin->set_shadow_type(Gtk::SHADOW_IN);
  else
    _swin->set_shadow_type(Gtk::SHADOW_NONE);
  _swin->show();
  setup();
}

mforms::gtk::ScrollPanelImpl::~ScrollPanelImpl() {
  if (_swin)
    delete _swin;
}

bool mforms::gtk::ScrollPanelImpl::create(::mforms::ScrollPanel *self, mforms::ScrollPanelFlags flags) {
  return new mforms::gtk::ScrollPanelImpl(self, flags) != 0;
}

void mforms::gtk::ScrollPanelImpl::add(::mforms::ScrollPanel *self, ::mforms::View *child) {
  mforms::gtk::ScrollPanelImpl *panel = self->get_data<mforms::gtk::ScrollPanelImpl>();

  panel->_swin->add(*child->get_data<ViewImpl>()->get_outer());

  // remove border around viewport
  ((Gtk::Viewport *)panel->_swin->get_child())->set_shadow_type(Gtk::SHADOW_NONE);
}

void mforms::gtk::ScrollPanelImpl::remove(::mforms::ScrollPanel *self) {
  mforms::gtk::ScrollPanelImpl *panel = self->get_data<mforms::gtk::ScrollPanelImpl>();

  panel->_swin->remove();
}

void mforms::gtk::ScrollPanelImpl::set_visible_scrollers(::mforms::ScrollPanel *self, bool vertical, bool horizontal) {
  mforms::gtk::ScrollPanelImpl *panel = self->get_data<mforms::gtk::ScrollPanelImpl>();
  panel->_vertical = vertical;
  panel->_horizontal = horizontal;

  Gtk::PolicyType hPolicy = Gtk::POLICY_AUTOMATIC, vPolicy = Gtk::POLICY_AUTOMATIC;
  if (!panel->_vertical)
    vPolicy = Gtk::POLICY_NEVER;
  else {
    if (panel->_autohide)
      vPolicy = Gtk::POLICY_AUTOMATIC;
    else
      vPolicy = Gtk::POLICY_ALWAYS;
  }

  if (!panel->_horizontal)
    hPolicy = Gtk::POLICY_NEVER;
  else {
    if (panel->_autohide)
      hPolicy = Gtk::POLICY_AUTOMATIC;
    else
      hPolicy = Gtk::POLICY_ALWAYS;
  }
  panel->_swin->set_policy(hPolicy, vPolicy);
}

void mforms::gtk::ScrollPanelImpl::set_autohide_scrollers(::mforms::ScrollPanel *self, bool flag) {
  ScrollPanelImpl *panel = self->get_data<mforms::gtk::ScrollPanelImpl>();

  panel->_autohide = flag;
  panel->_swin->set_policy(
    panel->_autohide ? Gtk::POLICY_AUTOMATIC : (panel->_horizontal ? Gtk::POLICY_ALWAYS : Gtk::POLICY_NEVER),
    panel->_autohide ? Gtk::POLICY_AUTOMATIC : (panel->_vertical ? Gtk::POLICY_ALWAYS : Gtk::POLICY_NEVER));
}

void mforms::gtk::ScrollPanelImpl::scroll_to_view(mforms::ScrollPanel *self, mforms::View *child) {
  mforms::gtk::ScrollPanelImpl *panel = self->get_data<mforms::gtk::ScrollPanelImpl>();
  if (!panel)
    throw std::logic_error("self->get_data returned 0. Check mforms::gtk::ScrollPanelImpl::scroll_to_view.");
  else {
    mforms::gtk::ViewImpl *child_impl = self->get_data<mforms::gtk::ViewImpl>();
    if (child_impl) {
      // Gtk::Widget *child_widget = child_impl->get_outer();
      Glib::RefPtr<Gtk::Adjustment> vadj = panel->_swin->get_vadjustment();
      if (vadj) {
        const int y = child_impl->get_y(child);
        vadj->set_value(y);
      }
    }
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::ScrollPanelImpl::set_padding_impl(int left, int top, int right, int bottom) {
  _swin->set_border_width(left);
}

//------------------------------------------------------------------------------
base::Rect mforms::gtk::ScrollPanelImpl::get_content_rect(mforms::ScrollPanel *self) {
  mforms::gtk::ScrollPanelImpl *panel = self->get_data<mforms::gtk::ScrollPanelImpl>();
  base::Rect rect;
  Gtk::Viewport *vp;

  if (panel && (vp = dynamic_cast<Gtk::Viewport *>(panel->_swin->get_child()))) {
    rect.pos.y = panel->_swin->get_vadjustment()->get_value();
    rect.pos.x = panel->_swin->get_hadjustment()->get_value();
    rect.size.width = vp->get_window()->get_width();
    rect.size.height = vp->get_window()->get_height();
  }

  return rect;
}

void mforms::gtk::ScrollPanelImpl::scroll_to(mforms::ScrollPanel *self, int x, int y) {
  mforms::gtk::ScrollPanelImpl *panel = self->get_data<mforms::gtk::ScrollPanelImpl>();
  panel->_swin->get_vadjustment()->set_value(y);
  panel->_swin->get_hadjustment()->set_value(x);
}

void mforms::gtk::ScrollPanelImpl::init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_spanel_impl.create = &mforms::gtk::ScrollPanelImpl::create;
  f->_spanel_impl.add = &mforms::gtk::ScrollPanelImpl::add;
  f->_spanel_impl.remove = &mforms::gtk::ScrollPanelImpl::remove;
  f->_spanel_impl.set_visible_scrollers = &mforms::gtk::ScrollPanelImpl::set_visible_scrollers;
  f->_spanel_impl.set_autohide_scrollers = &mforms::gtk::ScrollPanelImpl::set_autohide_scrollers;
  f->_spanel_impl.scroll_to_view = &mforms::gtk::ScrollPanelImpl::scroll_to_view;
  f->_spanel_impl.get_content_rect = &mforms::gtk::ScrollPanelImpl::get_content_rect;
  f->_spanel_impl.scroll_to = &mforms::gtk::ScrollPanelImpl::scroll_to;
}
