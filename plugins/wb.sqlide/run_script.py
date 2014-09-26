# Copyright (c) 2014 Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the
# License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301  USA

import os

import re
import grt
import mforms

from threading import Thread
from Queue import Queue, Empty

from workbench.log import log_info, log_error
from workbench.client_utils import MySQLScriptImporter


class RunPanel(mforms.Table):
    def __init__(self, editor, log_callback):
        mforms.Table.__init__(self)
        self.set_managed()
        self.set_release_on_add()

        self.set_row_count(2)
        self.set_column_count(1)

        self.set_padding(-1)

        self.label = mforms.newLabel("Running script...")
        self.add(self.label, 0, 1, 0, 1, mforms.HFillFlag)
        
        self.progress = mforms.newProgressBar()
        self.add(self.progress, 0, 1, 1, 2, mforms.HFillFlag)
        
        self.progress.set_size(400, -1)
        
        self.log_callback = log_callback
        self.editor = editor
        
        self.importer = MySQLScriptImporter(editor.connection)
        self.importer.report_progress = self.report_progress
        self.importer.report_output = self.report_output
        
        self._worker_queue = Queue()
        self._worker = None
        self._progress_status = None
        self._progress_value = 0

        self._update_timer = None

    def __del__(self):
        if self._update_timer:
            mforms.Utilities.cancel_timeout(self._update_timer)

    @property
    def is_busy(self):
        return self._worker != None


    def report_progress(self, status, current, total):
        self._progress_status = status
        if status:
            self._worker_queue.put(status)
        if total > 0:
            self._progress_value = float(current) / total


    def report_output(self, message):
        log_info("%s\n" % message)
        self._worker_queue.put(message)


    def update_ui(self):
        try:
            while True:
                data = self._worker_queue.get_nowait()
                if data is None:
                    self._worker.join()
                    self._worker = None
                    self._update_timer = None
                    self.progress.show(False)
                    self.log_callback(None)
                    return False

                if isinstance(data, Exception):
                    self.log_callback(str(data)+"\n")
                    if isinstance(data, grt.DBError) and data.args[1] == 1044:
                        mforms.Utilities.show_error("Run SQL Script",
                                                    "The current MySQL account does not have enough privileges to execute the script.", "OK", "", "")
                    elif isinstance(data, grt.DBLoginError):
                        username = self.editor.connection.parameterValues["userName"]
                        host = self.editor.connection.hostIdentifier
                        mforms.Utilities.forget_password(host, username)

                        mforms.Utilities.show_error("Run SQL Script",
                                                    "Error executing SQL script.\n"+str(data), "OK", "", "")
                    else:
                        mforms.Utilities.show_error("Run SQL Script",
                                                    "Error executing SQL script.\n"+str(data), "OK", "", "")
                else:
                    self.log_callback(data+"\n")

                if self._progress_status is not None:
                    self.label.set_text(self._progress_status)
                self.progress.set_value(self._progress_value)
        except Empty:
            pass

        if self._progress_status is not None:
            self.label.set_text(self._progress_status)
        self.progress.set_value(self._progress_value)

        return True
            
            
    def work(self, file, schema, charset):
        try:
            log_info("Executing %s...\n" % file)
            self._progress_status = "Executing %s..." % file
            self._progress_value = 0
            self.importer.import_script(file, default_schema = schema, default_charset = charset)

            log_info("Run script finished\n")
        except grt.DBLoginError, e:
            log_error("MySQL login error running script: %s\n" % e)
            self._worker_queue.put(e)
        except grt.DBError, e:
            log_error("MySQL error running script: %s\n" % e)
            self._worker_queue.put(e)
        except Exception, e:
            import traceback
            log_error("Unexpected exception running script: %s\n%s\n" % (e, traceback.format_exc()))
            self._worker_queue.put(e)
        self._worker_queue.put(None)


    def start(self, what, default_db, default_charset):
        parameterValues = self.editor.connection.parameterValues
        pwd = parameterValues["password"]
        if not pwd:
            username = parameterValues["userName"]
            host = self.editor.connection.hostIdentifier

            ok, pwd = mforms.Utilities.find_cached_password(host, username)
            if not ok:
                accepted, pwd = mforms.Utilities.find_or_ask_for_password("Run SQL Script", host, username, False)
                if not accepted:
                    return
        self.importer.set_password(pwd)

        self._worker = Thread(target = self.work, args = (what, default_db, default_charset))
        self._worker.start()
    
        self._update_timer = mforms.Utilities.add_timeout(0.2, self.update_ui)




