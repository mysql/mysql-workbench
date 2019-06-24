/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <libxml/tree.h>
#include "Scintilla.h"

#include "base/notifications.h"

#include "mforms/view.h"
#include "mforms/utilities.h"

/**
 * Provides a code editor with syntax highlighting for mforms.
 */

class TiXmlDocument;
class TiXmlElement;

namespace mforms {

  class CodeEditor;
  class Menu;
  class FindPanel;

  enum SyntaxHighlighterLanguage {
    LanguageNone,
    LanguageMySQL56,
    LanguageMySQL57,
    LanguageMySQL80,

    LanguageHtml, // includes embedded xml, javascript, php, vb, python
    LanguagePython,
    LanguageCpp, // Lexer for C++, C, Java, and JavaScript (which includes JSON).
    LanguageJS,
    LanguageJson,

    LanguageMySQL = LanguageMySQL80, // Always the latest (released) language.
  };

  /**
   * A number of flags used to specify additional markup for a line (shown in the gutter).
   */
  enum LineMarkup {
    LineMarkupNone = 0,               // No markup for the given line.
    LineMarkupStatement = 1 << 0,     // Marks a line as having a statement starting on it.
    LineMarkupError = 1 << 1,         // Marks a syntax error in that line.
    LineMarkupBreakpoint = 1 << 2,    // Line has a marker set for a break point.
    LineMarkupBreakpointHit = 1 << 3, // Line has a marker set for a break point which is currently hit.
    LineMarkupCurrent = 1 << 4,       // Current execution line.
    LineMarkupErrorContinue = 1 << 5, // Marker for a failed sql statement (execution).

    LineMarkupAll = 0xFF, // All markup, useful for remove_markup.
  };

  // A collection of markup, attached to the original line and going to be removed
  // or moved to a new line.
  typedef struct {
    int original_line;
    int new_line;
    LineMarkup markup;
  } LineMarkupChangeEntry;
  typedef std::vector<LineMarkupChangeEntry> LineMarkupChangeset;

#ifndef SWIG
  inline LineMarkup operator|(LineMarkup a, LineMarkup b) {
    return (LineMarkup)((int)a | (int)b);
  }
#endif

  // Indicators for a portion of text. Can span more than a single line or only part of a line.
  enum RangeIndicator {
    RangeIndicatorNone = 0,
    RangeIndicatorError = 1 << 0, // Red squiggles under a range of text.
  };

  enum CodeEditorFeature {
    FeatureNone = 0,
    FeatureWrapText = 1 << 0, // Enables word wrapping.
    FeatureGutter = 1 << 1,   // Show/Hide gutter.
    FeatureReadOnly = 1 << 2,
    FeatureShowSpecial = 1 << 3,       // Show white spaces and line ends with special chars.
    FeatureUsePopup = 1 << 4,          // Use built-in context menu.
    FeatureConvertEolOnPaste = 1 << 5, // Convert line endings to the current value in the editor
                                       // when pasting text.
    FeatureScrollOnResize = 1 << 6,    // Scroll caret into view if it would be hidden by a resize action.
    FeatureFolding = 1 << 7,           // Enable code folding.
    FeatureAutoIndent = 1 << 8,        // Auto indent the new line on pressing enter.

    FeatureAll = 0xFFFF,
  };

#ifndef SWIG
  inline CodeEditorFeature operator|(CodeEditorFeature a, CodeEditorFeature b) {
    return (CodeEditorFeature)((int)a | (int)b);
  }
#endif

  enum AutoCompletionEventType {
    AutoCompletionSelection,   // The user selected an entry in the auto completion list.
    AutoCompletionCancelled,   // Auto completion was cancelled.
    AutoCompletionCharDeleted, // A character was deleted while auto completion was active.
  };

  enum EndOfLineMode {
    EolCRLF = 0,
    EolCR = 1,
    EolLF = 2, // Default
  };

  enum FindFlags {
    FindDefault = 0,
    FindMatchCase = (1 << 0),
    FindWrapAround = (1 << 1),
    FindWholeWords = (1 << 2),
    FindRegex = (1 << 3)
  };

#ifndef SWIG
  inline FindFlags operator|(FindFlags a, FindFlags b) {
    return (FindFlags)((int)a | (int)b);
  }

