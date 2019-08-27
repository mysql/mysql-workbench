/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grt/grt_manager.h"
#include "binary_data_editor.h"
#include "geom_draw_box.h"
#include "grt/spatial_handler.h"
#include "base/log.h"
#include "base/string_utilities.h"
#include <glib/gstdio.h>
#ifdef _MSC_VER
#include <io.h>
#endif

DEFAULT_LOG_DOMAIN("BlobViewer");

#include "mforms/scrollpanel.h"
#include "mforms/imagebox.h"
#include "mforms/textbox.h"
#include "mforms/selector.h"
#include "mforms/treeview.h"
#include "mforms/code_editor.h"
#include "mforms/find_panel.h"
#include "mforms/filechooser.h"
#include "mforms/label.h"

BinaryDataViewer::BinaryDataViewer(BinaryDataEditor *owner) : mforms::Box(false), _owner(owner) {
}

//--------------------------------------------------------------------------------

class ImageDataViewer : public BinaryDataViewer {
public:
  ImageDataViewer(BinaryDataEditor *owner, bool read_only)
    : BinaryDataViewer(owner), _scroll(mforms::ScrollPanelNoFlags) {
    _image.set_scale_contents(false);
    add(&_scroll, true, true);
    _scroll.add(&_image);
  }

  virtual void data_changed() {
    _image.set_image_data(_owner->data(), _owner->length());
  }

  static bool can_display(const char *data, size_t length) {
    if (length > 4) {
      if (data[0] == (char)0x89 && strncmp(data + 1, "PNG", 3) == 0)
        return true;
      if (data[0] == (char)0xff && data[1] == (char)0xd8) // jpeg
        return true;
      if (strncmp(data, "BM", 2) == 0) // bmp
        return true;
      if (strncmp(data, "GIF", 3) == 0)
        return true;
      if ((strncmp(data, "II", 2) == 0 || strncmp(data, "MM", 2) == 0) && data[2] == 42) // tiff
        return true;
    }
    return false;
  }

private:
  mforms::ScrollPanel _scroll;
  mforms::ImageBox _image;
};

//--------------------------------------------------------------------------------

class HexDataViewer : public BinaryDataViewer {
public:
  HexDataViewer(BinaryDataEditor *owner, bool read_only)
    : BinaryDataViewer(owner),
      _tree(mforms::TreeShowColumnLines | mforms::TreeShowRowLines | mforms::TreeFlatList),
      _box(true) {
    _offset = 0;
    _block_size = 8 * 1024;

    add(&_tree, true, true);
    add(&_box, false, true);

    _box.set_spacing(8);
    _box.add(&_first, false, true);
    _box.add(&_back, false, true);
    _box.add(&_next, false, true);
    _box.add(&_last, false, true);
    _box.add(&_label, true, true);

    _label.set_text("Viewing Range 0 to 16KB");
    _first.set_text("<< First");
    _back.set_text("< Previous");
    _next.set_text("Next >");
    _last.set_text("Last >>");
    scoped_connect(_first.signal_clicked(), std::bind(&HexDataViewer::go, this, -2));
    scoped_connect(_back.signal_clicked(), std::bind(&HexDataViewer::go, this, -1));
    scoped_connect(_next.signal_clicked(), std::bind(&HexDataViewer::go, this, 1));
    scoped_connect(_last.signal_clicked(), std::bind(&HexDataViewer::go, this, 2));

    _tree.add_column(mforms::StringColumnType, "Offset", 100, true);

    for (int i = 0; i < 16; i++)
      _tree.add_column(mforms::StringColumnType, base::strfmt("%X", i), 25, !read_only);
    _tree.end_columns();

    _tree.set_cell_edit_handler(std::bind(&HexDataViewer::set_cell_value, this, std::placeholders::_1,
                                          std::placeholders::_2, std::placeholders::_3));
  }

  virtual void data_changed() {
    if (_offset >= _owner->length())
      _offset = (_owner->length() / _block_size) * _block_size;

    refresh();
  }

  void go(int step) {
    switch (step) {
      case -2:
        _offset = 0;
        break;
      case -1:
        if (_block_size > _offset)
          _offset = 0;
        else
          _offset -= _block_size;
        break;
      case 1:
        _offset += _block_size;
        if (_offset >= _owner->length())
          _offset = (_owner->length() / _block_size) * _block_size;
        break;
      case 2:
        _offset = (_owner->length() / _block_size) * _block_size;
        break;
    }
    refresh();
  }