class ParameterDialog(mforms.Form):
    def __init__(self, editor):
        mforms.Form.__init__(self, mforms.Form.main_form(), mforms.FormDialogFrame)

        self.editor = editor
        box = mforms.Box(False)
        box.set_padding(12)
        box.set_spacing(8)

        box.add(mforms.newLabel("Preview the first lines of the script below and click [Run] to start executing.\nNote: the preview below may display non-ASCII characters incorrectly, even if the MySQL server can treat them correctly."), False, True)
        self.file_info = mforms.newLabel("")
        box.add(self.file_info, False, True)
        self.text = mforms.newCodeEditor(None)
        self.text.set_language(mforms.LanguageMySQL)
        
        box.add(self.text, True, True)


        table = mforms.newTable()
        table.set_padding(20)
        table.set_row_count(2)
        table.set_column_count(3)
        table.set_row_spacing(8)
        table.set_column_spacing(4)
        table.add(mforms.newLabel("Default Schema Name:"), 0, 1, 0, 1, 0)
        self.schema = mforms.newSelector(mforms.SelectorCombobox)
        table.add(self.schema, 1, 2, 0, 1, mforms.HFillFlag|mforms.HExpandFlag)

        help = mforms.newLabel("Schema to be used unless explicitly specified in the script.\nLeave blank if the script already specified it,\npick a schema from the drop down or type a name to\ncreate a new one.")
        help.set_style(mforms.SmallHelpTextStyle)
        table.add(help, 2, 3, 0, 1, mforms.HFillFlag)

        table.add(mforms.newLabel("Default Character Set:"), 0, 1, 1, 2, 0)
        self.charset = mforms.newSelector()
        self.charset.add_changed_callback(self.update_preview)
        l = [""]
        for ch in grt.root.wb.rdbmsMgmt.rdbms[0].characterSets:
            l.append(ch.name)
        self.charset.add_items(sorted(l))
        table.add(self.charset, 1, 2, 1, 2, mforms.HFillFlag|mforms.HExpandFlag)

        help = mforms.newLabel("Default character set to use when executing the script,\nunless specified in the script.")
        help.set_style(mforms.SmallHelpTextStyle)
        table.add(help, 2, 3, 1, 2, mforms.HFillFlag)

        box.add(table, False, True)

        self.ok = mforms.newButton()
        self.ok.set_text("Run")

        self.cancel = mforms.newButton()
        self.cancel.set_text("Cancel")

        hbox = mforms.Box(True)
        hbox.set_spacing(8)
        mforms.Utilities.add_end_ok_cancel_buttons(hbox, self.ok, self.cancel)
        box.add_end(hbox, False, True)

        self.set_content(box)

    def run(self, file):
        self.set_title("Run SQL Script - %s" % file)

        known_schemas = [""]
        result = self.editor.executeManagementQuery("SHOW SCHEMAS", 0)
        if result:
            while result.nextRow():
                s = result.stringFieldValue(0)
                if s not in ["performance_schema", "mysql", "information_schema"]:
                    known_schemas.append(s)
            self.schema.add_items(sorted(known_schemas))

        try:
            self.preview_data = open(file).read(4098)
            self.detected_charset = None
            r = re.compile("/\*[^ ]* SET NAMES ([a-zA-Z0-9_]*) \*/")
            # try to detect the charset name from the SET NAMES line
            for line in self.preview_data.split("\n"):
                if line.startswith("/*"):
                    g = r.match(line)
                    if g:
                        c = g.groups()[0]
                        log_info("Character set of file %s detected to be %s\n" % (file, c))
                        self.detected_charset = c
                        break

            self.file_info.set_text("%i total bytes in file, displaying first %i bytes" % (os.stat(file).st_size, len(self.preview_data)))
            self.update_preview()
        except Exception, e:
            mforms.Utilities.show_error("Run SQL Script", str(e), "OK", "", "")
            return

        self.set_size(800, 600)
        self.center()
        return self.run_modal(self.ok, self.cancel)


    def get_default_charset(self):
        c = self.charset.get_string_value().encode("utf8")
        if c == "Default":
            return ""
        return c

    def get_default_schema(self):
        return self.schema.get_string_value().encode("utf8")

    def update_preview(self):
        c = self.charset.get_string_value()
        data = self.preview_data
        if c and c != "utf8":
            tmp = data
            while tmp:
                try:
                    data = tmp.decode(c).encode("utf8")
                    break
                except (UnicodeDecodeError, LookupError):
                    tmp = tmp[:-1]
            if not tmp:
                log_error("Could not convert file %s from %s to utf8\n" % (file, c))
        self.text.set_features(mforms.FeatureReadOnly, False)
        self.text.set_text(data)
        self.text.set_features(mforms.FeatureReadOnly, True)


