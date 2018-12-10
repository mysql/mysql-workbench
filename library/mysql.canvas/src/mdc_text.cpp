/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mdc_text.h"
#include "mdc_canvas_view.h"

/**
 * @file mdc_text.cpp
 *
 * Implementation of the text rendering classes.
 */

using namespace mdc;
using namespace base;

/**
 * Constructor of text figure class.
 *
 * @param layer The layer on which the text figure will be located.
 */
TextFigure::TextFigure(Layer *layer) : Figure(layer) {
  _font = layer->get_view()->get_default_font();
  CairoCtx *cr(get_layer()->get_view()->cairoctx());
  cr->get_font_extents(_font, _font_extents);

  _text_layout = 0;

  _multi_line = false;
  _allow_shrinking = false;
  _allow_wrapping = false;

  _align = AlignLeft;

  _fill_background = false;
  _draw_outline = false;
  _highlight_through_text = false;
  scoped_connect(signal_bounds_changed(), std::bind(&TextFigure::reset_shrinked_text, this));
}

/**
 * Destructor of the text figure.
 */
TextFigure::~TextFigure() {
  delete _text_layout;
}

/**
 * Sets a flag that indicates whether the background must be filled or not.
 */
void TextFigure::set_fill_background(bool flag) {
  _fill_background = flag;
}

/**
 * Sets a flag that indicates whether the a white outline should be drawn around the text.
 */
void TextFigure::set_draw_outline(bool flag) {
  _draw_outline = flag;
}

void TextFigure::set_allow_wrapping(bool flag) {
  _allow_wrapping = flag;
}

/**
 * Resets the shorted text member.
 */
void TextFigure::reset_shrinked_text() {
  _shrinked_text.clear();
  base::Size size = get_size();
  if (_text_layout && !_auto_sizing && (_allow_wrapping || _allow_shrinking)) {
    base::Size lsize(size.width - 2 * _xpadding, size.height - 2 * _ypadding);
    if (lsize != _text_layout->get_size()) {
      _text_layout->set_size(lsize);
      set_needs_relayout();
    }
  }
}

/**
 * Returns the width for a piece of text depending on the used font setting.
 *
 * @param cr The context to which rendering will occur.
 * @param font The actual font specification to be used for rendering.
 * @param ptr The text to compute the width for.
 * @param offset Limits the length of the text to be considered for computation.
 *
 * @return A value describing the computed text width.
 */
static double get_text_width(CairoCtx *cr, const FontSpec &font, gchar *ptr, int offset,
                             cairo_text_extents_t &extents) {
  gchar save = ptr[offset];
  ptr[offset] = 0;

  cr->get_text_extents(font, ptr, extents);
  ptr[offset] = save;

  return extents.x_advance;
}

static std::string fit_text_to_width(CairoCtx *cr, const FontSpec &font, const std::string &text, double width,
                                     cairo_text_extents_t &extents) {
  gchar *ptr, *p, *prev;
  // calculate length in characters of string and also a mapping from character -> offset
  prev = p = ptr = g_strdup(text.c_str());
  while (p != NULL && *p) {
    double w;

    if (floor(w = get_text_width(cr, font, ptr, (int)(p - ptr), extents)) >= width) {
      g_free(ptr);
      return text.substr(0, prev - ptr);
    }

    prev = p;
    p = g_utf8_next_char(p);
  }

  g_free(ptr);

  return text;
}

static base::Range fit_text_to_width_word_wrap(CairoCtx *cr, const FontSpec &font, const std::string &text,
                                               double width, cairo_text_extents_t &extents) {
  gchar *ptr, *p, *prev, *pp, *start;
  // calculate length in characters of string and also a mapping from character -> offset
  prev = pp = p = ptr = g_strdup(text.c_str());

  // skip whitespaces at the beginning of the text
  while (*p == ' ')
    ++p;
  start = prev = pp = p;

  // pp points to the start of the word break sequence, while p points to the end (beginning of next word)
  while (p != NULL && *p) {
    prev = pp;

    while (*p == ' ') // skip spaces in words, as they will be all trimmed at the end of the line
      ++p;
    pp = strchr(p, ' '); // try to find the next word
    if (!pp && p < ptr + text.length())
      pp = ptr + text.length();
    p = pp;

    // the ptr param must be mutable
    if (p && get_text_width(cr, font, start, (int)(p - start), extents) > width) {
      g_free(ptr);
      return base::Range(start - ptr, prev - start);
    }
  }

  if (p && get_text_width(cr, font, start, (int)(p - start), extents) < width) {
    g_free(ptr);
    return base::Range(start - ptr, p - start);
  }
  g_free(ptr);
  return base::Range(0, text.length());
}