  inline FindFlags& operator|=(FindFlags& a, FindFlags b) {
    a = (FindFlags)((int)a | (int)b);
    return a;
  }
#endif

#ifndef SWIG
  /**
   * Helper class to manage editor configuration files.
   */
  class MFORMS_EXPORT CodeEditorConfig {
  private:
    std::vector<std::string> _languages;
    SyntaxHighlighterLanguage _used_language;

    std::map<std::string, std::string> _keywords;
    std::map<std::string, std::string> _properties;
    std::map<std::string, std::string> _settings;
    std::map<int, std::map<std::string, std::string> > _styles;

    xmlDocPtr _xmlDocument;
    xmlNodePtr _xmlLanguageElement;

  protected:
    void parse_properties();
    void parse_settings();
    void parse_keywords();
    void parse_styles();

  public:
    CodeEditorConfig(SyntaxHighlighterLanguage language);
    ~CodeEditorConfig();

    std::vector<std::string> get_languages() {
      return _languages;
    };

    // TODO: add setters when customization is required.
    std::map<std::string, std::string> get_keywords() {
      return _keywords;
    };

    std::map<std::string, std::string> get_properties() {
      return _properties;
    };

    std::map<std::string, std::string> get_settings() {
      return _settings;
    };
    
    std::map<int, std::map<std::string, std::string> > get_styles() {
      return _styles;
    };
  };
#endif // !SWIG

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct CodeEditorImplPtrs {
    bool (*create)(CodeEditor* self, bool showInfo);
    sptr_t (*send_editor)(CodeEditor* self, unsigned int message, uptr_t wParam, sptr_t lParam);
    void (*set_status_text)(CodeEditor* self, const std::string& text);
  };

  struct MarginSizes {
    sptr_t margin1;
    sptr_t margin2;
    sptr_t margin3;
    sptr_t margin4;
  };
#endif
#endif

  class MFORMS_EXPORT CodeEditor : public View, public base::Observer {
  public:
    enum EditorMargin { LineNumberMargin, MarkersMargin, FolderMargin, TextMargin };

    CodeEditor(void* host = NULL, bool showInfo = true);
    ~CodeEditor();

    /** Set editor colors based on the current OS appearance. */
    void updateColors();

    /** Set custom color and size of editor margins. */
    void setWidth(EditorMargin margin, int size, const std::string& adjustText = "");
    void setColor(EditorMargin margin, base::Color color, bool foreground = false);
    void showMargin(EditorMargin, bool show = true);
    void setMarginText(const std::string& str);
    void setMarginText(const std::string& str, size_t line);
    void setScrollWidth(size_t width);
    int getLineHeight(int line);

    /** Replaces the text in the editor. */
    void set_text(const char* text);
    void set_value(const std::string& text);

    /** Replaces the text in the editor but preserves top line, caret position and selection.
     *  This might not always work, especially when replacing large text by small text.
     */
    void set_text_keeping_state(const char* text);

    /** Appends the given number of chars to end of the document. Allows to add nulls too.
     *  The length is (as always) a byte count.
     */
    void append_text(const char* text, size_t length);

    /** Replaces the selected text in the editor by the new text. If no text is selected then
     *  the new text is inserted at the caret position. */
    void replace_selected_text(const std::string& text);

    /** Returns a copy of the text which is currently in the editor. If selection_only is true only
     *  the current selection is returned. If there is no selection then the result is an empty
     *  string in that case.
     */
    const std::string get_text(bool selection_only);
    virtual std::string get_string_value() override {
      return get_text(false);
    }

    /** Returns the text in the given range (inclusive endpoints), regardless of the selection state.
     *  The range is automatically adjusted if it lies outside the available total text range.
     */
    const std::string get_text_in_range(size_t start, size_t end);

    /** Returns a direct pointer to the text in the editor control (no copying takes place) and its length
     *  in bytes. The text can contain embedded nulls and should therefore not be handled like
     *  a null terminated string (even though it is actually null terminated).
     *  The call takes care to make the text a continuous block of characters terminated with an additional null.
     *  The returned pointer is valid until the next change in the editor happens, so use it only
     *  for short term tasks (e.g. error parsing, direct search etc.).
     *  Don't change the text in any way or the editor might get out of sync.
     */
    std::pair<const char*, size_t> get_text_ptr();

