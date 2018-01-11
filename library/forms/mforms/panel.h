/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <mforms/container.h>

namespace mforms {
  enum PanelType {
    // TODO: re-arrange names so that the all begin with Panel.., as this sorts much better
    //       in lists etc.
    TransparentPanel,  // just a container with no background
    FilledPanel,       // just a container with color filled background
    BorderedPanel,     // container with native border
    LineBorderPanel,   // container with a solid line border
    TitledBoxPanel,    // native grouping box with a title with border
    TitledGroupPanel,  // native grouping container with a title (may have no border)
    FilledHeaderPanel, // Just like a filled panel but additionally has a header bar with a title.
    StyledHeaderPanel  // Panel which has top round corners for win, rect + gradient fill for osx, plain filled rect for
                       // linux
  };

  class Panel;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct MFORMS_EXPORT PanelImplPtrs {
    bool (*create)(Panel *, PanelType type);
    void (*set_back_color)(Panel *, const std::string &);
    void (*set_title)(Panel *, const std::string &);

    void (*set_active)(Panel *, bool);
    bool (*get_active)(Panel *);

    void (*add)(Panel *self, View *);
    void (*remove)(Panel *self, View *);
  };
#endif
#endif

  /** A generic single item container with optional border. */
  class MFORMS_EXPORT Panel : public Container {
  public:
    /** Constructor.

     Type of panel:
     TransparentPanel    just a container with no background
     FilledPanel         just a container with color filled background
     BorderedPanel       container with native border
     LineBorderPanel     container with a solid line border
     TitledBoxPanel      native grouping box with a title with border
     TitledGroupPanel    native grouping container with a title (may have no border)
     */
    Panel(PanelType type);

    /** Sets title of panel, if supported. */
    void set_title(const std::string &title);
    /** Sets background color of panel, if supported. */
    virtual void set_back_color(const std::string &color);

    /** Sets state of the panel checkbox, if supported. */
    void set_active(bool);
    /** Gets state of panel checkbox, if supported. */
    bool get_active();

    /** Sets the content of the panel. */
    void add(View *subview);
    /** Removes the content view of the panel. */
    virtual void remove(View *subview);

  protected:
    PanelImplPtrs *_panel_impl;
  };
};