void TextFigure::draw_contents(CairoCtx *cr, const Rect &bounds) {
  if (_fill_background) {
    cr->set_color(_fill_color);
    cr->rectangle(bounds);
    cr->fill();
  }

  if (_text_layout) {
    Point pos = bounds.pos;
    Size size = bounds.size;

    pos.x += _xpadding;
    pos.y += _ypadding;

    size.width -= 2 * _xpadding;
    size.height -= 2 * _ypadding;

    if (_highlight_color && _highlighted) {
      cr->set_color(*_highlight_color);
      _text_layout->render(cr, pos - Point(1, 0), size, _align);
      _text_layout->render(cr, pos + Point(1, 0), size, _align);
      _text_layout->render(cr, pos - Point(0, 1), size, _align);
      _text_layout->render(cr, pos + Point(0, 1), size, _align);
    } else if (_draw_outline) {
      cr->set_color(base::Color::white());
      _text_layout->render(cr, pos - Point(1, 0), size, _align);
      _text_layout->render(cr, pos + Point(1, 0), size, _align);
      _text_layout->render(cr, pos - Point(0, 1), size, _align);
      _text_layout->render(cr, pos + Point(0, 1), size, _align);
    }

    cr->set_color(_pen_color);
    _text_layout->render(cr, pos, size, _align);
  } else {
    cairo_text_extents_t extents;
    double x, y;
    Point text_pos;

    cr->set_font(_font);
    cr->get_text_extents(_font, _text.c_str(), extents);

    x = bounds.left() + _xpadding;
    y = bounds.bottom() - (bounds.height() - _font_extents.height) / 2 - _font_extents.descent;
    if ((bounds.height() - _font_extents.height) > _font_extents.descent / 2)
      y += _font_extents.descent / 4;
    switch (_align) {
      case AlignLeft:
        text_pos = Point(x, y);
        break;
      case AlignCenter:
        text_pos = Point(x + ceil((bounds.size.width - 2 * _xpadding - extents.width) / 2), y);
        break;
      case AlignRight:
        text_pos = Point(x + bounds.right() - extents.width, y);
        break;
    }

    std::string text(_text);

    if (extents.width > bounds.size.width - 2 * _xpadding) {
      // the text doesnt fit in the space we have, so check how much of it does fit
      if (_shrinked_text.empty()) {
        cr->get_text_extents(_font, "\xe2\x80\xa6", extents);

        _shrinked_text =
          fit_text_to_width(cr, _font, _text, bounds.size.width - 2 * _xpadding - extents.x_advance, extents);
        _shrinked_text.append("\xe2\x80\xa6");
      }
      text = _shrinked_text;
    }

    if (_highlight_color && _highlighted && _highlight_through_text) {
      cr->set_color(*_highlight_color);
      for (int x = -3; x <= 3; x++) {
        for (int y = -3; y <= 3; y++) {
          cr->move_to(text_pos + Point(x, y));
          cr->show_text(text);
        }
      }
    }
    if (_draw_outline) {
      cr->set_color(base::Color::white());
      cr->move_to(text_pos + Point(1, 0));
      cr->show_text(text);
      cr->move_to(text_pos - Point(1, 0));
      cr->show_text(text);
      cr->move_to(text_pos + Point(0, 1));
      cr->show_text(text);
      cr->move_to(text_pos - Point(0, 1));
      cr->show_text(text);
    }
    cr->set_color(_pen_color);
    cr->move_to(text_pos);
    cr->show_text(text);

    cr->check_state();
  }
}

void TextFigure::draw_contents(CairoCtx *cr) {
  draw_contents(cr, get_bounds());
}

void TextFigure::set_multi_line(bool flag) {
  if (flag != _multi_line) {
    _multi_line = flag;

    if (!flag) {
      delete _text_layout;
      _text_layout = 0;

      _shrinked_text = "";
    } else {
      _text_layout = new TextLayout();
      _text_layout->set_text(_text);
      _text_layout->set_font(_font);
      reset_shrinked_text();
    }
  }
}

void TextFigure::set_allow_shrinking(bool flag) {
  _allow_shrinking = flag;
  _shrinked_text = "";
  reset_shrinked_text();
}

void TextFigure::set_font(const FontSpec &font) {
  if (_font != font) {
    _font = font;

    if (_text_layout)
      _text_layout->set_font(font);

    CairoCtx *cr(get_layer()->get_view()->cairoctx());
    cr->get_font_extents(font, _font_extents);

    _shrinked_text = "";
    set_needs_relayout();
  }
}

void TextFigure::set_text_alignment(TextAlignment align) {
  if (_align != align) {
    _align = align;
    set_needs_render();
  }
}

void TextFigure::set_text(const std::string &text) {
  if (_text != text) {
    _text = text;
    _shrinked_text = "";

    if (_text_layout)
      _text_layout->set_text(text);

    // set_needs_render();
    set_needs_relayout();
  }
}

void TextFigure::auto_size() {
  Size size;

  size = get_text_size();
  size.width += _xpadding * 2;
  size.height += _ypadding * 2;

  // set_fixed_size(size);
  resize_to(size);
}

Size TextFigure::get_text_size() {
  if (_text_layout) {
    _text_layout->relayout(get_layer()->get_view()->cairoctx());
    return _text_layout->get_size();
  }
  Size size;
  cairo_text_extents_t extents;

  get_layer()->get_view()->cairoctx()->get_text_extents(_font, _text, extents);

  size.width = ceil(extents.x_advance);
  size.height = ceil(_font_extents.height);

  return size;
}

