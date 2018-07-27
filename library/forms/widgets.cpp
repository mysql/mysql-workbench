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

#include "mforms/mforms.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_BE)

using namespace std;

using namespace mforms;
using namespace base;

// Animation timer. One timer for all widget animations.
static int animation_timer_refcount = 0;
static base::Mutex animation_timer_mutex;
static int animation_timer_id;
static vector<BaseWidget*> animated_widgets;

#ifdef __APPLE__
#define WIDGET_FONT "Lucida Grande"
#define WIDGET_SMALL_FONT "Lucida Grande"

// The HTML style color is for mforms control, the components used in cairo.
// Both must be the same color otherwise display will be wrong.
#define BACKGROUND_COLOR "#DBD9D9"
#define BK_RED 219 / 255.0
#define BK_GREEN 217 / 255.0
#define BK_BLUE 217 / 255.0
#elif _MSC_VER
#define WIDGET_FONT "Tahoma"
#define WIDGET_SMALL_FONT "Arial"

#define BACKGROUND_COLOR "#FFFFFF"
#define BK_RED 1
#define BK_GREEN 1
#define BK_BLUE 1
#else
#define WIDGET_FONT "Helvetica"
#define WIDGET_SMALL_FONT "Helvetica"

#define BACKGROUND_COLOR "#DBD9D9"
#define BK_RED 219 / 255.0
#define BK_GREEN 217 / 255.0
#define BK_BLUE 217 / 255.0
#endif

#define WIDGET_NORMAL_FONT_SIZE 11
#define WIDGET_DESCRIPTION_FONT_SIZE 11
#define WIDGET_WARNING_FONT_SIZE 9

//--------------------------------------------------------------------------------------------------

/**
 * Animation timer callback. Triggers all registered step() methods.
 */