  void refresh() {
    suspend_layout();

    unsigned char *ptr = (unsigned char *)_owner->data() + _offset;
    _tree.clear();
    size_t offs;
    size_t length = std::min<size_t>(_offset + _block_size, _owner->length());
    for (offs = _offset; offs < length; offs += 16) {
      mforms::TreeNodeRef row = _tree.add_node();
      row->set_string(0, base::strfmt("0x%08x", (unsigned int)offs));

      for (size_t i = offs, min = std::min<size_t>(offs + 16, length); i < min; ++i) {
        row->set_string((int)(i + 1 - offs), base::strfmt("%02x", *ptr));
        ptr++;
      }
    }
    resume_layout();
    _label.set_text(base::strfmt("Viewing Range %i to %i", (int)_offset, (int)(_offset + _block_size)));

    if (_offset == 0) {
      _back.set_enabled(false);
      _first.set_enabled(false);
    } else {
      _back.set_enabled(true);
      _first.set_enabled(true);
    }
    if (_offset + _block_size < _owner->length() - 1) {
      _next.set_enabled(true);
      _last.set_enabled(true);
    } else {
      _next.set_enabled(false);
      _last.set_enabled(false);
    }
  }

private:
  mforms::TreeView _tree;
  mforms::Box _box;
  mforms::Button _first;
  mforms::Button _back;
  mforms::Label _label;
  mforms::Button _next;
  mforms::Button _last;
  size_t _offset;
  size_t _block_size;

  void set_cell_value(mforms::TreeNodeRef node, int column, const std::string &value) {
    size_t offset = _offset + _tree.row_for_node(node) * 16 + (column - 1);

    if (offset < _owner->length()) {
      int i;
      if (sscanf(value.c_str(), "%x", &i) != 1)
        return;
      if (i < 0 || i > 255)
        return;
      node->set_string(column, base::strfmt("%02x", i));

      *(unsigned char *)(_owner->data() + offset) = i;
      _owner->notify_edit();
    }
  }
};

//--------------------------------------------------------------------------------

class TextDataViewer : public BinaryDataViewer {
public:
  TextDataViewer(BinaryDataEditor *owner, const std::string &encoding, bool read_only)
    : BinaryDataViewer(owner), _text(), _encoding(encoding) {
    if (_encoding.empty())
      _encoding = "UTF-8";

    add(&_message, false, true);
    add_end(&_text, true, true);

    _text.set_language(mforms::LanguageNone);
    _text.set_features(mforms::FeatureWrapText, true);
    _text.set_features(mforms::FeatureReadOnly, read_only);

    scoped_connect(_text.signal_changed(), std::bind(&TextDataViewer::edited, this));

    _text.set_show_find_panel_callback(std::bind(&TextDataViewer::embed_find_panel, this, std::placeholders::_2));
  }

  virtual void data_changed() {
    GError *error = 0;
    gchar *converted = NULL;
    gsize bread, bwritten;
    if (!_owner->data() ||
        !(converted = g_convert(_owner->data(), (gssize)_owner->length(), "UTF-8", _encoding.c_str(), &bread, &bwritten,
                                &error)) ||
        _owner->length() != bread) {
      std::string message = "Data could not be converted to UTF-8 text";
      if (error) {
        message.append(": ").append(error->message);
        g_error_free(error);
      }
      g_free(converted);
      if (_owner->length()) {
        _message.set_text(message);
        _text.set_features(mforms::FeatureReadOnly, true);
      } else {
        _text.set_features(mforms::FeatureReadOnly, false);
      }
      _text.set_value("");
    } else {
      _message.set_text("");
      _text.set_features(mforms::FeatureReadOnly, false);
      _text.set_value(std::string(converted, bwritten));
      if (_owner == NULL || _owner->read_only())
        _text.set_features(mforms::FeatureReadOnly, true);
    }

    if (converted != NULL)
      g_free(converted);
  }

private:
  mforms::CodeEditor _text;
  mforms::Label _message;
  std::string _encoding;

  void edited() {
    std::string data = _text.get_string_value();
    gchar *converted;
    gsize bread, bwritten;
    GError *error = 0;

    if (_encoding != "utf8" && _encoding != "UTF8" && _encoding != "utf-8" && _encoding != "UTF-8") {
      converted = g_convert(data.data(), (gssize)data.length(), _encoding.c_str(), "UTF-8", &bread, &bwritten, &error);
      if (converted == NULL || data.length() != bread) {
        std::string message = base::strfmt("Data could not be converted back to %s", _encoding.c_str());
        if (error) {
          message.append(": ").append(error->message);
          g_error_free(error);
        }
        _message.set_text(message);
        if (converted != NULL)
          g_free(converted);
        return;
      }

      _owner->assign_data(converted, bwritten);
      g_free(converted);
      _message.set_text("");
    } else {
      _owner->assign_data(data.data(), data.length());
      _message.set_text("");
    }
  }