    /** Selects the text at the given range. If length is 0 it will just move the caret.
     * NOTE: Scintilla uses bytes everywhere when a position or length is set or read. So be very
     *       careful when doing char maths (we use utf-8 with a variable code length per character).
     */
    void set_selection(size_t start, size_t length);

    /** Gets the current selection range. */
    void get_selection(size_t& start, size_t& length);

    /** Removes the current selection without moving the caret. */
    void clear_selection();

    /** Gets the byte range for the given line. Returns false if the line number is invalid */
    bool get_range_of_line(ssize_t line, ssize_t& start, ssize_t& end);

    /** Sets the language for the syntax highlighter. */
    void set_language(SyntaxHighlighterLanguage language);

    /** Adds the given markup to a line if not yet there. Does not touch other markup. */
    void show_markup(LineMarkup markup, size_t line);

    /** Removes the given markup from that line, without affecting other markup (except for LineMarkupAll).
     *  If markup is LineMarkupAll then all markers are removed for the given line.
     *  If line is < 0 then all marker are removed from all lines.
     *  However, it is not possible to remove a specific marker from all lines.
     */
    void remove_markup(LineMarkup markup, ssize_t line);

    /**
     * Determines if the given line contains the given markup.
     * Returns true if at least one of the given markup types was found.
     */
    bool has_markup(LineMarkup markup, size_t line);

    /** Adds the given indicator styling to a range of characters. */
    void show_indicator(RangeIndicator indicator, size_t start, size_t length);

    /** Returns the indicator styling at the given position (if any). */
    RangeIndicator indicator_at(size_t position);

    /** Removes the given indicator styling from the given range. */
    void remove_indicator(RangeIndicator indicator, size_t start, size_t length);

    /** Returns the number of lines currently in the editor. */
    size_t line_count();

    /** The total length of the text in the editor in bytes. */
    size_t text_length();

    /** Returns the character position of the given line. */
    size_t position_from_line(size_t line_number);

    /** Returns the line number from the given character position. */
    size_t line_from_position(size_t position);

    virtual void set_font(const std::string& fontDescription) override; // e.g. "Trebuchet MS bold 9"

    /** Enables or disables different features in the editor which have a yes/no behavior. */
    void set_features(CodeEditorFeature features, bool flag);

    /** Toggles the given feature(s) to their opposite state. */
    void toggle_features(CodeEditorFeature features);

    void set_read_only(bool flag);

    void reset_undo_stack();

    /** Resets the editor's dirty state or queries it. */
    void reset_dirty();
    bool is_dirty();

    /** Retrieves or sets the position of the caret in the editor, specified as byte position. */
    size_t get_caret_pos();
    void set_caret_pos(size_t position);

    /** Retrieves the line and column (both zero-based) for a given byte position. */
    void get_line_column_pos(size_t position, size_t& line, size_t& column);

    /** Standard edit functions used from menus. */
    bool can_undo();
    void undo();
    bool can_redo();
    void redo();
    bool can_cut();
    void cut();
    bool can_copy();
    void copy();
    bool can_paste();
    void paste();
    bool can_delete();
    void do_delete();
    void select_all();

    /** Sets the given text in the status field of the editor. Not all platforms support this, though. */
    void set_status_text(const std::string& text);

    // ----- Find and replace
    void show_find_panel(bool replace);
    void hide_find_panel();
    FindPanel* get_find_panel() {
      return _find_panel;
    };

    /** Used to set a callback which is called to set up the layout for the find panel and
     *  shows/hides it as requested. This allows to decouple platform specific needs for embedding
     *  the find panel in various parts of the application (even non-mforms).
     */
    void set_show_find_panel_callback(std::function<void(CodeEditor*, bool)> callback);

    /** Searches for the given text according to the parameters and selects the first occurrence.
     *  Returns true if something was found, false otherwise. */
    bool find_and_highlight_text(const std::string& search_text, FindFlags flags, bool scroll_to, bool backwards);

    /** Searches for the given text according to the parameters and replaces it by the text.
     *  Returns the number of replaced text occurrences. */
    size_t find_and_replace_text(const std::string& search_text, const std::string& new_text, FindFlags flags,
                                 bool do_all);