Size TextFigure::calc_min_size() {
  Size size = get_text_size();

  if (_allow_shrinking && !_auto_sizing)
    size.width = 0;

  size.width += _xpadding * 2;
  size.height += _ypadding * 2;

  return size;
}

//------------------------------------------------------------------------------------------------

void TextLayout::break_paragraphs() {
  size_t start;
  const char *str = _text.c_str();
  const char *ptr;

  start = 0;

  _paragraphs.clear();

  while (str[start]) {
    ptr = strchr(str + start, '\n');
    if (ptr) {
      Paragraph para;

      para.text_offset = start;
      para.text_length = ptr - (str + start);

      _paragraphs.push_back(para);

      start += para.text_length + 1; // +1 to skip the \n
    } else {
      Paragraph para;

      para.text_offset = start;
      para.text_length = _text.length() - start;

      _paragraphs.push_back(para);

      start += para.text_length;
    }
  }
}

void TextLayout::relayout(CairoCtx *cr) {
  if (_needs_relayout) {
    _needs_relayout = false;

    _lines.clear();

    for (std::vector<Paragraph>::iterator iter = _paragraphs.begin(); iter != _paragraphs.end(); ++iter)
      layout_paragraph(cr, *iter);
  }
}

void TextLayout::layout_paragraph(CairoCtx *cr, Paragraph &para) {
  cairo_text_extents_t ext;

  cr->get_text_extents(_font, std::string(_text.c_str() + para.text_offset, para.text_length), ext);

  if (_fixed_size.width < 0 || ext.width < _fixed_size.width) {
    Line line;

    line.text_offset = para.text_offset;
    line.text_length = para.text_length;
    line.offset = Point(ceil(ext.x_bearing), ceil(ext.height * 2 + ext.y_bearing));
    line.size = Size(ceil(std::max(ext.width, ext.x_advance)), ceil(std::max(ext.height, ext.y_advance)));

    _lines.push_back(line);
  } else {
    Line line;
    size_t offset = para.text_offset;
    size_t length = para.text_length;

    for (;;) {
      base::Range range =
        fit_text_to_width_word_wrap(cr, _font, std::string(_text.c_str() + offset, length), _fixed_size.width, ext);
      offset += range.position;
      line.text_offset = offset;
      if (range.size <= 0)
        line.text_length = 1;
      else
        line.text_length = range.size;

      line.offset = Point(ceil(ext.x_bearing), ceil(ext.height * 2 + ext.y_bearing));
      line.size = Size(ceil(std::max(ext.width, ext.x_advance)), ceil(std::max(ext.height, ext.y_advance)));

      _lines.push_back(line);

      if (line.text_offset + line.text_length >= para.text_length)
        break;

      offset += range.position + line.text_length;
      length -= range.position + line.text_length;
    }
  }
}

TextLayout::TextLayout() {
  _fixed_size = Size(-1, -1);
  _needs_relayout = true;

  _font = FontSpec("Helvetica");
}

TextLayout::~TextLayout() {
}

void TextLayout::set_text(const std::string &text) {
  _text = text;

  break_paragraphs();

  _needs_relayout = true;
}

void TextLayout::set_font(const FontSpec &font) {
  _font = font;

  _needs_relayout = true;
}

void TextLayout::set_size(const base::Size &s) {
  _fixed_size = s;
  _needs_relayout = true;
}

Size TextLayout::get_size() {
  Size size = _fixed_size;
  double w = 0, h = 0;
  double line_spacing = floor(_font.size / 4) + 1;
  double line_height = 0;

  for (std::vector<Line>::const_iterator iter = _lines.begin(); iter != _lines.end(); ++iter) {
    w = std::max(w, iter->size.width);
    line_height = std::max(line_height, iter->size.height);
  }

  if (!_lines.empty())
    h = line_spacing * (_lines.size() - 1) + line_height * _lines.size();

  if (size.width < 0)
    size.width = ceil(w);
  if (size.height < 0)
    size.height = ceil(h);

  return size;
}

void TextLayout::render(CairoCtx *cr, const Point &pos, const Size &size, TextAlignment align) {
  double x = pos.x;
  double y = pos.y;
  double line_spacing = floor(_font.size / 4) + 1;
  double line_height;

  if (_needs_relayout)
    relayout(cr);

  line_height = 0;
  for (std::vector<Line>::const_iterator iter = _lines.begin(); iter != _lines.end(); ++iter)
    line_height = std::max(line_height, iter->size.height);

  cr->save();
  cr->set_font(_font);
  for (std::vector<Line>::const_iterator iter = _lines.begin(); iter != _lines.end(); ++iter) {
    y += line_height;

    switch (align) {
      case AlignLeft:
        cr->move_to(x, y);
        break;
      case AlignCenter:
        cr->move_to(x + (size.width - iter->size.width) / 2, y + iter->offset.y);
        break;
      case AlignRight:
        cr->move_to(x + (size.width - iter->size.width), y + iter->offset.y);
        break;
    }
    cr->show_text(std::string(_text.c_str() + iter->text_offset, iter->text_length));

    y += line_spacing;
  }
  cr->restore();
}