  void embed_find_panel(bool show) {
    mforms::View *panel = _text.get_find_panel();
    if (show) {
      if (!panel->get_parent())
        add(panel, false, true);
    } else {
      remove(panel);
      _text.focus();
    }
  }
};

//--------------------------------------------------------------------------------

class JsonDataViewer : public BinaryDataViewer {
public:
  JsonDataViewer(BinaryDataEditor *owner, rapidjson::Value &value, const std::string &encoding)
    : BinaryDataViewer(owner), _encoding(encoding), _currentDelayTimer(nullptr){
    set_spacing(8);
    _jsonView.setJson(value);
    add(&_jsonView, true, true);
    scoped_connect(_jsonView.editorDataChanged(), std::bind(&JsonDataViewer::edited, this, std::placeholders::_1));

    _jsonView.setTextProcessingStopHandler([this]() {
      if (_currentDelayTimer != nullptr) {
        bec::GRTManager::get()->cancel_timer(_currentDelayTimer);
        _currentDelayTimer = nullptr;
      }
    });
    _jsonView.setTextProcessingStartHandler([this](std::function<bool()> callback) {
      _currentDelayTimer = bec::GRTManager::get()->run_every([=]() -> bool { return callback(); }, 0.25);
    });
  }

  virtual void data_changed() {
    if (!_owner->data()) {
      _jsonView.clear();
      return;
    }
    GError *error = NULL;
    gsize bread = 0, bwritten = 0;
    char *converted = g_convert(_owner->data(), static_cast<gssize>(_owner->length()), "UTF-8", _encoding.c_str(),
                                &bread, &bwritten, &error);
    if (!converted || _owner->length() != bread) {
      _jsonView.clear();
      return;
    }
    std::string dataToTest = converted;
    size_t pos = dataToTest.find_first_not_of(SPACES);
    if (pos != std::string::npos && dataToTest.at(pos) != '{' && dataToTest.at(pos) != '[') {
      _jsonView.clear();
      return;
    }

    rapidjson::Value value;
    rapidjson::Document d;
    d.Parse(converted);
    if (!d.HasParseError()) {
      value.CopyFrom(d, d.GetAllocator());
      _jsonView.setJson(value);
    } else {
      _jsonView.setText(converted);
    }
  }

  virtual ~JsonDataViewer() {
    if (_currentDelayTimer) {
      bec::GRTManager::get()->cancel_timer(_currentDelayTimer);
      _currentDelayTimer = nullptr;
    }
  }

private:
  void edited(const std::string &text) {
    _owner->assign_data(text.data(), text.length());
  }

  mforms::JsonTabView _jsonView;
  std::string _encoding;
  bec::GRTManager::Timer *_currentDelayTimer;
};

//--------------------------------------------------------------------------------

class GeomDataViewer : public BinaryDataViewer {
public:
  GeomDataViewer(BinaryDataEditor *owner, bool read_only) : BinaryDataViewer(owner) {
    set_spacing(8);
    add(&_drawbox, true, true);
  }

  virtual void data_changed() {
    _drawbox.set_data(std::string(_owner->data(), _owner->length()));
  }

private:
  GeomDrawBox _drawbox;
};

//--------------------------------------------------------------------------------

class GeomTextDataViewer : public BinaryDataViewer {
public:
  GeomTextDataViewer(BinaryDataEditor *owner, bool read_only)
    : BinaryDataViewer(owner), _text(mforms::VerticalScrollBar) {
    set_spacing(8);
    add(&_selector, false, true);
    add(&_text, true, true);
    add_end(&_srid, false, false);
    _text.set_read_only(read_only && false);
    // TODO: data editing (need to figure out a way to send WKT data to the server when saving)

    _selector.add_item("View as WKT");
    _selector.add_item("View as GeoJSON");
    _selector.add_item("View as GML");
    _selector.add_item("View as KML");

    _selector.signal_changed()->connect(std::bind(&GeomTextDataViewer::data_changed, this));
  }

