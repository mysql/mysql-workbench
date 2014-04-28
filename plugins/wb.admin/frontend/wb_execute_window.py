# Copyright (c) 2012, 2013 Oracle and/or its affiliates. All rights reserved.
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


import mforms
import threading
from Queue import Queue, Empty
from wb_admin_utils import weakcb


class Task(threading.Thread):
    def __init__(self, ctrl_be, command, queue):
        threading.Thread.__init__(self)
        self.ctrl_be = ctrl_be
        self.command = command
        self.queue = queue
        self.returncode = None

    def handle_output(self, text):
        if text is not None:
            self.queue.put(text)


    def run(self):
        try:
            self.returncode = self.command(output_handler = self.handle_output)
            if self.returncode != 0:
                self.queue.put("Command exited with status %s\n" % self.returncode)
            self.queue.put(None)
        except Exception:
            import traceback
            self.queue.put("\nError executing command:\n"+traceback.format_exc())
            self.queue.put(None)



class CommandExecutePanel(mforms.Form):
    def __init__(self, ctrl_be, title, descr, stop_callback = None, close_callback=None, progress_parser_callback=None):
        mforms.Form.__init__(self, mforms.Form.main_form(), mforms.FormDialogFrame)
        self.ctrl_be = ctrl_be

        self._done = False
        self._update_tm = None

        self.finished_callback = None
        self.stop_callback = stop_callback
        self.close_callback = close_callback
        self.progress_parser_callback = progress_parser_callback

        self.show(False)

        self.box = mforms.newBox(False)
        self.set_content(self.box)
        self.box.set_padding(12)
        self.box.set_spacing(20)

        self.set_title(title)
        self.label = mforms.newLabel(descr)
        self.box.add(self.label, False, True)

        hb = mforms.newBox(True)
        self.progress = mforms.newProgressBar()
        self.progress_label = mforms.newLabel("")
        self.progress_label.set_size(100, -1)
        hb.add(self.progress_label, False, True)
        hb.add(self.progress, True, True)
        self.box.add(hb, False, True)

        self.logbox = mforms.newTextBox(mforms.VerticalScrollBar)
        self.logbox.set_read_only(True)
        panel = mforms.newPanel(mforms.TitledBoxPanel)
        panel.set_title("Command Output")

        self.logbox.set_padding(8)
        panel.add(self.logbox)
        self.box.add(panel, True, True)


        bbox = mforms.newBox(True)
        self.box.add_end(bbox, False, True)

        self.stop = mforms.newButton()
        if stop_callback:
            self.stop.set_text("Stop")
        else:
            self.stop.set_text("Close")
        self.stop.add_clicked_callback(self.do_stop)
        bbox.add_end(self.stop, False, True)

        self.set_size(700, 500)
        self.center()


    def _update(self):
        while True:
            try:
                msg = self.queue.get_nowait()
            except Empty:
                break
            if msg is None:
                self.progress.stop()
                self._done = True
                self.stop.set_text("Close")
                self.stop.set_enabled(True)

                if self.finished_callback(self.task.returncode):
                    self.progress_label.set_text("Finished")
                else:
                    self.progress_label.set_text("FAILED")
                self._update_tm = None
                return False
            else:
                if self.progress_parser_callback:
                    progress = self.progress_parser_callback(msg)
                    if progress is not None:
                        self.progress.set_value(float(progress))
                self.logbox.append_text_and_scroll(msg, True)
        return True

    def close(self):
        if self._update_tm:
            mforms.Utilities.cancel_timeout(self._update_tm)
            self._update_tm = None
        mforms.Form.close(self)


    def do_stop(self):
        if self.stop_callback:
            self.stop_callback()
        if self._done:
            self.close_callback()


    def run_command(self, command, on_finish=lambda: None):
        if self.stop_callback:
            self.stop.set_text("Stop")
            self.stop.set_enabled(True)
        else:
            self.stop.set_text("Close")
            self.stop.set_enabled(False)
        self.progress_label.set_text("Executing...")
        if self.progress_parser_callback:
            self.progress.set_indeterminate(False)
            self.progress.set_value(0)
        else:
            self.progress.set_indeterminate(True)
        self.progress.start()
        self._done = False
        self.queue = Queue()
        self.finished_callback = on_finish
        self.task = Task(self.ctrl_be, command, self.queue)
        self._update_tm = mforms.Utilities.add_timeout(0.5, weakcb(self, "_update"))
        self.task.start()


    def get_log_text(self):
        return self.logbox.get_string_value().encode("utf8")


    def append_log(self, text):
        self.logbox.append_text_and_scroll(text, True)

