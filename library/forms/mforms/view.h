/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <boost/signals2.hpp>

#include "mforms/base.h"
#include "mforms/utilities.h"

#include "base/geometry.h"
#include "base/trackable.h"
#include "base/drawing.h"

namespace mforms {

  // Predefined drag formats used during a drag session. Custom definitions are possible too.
  const std::string DragFormatText = "com.mysql.workbench.text";     // UTF-8 encoded text.
  const std::string DragFormatFileName = "com.mysql.workbench.file"; // A plain file name (UTF-8 encoded).

  class View;

  enum Alignment {
    NoAlign,
    BottomLeft,
    BottomCenter,
    BottomRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    TopLeft,
    TopCenter,
    TopRight
  };

  struct TextAttributes {
#ifndef SWIG
    bool bold;
    bool italic;
    base::Color color;
    TextAttributes() : bold(false), italic(false), color(base::Color::invalid()) {
    }
#endif
    TextAttributes(const std::string &c, bool b, bool i) : bold(b), italic(i), color(base::Color::parse(c)) {
    }
  };

  // Mouse button indicators for mouse handling routines.
  enum MouseButton {
    MouseButtonLeft = 0,
    MouseButtonRight = 1,
    MouseButtonOther = 2,

    MouseButtonNone =
      0x80, // Sometimes the values for the mouse buttons are hard coded, so better use a high value for this.
            // TODO: identify hard coded values and replace them.
  };

  enum class Modifier {
    NoModifier   = 0x00,
    ShiftLeft    = 0x01,
    ShiftRight   = 0x02,
    ControlLeft  = 0x04,
    ControlRight = 0x08,
    AltLeft      = 0x10,
    AltRight     = 0x20,
    MetaLeft     = 0x40,
    MetaRight    = 0x80
  };

  enum DragOperation {
    DragOperationNone = 0,
    DragOperationCopy = 1 << 0,
    DragOperationMove = 1 << 1,
    DragOperationAll = DragOperationCopy | DragOperationMove,
  };

#ifndef SWIG
  inline DragOperation operator|(DragOperation a, DragOperation b) {
    return (DragOperation)((int)a | (int)b);
  }

  inline DragOperation operator&(DragOperation a, DragOperation b) {
    return (DragOperation)((int)a & (int)b);
  }

  inline DragOperation &operator|=(DragOperation &a, DragOperation b) {
    return a = (DragOperation)((int)a | (int)b);
  }
#endif

  // Position relative to the target. There's no general rule what this means. View descendants
  // decide what to use (e.g. a node in a treeview).
  enum DropPosition {
    DropPositionUnknown,
    DropPositionLeft,
    DropPositionRight,
    DropPositionOn,
    DropPositionTop,
    DropPositionBottom,
  };

  struct DragDetails {
    base::Point location;            // Position of the mouse in client coordinates.
    DragOperation allowedOperations; // A combination of flags that determine the allowed actions.

    cairo_surface_t *image; // The drag image to show (owned by the initiator of the operation.
                            // This must be an image surface in ARGB32 format.
                            // When doing text dragging this image can be NULL in which case
                            // the platforms generate a drag image from the given text.
    base::Point hotspot;    // The position of the mouse within the drag image.

    DragDetails() {
      location = base::Point();
      allowedOperations = DragOperationNone;
      image = NULL;
      hotspot = base::Point();
    }
  };

#ifndef SWIG
  /**
   * Delegate class for events caused by a drop operation. Must be implemented by objects
   * that want to accept a drop operation (not necessarily mforms objects).
   * The sender is the View that initiated the operation. If the operation started outside WB
   * (or by non-mforms code) the sender is NULL.
   * The drop point p is given in the receiver's coordinate space.
   */
  class MFORMS_EXPORT DropDelegate {
  public:
    /**
     * Called constantly while the mouse is moving over the receiver during a drag operation.
     * Return a combination of the drag operations that are supported with the offered formats.
     * This function must always be implemented by the delegate object. All others are optional.
     */
    virtual DragOperation drag_over(View *sender, base::Point p, DragOperation allowedOperations,
                                    const std::vector<std::string> &formats) = 0;

    /**
     * Called when files were dropped on the receiver (only called if drag_over returned true).
     * This callback is specifically for the predefined format DragFormatFileName.
     * Return the operation that actually took place. This will tell the drag initiator what happened
     * so it can update its structures.
     */
    virtual DragOperation files_dropped(View *sender, base::Point p, DragOperation allowedOperations,
                                        const std::vector<std::string> &file_names) {
      return DragOperationNone;
    }