  virtual void data_changed() {
    std::string text;
    spatial::Importer importer;
    importer.import_from_mysql(std::string(_owner->data(), _owner->length()));
    switch (_selector.get_selected_index()) {
      case 0:
        text = importer.as_wkt();
        break;
      case 1:
        text = importer.as_json();
        break;
      case 2:
        text = importer.as_gml();
        break;
      case 3:
        text = importer.as_kml();
        break;
    }
    _text.set_value(text);
    _srid.set_text("SRID: " + std::to_string(importer.getSrid()));
  }

private:
  mforms::TextBox _text;
  mforms::Selector _selector;
  mforms::Label _srid;
  std::string _encoding;
};

//--------------------------------------------------------------------------------

BinaryDataEditor::BinaryDataEditor(const char *data, size_t length, bool read_only)
  : mforms::Form(0), _box(false), _hbox(true), _read_only(read_only) {
  set_name("BLOB Editor");
  setInternalName("blob_editor");
  _data = 0;
  _length = 0;

  grt::IntegerRef tab = grt::IntegerRef::cast_from(bec::GRTManager::get()->get_app_option("BlobViewer:DefaultTab"));

  setup();
  assign_data(data, length);

  add_viewer(new HexDataViewer(this, read_only), "Binary");
  add_viewer(new TextDataViewer(this, "LATIN1", read_only), "Text");
  if (ImageDataViewer::can_display(data, length))
    add_viewer(new ImageDataViewer(this, read_only), "Image");

  int activeTab = 0;
  if (tab.is_valid())
    activeTab = (int)*tab;
  if (tab.is_valid() && *tab >= _tab_view.page_count()) {
    grt::DictRef dict(grt::DictRef::cast_from(bec::GRTManager::get()->get_app_option("")));
    if (dict.is_valid())
      dict.gset("BlobViewer:DefaultTab", 0);
    activeTab = 0;
  }
  _tab_view.set_active_tab(activeTab);

  tab_changed();
}

BinaryDataEditor::BinaryDataEditor(const char *data, size_t length, const std::string &text_encoding,
                                   const std::string &datatype, bool read_only)
  : mforms::Form(mforms::Form::main_form()), _type(datatype), _box(false), _hbox(true), _read_only(read_only) {
  set_name("BLOB Editor");
  setInternalName("blob_editor");
  _data = 0;
  _length = 0;
  _updating = false;

  grt::IntegerRef tab = grt::IntegerRef::cast_from(bec::GRTManager::get()->get_app_option("BlobViewer:DefaultTab"));

  setup();
  add_viewer(new HexDataViewer(this, read_only), "Binary");
  if (datatype == "GEOMETRY") {
    add_viewer(new GeomTextDataViewer(this, read_only), "Text");
    add_viewer(new GeomDataViewer(this, read_only), "Image");
  } else
    add_viewer(new TextDataViewer(this, text_encoding, read_only), "Text");
  if (ImageDataViewer::can_display(data, length))
    add_viewer(new ImageDataViewer(this, read_only), "Image");

  assign_data(data, length);
  add_json_viewer(read_only, text_encoding, "JSON");

  int activeTab = 0;
  if (tab.is_valid())
    activeTab = (int)*tab;
  if (tab.is_valid() && *tab >= _tab_view.page_count()) {
    grt::DictRef dict(grt::DictRef::cast_from(bec::GRTManager::get()->get_app_option("")));
    if (dict.is_valid())
      dict.gset("BlobViewer:DefaultTab", 0);
    activeTab = 0;
  }

  _tab_view.set_active_tab(activeTab);
  tab_changed();
}

BinaryDataEditor::~BinaryDataEditor() {
  g_free(_data);
}

void BinaryDataEditor::setup() {
  set_title("Edit Data");
  set_content(&_box);
  _box.set_padding(12);
  _box.set_spacing(12);

  _box.add(&_tab_view, true, true);
  _box.add(&_length_text, false, true);
  _box.add(&_hbox, false, true);

  _hbox.add(&_export, false, true);
  if (!_read_only)
    _hbox.add(&_import, false, true);

  if (!_read_only)
    _hbox.add_end(&_save, false, true);
  _hbox.add_end(&_close, false, true);
  _hbox.set_spacing(12);

  _save.set_text("Apply");
  _close.set_text("Close");
  _export.set_text("Save...");
  _import.set_text("Load...");

  scoped_connect(_tab_view.signal_tab_changed(), std::bind(&BinaryDataEditor::tab_changed, this));

  scoped_connect(_save.signal_clicked(), std::bind(&BinaryDataEditor::save, this));
  scoped_connect(_close.signal_clicked(), std::bind(&BinaryDataEditor::close, this));
  scoped_connect(_import.signal_clicked(), std::bind(&BinaryDataEditor::import_value, this));
  scoped_connect(_export.signal_clicked(), std::bind(&BinaryDataEditor::export_value, this));

  set_size(800, 500); // Golden ratio.
  center();
}

