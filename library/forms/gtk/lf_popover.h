/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once
#include "mforms/popover.h"
#include "lf_mforms.h"
#include "lf_view.h"

namespace mforms {
  class PopoverWidget {
  protected:
    PopoverStyle _style;

  public:
    PopoverWidget(PopoverStyle style) : _style(style){};
    virtual ~PopoverWidget(){};
    virtual void showPopover(const int x, const int y, const StartPosition pos) = 0;
    virtual void close() = 0;
    virtual void setSize(const int w, const int h) = 0;
    virtual void setContent(Gtk::Widget *w) = 0;
    virtual void setName(const std::string &name) = 0;
    PopoverStyle getStyle() const {
      return _style;
    };
  };

  class PopoverNormal : public PopoverWidget {
  private:
    Gtk::Popover *_popover;
    Gtk::Widget *_owner;

  public:
    PopoverNormal(mforms::View *owner);
    ~PopoverNormal();
    virtual void showPopover(const int x, const int y, const StartPosition pos) override;
    virtual void close() override;
    virtual void setSize(const int w, const int h) override;
    virtual void setContent(Gtk::Widget *w) override;
    virtual void setName(const std::string &name) override;
  };

  class PopoverTooltip : public PopoverWidget, public Gtk::Window {
  private:
    Gtk::Window *_parent;
    Gtk::Box *_hbox;
    mforms::StartPosition _contentPos;
    int _handleX;
    int _handleY;
    void adjustPosition();

  public:
    PopoverTooltip(mforms::View *owner);
    virtual ~PopoverTooltip(){};
    virtual void showPopover(const int x, const int y, const StartPosition pos) override;
    virtual void close() override;
    virtual void setSize(const int w, const int h) override;
    virtual void setContent(Gtk::Widget *w) override;
    virtual void setName(const std::string &name) override;
    bool tooltipSignalEvent(GdkEvent *ev);
    void parentKeyRelease(GdkEventKey *ev);
  };
}
