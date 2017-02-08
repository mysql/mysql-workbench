# Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

import locale
import threading
from collections import deque

import grt
import mforms

import wizard_page_widget

os_icon_suffix = ""
import sys
if sys.platform=="darwin":
    os_icon_suffix = "_mac"


class WizardTaskError(Exception):
    pass


class WizardTask(mforms.Box):
    def __init__(self, owner, func, label):
        mforms.Box.__init__(self, True)
        self.set_managed()
        self.set_release_on_add()
        
        self.owner = owner
        self.label = label
        self.func = func
        
        self._icon = mforms.newImageBox()
        self._label = mforms.newLabel(label)
        
        self._enabled = True
        
        self.set_spacing(8)
        self.add(self._icon, False, True)
        self.add(self._label, True, True)

        self.reset()

    def set_text(self, text):
        self._label.set_text(text)

    def run(self):
        self.set_running()
        grt.log_info("Wizard", "Execute '%s'\n" % self.label)
        self.owner.send_info("%s..." % self.label)
        try:
            self.func()
        except grt.UserInterrupt:
            self.owner.send_info("%s cancelled" % self.label)
            self.set_aborted()
            return
        except Exception, e:
            if not isinstance(self, WizardThreadedTask):
                print
                import traceback
                traceback.print_exc()

            msg = "Error during %s: %s" % (self.label, self.owner.format_exception_text(e))
            self.owner.send_error(msg)
            #grt.log_error("Wizard", msg)
            self.set_failed(msg)
            raise e

        self.owner.send_info("%s done" % self.label)
        #grt.log_info("Wizard", "%s done\n" % self.label)
        self.set_finished()
            

    @property
    def is_enabled(self):
        return self._enabled

    @property
    def is_running(self):
        return self._running

    @property
    def is_finished(self):
        return self._finished

    @property
    def is_failed(self):
        return self._failed
    
    @property
    def is_aborted(self):
        return self._aborted
        
    @property
    def is_pending(self):
        return self.is_enabled and not self.is_finished and not self.is_failed and not self.is_aborted
    
    def reset(self):
        self._error_message = None
        self._running = False
        self._finished = False
        self._failed = False
        self._aborted = False
        self._label.set_text(self.label)
        if self._enabled:
            self._icon.set_image("task_unchecked%s.png" % os_icon_suffix)
        else:
            self._icon.set_image("task_disabled%s.png" % os_icon_suffix)
        
    def set_running(self):
        self._running = True
        self._icon.set_image("task_executing%s.png" % os_icon_suffix)

    def set_aborted(self):
        self._aborted = True
        self._running = False
        self._icon.set_image("task_error%s.png" % os_icon_suffix)
        
    def set_failed(self, msg):
        self._running = False
        self._failed = True
        self._error_message = msg
        self._icon.set_image("task_error%s.png" % os_icon_suffix)

    def set_error_icon(self):        
        self._icon.set_image("task_error%s.png" % os_icon_suffix)

    def set_warning_icon(self):        
        self._icon.set_image("task_warning%s.png" % os_icon_suffix)

    def set_finished(self):
        self._running = False
        self._finished = True
        self._icon.set_image("task_checked%s.png" % os_icon_suffix)

    def set_enabled(self, flag):
        if self._enabled != flag:
            self._enabled = flag
            if flag:
                self._icon.set_image("task_unchecked%s.png" % os_icon_suffix)
            else:
                self._icon.set_image("task_disabled%s.png" % os_icon_suffix)


class WizardThreadedTask(WizardTask):
    def __init__(self, owner, func, label):
        WizardTask.__init__(self, owner, func, label)
  
        self._thread = None
        
        
    class TaskThread(threading.Thread):
        def __init__(self, owner):
            threading.Thread.__init__(self)

            self.owner = owner
            
        def run(self):
            self.owner.thread_work()

        
  
    def run(self):
        self.set_running()
        self.owner.send_info("%s...." % self.label)
        grt.log_info("Wizard", "Starting thread for '%s'\n" % self.label)
        self._thread = self.TaskThread(self)
        self._thread.start()


    def reset(self):
        self._finished = False
        WizardTask.reset(self)


    def thread_work(self):
        try:
            self.func()
        except grt.UserInterrupt:
            self.owner.send_info("%s cancelled" % self.label)
            mforms.Utilities.perform_from_main_thread(self.set_aborted, False)
            return
        except Exception, exc:
            print
            import traceback
            traceback.print_exc()
            grt.log_error("Wizard", "Thread '%s' failed: %s\n" % (self.label, exc))
            self.owner.send_error("%s: %s" % (self.label, str(exc)))
            mforms.Utilities.perform_from_main_thread(lambda self=self,exc=exc:self.set_failed("Error: %s" % self.owner.format_exception_text(exc)), False)
            return

        self.owner.send_info("%s finished" % self.label)
        mforms.Utilities.perform_from_main_thread(self.set_finished, False)