    /** Searches for the next placeholder char combination and selects it when found. The text is
     * also scrolled into view. */
    void jump_to_next_placeholder();

    //----- Auto completion -----

    /** Shows the auto completion list at the current cursor position.
     *
     *  @param chars_entered The number chars already entered for the word which is being completed.
     *  @param entries A list of strings to show in the auto completion window. The variant with the
     *                 int part additionally takes an image id for each entry. Images must be registered
     *                 first with auto_completion_register_images. Use -1 as id when no image is needed.
     *                 The list should be sorted to make matching working properly.
     */
    void auto_completion_show(size_t chars_entered, const std::vector<std::pair<int, std::string> >& entries);
    void auto_completion_show(size_t chars_entered, const std::vector<std::string>& entries);

    /** Can be used to cancel auto completion while it is in progress (i.e. during handling an
     * auto completion event. Has no effect otherwise. */
    void auto_completion_cancel();

    /** Used to set a few simple options for auto completion.
     *
     * @param ignore_case If true matching of characters to list members is not case sensitive.
     * @param choose_single If true and only one entry is in the auto completion list then this entry
     *                      is automatically used without showing the list.
     * @param auto_hide If true the list will automatically be hidden when there's no matching entry left.
     * @param drop_rest_of_word When an item is selected, any word characters following the caret are
     *                          first erased if this parameter is set true.
     * @param cancel_at_start If true the list is hidden when the caret moves to position it was when
     *                        auto completion started.
     */
    void auto_completion_options(bool ignore_case, bool choose_single, bool auto_hide, bool drop_rest_of_word,
                                 bool cancel_at_start);

    /** Configures the maximum size of the auto completion list. If not set then the largest entry
     *  in the list is used to determine the total width and the height is set to show 5 entries.
     *
     *  @param width The number of characters to show at most. Longer entries will be shorted using ellipses.
     *  @param  height The number of entries (rows) to show. If there are more entries a vertical scrollbar is shown.
     */
    void auto_completion_max_size(int width, int height);

    /** Used to load images (png or xpm type) into the editor and associate them with image ids, which can
     *  be used to display them together with the text in the auto completion list.
     *
     *  @param images A list of image file names that get loaded.
     */
    void auto_completion_register_images(const std::vector<std::pair<int, std::string> >& images);

    /** Returns true if auto completion is currently active (i.e. the list is visible). */
    bool auto_completion_active();

    /** The characters in the given string automatically cancel auto completion. By default no stops are defined. */
    void auto_completion_stops(const std::string& stops);

    /** If a fill-up character is typed while auto completion is active, the currently selected entry
     *  item in the list is added into the document, then the fill-up character is added. By default
     *  there is no fill-up character defined. */
    void auto_completion_fillups(const std::string& fillups);

    /** Show the editor's calltip window close to the given position or hide it. */
    void show_calltip(bool show, size_t position, const std::string& value);

    /** Sets the EOL mode used by the editor. If @convert is true all lines in the document are converted
     *  to use the new line ending.
     */
    void set_eol_mode(mforms::EndOfLineMode mode, bool convert = false);

    /** Sets a context menu to be attached to the editor, to be shown on right click.

     Note: Ownership of the context menu remains with the caller and it will not be freed
     when this object is deleted. */
    void set_context_menu(Menu* menu) {
      _context_menu = menu;
    };

    /** Returns the context menu object attached to the editor. */
    Menu* get_context_menu() {
      return _context_menu;
    }

    /** Returns the host which is controlling this editor instance (if any). */
    void* get_host() {
      return _host;
    }

    /** Direct access to the editor backend, for everything not covered here. */
    sptr_t send_editor(unsigned int message, uptr_t wParam, sptr_t lParam);

#ifndef SWIG
    /** Signal emitted when content is edited
     *  Parameters are:
     *    The (byte) position in the text where the change happened.
     *    The length of the change (in bytes).
     *    The number of lines which have been added (if positive) or removed (if negative).
     *    True if text was inserted.
     */
    boost::signals2::signal<void(Sci_Position, Sci_Position, Sci_Position, bool)>* signal_changed() {
      return &_change_event;
    }