void BinaryDataEditor::notify_edit() {
  _length_text.set_text(base::strfmt("Data Length: %i bytes", (int)_length));
}

void BinaryDataEditor::assign_data(const char *data, size_t length, bool steal_pointer) {
  if (_updating)
    return;

  if (data != _data) {
    g_free(_data);
    if (steal_pointer)
      _data = (char *)data;
    else
      _data = (char *)g_memdup(data, (guint)length);

    for (size_t i = 0; i < _viewers.size(); i++)
      _pendingUpdates.insert(_viewers[i]);
  }
  _length = length;

  _length_text.set_text(base::strfmt("Data Length: %i bytes", (int)_length));
}

void BinaryDataEditor::tab_changed() {
  int i = _tab_view.get_active_tab();
  if (i < 0)
    i = 0;

  grt::DictRef dict(grt::DictRef::cast_from(bec::GRTManager::get()->get_app_option("")));
  if (dict.is_valid())
    dict.gset("BlobViewer:DefaultTab", i);
  if (i >= _tab_view.page_count()) {
    grt::DictRef dict(grt::DictRef::cast_from(bec::GRTManager::get()->get_app_option("")));
    if (dict.is_valid())
      dict.gset("BlobViewer:DefaultTab", 0);
    i = 0;
  }
  try {
    _updating = true;
    if (_pendingUpdates.count(_viewers[i]) > 0 && _data != NULL)
      _viewers[i]->data_changed();
    _pendingUpdates.erase(_viewers[i]);
    _updating = false;
  } catch (std::exception &exc) {
    logError("Error displaying binary data: %s\n", exc.what());
  }
}

void BinaryDataEditor::add_viewer(BinaryDataViewer *viewer, const std::string &title) {
  _viewers.push_back(viewer);
  _pendingUpdates.insert(viewer);

  _tab_view.add_page(mforms::manage(viewer), title);
}

void BinaryDataEditor::add_json_viewer(bool read_only, const std::string &text_encoding, const std::string &title) {
  if (!data())
    return;
  GError *error = NULL;
  gsize bread = 0, bwritten = 0;
  char *converted =
    g_convert(data(), static_cast<gssize>(length()), "UTF-8", text_encoding.c_str(), &bread, &bwritten, &error);
  if (!converted || length() != bread) {
    // convert problem
    return;
  }
  std::string dataToTest = converted;
  size_t pos = dataToTest.find_first_not_of(SPACES);
  if (pos != std::string::npos && dataToTest.at(pos) != '{' && dataToTest.at(pos) != '[')
    return;

  rapidjson::Value value;
  rapidjson::Document d;
  d.Parse(converted);
  if (!d.HasParseError()) {
    value.CopyFrom(d, d.GetAllocator());
    add_viewer(new JsonDataViewer(this, value, text_encoding), title.c_str());
    _type = "JSON";
  }
}

void BinaryDataEditor::save() {
  signal_saved();
  close();
}

void BinaryDataEditor::import_value() {
  mforms::FileChooser chooser(mforms::OpenFile);

  chooser.set_title("Import Field Data");
  if (chooser.run_modal()) {
    std::string path = chooser.get_path();
    GError *error = 0;
    char *data;
    gsize length;

    if (!g_file_get_contents(path.c_str(), &data, &length, &error)) {
      mforms::Utilities::show_error(base::strfmt("Could not import data from %s", path.c_str()), error->message, "OK");
      g_error_free(error);
    } else {
      assign_data(data, length, true);
      tab_changed();
    }
  }
}

void BinaryDataEditor::export_value() {
  mforms::FileChooser chooser(mforms::SaveFile);
  chooser.set_title("Export Field Data");
  chooser.set_extensions("Text files (*.txt)|*.txt|All Files (*.*)|*.*", "txt");
  if (chooser.run_modal()) {
    std::string path = chooser.get_path();
    GError *error = 0;

    if (!g_file_set_contents(path.c_str(), _data, (gssize)_length, &error)) {
      mforms::Utilities::show_error(base::strfmt("Could not export data to %s", path.c_str()), error->message, "OK");
      g_error_free(error);
    }
  }
}