static bool on_timer(int task_id) {
  base::MutexLock lock(animation_timer_mutex);

  for (vector<BaseWidget*>::const_iterator iterator = animated_widgets.begin(); iterator != animated_widgets.end();
       iterator++)
    (*iterator)->step();

  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Starts the animation timer if not yet done and increases the ref count for it, so it does not
 * get freed before the last consumer is gone.
 */
static void start_animation_timer_for(BaseWidget* widget) {
  base::MutexLock lock(animation_timer_mutex);

  animated_widgets.push_back(widget);
  if (animation_timer_refcount == 0)
    animation_timer_id = ThreadedTimer::add_task(TimerFrequency, 30, false, on_timer);
  animation_timer_refcount++;
}

//--------------------------------------------------------------------------------------------------

/**
 * Decreases the animation timer ref count and frees the timer if no consumer is left.
 */
static void stop_animation_timer_for(BaseWidget* widget) {
  base::MutexLock lock(animation_timer_mutex);

  for (vector<BaseWidget*>::iterator iterator = animated_widgets.begin(); iterator != animated_widgets.end();
       iterator++)
    if (*iterator == widget) {
      animated_widgets.erase(iterator);
      break;
    }

  if (animation_timer_refcount <= 0)
    logWarning("Unbalanced feedback timer deactivation in LineDiagramWidget.");
  animation_timer_refcount--;
  if (animation_timer_refcount <= 0)
    ThreadedTimer::remove_task(animation_timer_id);
}

//--------------------------------------------------------------------------------------------------

BaseWidget::BaseWidget() {
  _background = NULL;
  _lower_limit = 0;
  _upper_limit = 1;
  _right_align = false; // Right alignment of the description.
  _auto_scale = false;
  _layout_width = 100;
  _layout_height = 64;
  _layout_surface = NULL;
  _layout_context = NULL;

  _description = "";
  _description_offset = 0;

  _last_width = 0;
  _last_height = 0;
}

//--------------------------------------------------------------------------------------------------

BaseWidget::~BaseWidget() {
  destroy_background();
}

//--------------------------------------------------------------------------------------------------

void BaseWidget::lock() {
  _lock.lock();
}

//--------------------------------------------------------------------------------------------------

void BaseWidget::unlock() {
  _lock.unlock();
}

//--------------------------------------------------------------------------------------------------

void BaseWidget::destroy_background() {
  if (_background != NULL) {
    cairo_surface_destroy(_background);
    _background = NULL;
  }
  if (_layout_surface != NULL) {
    cairo_surface_destroy(_layout_surface);
    _layout_surface = NULL;
  }
  if (_layout_context != NULL) {
    cairo_destroy(_layout_context);
    _layout_context = NULL;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Normalizes the input value to the range [0..1] depending on the set input value range.
 */
double BaseWidget::normalize(double input) {
  if (_upper_limit - _lower_limit == 0)
    return 0;

  if (input < _lower_limit)
    input = _lower_limit;
  if (input > _upper_limit)
    input = _upper_limit;
  return (input - _lower_limit) / (_upper_limit - _lower_limit);
}

//--------------------------------------------------------------------------------------------------

/**
 * Computes new value ranges for the widget if necessary.
 *
 * @result True if the ranges were updated, otherwise false.
 */
bool BaseWidget::compute_scale(double min, double max) {
  double new_upper = _upper_limit;
  double new_lower = _lower_limit;

  // Find the lowest threshold which is greater than the currently highest value.
  // This will become the new upper limit. Similar for the lower limit.
  for (ThresholdList::const_iterator iterator = _upper_thresholds.begin(); iterator != _upper_thresholds.end();
       iterator++)
    if (*iterator > max) {
      new_upper = *iterator;
      break;
    }
  for (ThresholdList::const_iterator iterator = _lower_thresholds.begin(); iterator != _lower_thresholds.end();
       iterator++)
    if (*iterator < min) {
      new_lower = *iterator;
      break;
    }

  bool result = (new_upper != _upper_limit) || (new_lower != _lower_limit);
  if (result)
    set_value_range(new_lower, new_upper);

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Updates the value range depending on the auto scale mode and what is the
 * currently largest value in the widget.
 */
void BaseWidget::auto_scale(double value) {
  if (_auto_scale) {
    double min, max;
    get_minmax_values(&min, &max);
    min = min * (_upper_limit - _lower_limit) - _lower_limit;
    max = max * (_upper_limit - _lower_limit) - _lower_limit;
    if (value > max)
      max = value;
    if (value < min)
      min = value;
    compute_scale(min, max);
  }
}

//--------------------------------------------------------------------------------------------------

void BaseWidget::get_minmax_values(double* min, double* max) {
  *min = 0;
  *max = 0;
}

//--------------------------------------------------------------------------------------------------

void BaseWidget::set_right_align(bool flag) {
  lock();
  if (_right_align != flag) {
    _right_align = flag;
    set_layout_dirty(true);
    set_needs_repaint();
  }
  unlock();
}

//--------------------------------------------------------------------------------------------------

void BaseWidget::enable_auto_scale(bool enable) {
  lock();
  _auto_scale = enable; // Change will be visible when next value comes in.
  unlock();
}

//--------------------------------------------------------------------------------------------------

/**
 * Allows to specify in what range input values are to be expected. This range will be used to
 * normalize input values and to clamp them if they are outside it.
 *
 * @param low The lower bound of the input value range. Can be less than zero. Must be less than the high bound.
 * @param high The upper bound of the input value range. Can be less than zero. Must be more than the lower bound.
 */
void BaseWidget::set_value_range(double low, double high) {
  if (low <= high && (low != _lower_limit || high != _upper_limit)) {
    // Precompute transformation factors for recomputing existing values.
    // To compute the original value we have:
    //   (I)   O = Vo * (Uo - Lo) + Lo    (value-old, upper-old, lower-old).
    // To compute the new value from the original value we do (see also normalize()):
    //   (II)  Vn = (O - Ln) / (Un - Ln)   (value-new, lower-new, upper-new).
    // Inserting (I) into (II) gives the two factors we will pass on.
    double scale = (low == high) ? 0 : (_upper_limit - _lower_limit) / (high - low);
    double offset = (low == high) ? 0 : (_lower_limit - low) / (high - low);

    _lower_limit = low;
    _upper_limit = high;

    range_updated(scale, offset);

    set_needs_repaint();
  }
}

//--------------------------------------------------------------------------------------------------

void BaseWidget::set_thresholds(ThresholdList lower_thresholds, ThresholdList upper_thresholds) {
  // Changes in thresholds are visible on next auto scale and repaint.
  _lower_thresholds = lower_thresholds;
  _upper_thresholds = upper_thresholds;
  set_needs_repaint();
}

//--------------------------------------------------------------------------------------------------

double BaseWidget::get_upper_range() const {
  return _upper_limit;
}

//--------------------------------------------------------------------------------------------------

void BaseWidget::set_description(const std::string& description) {
  if (_description != description) {
    _description = description;
    create_context_for_layout();
    if (layout(_layout_context))
      set_layout_dirty(true);
    set_needs_repaint();
  }
}

//--------------------------------------------------------------------------------------------------

void BaseWidget::repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) {
  if (is_layout_dirty() || _last_height != get_height() || _last_width != get_width())
    layout(cr);

  prepare_background();

  lock();

  // Now draw background graphics created by the widgets.
  if (_background != NULL) {
    cairo_set_source_surface(cr, _background, 0, 0);
    cairo_paint(cr);
  }

  if (_description != "") {
    cairo_select_font_face(cr, WIDGET_SMALL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, WIDGET_DESCRIPTION_FONT_SIZE);

    cairo_set_source_rgb(cr, 0x5f / 255.0, 0x5f / 255.0, 0x5f / 255.0);
    cairo_move_to(cr, _description_offset, get_height() - 4);
    cairo_show_text(cr, _description.c_str());
    cairo_stroke(cr);
  }
  unlock();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the computed size for this widget (min size).
 */
base::Size BaseWidget::getLayoutSize(base::Size proposedSize) {
  if (is_layout_dirty()) {
    create_context_for_layout();
    layout(_layout_context);
  }

  return base::Size(_layout_width, _layout_height);
}

//--------------------------------------------------------------------------------------------------

/**
 * Computes the layout of the widget description, determining so its size.
 *
 * @result Returns true if size did change, otherwise false.
 */
bool BaseWidget::layout(cairo_t* cr) {
  lock();

  set_layout_dirty(false);
  _last_height = get_height();
  _last_width = get_width();

  bool result = false;
  if (_description != "") {
    cairo_select_font_face(cr, WIDGET_SMALL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, WIDGET_DESCRIPTION_FONT_SIZE);

    // Make description area height text independent or it will vary with the text.
    cairo_font_extents_t extents;
    cairo_font_extents(cr, &extents);
    _diagram_area = base::Rect(0, 0, get_width(), get_height() - (int)ceil(extents.height) - 4);

    cairo_text_extents_t text_extents;
    cairo_text_extents(cr, _description.c_str(), &text_extents);

    int new_width = (int)ceil(text_extents.width);
    if (new_width > _layout_width) {
      _layout_width = new_width; // Grow only, otherwise the layout might constantly change.
      result = true;
    }

    _description_offset = _right_align ? get_width() - (int)ceil(text_extents.x_advance) : 0;
  }

  unlock();

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates a cairo context on a small image surface, to be used for layouting.
 */
void BaseWidget::create_context_for_layout() {
  if (_layout_surface == NULL)
    _layout_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, get_width(), get_height());
  if (_layout_context == NULL)
    _layout_context = cairo_create(_layout_surface);
}

//----------------- WidgetSeparator ----------------------------------------------------------------

void WidgetSeparator::repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) {
  //  cairo_set_source_rgb(cr, BK_RED, BK_GREEN, BK_BLUE); // Same as the widget box.
  //  cairo_rectangle(cr, 0, 0, get_width(), get_height());
  //  cairo_fill(cr);

  double offset = get_width() / 2;

  cairo_set_line_width(cr, 1);

  cairo_set_source_rgb(cr, 179 / 255.0, 179 / 255.0, 179 / 255.0);
  cairo_move_to(cr, offset, 0);
  cairo_line_to(cr, offset, get_height());
  cairo_stroke(cr);

  cairo_set_source_rgb(cr, 240 / 255.0, 240 / 255.0, 240 / 255.0);
  cairo_move_to(cr, offset + 0.5, 0);
  cairo_line_to(cr, offset + 0.5, get_height());
  cairo_stroke(cr);
}

//----------------- HeartbeatWidget ----------------------------------------------------------------

HeartbeatWidget::HeartbeatWidget() : BaseWidget() {
  memset(_luminance, 0, sizeof(_luminance));
  memset(_deflection, 0, sizeof(_deflection));
  _pivot = 0;
  start_animation_timer_for(this);
}

//--------------------------------------------------------------------------------------------------

HeartbeatWidget::~HeartbeatWidget() {
  stop_animation_timer_for(this);
}

//--------------------------------------------------------------------------------------------------

/**
 * Sets the heartbeat value at the current pivot position. The value must be in the range of 0..1.
 */
void HeartbeatWidget::set_value(double value) {
  value = normalize(value);

  lock();

  // Set the deflection at the pivot point to the new value.
  _deflection[_pivot] = value;

  unlock();
}

//--------------------------------------------------------------------------------------------------

/**
 * Computes the values for the next paint operation and triggers invalidation so that
 * the widget is redrawn.
 */
void HeartbeatWidget::step() {
  lock();

  // Decrease luminance each point by one step to simulate a fade-out.
  // Go backwards and stop when we reach the first 0 value.
  static double luminance_step = 1.5 / HEARTBEAT_DATA_SIZE;
  int index = _pivot - 1;
  while (true) {
    if (index < 0)
      index = HEARTBEAT_DATA_SIZE - 1;
    if (index == _pivot)
      break;

    _luminance[index] -= luminance_step;
    if (_luminance[index] < 0)
      _luminance[index] = 0;
    if (_luminance[index] == 0)
      break;
    index--;
  }

  _luminance[_pivot] = 1;

  // Precompute next deflection (simulate swing-out) and move pivot point.
  double current_value = _deflection[_pivot];
  _pivot++;
  if (_pivot == HEARTBEAT_DATA_SIZE)
    _pivot = 0;

  _deflection[_pivot] = -0.5 * current_value;

  unlock();

  set_needs_repaint();
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates the static background (including the dotted graph lines) if not yet done or if the
 * size of the control changed.
 */
void HeartbeatWidget::prepare_background() {
  if (_background == NULL || cairo_image_surface_get_width(_background) != _diagram_area.width() ||
      cairo_image_surface_get_height(_background) != _diagram_area.height()) {
    destroy_background();
    _background =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)_diagram_area.width(), (int)_diagram_area.height());
    cairo_t* cr = cairo_create(_background);

    // Erase background to constant color.
    cairo_set_source_rgb(cr, 48 / 255.0, 54 / 255.0, 59 / 255.0);
    cairo_paint(cr);

    // Top line is a gradient.
    cairo_pattern_t* gradient = cairo_pattern_create_linear(0, 0, 0, 5);
    cairo_pattern_add_color_stop_rgba(gradient, 0, 1, 1, 1, 0.2);
    cairo_pattern_add_color_stop_rgba(gradient, 1, 1, 1, 1, 0);

    cairo_set_source(cr, gradient);
    cairo_set_line_width(cr, 5);
    cairo_move_to(cr, 2.5, 3);
    cairo_line_to(cr, _diagram_area.width() - 2.5, 3);
    cairo_stroke(cr);

    cairo_pattern_destroy(gradient);

    // Dotted graph lines.
    double dashes[] = {3.0, 2.0};

    cairo_set_dash(cr, dashes, 2, 0);
    cairo_set_source_rgb(cr, 72 / 255.0, 78 / 255.0, 83 / 255.0);
    cairo_set_line_width(cr, 1);

    double y = 4.5;
    while (y < _diagram_area.height()) {
      cairo_move_to(cr, 4.5, y);
      cairo_line_to(cr, _diagram_area.width() - 4.5, y);
      y += 7;
    }
    cairo_stroke(cr);

    cairo_destroy(cr);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Called when the user changed the value range. Transform existing values into that new range.
 * The given parameters are precomputed values that allow to do a simple transformation.
 */
void HeartbeatWidget::range_updated(double scale, double offset) {
  lock();
  for (int i = 0; i < HEARTBEAT_DATA_SIZE; i++)
    _deflection[i] = _deflection[i] * scale + offset;

  unlock();
}

//--------------------------------------------------------------------------------------------------

void HeartbeatWidget::get_minmax_values(double* min, double* max) {
  *min = 0;
  *max = 0;

  lock();
  for (int i = 0; i < HEARTBEAT_DATA_SIZE; i++) {
    if (_deflection[i] > *max)
      *max = _deflection[i];
    if (_deflection[i] < *min)
      *min = _deflection[i];
  }

  unlock();
}

//--------------------------------------------------------------------------------------------------

void HeartbeatWidget::repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) {
  BaseWidget::repaint(cr, areax, areay, areaw, areah);

  Rect bounds = _diagram_area;

  // Draw the heartbeat line.
  cairo_set_line_width(cr, 2);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

  cairo_move_to(cr, 8, bounds.height() / 2);
  bounds.size.width -= 16; // Leave 8 pixels room left and right.

  lock();
  for (double pixel = 0; pixel < bounds.width(); pixel++) {
    // Transform pixel index into data array index. Merge values for fractions.
    int index1 = (int)floor(pixel * HEARTBEAT_DATA_SIZE / bounds.width());
    int index2 = index1 + 1;
    if (index2 == HEARTBEAT_DATA_SIZE)
      index2 = 0;
    double fraction = (pixel * HEARTBEAT_DATA_SIZE / bounds.width()) - index1;

    double deflection = _deflection[index1] * (1 - fraction) + _deflection[index2] * fraction;
    double luminance = _luminance[index1] * (1 - fraction) + _luminance[index2] * fraction;

    double x = 8.5 + pixel;
    double y = -deflection * (bounds.height() - 8) / 2 + bounds.height() / 2;
    cairo_set_source_rgba(cr, 102 / 255.0, 171 / 255.0, 251 / 255.0, luminance);
    cairo_line_to(cr, x, y);
    cairo_stroke(cr);

    cairo_move_to(cr, x, y);
  }
  unlock();
}

//----------------- ServerStatusWidget ---------------------------------------------------------------

ServerStatusWidget::ServerStatusWidget() {
  _status = -1;
  _image_unknown = Utilities::load_icon("admin_info_unknown.png", true);
  _image_running = Utilities::load_icon("admin_info_running.png", true);
  _image_stopped = Utilities::load_icon("admin_info_stopped.png", true);
  _image_offline = Utilities::load_icon("admin_info_offline.png", true);
}

//--------------------------------------------------------------------------------------------------

ServerStatusWidget::~ServerStatusWidget() {
  cairo_surface_destroy(_image_unknown);
  cairo_surface_destroy(_image_running);
  cairo_surface_destroy(_image_stopped);
  cairo_surface_destroy(_image_offline);
}

//--------------------------------------------------------------------------------------------------

void ServerStatusWidget::set_server_status(int status) {
  // Sanity check.
  if (status < -1 || status > 2)
    status = -1;

  lock();
  if (_status != status) {
    _status = status;
    set_layout_dirty(true);
    set_needs_repaint();
  }
  unlock();
}

//--------------------------------------------------------------------------------------------------

#define LINE_SPACING 4 // Extra spacing between lines.

void ServerStatusWidget::repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) {
  BaseWidget::repaint(cr, areax, areay, areaw, areah);

  lock();

  cairo_save(cr);

  cairo_surface_t* icon;
  switch (_status) {
    case 2:
      icon = _image_offline;
      break;
    case 1:
      icon = _image_running;
      break;
    case 0:
      icon = _image_stopped;
      break;
    default:
      icon = _image_unknown;
  }

  // Icon.
  if (icon != NULL) {
    float scale;
    if (mforms::Utilities::is_hidpi_icon(icon) && (scale = mforms::App::get()->backing_scale_factor()) > 1)
      cairo_scale(cr, 1 / scale, 1 / scale);
    cairo_set_source_surface(cr, icon, 0, 0);
    cairo_paint(cr);
  }

  cairo_restore(cr);
  unlock();
}

//--------------------------------------------------------------------------------------------------

/**
 * This function computes the overall layout of the widget and sets its size.
 */
bool ServerStatusWidget::layout(cairo_t* cr) {
  _layout_width = 0;
  _layout_height = 0;
  BaseWidget::layout(cr);

  cairo_save(cr);

  cairo_select_font_face(cr, WIDGET_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, WIDGET_NORMAL_FONT_SIZE);

  lock();

  // Icon and main text position.
  // Icon size for left text offset and total size.
  cairo_surface_t* icon;
  switch (_status) {
    case 2:
      icon = _image_offline;
      break;
    case 1:
      icon = _image_running;
      break;
    case 0:
      icon = _image_stopped;
      break;
    default:
      icon = _image_unknown;
  }

  if (icon != NULL) {
    _layout_width = MAX(_layout_width, cairo_image_surface_get_width(icon));
    _layout_height += cairo_image_surface_get_height(icon) + LINE_SPACING;

    float scale;
    if (mforms::Utilities::is_hidpi_icon(icon) && (scale = mforms::App::get()->backing_scale_factor()) > 1) {
      _layout_width = (int)(_layout_width / scale);
      _layout_height = (int)(_layout_height / scale);
    }
  }

  cairo_restore(cr);

  unlock();

  return true;
}

//----------------- BarGraphWidget -----------------------------------------------------------------------

#define BAR_WIDTH 31

BarGraphWidget::BarGraphWidget() : _value(0), _value_gradient(NULL), _grid(NULL) {
  _layout_width = BAR_WIDTH;
}

//--------------------------------------------------------------------------------------------------

BarGraphWidget::~BarGraphWidget() {
}

//--------------------------------------------------------------------------------------------------

void BarGraphWidget::prepare_background() {
  Rect bounds = _diagram_area;

  if (_background == NULL || cairo_image_surface_get_height(_background) != bounds.height()) {
    destroy_background();
    _background = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, BAR_WIDTH, (int)bounds.height());
    cairo_t* cr = cairo_create(_background);

    // The background is a constant color with a gradient overlay and a darker border.
    cairo_set_source_rgb(cr, 52 / 255.0, 54 / 255.0, 56 / 255.0);
    cairo_paint(cr);

    cairo_pattern_t* gradient = cairo_pattern_create_linear(0, 0, 0, bounds.height() - 2);
    cairo_pattern_add_color_stop_rgba(gradient, 0, 122 / 255.0, 140 / 255.0, 154 / 255.0, 0.4);
    cairo_pattern_add_color_stop_rgba(gradient, 0.08, 151 / 255.0, 170 / 255.0, 184 / 255.0, 0.6);
    cairo_pattern_add_color_stop_rgba(gradient, 0.09, 105 / 255.0, 122 / 255.0, 135 / 255.0, 0.6);
    cairo_pattern_add_color_stop_rgba(gradient, 1, 60 / 255.0, 66 / 255.0, 71 / 255.0, 0.6);

    cairo_rectangle(cr, 1, 1, BAR_WIDTH - 2, bounds.height() - 2);
    cairo_set_source(cr, gradient);
    cairo_fill(cr);
    cairo_pattern_destroy(gradient);

    cairo_destroy(cr);

    // Grid overlay.
    _grid = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, BAR_WIDTH, (int)bounds.height());
    cr = cairo_create(_grid);
    cairo_set_source_rgba(cr, 52 / 255.0, 54 / 255.0, 56 / 255.0, 0.2);
    cairo_set_line_width(cr, 1);
    for (double y = bounds.height() - 3.5; y > 1; y -= 3) {
      cairo_move_to(cr, 0.5, y);
      cairo_line_to(cr, BAR_WIDTH - 0.5, y);
    }
    cairo_stroke(cr);

    cairo_move_to(cr, BAR_WIDTH / 2 + 0.5, bounds.height() - 0.5);
    cairo_line_to(cr, BAR_WIDTH / 2 + 0.5, 0.5);
    cairo_stroke(cr);

    cairo_destroy(cr);

    create_value_gradient();
  }
}

