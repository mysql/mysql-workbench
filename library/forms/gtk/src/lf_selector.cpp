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

#include "../lf_mforms.h"
#include "../lf_selector.h"

namespace mforms {
  namespace gtk {

    // enum SelectorStyle
    //{
    //  SelectorSimple,       // The value list is always visible. The value is freely editable.
    //  SelectorDropDown,     // The value list is shown when clicking the arrow. The value is freely editable.
    //  SelectorDropDownList  // The value list is shown when clicking the arrow. The value can only be selected out of
    //  the
    //                        // values in the list.
    //};

    //==============================================================================
    //
    //==============================================================================
    class SelectorImpl::Impl : public sigc::trackable {
    public:
      virtual ~Impl(){};

      virtual Gtk::Widget *widget() = 0;
      virtual void clear() = 0;
      virtual int add_item(const std::string &item) = 0;
      virtual void add_items(const std::list<std::string> &items) = 0;
      virtual std::string get_item(const int index) const = 0;
      virtual std::string get_text() const = 0;
      virtual void set_index(const int index) = 0;
      virtual int get_index() const = 0;
      virtual int get_item_count() const = 0;
      virtual void set_value(const std::string &){}; // It is only defined for editable combobox
    };

    //==============================================================================
    //
    //==============================================================================
    class SelectorPopupImpl : public SelectorImpl::Impl {
      //  private:

    public:
      SelectorPopupImpl(mforms::Selector *self) : _self(self), do_not_call_callback(false) {
        //      _list.signal_changed().connect(sigc::mem_fun(self, &mforms::Selector::callback));
        _list.signal_changed().connect(sigc::mem_fun(*this, &mforms::gtk::SelectorPopupImpl::wrap_callback_call));
        _list.set_row_separator_func(sigc::mem_fun(*this, &SelectorPopupImpl::is_separator));
      }

      Gtk::Widget *widget() {
        return &_list;
      }

      virtual void clear() {
        do_not_call_callback = true;
        _items.clear();
        _list.remove_all();
        do_not_call_callback = false;
      }

      bool is_separator(const Glib::RefPtr<Gtk::TreeModel> &model, const Gtk::TreeModel::iterator &iter) {
        Gtk::TreeRow row = *iter;
        Glib::ustring value;
        row.get_value(0, value);
        return value == "-";
      }

      virtual int add_item(const std::string &item) {
        _list.append(item);
        _items.push_back(item);
        if (_items.size() == 1)
          _list.set_active(0);
        return _items.size();
      }

      virtual void add_items(const std::list<std::string> &items) {
        std::list<std::string>::const_iterator it = items.begin();
        const std::list<std::string>::const_iterator last = items.end();
        for (; it != last; ++it) {
          _list.append(*it);
          _items.push_back(*it);
        }
        if (_items.size() > 0)
          _list.set_active(0);
      }

      virtual std::string get_item(const int index) const {
        if (index < 0 || index >= (int)_items.size())
          return "";
        return _items[index];
      }

      virtual std::string get_text() const {
        return _list.get_active_text();
      }

      virtual void set_index(const int index) {
        _list.set_active(index);
      }

      virtual int get_index() const {
        return _list.get_active_row_number();
      }

      virtual int get_item_count() const {
        return _items.size();
      }

    private:
      Gtk::ComboBoxText _list;
      std::vector<std::string> _items; // to impl get_item with GTK. [The rest of the comment is censored]
      mforms::Selector *_self;
      bool do_not_call_callback;

      virtual void wrap_callback_call() {
        if (do_not_call_callback)
          return;
        else
          _self->callback();
      }
    };

    //==============================================================================
    //
    //==============================================================================
    class SelectorComboboxImpl : public SelectorImpl::Impl {
    public:
      SelectorComboboxImpl(mforms::Selector *self) : _list(true) {
        _list.signal_changed().connect(sigc::mem_fun(self, &mforms::Selector::callback));
        _list.get_entry()->signal_insert_at_cursor().connect(
          sigc::hide(sigc::mem_fun(self, &mforms::Selector::callback)));
      }

      Gtk::Widget *widget() {
        return &_list;
      }

      virtual void clear() {
        _items.clear();
        _list.remove_all();
      }

      virtual int add_item(const std::string &item) {
        _items.push_back(item);
        _list.append(item);
        return _items.size();
      }

      virtual void add_items(const std::list<std::string> &items) {
        std::list<std::string>::const_iterator it = items.begin();
        const std::list<std::string>::const_iterator last = items.end();
        for (; it != last; ++it) {
          _list.append(*it);
          _items.push_back(*it);
        }
      }

      virtual std::string get_item(const int index) const {
        if (index < 0 || index >= (int)_items.size())
          return "";
        return _items[index];
      }

      virtual std::string get_text() const {
        return _list.get_entry()->get_text();
      }

      virtual void set_index(const int index) {
        _list.set_active(index);
      }

      virtual int get_index() const {
        return _list.get_active_row_number();
      }

      virtual int get_item_count() const {
        return _items.size();
      }

