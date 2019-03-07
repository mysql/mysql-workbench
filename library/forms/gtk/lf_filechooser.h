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

#ifndef _LF_FILECHOOSER_H_
#define _LF_FILECHOOSER_H_

#include "mforms/mforms.h"

#include "lf_mforms.h"
#include "lf_view.h"
#include "base/file_utilities.h"

#define RESPONSE_OK 1
#define RESPONSE_CANCEL 0

namespace mforms {
  namespace gtk {

    class FileChooserImpl : public ViewImpl {
      Gtk::FileChooserDialog *_dlg;
      Gtk::Widget *get_outer() const {
        return _dlg;
      }

      Gtk::Table *_options_table;
      std::map<std::string, Gtk::ComboBoxText *> _combos;
      std::map<std::string, std::map<std::string, std::string> > _option_values;
      std::map<std::string, std::string> _ext_list;
      std::string _default_extension;

      static std::vector<std::string> split_string(const std::string &s, const std::string &sep) {
        std::vector<std::string> parts;
        std::string ss = s;

        std::string::size_type p;

        if (s.empty())
          return parts;

        p = ss.find(sep);
        while (!ss.empty() && p != std::string::npos) {
          parts.push_back(ss.substr(0, p));
          ss = ss.substr(p + sep.size());

          p = ss.find(sep);
        }
        parts.push_back(ss);

        return parts;
      }

      static bool create(::mforms::FileChooser *self, ::mforms::Form *owner, ::mforms::FileChooserType type,
                         const bool show_hidden) {
        return new FileChooserImpl(self, owner, type, show_hidden) != 0;
      }

      static void set_title(::mforms::FileChooser *self, const std::string &title) {
        FileChooserImpl *dlg = self->get_data<FileChooserImpl>();
        if (dlg)
          dlg->_dlg->set_title(title);
      }

      static bool show_modal(::mforms::FileChooser *self) {
        FileChooserImpl *dlg = self->get_data<FileChooserImpl>();
        bool res;
        int dialog_result = dlg->_dlg->run();
        res = dialog_result == RESPONSE_OK || dialog_result == GTK_RESPONSE_ACCEPT;
        dlg->_dlg->hide();
        return res;
      }

      static void set_directory(FileChooser *self, const std::string &path) {
        FileChooserImpl *dlg = self->get_data<FileChooserImpl>();
        if (dlg) {
          dlg->_dlg->set_current_folder(path);
        }
      }

      static std::string get_directory(FileChooser *self) {
        FileChooserImpl *dlg = self->get_data<FileChooserImpl>();
        if (dlg) {
          return dlg->_dlg->get_current_folder();
        }
        return "";
      }

      static std::string get_path(FileChooser *self) {
        FileChooserImpl *dlg = self->get_data<FileChooserImpl>();
        return dlg ? dlg->_dlg->get_filename() : "";
      }

      static void set_path(FileChooser *self, const std::string &path) {
        FileChooserImpl *dlg = self->get_data<FileChooserImpl>();
        dlg->_dlg->set_filename(path);

        std::string ext = base::extension(path);
        Gtk::ComboBoxText *combo = dlg->_combos["format"];
        if (combo) {
          std::vector<std::string> &extensions(self->_selector_options["format"]);
          if (!ext.empty()) {
            std::vector<std::string>::const_iterator it =
              std::find(extensions.begin(), extensions.end(), ext.substr(1));
            if (it != extensions.end())
              combo->set_active(it - extensions.begin());
          }
        }
      }