//--------------------------------------------------------------------------------------------------

void BarGraphWidget::destroy_background() {
  BaseWidget::destroy_background();

  if (_value_gradient != NULL)
    cairo_pattern_destroy(_value_gradient);
  _value_gradient = NULL;
  if (_grid != NULL)
    cairo_surface_destroy(_grid);
  _grid = NULL;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called when the user changed the value range. Transform existing values into that new range.
 * The given parameters are precomputed values that allow do a simple transformation.
 */
void BarGraphWidget::range_updated(double scale, double offset) {
  lock();
  _value = _value * scale + offset;
  unlock();
}

//--------------------------------------------------------------------------------------------------

void BarGraphWidget::get_minmax_values(double* min, double* max) {
  lock();
  *min = _value;
  *max = _value;
  unlock();
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates the gradient for the value display, which depends on both, the control's dimension
 * as well as the actual value.
 */
void BarGraphWidget::create_value_gradient() {
  // The value gradient is always fully shown (i.e. over the full range), regardless of the value.
  // Though the value changes, so we have to create the gradient exactly for each value to make it work as we want.
  if (_value_gradient != NULL)
    cairo_pattern_destroy(_value_gradient);

  Rect bounds = _diagram_area;
  double split_point = (bounds.height() - 2) * _value;
  split_point = 3 * (split_point / 3); // Integer div.

  _value_gradient = cairo_pattern_create_linear(0, bounds.height() - 1 - split_point, 0, bounds.height() - 1);
  cairo_pattern_add_color_stop_rgb(_value_gradient, 0, 102 / 255.0, 171 / 255.0, 251 / 255.0);
  cairo_pattern_add_color_stop_rgb(_value_gradient, 1, 0 / 255.0, 119 / 255.0, 189 / 255.0);
}

//--------------------------------------------------------------------------------------------------

void BarGraphWidget::set_value(double value) {
  value = normalize(value);
  if (_value != value) {
    _value = value;
    create_value_gradient();
    set_needs_repaint();
  }
}

//--------------------------------------------------------------------------------------------------

void BarGraphWidget::repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) {
  BaseWidget::repaint(cr, areax, areay, areaw, areah);

  // Fill body with thick lines in either dark gray or a blue gradient depending on the current bar value.
  Rect bounds = _diagram_area;

  lock();

  // Compute the split point as multiple of 3 to simulate a discret bar graph.
  double split_point = (bounds.height() - 2) * _value;
  split_point = 3 * (int)(split_point / 3);

  cairo_set_source(cr, _value_gradient);
  cairo_rectangle(cr, 1, bounds.height() - 1 - split_point, BAR_WIDTH - 2, split_point);
  cairo_fill(cr);

  // Finally the grid overlay.
  cairo_set_source_surface(cr, _grid, 0, 0);
  cairo_paint(cr);
  unlock();
}

//----------------- LineDiagramWidget --------------------------------------------------------------

LineDiagramWidget::LineDiagramWidget() {
  memset(_deflection, 0, sizeof(_deflection));
  memset(_timestamp, 0, sizeof(_timestamp));
  _time_in_view = 60; // Corresponds to a zoom factor of 100%.
  _next_value = 0;
  _value_gradient = NULL;
  _grid = NULL;
  _content_alpha = 1;
  _warning_alpha = 0;
  _warning_extents.width = 0;
  _sleep_start = 0;
  _sleep_mode = Awake;
  _last_shift = 0;

  _clock = g_timer_new();
  g_timer_start(_clock);

  start_animation_timer_for(this);
}

//--------------------------------------------------------------------------------------------------

LineDiagramWidget::~LineDiagramWidget() {
  stop_animation_timer_for(this);

  lock();
  g_timer_destroy(_clock);
  unlock();
}

//--------------------------------------------------------------------------------------------------

void LineDiagramWidget::repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) {
  BaseWidget::repaint(cr, areax, areay, areaw, areah);

  Rect bounds = _diagram_area;

  lock();

  cairo_push_group(cr); // Prepare composing.

  // if (_sleep_mode != Sleeping)
  {
    cairo_set_line_width(cr, 1);
    cairo_set_source(cr, _value_gradient);

    // Search backwards for the first value which has been measured
    // at least _time_in_view before.
    double current_time = g_timer_elapsed(_clock, NULL);
    int index = LINE_SERIES_DATA_SIZE - 1;
    while ((index > 0) && (_timestamp[index] > 0) && ((current_time - _timestamp[index]) < _time_in_view))
      index--;
    double start_time = current_time - _time_in_view;

    // The first point just sets a location for the first curve to start.
    // This can be outside of the current window but we need it to correctly draw the first curve part.
    double deflection = _deflection[index];
    double x = (_timestamp[index] - start_time) * bounds.width() / _time_in_view;
    double y = (bounds.height() - 2) * (1 - deflection) + 0.5;
    cairo_move_to(cr, x, y);

    for (; index < LINE_SERIES_DATA_SIZE; index++) {
      // Compute control points for current position and the next position.
      double x1 = x;
      double y1 = y;

      deflection = _deflection[index];
      x = (_timestamp[index] - start_time) * bounds.width() / _time_in_view;
      y = (bounds.height() - 2) * (1 - deflection) + 0.5;

      double x2 = x;
      double y2 = y;

      // Move horizontal control point location to the middle between both endpoints.
      double dX = (x2 - x1) / 2;
      x1 += dX;
      x2 -= dX;

      cairo_curve_to(cr, x1, y1, x2, y2, x, y);
    }

    // Close the figure for proper filling.
    cairo_line_to(cr, bounds.width() - 1.5, y);
    cairo_line_to(cr, bounds.width() - 1.5, bounds.height() - 1.5);
    cairo_line_to(cr, 1.5, bounds.height() - 1.5);

    cairo_fill(cr);

    // Part of the border has been overdrawn, fix it.
    cairo_set_source_rgb(cr, 52 / 255.0, 54 / 255.0, 56 / 255.0);
    cairo_rectangle(cr, 0.5, 0.5, bounds.width() - 1, bounds.height() - 1);
    cairo_stroke(cr);
  }

  // Finally the grid overlay.
  cairo_set_source_surface(cr, _grid, 0, 0);
  cairo_paint(cr);

  // Compose background, content and feedback.
  cairo_pop_group_to_source(cr);
  cairo_paint_with_alpha(cr, _content_alpha);
  show_feedback(cr, bounds);

  unlock();
}