      virtual void set_value(const std::string &value) {
        _list.get_entry()->set_text(value);
      }

    private:
      Gtk::ComboBoxText _list;
      std::vector<std::string> _items; // to impl get_item with GTK. [The rest of the comment is censored]
    };

    //------------------------------------------------------------------------------
    SelectorImpl::SelectorImpl(::mforms::Selector *self, ::mforms::SelectorStyle style) : ViewImpl(self), _pimpl(0) {
      _outerBox = Gtk::manage(new Gtk::Box());
      // TODO: implement selector styles.
      //_pimpl= Gtk::manage(new Gtk::ComboBoxText());
      //_pimpl->show();
      if (style == SelectorCombobox)
        _pimpl = new SelectorComboboxImpl(self);
      else if (style == SelectorPopup)
        _pimpl = new SelectorPopupImpl(self);

      _outerBox->pack_start(*_pimpl->widget(), true, true);
      _outerBox->show_all();
      _pimpl->widget()->set_halign(Gtk::ALIGN_CENTER);
      _pimpl->widget()->set_valign(Gtk::ALIGN_CENTER);
    }

    //------------------------------------------------------------------------------
    SelectorImpl::~SelectorImpl() {
      delete _pimpl;
    }

    //------------------------------------------------------------------------------
    bool SelectorImpl::create(::mforms::Selector *self, ::mforms::SelectorStyle style) {
      return new SelectorImpl(self, style) != 0;
    }

    //------------------------------------------------------------------------------
    void SelectorImpl::clear(::mforms::Selector *self) {
      SelectorImpl *sel = self->get_data<SelectorImpl>();

      sel->_pimpl->clear();
    }

    //------------------------------------------------------------------------------
    int SelectorImpl::add_item(::mforms::Selector *self, const std::string &item) {
      SelectorImpl *sel = self->get_data<SelectorImpl>();

      int ret = 0;
      if (sel) {
        sel->_pimpl->add_item(item);
        ret = sel->_pimpl->get_item_count();
        if (ret == 1)
          sel->_pimpl->set_index(0);
      }

      return ret;
    }

    //------------------------------------------------------------------------------
    void SelectorImpl::add_items(::mforms::Selector *self, const std::list<std::string> &items) {
      SelectorImpl *sel = self->get_data<SelectorImpl>();

      if (sel)
        sel->_pimpl->add_items(items);
    }

    //------------------------------------------------------------------------------
    std::string SelectorImpl::get_item(::mforms::Selector *self, int index) {
      SelectorImpl *sel = self->get_data<SelectorImpl>();
      if (sel) {
        std::string value = sel->_pimpl->get_item(index);
        return value;
      }
      return "";
    }

    //------------------------------------------------------------------------------
    std::string SelectorImpl::get_text(::mforms::Selector *self) {
      SelectorImpl *sel = self->get_data<SelectorImpl>();
      if (sel) {
        std::string value = sel->_pimpl->get_text();
        return value;
      }
      return "";
    }

    //------------------------------------------------------------------------------
    void SelectorImpl::set_index(::mforms::Selector *self, int index) {
      SelectorImpl *sel = self->get_data<SelectorImpl>();

      if (sel)
        sel->_pimpl->set_index(index);
    }

    //------------------------------------------------------------------------------
    int SelectorImpl::get_index(::mforms::Selector *self) {
      SelectorImpl *sel = self->get_data<SelectorImpl>();
      int ret = -1;

      if (sel)
        ret = sel->_pimpl->get_index();

      return ret;
    }

    //------------------------------------------------------------------------------
    int SelectorImpl::get_item_count(::mforms::Selector *self) {
      SelectorImpl *sel = self->get_data<SelectorImpl>();

      int ret = -1;
      if (sel)
        ret = (int)sel->_pimpl->get_item_count();

      return ret;
    }

    //------------------------------------------------------------------------------
    void SelectorImpl::set_value(::mforms::Selector *self, const std::string &value) {
      SelectorImpl *sel = self->get_data<SelectorImpl>();

      if (sel)
        sel->_pimpl->set_value(value);
    }

    Gtk::Widget *SelectorImpl::get_outer() const {
      return _outerBox;
    }
    Gtk::Widget *SelectorImpl::get_inner() const {
      return _pimpl->widget();
    }

    //------------------------------------------------------------------------------
    void SelectorImpl::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_selector_impl.create = &SelectorImpl::create;
      f->_selector_impl.clear = &SelectorImpl::clear;
      f->_selector_impl.add_item = &SelectorImpl::add_item;
      f->_selector_impl.add_items = &SelectorImpl::add_items;
      f->_selector_impl.get_item = &SelectorImpl::get_item;
      f->_selector_impl.set_index = &SelectorImpl::set_index;
      f->_selector_impl.get_index = &SelectorImpl::get_index;
      f->_selector_impl.get_text = &SelectorImpl::get_text;
      f->_selector_impl.get_item_count = &SelectorImpl::get_item_count;
      f->_selector_impl.set_value = &SelectorImpl::set_value;
    }
  }
}