      static void add_selector_option(FileChooser *self, const std::string &name, const std::string &label,
                                      const std::vector<std::pair<std::string, std::string> > &values) {
        FileChooserImpl *dlg = self->get_data<FileChooserImpl>();
        int row;
        if (!dlg->_options_table) {
          dlg->_options_table = Gtk::manage(new Gtk::Table(1, 2));
          dlg->_options_table->set_col_spacings(4);
          dlg->_dlg->set_extra_widget(*dlg->_options_table);
          row = 0;
        } else {
          row = dlg->_options_table->property_n_rows().get_value();
          dlg->_options_table->property_n_rows().set_value(row + 1);
        }
        Gtk::ComboBoxText *combo = 0;
        if (!(combo = dlg->_combos[name])) {
          combo = dlg->_combos[name] = Gtk::manage(new Gtk::ComboBoxText());
          dlg->_options_table->attach(*Gtk::manage(new Gtk::Label(label)), 0, 1, row, row + 1, Gtk::FILL, Gtk::FILL);
          dlg->_options_table->attach(*dlg->_combos[name], 1, 2, row, row + 1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL);
          dlg->_options_table->show_all();
        }
        combo->remove_all();
        for (std::vector<std::pair<std::string, std::string> >::const_iterator iter = values.begin();
             iter != values.end(); ++iter) {
          combo->append(iter->first);
          dlg->_option_values[name][iter->first] = iter->second;
          dlg->_ext_list.insert(std::make_pair(iter->first, iter->second));
        }
        combo->set_active(0);
      }

      static std::string get_selector_option_value(FileChooser *self, const std::string &name) {
        FileChooserImpl *dlg = self->get_data<FileChooserImpl>();

        if (name != "format")
          return dlg->_option_values["format"][name];

        if (dlg->_combos[name]) {
          int i = dlg->_combos[name]->get_active_row_number();
          if (i >= 0)
            return self->_selector_options[name][i];
        }

        return "";
      }

      static void set_extensions(FileChooser *self, const std::string &extensions, const std::string &default_extension,
                                 bool allow_all_file_types = true) {
        FileChooserImpl *dlg = self->get_data<FileChooserImpl>();
        if (dlg) {
          // extensions format:
          // AAA Files (*.aaa)|*.aaa,BBB Files (*.bbb)
          std::vector<std::pair<std::string, std::string> > exts(self->split_extensions(extensions));

          for (std::vector<std::pair<std::string, std::string> >::const_iterator iter = exts.begin();
               iter != exts.end(); ++iter) {
            Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
            filter->add_pattern(iter->second);
            filter->set_name(iter->first);
            dlg->_dlg->add_filter(filter);
            if (iter->second.substr(2) == default_extension)
              dlg->_dlg->set_filter(filter);

            dlg->_ext_list.insert(std::make_pair(iter->first, iter->second));
            dlg->_ext_list[iter->first].erase(0, 2); // Remove the *.

            if (dlg->_default_extension.empty()) // First extension set default ext
            {
              dlg->_default_extension = iter->second;
              if (dlg->_default_extension.size() > 0)
                dlg->_default_extension.erase(0, 2);
            }
          }

          if (allow_all_file_types) {
            Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
            filter->add_pattern("*");
            filter->set_name("All Files");
            dlg->_dlg->add_filter(filter);
          }
        }
      }

