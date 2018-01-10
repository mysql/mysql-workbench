/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

/**
 * This file contains a collection of small graphical elements used to display heartbeats
 * values over time and other visual sugar.
 * In general these widgets are all thread-safe and can be manipulated from any thread
 * (particularly useful for animations and other timed updates).
 */

#pragma once

#include <list>

#include "glib.h"
#include "cairo/cairo.h"

#include "mforms/base.h"
#include "mforms/drawbox.h"
#include "mforms/box.h"
#include "mforms/label.h"
#include "mforms/table.h"
#include "base/threaded_timer.h"
#include "base/geometry.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace mforms {

  typedef std::list<double> ThresholdList;

  class MFORMS_EXPORT BaseWidget : public DrawBox {
  public:
    BaseWidget();
    ~BaseWidget();

    void set_right_align(bool flag);
    void enable_auto_scale(bool enable);
    double get_upper_range() const;
    void set_value_range(double low, double high);
    void set_thresholds(ThresholdList lower_thresholds, ThresholdList upper_thresholds);
    void set_description(const std::string& description);
#ifndef SWIG
    virtual void repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) override;
    virtual base::Size getLayoutSize(base::Size proposedSize) override;

    virtual void step() {};
#endif

  protected:
    cairo_surface_t* _background;
    bool _auto_scale;
    bool _right_align;
    base::Rect _diagram_area;
    int _layout_width;
    int _layout_height;

    void lock();
    void unlock();
    double normalize(double input);
    virtual void prepare_background() {};
    virtual void destroy_background();
    virtual void range_updated(double scale, double offset) {};
    virtual bool layout(cairo_t* cr);
    void auto_scale(double value);
    bool compute_scale(double min, double max);
    virtual void get_minmax_values(double* min, double* max);

  private:
    base::Mutex _lock;
    double _lower_limit;
    double _upper_limit;
    ThresholdList _lower_thresholds;
    ThresholdList _upper_thresholds;
    std::string _description;

    int _last_width; // Needed to detect when we need a re-layout.
    int _last_height;

    int _description_offset; // Depends on alignment.

    cairo_t* _layout_context;
    cairo_surface_t* _layout_surface;
    void create_context_for_layout();
  };

  class MFORMS_EXPORT WidgetSeparator : public DrawBox {
  protected:
    virtual void repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) override;
  };

// Number of values to use for the heartbeat.
#define HEARTBEAT_DATA_SIZE 80

  class MFORMS_EXPORT HeartbeatWidget : public BaseWidget {
  public:
    HeartbeatWidget();
    ~HeartbeatWidget();
#ifndef SWIG
    virtual void repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) override;
    virtual void step() override;
#endif
    void set_value(double value);

  protected:
    virtual void prepare_background() override;
    virtual void range_updated(double scale, double offset) override;
    virtual void get_minmax_values(double* min, double* max) override;

  private:
    int _pivot;
    double _luminance[HEARTBEAT_DATA_SIZE];
    double _deflection[HEARTBEAT_DATA_SIZE];
  };

  /**
   * The ServerStatusWidget shows the server's running state.
   */
  class MFORMS_EXPORT ServerStatusWidget : public BaseWidget {
  public:
    ServerStatusWidget();
    ~ServerStatusWidget();

#ifndef SWIG
    virtual void repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) override;
#endif
    void set_server_status(int status);

  protected:
    virtual bool layout(cairo_t* cr) override;

  private:
    int _status; // -1 for unknown, 0 for not-running, 1 for running, 2 offline

    cairo_surface_t* _image_unknown;
    cairo_surface_t* _image_running;
    cairo_surface_t* _image_stopped;
    cairo_surface_t* _image_offline;
  };

  /**
   * The bar graph widget is a widget which shows a single value similar to a gauge reading.
   */
  class MFORMS_EXPORT BarGraphWidget : public BaseWidget {
  public:
    BarGraphWidget();
    ~BarGraphWidget();

#ifndef SWIG
    virtual void repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) override;
#endif
    void set_value(double value);

  protected:
    virtual void prepare_background() override;
    virtual void destroy_background() override;
    void create_value_gradient();
    virtual void range_updated(double scale, double offset) override;
    virtual void get_minmax_values(double* min, double* max) override;

  private:
    double _value;
    cairo_pattern_t* _value_gradient;
    cairo_surface_t* _grid;
  };

/**
 * The line diagram widget shows a series of values in the order they came in, scrolling
 * the display area slowly to fade out the oldest values making so room for new entries.
 */
#define LINE_SERIES_DATA_SIZE 500

  class MFORMS_EXPORT LineDiagramWidget : public BaseWidget {
  public:
    LineDiagramWidget();
    ~LineDiagramWidget();

#ifndef SWIG
    virtual void repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) override;
    virtual void step() override;
#endif
    void set_value(double value);

  protected:
    virtual void prepare_background() override;
    virtual void destroy_background() override;
    virtual void range_updated(double scale, double offset) override;
    virtual void get_minmax_values(double* min, double* max) override;

  private:
    double _next_value;
    double _deflection[LINE_SERIES_DATA_SIZE];
    double _timestamp[LINE_SERIES_DATA_SIZE];
    int _time_in_view; // Seconds to show in the widget.
    cairo_pattern_t* _value_gradient;
    GTimer* _clock; // Used to generate timestamps for incoming values.
    cairo_surface_t* _grid;
    double _content_alpha; // Used to blend out the content for special feedback.
    double _warning_alpha; // Alpha for warning feedback.
    double _last_shift;    // The animation timer is faster than our shift needs to be.
                           // Hence determine the distance since the last shift before we do the next.
    double _sleep_start;   // 0 if not sleeping, otherwise the timestamp of when
                           // sleeping started.
    enum { Awake, GoSleeping, Sleeping, Awaking } _sleep_mode;

    cairo_text_extents_t _warning_extents;

    void show_feedback(cairo_t* cr, const base::Rect& bounds);
    void begin_sleeping(double timestamp);
    void end_sleeping(double timestamp);
    bool feedback_step();
  };
}

#endif // !DOXYGEN_SHOULD_SKIP_THIS