class WizardProgressPage(wizard_page_widget.WizardPage):
    def __init__(self, main, header_label, description = None, use_private_message_handling=False):
        wizard_page_widget.WizardPage.__init__(self, main, header_label)
      
        self._use_private_message_handling = use_private_message_handling
      
        self._description = mforms.newLabel(description or "The following tasks will now be performed. Please monitor the execution.")
        self.content.add(self._description, False, True)
        
        self._tasks_box = mforms.newBox(False)
        self._tasks_box.set_padding(24)
        self._tasks_box.set_spacing(8)
        self.content.add(self._tasks_box, False, True)
        
        self._status_label = mforms.newLabel("Click [Next >] to execute.")
        self.content.add(self._status_label, False, True)
        
        self._progress = mforms.newProgressBar()
        self.content.add(self._progress, False, True)
        self._progress.show(False)
        
        self._detail_label = mforms.newLabel("")
        self.content.add(self._detail_label, False, True)

        self._timer = None
        self._tasks = []
        self._currently_running_task_index = None
        self._progress_indeterminate = False
        self._tasks_finished = False
        
        self._log_box = mforms.newPanel(mforms.TitledBoxPanel)
        self._log_box.set_title("Message Log")
        
        self._log_text = mforms.newTextBox(mforms.VerticalScrollBar)
        self._log_text.set_name('WizardProgressLogText')
        self._log_text.set_read_only(True)
        self._log_text.set_padding(16)
        self._log_box.add(self._log_text)
        self._log_box.show(False)
        self.content.add_end(self._log_box, True, True)
        self._log_queue = deque()
        
        self._showing_logs = False
        self._cancel_requested = False
        self._tasks_held = False
        self.advanced_button.set_text("Show Logs")
        
        self._log_progress_text = True
        self._autostart = False

    
    def clear_tasks(self):
        for task in self._tasks:
            self._tasks_box.remove(task)
        self._tasks = []
        self.reset()
        
    def format_exception_text(self, e):
        return str(e)    
        
    def add_task(self, func, label):
        """ Add a task to be executed from the main thread. The callback must be as in
          callback(progress), where progress is a function to be optionally executed to
          report progress. 
           
          The callback must return True if it finished executing or False if it was cancelled 
          by the user. If an error occurs during the execution of the task, an exception must 
          be raised.
        """
        wtask = WizardTask(self, func, label)
        self._tasks.append(wtask)
        self._tasks_box.add(wtask, False, True)
        return wtask
    
    
    def add_threaded_task(self, func, label):
        """ Add a task to be executed from a worker thread. The callback must be as in
          callback(). Progress information must be reported using grt.send_progress()
           
          The callback must return True if it finished executing or False if it was cancelled 
          by the user. If an error occurs during the execution of the task, an exception must 
          be raised.
        """
        wtask = WizardThreadedTask(self, func, label)
        self._tasks.append(wtask)
        self._tasks_box.add(wtask, False, True)
        return wtask


    def update_progress(self, pct, status):
        self._progress_info = (pct, status)


    def go_next(self):
        if not self._tasks_finished:
            if self._currently_running_task_index is None:
                self.start()
            else:
                self.resume()
        else:
            wizard_page_widget.WizardPage.go_next(self)

    def go_advanced(self):
        self._showing_logs = not self._showing_logs
        if self._showing_logs:
            self.advanced_button.set_text("Hide Logs")
        else:
            self.advanced_button.set_text("Show Logs")
        self._log_box.show(self._showing_logs)
        self.relayout()
    
    def go_cancel(self):
        self._cancel_requested = True

    def page_activated(self, advancing):
        super(WizardProgressPage, self).page_activated(advancing)
        
        self.next_button.set_enabled(self._tasks_finished or self._currently_running_task_index is None)

        if advancing and self._autostart:
            if self._currently_running_task_index is None and not self._tasks_finished:
                self.start()

    def reset(self, clear_log_box= False):
        self._cancel_requested = False
        self._tasks_finished = False
        self._progress_info = None
        self._currently_running_task_index = None
        self._progress.set_value(0)
        self._status_label.set_text("Click [Next >] to execute.")
        self._detail_label.set_text("")
        for task in self._tasks:
            task.reset()
        if clear_log_box:
            self._log_text.set_value("")

    def _handle_task_output(self, msg_type, text, detail):
        self._log_queue.append((msg_type, text, detail))
        if msg_type == "PROGRESS" and self._log_progress_text:
            text = text.split(":", 1)[-1]
            if detail:
                progress_text = "- %s: %s\n" % (text, detail)
            else:
                progress_text = "- %s\n" % (text)
            self._log_queue.append(("OUTPUT", progress_text, ""))

        return True

    def query_cancel_status(self):
        return self._cancel_requested

    def send_raw(self, msg):
        self._log_queue.append(("OUTPUT", msg, ""))

    def send_info(self, msg):
        grt.log_debug("Wizard", msg + "\n")
        self._handle_task_output("INFO", msg, "")
        
    def send_error(self, msg):
        grt.log_debug("Wizard", "ERROR: %s\n" % msg )
        self._handle_task_output("ERROR", msg, "")
        
    def send_warning(self, msg):
        grt.log_debug("Wizard", "WARNING: %s\n" % msg )
        self._handle_task_output("WARNING", msg, "")

    def send_progress(self, pct, msg):
        self._handle_task_output("PROGRESS", "%s:%s"%(locale.str(pct), msg), "")

    def start(self):
        self._warnings = 0
        self._errors = 0
        self._cancel_requested = False
        self._task_warnings = []
        self._task_errors = []
        self._progress_info = None
        self._currently_running_task_index = 0
        self._progress.show()
        self.next_button.set_enabled(False)
        self.back_button.set_enabled(False)
        self.cancel_button.set_enabled(True)
        
        if not self._use_private_message_handling:
            grt.push_status_query_handler(self.query_cancel_status)
            grt.push_message_handler(self._handle_task_output)
        self.send_info("Starting...")
        self._timer = mforms.Utilities.add_timeout(0.1, self.update_status)
        self.relayout()
        
    def resume(self):
        self._progress.show()
        self.next_button.set_enabled(False)
        self.back_button.set_enabled(False)
        self.cancel_button.set_enabled(True)
        
        if not self._use_private_message_handling:
            grt.push_status_query_handler(self.query_cancel_status)
            grt.push_message_handler(self._handle_task_output)
        self.send_info("Resuming...")

        self._tasks_held = False
        self._timer = mforms.Utilities.add_timeout(0.1, self.update_status)
        self.relayout()

        
    def tasks_finished(self):
        pass


    def tasks_failed(self, canceled):
        pass
        
    def _hold(self):
        self.send_info("Processing Held")
        self._flush_messages()
        self.send_info("\n")

        self._progress.show(False)
        self._progress.stop()
        self._progress_indeterminate = False
        self.next_button.set_enabled(True)
        self.back_button.set_enabled(True)
        self.cancel_button.set_enabled(True)

        self._tasks_held = True
        self.relayout()



    def _finished(self):
        if self._warnings > 0 or self._errors > 0:
            self.send_info("Tasks finished with warnings and/or errors; view the logs for details")
        self.send_info(self.final_message())
        self._flush_messages()
        self.send_info("\n\n")
        if not self._use_private_message_handling:
            grt.pop_message_handler()
            grt.pop_status_query_handler()
    
        self._progress.show(False)
        self._progress.stop()
        self._progress_indeterminate = False
        self._tasks_finished = True
        self.next_button.set_enabled(True)
        self.back_button.set_enabled(True)
        self.cancel_button.set_enabled(False)
        
        self._status_label.set_text("")
        
        # Updates the details label as needed
        self._detail_label.set_text(self.final_details())

        if self._errors > 0 or self._warnings > 0:
            if not self._showing_logs:
                self.go_advanced()
            self.tasks_failed(False)
        else:
            self.tasks_finished()
        self.relayout()


    def _failed(self):
        self.send_info("Failed")
        self._flush_messages()
        self.send_info("\n")
        if not self._use_private_message_handling:
            grt.pop_message_handler()
            grt.pop_status_query_handler()

        self._progress.show(False)
        self._progress.stop()
        self._progress_indeterminate = False
        self.next_button.set_enabled(False)
        self.back_button.set_enabled(True)
        self.cancel_button.set_enabled(False)
        
        self._detail_label.set_text(self.failure_message())

        if not self._showing_logs:
            self.go_advanced()

        self.tasks_failed(False)
        self.relayout()
        

    def _cancelled(self):
        self.send_info("Cancelled")
        self._flush_messages()
        self.send_info("\n")
        if not self._use_private_message_handling:
            grt.pop_message_handler()
            grt.pop_status_query_handler()

        self._progress.show(False)
        self._progress.stop()
        self._progress_indeterminate = False
        self.next_button.set_enabled(False)
        self.back_button.set_enabled(True)
        self.cancel_button.set_enabled(False)
        
        self._detail_label.set_text("Cancelled by user.")

        self.tasks_failed(True)
        self.relayout()


    def _flush_messages(self):
        while len(self._log_queue) > 0:
            mtype, text, detail  = self._log_queue.popleft()
            if "\n" in detail:
                detail = "\n    "+("\n    ".join(detail.split("\n")))
            if mtype == "INFO":
                if detail:
                    self._log_text.append_text_and_scroll("%s: %s\n" % (text, detail), True)
                else:
                    self._log_text.append_text_and_scroll("%s\n" % (text), True)
            elif mtype == "OUTPUT":
                # output text must already come with newlines
                if detail:
                    self._log_text.append_text_and_scroll("%s: %s" % (text, detail), True)
                else:
                    self._log_text.append_text_and_scroll("%s" % (text), True)
            elif mtype == "PROGRESS":
                pct, y, text = text.partition(":")
                try:
                    self.update_progress(locale.atof(pct), text)
                except Exception:
                    grt.log_debug('Wizard', 'Exception raised when converting "%s" to float using locale.atof(). Exception ignored\n' % pct)
            else:
                if mtype == "ERROR":
                    self._errors += 1
                    self._task_errors.append(text)
                elif mtype == "WARNING":
                    self._warnings += 1
                    self._task_warnings.append(text)
                if detail:
                    self._log_text.append_text_and_scroll("%s: %s: %s\n" % (mtype, text, detail), True)
                else:
                    self._log_text.append_text_and_scroll("%s: %s\n" % (mtype, text), True)

    def failure_message(self):
        """Subclass and override to change the text message to be shown when tasks failed to finish."""
        return "A task has failed executing."


    def final_message(self):
        """Subclass and override to change the text message to be shown when tasks finish successfully."""
        return "Finished performing tasks."


    def final_details(self):
        """Subclass and override to change the text message to be shown when tasks finish successfully."""
        ret_val = ""
        if self._errors > 0:
            ret_val = "There were errors during execution; please review log messages."
        elif self._warnings > 0:
            ret_val = "There were warnings during execution; please review log messages.\nClick [Next >] to continue if you think they are not important."
        else:
            ret_val = self.final_message() + " Click [Next >] to continue."

        return ret_val

    def verify_task_preconditions(self, label):
        """
        Will check for specific task conditions to be met, task will be identified by its label.
        If preconditions are met returns None, if not, returns a descriptive error message.
        """
        return None

    def update_status(self):
        self._flush_messages()

        if self._tasks_held:
            return False
    
        if self._progress_info:
            pct, text = self._progress_info
            self._progress_info = None
            if pct < 0:
                if not self._progress_indeterminate:
                    self._progress_indeterminate = True
                    self._progress.set_indeterminate(True)
                    self._progress.start()
            else:
                if self._progress_indeterminate:
                    self._progress_indeterminate = False
                    self._progress.stop()
                    self._progress.set_indeterminate(False)
                self._progress.set_value(pct)
            self._status_label.set_text(text)
    
        task = self._tasks[self._currently_running_task_index]

        if not task.is_running and task.is_pending:
            if self._cancel_requested:
                self._cancelled()
                return False

            try:
                error = self.verify_task_preconditions(task.label)
                if error is None:
                    task.run()
                else:
                    self._hold()
                    mforms.Utilities.show_error("Unable To Continue", error, "OK", "", "")


            except Exception, exc:
                #print
                #import traceback
                #traceback.print_exc()
                self.send_error("Exception in task '%s': %r\n" % (task.label, exc))
                self._status_label.set_text("%s" % self.format_exception_text(exc))
                self._failed()
                return False
    
        if task.is_failed:
            self._failed()
            return False
    
        if task.is_aborted:
            self._cancelled()
            return False
        
        if task.is_finished or not task.is_enabled: # go to next task
            if self._task_errors:
                task.set_error_icon()
            elif self._task_warnings:
                task.set_warning_icon()
            self._task_errors = []
            self._task_warnings = []
            self._currently_running_task_index += 1

            if self._currently_running_task_index == len(self._tasks):
                # last task was executed
                self._finished()
                return False

        return True