      FileChooserImpl(::mforms::FileChooser *form, ::mforms::Form *owner, ::mforms::FileChooserType type,
                      bool show_hidden)
        : ViewImpl(form), _options_table(0) {
        Gtk::Button *ok_button = NULL;
        // TODO: enable showing hidden files/folders.
        // FileChooserDialog *dialog = 0;
        switch (type) {
          case ::mforms::OpenFile:
            _dlg = new Gtk::FileChooserDialog("Open File...", Gtk::FILE_CHOOSER_ACTION_OPEN);
            _dlg->add_button(Gtk::Stock::CANCEL, RESPONSE_CANCEL);
            _dlg->add_button(Gtk::Stock::OPEN, RESPONSE_OK);
            _dlg->set_default_response(RESPONSE_OK);
            break;
          case ::mforms::SaveFile:
            _dlg = new Gtk::FileChooserDialog("Save File...", Gtk::FILE_CHOOSER_ACTION_SAVE);
            _dlg->add_button(
              Gtk::Stock::CANCEL,
              GTK_RESPONSE_CANCEL); // GTK_RESPONSE_CANCEL - Changing this will alter the behavior of the dialog
            ok_button = _dlg->add_button(
              Gtk::Stock::SAVE,
              GTK_RESPONSE_ACCEPT); // GTK_RESPONSE_ACCEPT - Changing this will alter the behavior of the dialog
            _dlg->set_do_overwrite_confirmation(true);
            _dlg->set_default_response(GTK_RESPONSE_ACCEPT);

            //  The save dialog has a strange behavior regarding the overwrite of files. For the stock dialog
            //  to be displayed as expected we need to add the file extension if required. To do so, we need
            //  to do it before the dialog checks if the file exists. The signal sequence seems different
            //  depending on the widget that has the focus. So, to add the file extension for the text box
            //  we need the signal_activate from the ok button (taking advantage of the default action).
            //  To add the extension when clicking the ok button, we need the signal_pressed from the ok button.
            //  The signal_clicked is triggered too late in the sequence, making it unusable.
            ok_button->signal_activate().connect(sigc::bind(&FileChooserImpl::on_ok_button_clicked, this), false);
            ok_button->signal_pressed().connect(sigc::bind(&FileChooserImpl::on_ok_button_clicked, this), false);
            break;
          case ::mforms::OpenDirectory:
            _dlg = new Gtk::FileChooserDialog("Open Directory...", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
            _dlg->add_button(Gtk::Stock::CANCEL, RESPONSE_CANCEL);
            _dlg->add_button(Gtk::Stock::OPEN, RESPONSE_OK);
            _dlg->set_default_response(RESPONSE_OK);
            break;
        }
        if (owner) {
          FormImpl *fi = owner->get_data<FormImpl>();
          if (fi && fi->get_window())
            _dlg->set_transient_for(*fi->get_window());
        }
      }

      //  Add the file extension related to the selected file type in the dialog
      void on_ok_button_clicked() {
        FileChooser *chooser = dynamic_cast<FileChooser *>(owner);

        //  If the dialog has no file format options, there's nothing to do here...
        std::string format_ext;
        if (chooser->_selector_options.find("format") == chooser->_selector_options.end()) {
          // We need to check if there are maybe extensions specified if so we've pick up currently selected.
          GtkFileFilter *filter = gtk_file_chooser_get_filter(
            ((Gtk::FileChooser *)_dlg)
              ->gobj()); // Because of bug in gtkmm we can't use get_filter, we fallback to c api
          if (filter) {
            std::map<std::string, std::string>::iterator it = _ext_list.find(gtk_file_filter_get_name(filter));
            if (it != _ext_list.end())
              format_ext = it->second;
          } else
            format_ext = _default_extension;
        } else {
          std::string format_human = get_selector_option_value(chooser, "format");
          format_ext = get_selector_option_value(chooser, format_human);
        }

        if (!format_ext.empty()) {
          std::string path = _dlg->get_filename();
          std::string ext = base::extension(path);
          if (!ext.empty() && ext[0] == '.')
            ext = ext.substr(1);

          if (ext != format_ext) {
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            std::map<std::string, std::string>::iterator it;
            for (it = _ext_list.begin(); it != _ext_list.end(); ++it) {
              if (it->second == ext) // If we support extension provided by user - don't change anything
                return;
            }

            path.append(".").append(format_ext);
          }

          _dlg->set_current_name(base::basename(path.c_str()));
          _dlg->set_filename(path);
        }
      }

      virtual ~FileChooserImpl() {
        delete _dlg;
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_filechooser_impl.create = &FileChooserImpl::create;
        f->_filechooser_impl.set_title = &FileChooserImpl::set_title;
        f->_filechooser_impl.run_modal = &FileChooserImpl::show_modal;
        f->_filechooser_impl.set_extensions = &FileChooserImpl::set_extensions;
        f->_filechooser_impl.set_directory = &FileChooserImpl::set_directory;
        f->_filechooser_impl.get_directory = &FileChooserImpl::get_directory;
        f->_filechooser_impl.get_path = &FileChooserImpl::get_path;
        f->_filechooser_impl.set_path = &FileChooserImpl::set_path;
        f->_filechooser_impl.add_selector_option = &FileChooserImpl::add_selector_option;
        f->_filechooser_impl.get_selector_option_value = &FileChooserImpl::get_selector_option_value;
      }
    };
  };
};

#endif