    /**
     * Called when text was dropped on the receiver (only called if drag_over returned a valid drag operation).
     * This callback is specifically for the predefined format DragFormatText.
     */
    virtual DragOperation text_dropped(View *sender, base::Point p, DragOperation allowedOperations,
                                       const std::string &text) {
      return DragOperationNone;
    }

    /**
     * Called when any custom data was dropped on the receiver (only called if drag_over returned a valid drag
     * operation).
     * This callback is for all custom data formats and only used for drag operations
     * within WB. It will never be called for data from other sources.
     * Note: any of the *_dropped functions can be called during a drop (in random order),
     *       since a single such operation can carry more than one format.
     */
    virtual DragOperation data_dropped(View *sender, base::Point p, DragOperation allowedOperations, void *data,
                                       const std::string &format) {
      return DragOperationNone;
    }
  };
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct ViewImplPtrs {
    void (*destroy)(View *self);

    int (*get_width)(const View *self);
    int (*get_height)(const View *self);
    int (*get_preferred_width)(View *self);
    int (*get_preferred_height)(View *self);
    void (*set_size)(View *self, int, int); // Sets the minsize as well.
    void (*set_min_size)(View *self, int, int);
    void (*set_padding)(View *self, int, int, int, int); // left, top, right, bottom

    int (*get_x)(const View *self);
    int (*get_y)(const View *self);
    void (*set_position)(View *self, int, int);
    std::pair<int, int> (*client_to_screen)(View *self, int, int);
    std::pair<int, int> (*screen_to_client)(View *self, int, int);

    void (*show)(View *self, bool);
    bool (*is_shown)(View *self);
    bool (*is_fully_visible)(View *self);

    void (*set_tooltip)(View *self, const std::string &);
    void (*set_name)(View *self, const std::string &);
    void (*set_font)(View *self, const std::string &);

    void (*set_enabled)(View *self, bool);
    bool (*is_enabled)(View *self);
    void (*relayout)(View *self);
    void (*set_needs_repaint)(View *self);

    void (*suspend_layout)(View *self, bool);
    void (*set_front_color)(View *self, const std::string &);
    std::string (*get_front_color)(View *self);
    void (*set_back_color)(View *self, const std::string &);
    std::string (*get_back_color)(View *self);
    // for containers only
    void (*set_back_image)(View *self, const std::string &, Alignment alignment);

    void (*flush_events)(View *self);
    void (*focus)(View *self);
    bool (*has_focus)(View *self); // TODO Windows

    void (*register_drop_formats)(View *self, DropDelegate *target, const std::vector<std::string> &);
    DragOperation (*drag_text)(View *self, DragDetails details, const std::string &text);
    DragOperation (*drag_data)(View *self, DragDetails details, void *data, const std::string &format);
    DropPosition (*get_drop_position)(View *self);
  };
#endif
#endif

  class Form;

  class MFORMS_EXPORT View : public Object, public base::trackable {
    friend class ControlFactory;

  private:
    std::string _internalName;
    bool _layout_dirty;

    boost::signals2::signal<void()> _signal_resized;
    boost::signals2::signal<bool()> _signal_mouse_leave;
    boost::signals2::signal<void()> _signal_got_focus;

  protected:
    View();

    ViewImplPtrs *_view_impl;
    View *_parent;
    std::vector<std::pair<View *, bool> > _subviews;

    void cache_view(View *sv);
    virtual void remove_from_cache(View *sv);
    void reorder_cache(View *sv, int position);
    int get_subview_index(View *sv);
    View *get_subview_at_index(int index);
    int get_subview_count();

    // This works only for containers so is made public in the Container subclass
    virtual void set_back_image(const std::string &path, Alignment alignment);

  public:
    virtual ~View();

    virtual void set_managed();

    View *find_subview(const std::string &name);
    bool contains_subview(View *subview);
    void clear_subviews();

    virtual void set_name(const std::string &name);
    void setInternalName(const std::string &name);
    std::string getInternalName() const;
    void set_tooltip(const std::string &text);
    virtual void set_font(const std::string &fontDescription); // e.g. "Trebuchet MS bold 9"
    void set_parent(View *parent);
    virtual View *get_parent() const;
    Form *get_parent_form() const;
    virtual int get_width() const;
    virtual int get_height() const;
    virtual int get_preferred_width();
    virtual int get_preferred_height();
    virtual int get_x() const;
    virtual int get_y() const;
    virtual void set_position(int x, int y);
    virtual void set_size(int width, int height);
    virtual void set_min_size(int width, int height);