    /** Signal emitted when the user clicks on the gutter.
     *  Parameters are:
     *    The margin (part of the gutter) the click occurred (0 = line numbers, 1 = markers, 2 = folding).
     *    The line in which this happened.
     *    The modifier keys that were pressed.
     */
    boost::signals2::signal<void(size_t, size_t, mforms::ModifierKey)>* signal_gutter_clicked() {
      return &_gutter_clicked_event;
    }

    /** Event sent when auto completion notifications from Scintilla come in.
     *  Parameters are:
     *    The type of auto completion that happened. Only used with AutoCompletionSelection.
     *    The start position of the word being completed. Only used with AutoCompletionSelection.
     *    The text of the selection.
     */
    boost::signals2::signal<void(AutoCompletionEventType, size_t, const std::string&)>* signal_auto_completion() {
      return &_auto_completion_event;
    };

    /** Signal emitted when the user keeps the mouse in one position for the dwell period or when
     *  when new events occur (text insertion, mouse move etc.) after dwelling started.
     *  Parameters are:
     *    Flag that tells if we are start dwelling or stop it.
     *    The position closest to the mouse pointer.
     *    x and y client coordinates where the mouse lingered.
     */
    boost::signals2::signal<void(bool, size_t, int, int)>* signal_dwell() {
      return &_dwell_event;
    }

    /** Signal emitted when the user typed an ordinary text character (as opposed to a command character).
     *  It can be used e.g. to trigger auto completion.
     *  Parameter is:
     *    The character code.
     */
    boost::signals2::signal<void(int)>* signal_char_added() {
      return &_char_added_event;
    }

    /** Signal emitted when the Scintilla backend removes a set marker (e.g. on editing, pasting, manual marker
     * setting).
     *  Parameters are:
     *    A vector of line + markup pairs.
     *    A flag telling if those markers where deleted or only updated (moved).
     */
    boost::signals2::signal<void(const LineMarkupChangeset& changeset, bool deleted)>* signal_marker_changed() {
      return &_marker_changed_event;
    }

    boost::signals2::signal<bool(mforms::KeyCode code, mforms::ModifierKey modifier, const std::string& text)>*
    key_event_signal() {
      return &_key_event_signal;
    };

    /** Signal emitted when the control loses input focus.
     */
    boost::signals2::signal<void()>* signal_lost_focus() {
      return &_signal_lost_focus;
    }

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    /** Called by the platform code forwarding us all scintilla notifications, so we can act on them. */
    void on_notify(SCNotification* notification);

    /** Called by the platform code forwarding us all scintilla commands, so we can act on them. */
    void on_command(int command);
    void lost_focus();
    bool key_event(mforms::KeyCode code, mforms::ModifierKey modifier, const std::string& text);

    virtual void resize() override;

#endif
#endif

  private:
    CodeEditorImplPtrs* _code_editor_impl;
    Menu* _context_menu;
    FindPanel* _find_panel;

    std::map<int, std::map<std::string, std::string>> _currentStyles; // Loaded styles for the currently configured langugage.

    void* _host;
    bool _scroll_on_resize;
    bool _auto_indent;

    MarginSizes _marginSize;

    void setupMarker(int marker, const std::string& name);
    void handleMarkerDeletion(size_t position, size_t length);
    void handleMarkerMove(Sci_Position position, Sci_Position linesAdded);
    char32_t getCharAt(size_t position);
    void updateBraceHighlighting();
    bool ensureImage(std::string const& name);

    void loadConfiguration(SyntaxHighlighterLanguage language);
    virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) override;

    boost::signals2::signal<void(Sci_Position, Sci_Position, Sci_Position, bool)> _change_event;
    boost::signals2::signal<void(size_t, size_t, mforms::ModifierKey)> _gutter_clicked_event;
    boost::signals2::signal<void(AutoCompletionEventType, size_t, const std::string&)> _auto_completion_event;
    boost::signals2::signal<void(bool, size_t, int, int)> _dwell_event;
    boost::signals2::signal<void(int)> _char_added_event;
    boost::signals2::signal<void()> _signal_lost_focus;
    boost::signals2::signal<void(const LineMarkupChangeset& changeset, bool deleted)> _marker_changed_event;
    boost::signals2::signal<bool(mforms::KeyCode code, mforms::ModifierKey modifier, const std::string& text)>
      _key_event_signal;

    std::function<void(CodeEditor*, bool)> _show_find_panel;
  };
};
