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
from functools import partial

import grt
import mforms

from workbench.ui import WizardPage
from migration_source_selection import request_password
from DataMigrator import DataMigrator

#==================================================================================
class Task(object):
    def __init__(self, owner, func, label):
        self.owner = owner
        self.label = label
        self.func = func
        
        self._enabled = True
        
        self.reset()

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
            if not isinstance(self, ThreadedTask):
                print
                import traceback
                traceback.print_exc()

            msg = "Error during %s: %s" % (self.label, self.owner.format_exception_text(e))
            self.owner.send_error(msg)
            self.set_failed(msg)
            raise e

        self.owner.send_info("%s done" % self.label)
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
        
    def set_running(self):
        self._running = True

    def set_aborted(self):
        self._aborted = True
        self._running = False
        
    def set_failed(self, msg):
        self._running = False
        self._failed = True
        self._error_message = msg

    def set_finished(self):
        self._running = False
        self._finished = True

    def set_enabled(self, flag):
        self._enabled = flag


#==================================================================================
class ThreadedTask(Task):
    def __init__(self, owner, func, label):
        super(ThreadedTask, self).__init__(owner, func, label)
  
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
        super(ThreadedTask, self).reset()


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



#==================================================================================
class ProgressMainView(WizardPage):

    def __init__(self, main, use_private_message_handling=False):
        super(ProgressMainView, self).__init__(main, 'Copy Databases')
      
        self.main = main
      
        self._tasks = []
        self._currently_running_task_index = None
        self._progress_indeterminate = False
        self._tasks_finished = False
        self._log_queue = deque()
        self._showing_logs = False
        self._log_progress_text = True
        self._cancel_requested = False


    def page_activated(self, advancing):
        self._schema_list = self.main._schema_selection_page.schema_selector.get_selected()
        self._schema_count = len(self._schema_list)

        super(ProgressMainView, self).page_activated(advancing)
        
        if advancing:
            self.reset()
            if self._currently_running_task_index is None and not self._tasks_finished:
                self.start()


    def create_ui(self):
        self.content.set_padding(20)

        self._description = mforms.newLabel('')
        self.content.add(self._description, False, True)
        
        self._schema_progress_box = mforms.newBox(False)
        self._schema_progress_box.set_padding(24)
        self._schema_progress_box.set_spacing(8)
        
        self._schema_label = mforms.newLabel('')
        self._schema_progress_box.add(self._schema_label, False, True)
        self._schema_progress = mforms.newProgressBar()
        self._schema_progress_box.add(self._schema_progress, False, True)
        self.content.add(self._schema_progress_box, False, True)
        
        self._progress_box = mforms.newBox(False)
        self._progress_box.set_padding(24)
        self._progress_box.set_spacing(8)

        self._status_label = mforms.newLabel('')
        self._progress_box.add(self._status_label, False, True)
        self._progress = mforms.newProgressBar()
        self._progress_box.add(self._progress, False, True)
        self.content.add(self._progress_box, False, True)

        self._detail_label = mforms.newLabel('')
        self._progress_box.add(self._detail_label, False, True)
        
        self._log_box = mforms.newPanel(mforms.TitledBoxPanel)
        self._log_box.set_title('Message Log')
        self._log_box.set_padding(12)
        
        self._log_text = mforms.newTextBox(mforms.VerticalScrollBar)
        self._log_text.set_read_only(True)
        self._log_box.add(self._log_text)
        self._log_box.show(False)
        self.content.add(self._log_box, True, True)
        
        self.advanced_button.set_text('Show Logs')

    
    def clear_tasks(self):
        self._tasks = []
        self.reset()
        
    def format_exception_text(self, e):
        return str(e)    
        

    def update_progress(self, pct, status):
        self._progress_info = (pct, status)


    def go_advanced(self):
        self._showing_logs = not self._showing_logs
        if self._showing_logs:
            self.advanced_button.set_text('Hide Logs')
        else:
            self.advanced_button.set_text('Show Logs')
        self._log_box.show(self._showing_logs)
        self.content.relayout()
    
    def go_cancel(self):
        self._cancel_requested = True


    def reset(self, clear_log_box= False):
        self._cancel_requested = False
        self._tasks_finished = False
        self._progress_info = None
        self._currently_running_task_index = None

        self._schema_progress_box.show(False)
        self._schema_progress.set_value(0)
        
        self._progress_box.show(False)
        self._progress.set_value(0)
        self._detail_label.set_text('')
        self._status_label.set_text('')

        self.next_button.set_enabled(True)
        
        self.create_tasks()

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
        grt.log_debug("Wizard", msg)
        self._handle_task_output("INFO", msg, "")
        
    def send_error(self, msg):
        grt.log_debug("Wizard", "ERROR: "+msg)
        self._handle_task_output("ERROR", msg, "")
        
    def send_warning(self, msg):
        grt.log_debug("Wizard", "WARNING: "+msg)
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

        self._schema_progress_box.show(self._schema_count > 1)
        self._progress_box.show()
        self._description.set_text('Please wait while the selected schemas are copied. This may take some time.')
        self.next_button.set_enabled(False)
        self.back_button.set_enabled(False)
        self.cancel_button.set_enabled(True)
        
        grt.push_status_query_handler(self.query_cancel_status)
        grt.push_message_handler(self._handle_task_output)
        self.send_info('Starting...')
        mforms.Utilities.add_timeout(0.1, self.update_status)


    def update_status(self):
        self._flush_messages()
    
        if self._progress_info:
            pct, text = self._progress_info
            self._progress_info = None
            if pct < 0:
                if not self._progress_indeterminate:
                    self._progress_indeterminate = True
            else:
                if self._progress_indeterminate:
                    self._progress_indeterminate = False
                self._progress.set_value(pct)
            self._status_label.set_text(text)
    
        schema_idx, task = self._tasks[self._currently_running_task_index]
        self._schema_label.set_text('Copying %d of %d schemas.\nCurrent schema: %s' % (schema_idx, self._schema_count,
                                                                                       self._schema_list[schema_idx-1])
                                   )
        self._schema_progress.set_value(float(schema_idx) / self._schema_count)

        if not task.is_running and task.is_pending:
            if self._cancel_requested:
                self._cancelled()
                return False

            try:
                task.run()
            except Exception, exc:
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
            self._task_errors = []
            self._task_warnings = []
            self._currently_running_task_index += 1

            if self._currently_running_task_index == len(self._tasks):
                # last task was executed
                self._finished()
                return False

        return True
        

    def _finished(self):
        if self._warnings > 0 or self._errors > 0:
            self.send_info("Tasks finished with warnings and/or errors; view the logs for details")
        self.send_info(self.final_message())
        self._flush_messages()
        self.send_info("\n\n")

        grt.pop_message_handler()
        grt.pop_status_query_handler()
    
        self._schema_progress_box.show(False)
        self._progress_box.show(False)
        self._progress.stop()
        self._progress_indeterminate = False
        self._tasks_finished = True
        self.next_button.set_enabled(True)
        self.back_button.set_enabled(True)
        self.cancel_button.set_enabled(False)
        
        self._status_label.set_text("")

        if not self._showing_logs:
            self.go_advanced()
        
        if self._errors > 0:
            self._description.set_text("Finished.\nThere were errors during execution; please review log messages.")
        elif self._warnings > 0:
            self._description.set_text("Finished.\nThere were warnings during execution; please review log messages.\nClick [Next >] to continue if you think they are not important.")
        else:
            self._description.set_text(self.final_message()+"\nClick [Next >] to continue.")


    def _failed(self):
        self.send_info("Failed")
        self._flush_messages()
        self.send_info("\n")
       
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
        

    def _cancelled(self):
        self.send_info("Cancelled")
        self._flush_messages()
        self.send_info("\n")
        
        grt.pop_message_handler()
        grt.pop_status_query_handler()

        self._progress.show(False)
        self._progress.stop()
        self._progress_indeterminate = False
        self.next_button.set_enabled(False)
        self.back_button.set_enabled(True)
        self.cancel_button.set_enabled(False)
        
        self._detail_label.set_text("Cancelled by user.")


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
                    grt.log_debug('Wizard', 'Exception raised when converting "%s" to float using locale.atof(). Exception ignored' % pct)
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

    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # TASK RELATED MEMBER FUNCTIONS START HERE
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        
    def create_transfer_log(self, target_table):
        log = grt.classes.GrtLogObject()
        log.logObject = target_table
        
        target_db_module = self.main.plan.migrationTarget.module_db()
        logSchema = target_db_module.quoteIdentifier(log.logObject.owner.name)
        logTable = target_db_module.quoteIdentifier(log.logObject.name)
        
        log.name = "%s.%s" % (logSchema, logTable)
        log.logObject = target_table
        
        self.main.plan.state.dataTransferLog.append(log)
      
    def get_log_object(self, target_table):
        for log in self.main.plan.state.dataTransferLog:
            if target_table == log.name:
                return log
    
    def add_log_entry(self, type, target_table, message):
        logObject = self.get_log_object(target_table)
        
        entry = grt.classes.GrtLogEntry()
        entry.entryType = type
        entry.name = message
        
        logObject.entries.append(entry)

    def create_tasks(self):
        self._tasks = []
        self.main.plan.state.dataBulkTransferParams["LiveDataCopy"] = 1
        source_password = self.main.plan.migrationSource.password
        if source_password is None:
            source_password = request_password(self.main.plan.migrationSource.connection)
        target_password = self.main.plan.migrationTarget.password
        if target_password is None:
            if self.main.plan.migrationTarget.connection.hostIdentifier == self.main.plan.migrationSource.connection.hostIdentifier:
                if self.main.plan.migrationTarget.connection.parameterValues['userName'] == self.main.plan.migrationSource.connection.parameterValues['userName']:
                    target_password = source_password
        if target_password is None:
            target_password = request_password(self.main.plan.migrationTarget.connection)
        self._transferer = DataMigrator(self, self.main.plan.state.dataBulkTransferParams, 
                      self.main.plan.migrationSource.connection, source_password,
                      self.main.plan.migrationTarget.connection, target_password)

        self._transferer.copytable_path = self.main.plan.wbcopytables_path_bin
        for idx, schema_name in enumerate(self.main.plan.migrationSource.selectedSchemataNames):
            self._tasks.extend(
              [
                (idx+1, ThreadedTask(self, partial(self._rev_eng_schema, schema_name), 'Reverse Engineering')),
                (idx+1, Task(self, self._migrate_schema, 'Migrating')),
                (idx+1, Task(self, self._fwd_eng_schema, 'Generating Code')),
                (idx+1, ThreadedTask(self, self._create_schema, 'Creating target schema')),
                (idx+1, Task(self, self._prepare_copy, 'Selecting tables to copy')),
                (idx+1, ThreadedTask(self, self._row_count, 'Counting table rows to copy')),
                (idx+1, ThreadedTask(self, self._data_copy, 'Copying table data')),
              ]
            )
            

    def _rev_eng_schema(self, schema_name):
        self.main.plan.migrationSource.selectedSchemataNames = [ schema_name ]
        self.main.plan.migrationSource.reverseEngineer()
        if self.main._schema_selection_page.innodb_switch.get_active():
            for table in self.main.plan.migrationSource.catalog.schemata[0].tables:
                table.tableEngine = 'InnoDB'

    def _migrate_schema(self):
        self.main.plan.migrate()

    def _fwd_eng_schema(self):
        self.main.plan.generateSQL()

    def _create_schema(self):
        self.main.plan.migrationTarget.connect()  #TODO: Is this necessary?
        self.main.plan.createTarget()      

    def _prepare_copy(self):
        # create work list
        tables = self.main.plan.migrationSource.catalog.schemata[0].tables

        if not tables:  # Do not copy data if there are no tables in the source schema
            return
                
        source_db_module = self.main.plan.migrationSource.module_db()
        target_db_module = self.main.plan.migrationTarget.module_db()
        
        self._working_set = {}
        for table in tables:
            schema_name = source_db_module.quoteIdentifier(table.owner.name)
            table_name = source_db_module.quoteIdentifier(table.name)
                
            targ_schema_name = target_db_module.quoteIdentifier(table.owner.name)
            targ_table_name = target_db_module.quoteIdentifier(table.name)

            self._working_set[schema_name+"."+table_name] = {"table" : table, 
                        "source_schema":schema_name, "source_table":table_name,
                        "target_schema":targ_schema_name, "target_table":targ_table_name,
                        "target_table_object":table}
            select_expression = []
            source_pk_list = []
            target_pk_list = []
            for column in table.columns:
                if column.generated:
                    continue
                if table.isPrimaryKeyColumn(column):
                    source_pk_list.append(source_db_module.quoteIdentifier(column.oldName))
                    target_pk_list.append(target_db_module.quoteIdentifier(column.name))
                cast = table.customData.get("columnTypeCastExpression:%s" % column.name, None)
                if cast:
                    select_expression.append(cast.replace("?", source_db_module.quoteIdentifier(column.oldName)))
                else:
                    select_expression.append(source_db_module.quoteIdentifier(column.oldName))

            self._working_set[schema_name+"."+table_name]["source_primary_key"] = ",".join(source_pk_list)
            self._working_set[schema_name+"."+table_name]["target_primary_key"] = ",".join(target_pk_list)
            self._working_set[schema_name+"."+table_name]["select_expression"] = ", ".join(select_expression)

    def _row_count(self):
        if not self.main.plan.migrationSource.catalog.schemata[0].tables:  # Do not copy data if there are no tables in the source schema
            return

        total = self._transferer.count_table_rows(self._working_set)

        self.send_info("%i total rows in %i tables need to be copied:" % (total, len(self._working_set)))

    def _data_copy(self):
        if not self.main.plan.migrationSource.catalog.schemata[0].tables:  # Do not copy data if there are no tables in the source schema
            return
        self.send_progress(0, 'Data copy starting')
        total = 0
        table_count = len (self._working_set);
        for task in self._working_set.values():
            total += task.get("row_count", 0)
            self.create_transfer_log(task["target_table_object"])

        self.send_info("") # newline
            
        if self._working_set:
            thread_count = 2
            self.send_info("Migrating data...")
            self._log_progress_text = False
            self._migrating_data = True
            try:
                succeeded_tasks = self._transferer.migrate_data(thread_count, self._working_set)
            finally:
                self._log_progress_text = True
                self._migrating_data = False

            self.send_info("") # newline
                
            self.send_info("Data copy results:")
            fully_copied = 0
            for task in self._working_set.values():
                info = succeeded_tasks.get(task["target_schema"]+"."+task["target_table"], None)
                row_count = task.get("row_count", 0)
                if info:
                    ok, count = info
                else:
                    count = 0
                    ok = False
                if ok and count == row_count:
                    fully_copied = fully_copied + 1
                    
                    target_table = "%s.%s" % (task["target_schema"], task["target_table"])
                    message = "Succeeded : copied %s of %s rows from %s.%s" % (count, row_count,task["source_schema"], task["source_table"])
                    self.add_log_entry(0, target_table, message)
                    
                    
                    self.send_info("- %s.%s has succeeded (%s of %s rows copied)" % (task["target_schema"], task["target_table"], count, row_count))
                else:
                    self.send_info("- %s.%s has FAILED (%s of %s rows copied)" % (task["target_schema"], task["target_table"], count, row_count))
                    
            self.send_info("%i tables of %i were fully copied" % (fully_copied, table_count))

            if self._transferer.interrupted:
                raise grt.UserInterrupt("Canceled by user")
        else:
            self.send_info("Nothing to be done")
