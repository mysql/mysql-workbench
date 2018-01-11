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

#ifndef _MDC_TEXT_H_
#define _MDC_TEXT_H_

#include "mdc_figure.h"

/**
 * @file mdc_text.h
 *
 * Declaration of everything pertaining to the text output on the canvas. The file defines alignment and layout of
 * text as well as the actual text figure.
 */

namespace mdc {

  /**
   * Determines how text is to be aligned within the figure.
   */
  enum TextAlignment { AlignLeft, AlignCenter, AlignRight };

  /**
   * Manages the layout of text including rendering to a Cairo context.
   */
  class TextLayout {
    struct Paragraph {
      size_t text_offset;
      size_t text_length;
    };

    struct Line {
      size_t text_offset;
      size_t text_length;

      base::Point offset;
      base::Size size;
    };

    // A list of paragraphs which are rendered individually.
    std::vector<Paragraph> _paragraphs;

    // The text to render.
    std::string _text;

    // Description for each line.
    std::vector<Line> _lines;

    // The font to use for rendering.
    FontSpec _font;

    base::Size _fixed_size;
    bool _needs_relayout;

    void break_paragraphs();
    void layout_paragraph(CairoCtx *cr, Paragraph &para);

  public:
    TextLayout();
    ~TextLayout();

    void relayout(CairoCtx *cr);
    void render(CairoCtx *cr, const base::Point &pos, const base::Size &size, TextAlignment align);

    void set_text(const std::string &text);

    void set_font(const FontSpec &font);

    void set_size(const base::Size &s);

    base::Size get_size();
  };

  /**
   * Implements the actual canvas figure used to render text. This class manages the layouter as well as needed
   * resources. It is also the interface through which other parts control text rendering.
   */
  class MYSQLCANVAS_PUBLIC_FUNC TextFigure : public Figure {
  public:
    TextFigure(Layer *layer);
    virtual ~TextFigure();

    virtual base::Size calc_min_size();

    void set_text(const std::string &text);
    const std::string &get_text() const {
      return _text;
    }

    void set_fill_background(bool flag);
    void set_draw_outline(bool flag);
    void set_highlight_through_text(bool flag) {
      _highlight_through_text = flag;
    }

    void set_font(const FontSpec &font);
    const FontSpec &get_font() {
      return _font;
    }

    void set_text_alignment(TextAlignment align);

    void set_multi_line(bool flag);
    void set_allow_shrinking(bool flag);
    void set_allow_wrapping(bool flag);

    virtual void draw_contents(CairoCtx *cr);

    void auto_size();

  protected:
    FontSpec _font;
    cairo_font_extents_t _font_extents;
    std::string _text;
    std::string _shrinked_text;

    TextAlignment _align;

    TextLayout *_text_layout;
    bool _multi_line;
    bool _allow_shrinking;
    bool _allow_wrapping;
    bool _fill_background;
    bool _draw_outline;
    bool _highlight_through_text;

    base::Size get_text_size();

    void reset_shrinked_text();

    void draw_contents(CairoCtx *cr, const base::Rect &bounds);
  };

} // end of mdc namespace

#endif /* _MDC_TEXT_H_ */