//--------------------------------------------------------------------------------------------------

void LineDiagramWidget::set_value(double value) {
  auto_scale(value);

  value = normalize(value);

  lock();
  double timestamp = g_timer_elapsed(_clock, NULL);

  // Move values one step to the left.
  memmove(&_deflection[0], &_deflection[1], sizeof(_deflection) - sizeof(_deflection[0]));
  _deflection[LINE_SERIES_DATA_SIZE - 1] = value;

  memmove(&_timestamp[0], &_timestamp[1], sizeof(_timestamp) - sizeof(_timestamp[0]));
  _timestamp[LINE_SERIES_DATA_SIZE - 1] = timestamp;

  unlock();
}

//--------------------------------------------------------------------------------------------------

void LineDiagramWidget::prepare_background() {
  Rect bounds = _diagram_area;

  if (_background == NULL || cairo_image_surface_get_height(_background) != bounds.height() ||
      cairo_image_surface_get_width(_background) != bounds.width()) {
    destroy_background();
    _background = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)bounds.width(), (int)bounds.height());
    cairo_t* cr = cairo_create(_background);

    // The background is a constant color with a gradient overlay and a darker border.
    cairo_set_source_rgb(cr, 52 / 255.0, 54 / 255.0, 56 / 255.0);
    cairo_paint(cr);

    cairo_pattern_t* gradient = cairo_pattern_create_linear(0, 0, 0, bounds.height() - 2);
    cairo_pattern_add_color_stop_rgba(gradient, 0, 122 / 255.0, 140 / 255.0, 154 / 255.0, 0.4);
    cairo_pattern_add_color_stop_rgba(gradient, 0.08, 151 / 255.0, 170 / 255.0, 184 / 255.0, 0.6);
    cairo_pattern_add_color_stop_rgba(gradient, 0.09, 105 / 255.0, 122 / 255.0, 135 / 255.0, 0.6);
    cairo_pattern_add_color_stop_rgba(gradient, 1, 60 / 255.0, 66 / 255.0, 71 / 255.0, 0.6);

    cairo_rectangle(cr, 1, 1, bounds.width() - 2, bounds.height() - 2);
    cairo_set_source(cr, gradient);
    cairo_fill(cr);
    cairo_pattern_destroy(gradient);

    cairo_destroy(cr);

    // Grid overlay.
    _grid = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)bounds.width(), (int)bounds.height());
    cr = cairo_create(_grid);
    cairo_set_source_rgba(cr, 52 / 255.0, 54 / 255.0, 56 / 255.0, 0.2);
    cairo_set_line_width(cr, 1);
    for (double y = bounds.height() - 3.5; y > 5; y -= 3) {
      cairo_move_to(cr, 0.5, y);
      cairo_line_to(cr, (int)bounds.width() - 0.5, y);
    }
    cairo_stroke(cr);

    double dX = 15 * 60 / _time_in_view; // That is: a 16 pixel wide rectangle at 1 min. total time.
    for (double x = bounds.width() - dX - 0.5; x > 1; x -= dX) {
      cairo_move_to(cr, x, 0.5);
      cairo_line_to(cr, x, (int)bounds.height() - 0.5);
    }
    cairo_stroke(cr);

    cairo_destroy(cr);

    _value_gradient = cairo_pattern_create_linear(0, 0, 0, bounds.height() - 2);
    cairo_pattern_add_color_stop_rgb(_value_gradient, 0, 102 / 255.0, 171 / 255.0, 251 / 255.0);
    cairo_pattern_add_color_stop_rgb(_value_gradient, 1, 0 / 255.0, 119 / 255.0, 189 / 255.0);
  }
}