class RunScriptForm(mforms.Form):
    def __init__(self, editor):
        mforms.Form.__init__(self, mforms.Form.main_form(), mforms.FormDialogFrame)

        self.editor = editor
        self.logbox = mforms.newTextBox(mforms.VerticalScrollBar)


    def report(self, text):
        if text is None:
            self.ok.set_enabled(True)
        else:
            self.logbox.append_text_and_scroll(text, True)


    def start_import(self, file, default_schema, default_charset):
        self.panel = RunPanel(self.editor, self.report)
        self.set_title("Run SQL Script")

        box = mforms.newBox(False)
        box.set_padding(12)
        box.set_spacing(12)

        box.add(self.panel, False, True)
        box.add(mforms.newLabel("Output:"), False, True)
        box.add(self.logbox, True, True)

        self.ok = mforms.newButton()
        self.ok.set_text("Close")
        self.ok.add_clicked_callback(self.close)

        #self.abort = mforms.newButton()
        #self.abort.set_text("Abort")

        hbox = mforms.Box(True)
        hbox.set_spacing(8)
        #hbox.add(self.abort, False, True)
        hbox.add_end(self.ok, False, True)
        box.add_end(hbox, False, True)

        self.set_content(box)

        self.set_size(800, 600)
        self.center()
        self.show()

        self.ok.set_enabled(False)
        self.panel.start(file, default_schema, default_charset)



    def run(self):
        chooser = mforms.FileChooser(mforms.OpenFile)
        chooser.set_title("Run SQL Script")
        chooser.set_extensions('SQL Scripts (*.sql)|*.sql', 'sql')
        if chooser.run_modal():
            dlg = ParameterDialog(self.editor)
            if dlg.run(chooser.get_path()):
                schema = dlg.get_default_schema()
                if schema:
                    self.editor.executeManagementCommand("CREATE SCHEMA IF NOT EXISTS `%s`" % schema, 1)

                self.start_import(chooser.get_path().encode("utf8"), dlg.get_default_schema(), dlg.get_default_charset())
                return True
        return False


    def run_file(self, path):
        dlg = ParameterDialog(self.editor)
        if dlg.run(path):
            schema = dlg.get_default_schema()
            if schema:
                self.editor.executeManagementCommand("CREATE SCHEMA IF NOT EXISTS `%s`" % schema, 1)

            self.start_import(path, dlg.get_default_schema(), dlg.get_default_charset())
            return True
        return False