    std::pair<int, int> client_to_screen(int x, int y);
    std::pair<int, int> screen_to_client(int x, int y);

    /**
     * Show/hide view.
     */
    void show(bool flag = true);

    /**
     * Check whenever view is visible.
     */
    bool is_shown();

    /**
     * Returns true if the view and all it's parents are visible.
     */
    bool is_fully_visible();

    /**
     * Enable view so user can interact with it.
     */
    void set_enabled(bool flag);

    /**
     * Check whenever view is enabled.
     */
    bool is_enabled();

    /**
     * Mark view to be repainted with next iteration.
     */
    void set_needs_repaint();

    virtual void relayout();
    virtual void set_layout_dirty(bool value);
    virtual bool is_layout_dirty();

    /**
     * Freeze gui updates for this view. This method is useful when there is a need to manipulate childs of this view.
     * After calling suspend_layout, resume_layout need to be called!
     */
    void suspend_layout();

    /**
     * Resume gui updates for this view. Should be called after suspend_layout was called.
     */
    void resume_layout();

    /**
     * Set view foreground color.
     */
    void set_front_color(const std::string &color);

    /**
     * Get view foreground color.
     */
    std::string get_front_color();

    /**
     * Set view background color.
     */
    void set_back_color(const std::string &color);

    /**
     * Get view background color.
     */
    std::string get_back_color();

// Below code is used only for debug purpose.
// It's using the object::retain_count.
#ifdef _0
    void show_retain_counts(int depth = 0);
#endif

    /**
     * Get string value from the view if it's holding some.
     */
    virtual std::string get_string_value() {
      return "";
    }

    /**
     * Get int value from the view if it's holding some.
     */
    virtual int get_int_value() {
      return 0;
    }

    /**
     * Get bool value from the view if it's holding some.
     */
    virtual bool get_bool_value() {
      return false;
    }

    /**
     * Iterate over all events that were queued and flush them, results in faster gui updates or faster signal calls.
     */
    virtual void flush_events();

    /**
     * Causes view to have keyboard focus.
     */
    virtual void focus();

    virtual bool has_focus();

#ifndef SWIG
    /**
     * Enables or disables the ability to accept a drag/drop operation (internal or from outside)
     * based on drop_formats.
     * @param target specifies a target that will receive drop events. It can be the same as the view
     *        or any other class (even non-visual) that implements this interface.
     */
    void register_drop_formats(DropDelegate *target, const std::vector<std::string> &drop_formats);

    /**
     * Starts an internal drag/drop operation with the given text/data and blocks until that op is finished.
     * The result tells the caller what actually happened.
     */
    DragOperation do_drag_drop(DragDetails details, const std::string &text);
    DragOperation do_drag_drop(DragDetails details, void *data, const std::string &format);

    /**
     *	Only valid during a drag operation. Returns a drop position value of the drop target
     *	(if one can be determined, like above or below a tree node).
     *	This is a helper to ease determination of the actual drop operation if that depends on the
     *	position within the target.
     */
    DropPosition get_drop_position();

#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
    // Events called by the platform code. Returns a bool value to indicate if the event was handled.
    // False means the platform should do whatever it needs with that event.
    // Note: these functions are only called if the platform control supports this type of mouse handling.
    //       Examples are: all containers (Panel, ScrollPanel, Box, Table) and DrawBox.
    virtual bool mouse_down(MouseButton button, int x, int y) {
      return false;
    }
    virtual bool mouse_up(MouseButton button, int x, int y) {
      return false;
    }
    virtual bool mouse_click(MouseButton button, int x, int y) {
      return false;
    }
    virtual bool mouse_double_click(MouseButton button, int x, int y) {
      return false;
    }
    virtual bool mouse_enter() {
      return false;
    }
    virtual bool mouse_leave();
    virtual bool mouse_move(MouseButton button, int x, int y) {
      return false;
    }

    virtual bool focusIn() {
    	return false;
    }

    virtual bool focusOut() {
    	return false;
    }

    virtual bool keyPress(KeyCode code, ModifierKey modifiers) {
    	return false;
    }

    virtual bool keyRelease(KeyCode code, ModifierKey modifiers) {
    	return false;
    }
#endif
#endif

    void focus_changed();

    /** Triggered by the platform layers when the size of the view changes. */
    virtual void resize();

    boost::signals2::signal<void()> *signal_resized() {
      return &_signal_resized;
    }
    boost::signals2::signal<bool()> *signal_mouse_leave() {
      return &_signal_mouse_leave;
    }
    boost::signals2::signal<void()> *signal_got_focus() {
      return &_signal_got_focus;
    }
  };
};