//--------------------------------------------------------------------------------------------------

void LineDiagramWidget::destroy_background() {
  BaseWidget::destroy_background();

  if (_value_gradient != NULL)
    cairo_pattern_destroy(_value_gradient);
  _value_gradient = NULL;
  if (_grid != NULL)
    cairo_surface_destroy(_grid);
  _grid = NULL;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called when the user changed the value range. Transform existing values into that new range.
 * The given parameters are precomputed values that allow to do a simple transformation.
 */
void LineDiagramWidget::range_updated(double scale, double offset) {
  lock();
  for (int i = 0; i < LINE_SERIES_DATA_SIZE; i++)
    _deflection[i] = _deflection[i] * scale + offset;

  unlock();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the largest and smallest value in the current time range.
 */
void LineDiagramWidget::get_minmax_values(double* min, double* max) {
  *min = 0;
  *max = 0;

  // Search backwards for the first value which has been measured
  // at least _time_in_view before.
  double current_time = g_timer_elapsed(_clock, NULL);
  int index = LINE_SERIES_DATA_SIZE - 1;
  while ((index > 0) && (_timestamp[index] > 0) && ((current_time - _timestamp[index]) < _time_in_view))
    index--;

  lock();
  for (; index < LINE_SERIES_DATA_SIZE; index++) {
    if (_deflection[index] > *max)
      *max = _deflection[index];
    if (_deflection[index] < *min)
      *min = _deflection[index];
  }

  unlock();
}

//--------------------------------------------------------------------------------------------------

#define DATA_TIMEOUT 15 // 15 seconds after which the diagram will go to sleep if no data comes in.

void LineDiagramWidget::step() {
  double timestamp = g_timer_elapsed(_clock, NULL);

  bool needs_repaint = false;
  if (_sleep_mode == Awake && timestamp - _last_shift >= 0.5) {
    // Two shifts per second.
    _last_shift = timestamp;
    auto_scale(0);
    needs_repaint = true;
  }

  lock();

  // Determine if we have to go sleeping or awake.
  if (_sleep_mode == Awake) {
    // We are not sleeping or going to sleep. Check if we need to.
    if (timestamp - _timestamp[LINE_SERIES_DATA_SIZE - 1] >= DATA_TIMEOUT) {
      begin_sleeping(timestamp);
      needs_repaint = true;
    }
  } else {
    // We are sleeping. Check if we can awake.
    if (_sleep_mode == Sleeping && (timestamp - _timestamp[LINE_SERIES_DATA_SIZE - 1] < DATA_TIMEOUT)) {
      end_sleeping(timestamp);
      needs_repaint = true;
    }
  }
  if (feedback_step())
    needs_repaint = true;

  if (needs_repaint)
    set_needs_repaint();

  unlock();
}

//--------------------------------------------------------------------------------------------------

#define WARNING_TEXT "No Data"

void LineDiagramWidget::show_feedback(cairo_t* cr, const Rect& bounds) {
  if (_sleep_mode != Awake) {
    cairo_select_font_face(cr, WIDGET_SMALL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, WIDGET_WARNING_FONT_SIZE);

    if (_warning_extents.width == 0)
      cairo_text_extents(cr, WARNING_TEXT, &_warning_extents);

    // Place the warning in the upper left corner.
    int y = (int)(bounds.top() + 4 - _warning_extents.y_bearing);
    int x = (int)(bounds.left() + 4);

    cairo_push_group(cr); // Prepare composing.

    cairo_set_source_rgb(cr, 220 / 255.0, 220 / 255.0, 220 / 255.0);
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, WARNING_TEXT);
    cairo_stroke(cr);

    cairo_pop_group_to_source(cr);
    cairo_paint_with_alpha(cr, _warning_alpha);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Triggered when, after some timeout, no new data arrives.
 * Note: lock has been acquired on call already.
 */
void LineDiagramWidget::begin_sleeping(double timestamp) {
  _sleep_mode = GoSleeping;
  _sleep_start = timestamp;
}

//--------------------------------------------------------------------------------------------------

/**
 * Triggered when, after we slept, new data arrives.
 * Note: lock has been acquired on call already.
 */
void LineDiagramWidget::end_sleeping(double timestamp) {
  _sleep_mode = Awaking;
  _sleep_start = timestamp;
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to compute the next feedback step, if there is any feedback.
 * Returns true if the widget must be repainted.
 */
bool LineDiagramWidget::feedback_step() {
  bool result = false;
  if (_sleep_mode != Awake) {
    // Determine if we have to go sleeping or awake.
    double timestamp = g_timer_elapsed(_clock, NULL);

    // Normalize time point to 6 seconds for one cycle and map it to radians.
    double time_point = 2 * M_PI * (timestamp - _sleep_start) / 6;

    switch (_sleep_mode) {
      case GoSleeping:
        // Go sleeping within first 3 seconds. After that normal sleeping alone takes place.
        if (timestamp - _sleep_start < 3) {
          // Compute content alpha value.
          // Minimum value is 25%. Divide by 2 as we shift the coordinate system 1 up (to
          // have no negative values).
          _content_alpha = 0.25 + 0.75 * 0.5 * (1 + cos(time_point));
        } else
          _sleep_mode = Sleeping;

        // Same computation for warning alpha (just the opposite direction).
        _warning_alpha = 0.25 + 0.75 * 0.5 * (1 + sin(time_point - M_PI / 2));

        // Just in case the GoSleeping phase could not be executed (the timer thread could not
        // trigger within the first second of the animation cycle) check the content alpha value
        // and set it to its endpoint to have a proper display.
        if (_sleep_mode == Sleeping && _content_alpha > 0.25)
          _content_alpha = 0.25;

        result = true;
        break;
      case Awaking:
        // When awaking simply do a linear interpolation for both alpha values (duration = 1 sec).
        // Once both have reached their end values we stop the animation.
        if (_content_alpha < 1)
          _content_alpha = min(1.0, timestamp - _sleep_start);
        if (_warning_alpha > 0)
          _warning_alpha = max(0.0, 1 - (timestamp - _sleep_start));
        if (_content_alpha == 1 && _warning_alpha == 0)
          _sleep_mode = Awake;

        result = true;
        break;
      case Awake:
      case Sleeping:
        break;
    }
  }
  return result;
}

//--------------------------------------------------------------------------------------------------
