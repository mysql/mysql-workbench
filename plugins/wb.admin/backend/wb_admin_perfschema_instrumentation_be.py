# Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

from workbench.log import log_error
from workbench.db_utils import QueryError
from workbench.change_tracker import ChangeTracker, ChangeCounter, ChangeNotifierList, ChangeNotifierDict


MYSQL_ERR_TABLE_DOESNT_EXIST = 1146

class PSInstrument(ChangeTracker):
    """
    Will hold the configuration of a single Performance
    Schema instrument keeping track of the changes to them
    """
    def __init__(self, enabled = False, timed = False):
        ChangeTracker.__init__(self)
        self.enabled = enabled
        self.timed = timed

class PSInstrumentGroup(ChangeNotifierDict):
    def __init__(self, parent, *args):
        self.__value_set_notification_cb = None

        ChangeNotifierDict.__init__(self)

        self.parent = parent
        self.enabled = 0
        self.timed = 0

        self.changes = {'enabled':0, 'timed':0}

    def count_change(self, change, attr, value):

        ChangeNotifierDict.count_change(self, change, attr, value)

        increment = 1 if change else -1
        self.changes[attr] += increment


    def set_children_state(self, attr, value):
        """
        Set the attribute's state to a specific value 
        on a complete branch of the hierarchy tree
        """
        for key in self.keys():
            if attr == 'enabled':
                self[key].enabled = value
            elif attr == 'timed':
                self[key].timed = value

            # If the tree is not a leaf node continues
            # updating the children nodes
            if not self.has_key('_data_'):
                self[key].set_children_state(attr, value)

    def update_parents_state(self, attr):
        """
        Updates the parent's states based on the children
        status on a line of the hierarchy tree.
        """
        parent = self.parent

        while parent:
            parent.set_state_from_children(attr)
            parent = parent.parent

    def set_state(self, attr, value):
        """
        Updates the status of an attribute in an element 
        of the hierarchy tree, including all the branch
        below it and its parents
        """
        if attr == 'enabled':
            self.enabled = value
        elif attr == 'timed':
            self.timed = value

        # Updates children nodes
        self.set_children_state(attr, value)
        self.update_parents_state(attr)
        
    def set_state_from_children(self, attr):
        """
        Sets the status of an element of the hierarchy based
        on the status of its immediate childrens
        """
        counter = 0
        for child in self.keys():
            if attr == 'enabled':
                new_value = self[child].enabled 
            elif attr == 'timed':
                new_value = self[child].timed 

            # If an element is already with state -1
            # All parents will automatically have this 
            # state
            if new_value < 0:
                counter = -1
                break
            else:
                counter += new_value

        value = -1
        if counter == 0:
            value = 0
        elif counter == len(self):
            value = 1

        if attr == 'enabled':
            self.enabled = value
        elif attr == 'timed':
            self.timed = value


    def set_initial_states(self):
        """
        Deep first method to set the initial states of
        the hierarchy groups based on the status of the
        leaf elements
        """
        if not self.has_key('_data_'):
            for key in self.keys():
                self[key].set_initial_states()

        self.set_state_from_children('enabled')
        self.set_state_from_children('timed')

    def __setattr__(self, name, value):
        ChangeNotifierDict.__setattr__(self, name, value)

        # If configured, notifies about a value being set
        if self.__value_set_notification_cb:
            self.__value_set_notification_cb(name, value)

    def set_value_set_notification(self, callback):
        self.__value_set_notification_cb = callback




class PSInstruments(ChangeCounter):
    """
    Database manager for PS Instruments its functions are loading/committing changes
    to the database.
    """
    def __init__(self, ctrl_be):
        ChangeCounter.__init__(self)
        self.ctrl_be = ctrl_be
        self.instruments = PSInstrumentGroup(None)

        self.count_changes_on(self.instruments)

    def load(self):
        try:
            result = self.ctrl_be.exec_query('SELECT * FROM performance_schema.setup_instruments')

            if result is not None:
                while result.nextRow():
                    # Inserts each instrument creating a tree structure
                    # with them
                    tokens = result.stringByName('NAME').split('/')
                    cur_dict = self.instruments
                    for token in tokens:
                        if not cur_dict.has_key(token):
                            cur_dict[token] = PSInstrumentGroup(cur_dict)

                        cur_dict = cur_dict[token]

                    # at this point cur_dict has the leaf node so sets all the data in it
                    enabled = 1 if result.stringByName('ENABLED') == 'YES' else 0
                    timed = 1 if result.stringByName('TIMED') == 'YES' else 0
                    cur_dict['_data_'] = PSInstrument(enabled, timed)
                    
            # Updates the groups states
            self.instruments.set_initial_states()

        except QueryError, err:
            if err.error == MYSQL_ERR_TABLE_DOESNT_EXIST:
                return
            else:
                raise

    def get_commit_statements(self):
        instrument_changes = self.get_changes()
        
        queries = []
        changes={'enabled':{True:[], False:[]}, 'timed':{True:[], False:[]}}

        for instrument in instrument_changes.keys():
            change_set = instrument_changes[instrument]


            exp_instrument = "^%s" % instrument
            if not instrument.endswith('*'):
                exp_instrument += '$'
            
            for change in change_set:
                (col, old_value, new_value) = change

                changes[col][new_value].append(exp_instrument)

        if changes['enabled'][True]:
            queries.append("UPDATE performance_schema.setup_instruments SET ENABLED = 'YES' WHERE NAME RLIKE '%s'" % ('|'.join(changes['enabled'][True])))
        
        if changes['enabled'][False]:
            queries.append("UPDATE performance_schema.setup_instruments SET ENABLED = 'NO' WHERE NAME RLIKE '%s'" % ('|'.join(changes['enabled'][False])))

        if changes['timed'][True]:
            queries.append("UPDATE performance_schema.setup_instruments SET TIMED = 'YES' WHERE NAME RLIKE '%s'" % ('|'.join(changes['timed'][True])))
        
        if changes['timed'][False]:
            queries.append("UPDATE performance_schema.setup_instruments SET TIMED = 'NO' WHERE NAME RLIKE '%s'" % ('|'.join(changes['timed'][False])))
        return queries


    def get_changes(self):
        changes = {}

        if self.instruments.change_count:
            self._get_changes_recursive([], self.instruments, changes, 'enabled')
            self._get_changes_recursive([], self.instruments, changes, 'timed')

        return changes

    def reset_changes(self):
        self.instruments.reset_changes()

    def revert_changes(self):
        self.instruments.revert_changes()
        self.instruments.set_initial_states()

    def set_change(self, instrument, storage, changes):
        if not storage.has_key(instrument):
            storage[instrument] = []

        storage[instrument].extend(changes)

    def _get_changes_recursive(self, path, data, changes, attr):
        """
        Retrieves the minimal change sets done on a specific attribute
        of the instruments.
        """
        for item in data.keys():
            # If there are changes
            if data[item].changes[attr]:
                new_path = list(path)
                new_path.append(item)

                # Gets the group attribute state
                if attr == 'enabled':
                    state = data[item].enabled
                elif attr == 'timed':
                    state = data[item].timed

                if state >= 0:
                    instrument = '/'.join(new_path)
                    if not data[item].has_key('_data_'):
                        instrument += '*'

                    new_value = True if state == 1 else False
                    self.set_change(instrument, changes, [(attr, "", new_value)])
                else:
                    self._get_changes_recursive(new_path, data[item], changes, attr)

class PSConsumer(ChangeTracker):
    """
    """
    def __init__(self, enabled):
        ChangeTracker.__init__(self)
        self.enabled = enabled


class PSConsumers(dict, ChangeCounter):
    def __init__(self, *args):
        """
        The consumers class receives the next arguments:
        - ctrl_be
        """
        dict.__init__(self)
        ChangeCounter.__init__(self)

        self.ctrl_be = args[0]
        self.consumers = {}

    def __setitem__(self, key, val):
        """
        Keeps track of all the elements added to the dictionary
        """
        dict.__setitem__(self, key, val)

        # Counts the changes that occur on the val
        self.count_changes_on(val)

    def load(self):
        try:
            result = self.ctrl_be.exec_query('SELECT * FROM performance_schema.setup_consumers')

            if result is not None:
                while result.nextRow():
                    # Inserts each instrument creating a tree structure
                    # with them
                    name = result.stringByName('NAME')
                    enabled = result.stringByName('ENABLED') == 'YES'

                    self[name] = PSConsumer(enabled)

        except QueryError, err:
            if err.error == MYSQL_ERR_TABLE_DOESNT_EXIST:
                return
            else:
                raise

    def get_commit_statements(self):
        queries = []
        enabled = []
        disabled = []
        for consumer in self.keys():
            if self[consumer].has_changed():
                if self[consumer].enabled:
                    enabled.append('"%s"' % consumer)
                else:
                    disabled.append('"%s"' % consumer)

        if enabled:
            queries.append("UPDATE performance_schema.setup_consumers SET enabled = 'YES' WHERE NAME IN (%s)" % ','.join(enabled))

        if disabled:
            queries.append("UPDATE performance_schema.setup_consumers SET enabled = 'NO' WHERE NAME IN (%s)" % ','.join(disabled))

        return queries

    def get_changes(self):
        changes = {}
        for key in self.keys():
            if self[key].has_changed():
                changes[key] = self[key].get_changes()

        return changes

    def revert_changes(self):
        for key in self.keys():
            self[key].revert_changes()

    def reset_changes(self):
        for key in self.keys():
            self[key].reset_changes()


class PSVariable(ChangeTracker):
    """
    """
    def __init__(self, value):
        ChangeTracker.__init__(self)
        self.value = value


class PSVariables(dict, ChangeCounter):
    def __init__(self, *args):
        """
        The consumers class receives the next arguments:
        - ctrl_be
        """
        dict.__init__(self)
        ChangeCounter.__init__(self)

        self.ctrl_be = args[0]
        self.consumers = {}

    def __setitem__(self, key, val):
        """
        Keeps track of all the elements added to the dictionary
        """
        dict.__setitem__(self, key, val)

        # Counts the changes that occur on the val
        self.count_changes_on(val)

    def load(self):
        try:
            result = self.ctrl_be.exec_query("SHOW variables LIKE 'performance_schema%'")

            if result is not None:
                while result.nextRow():
                    # Inserts each instrument creating a tree structure
                    # with them
                    name = result.stringByIndex(1)
                    value = result.stringByIndex(2)

                    self[name] = PSVariable(value)

        except QueryError, err:
            if err.error == MYSQL_ERR_TABLE_DOESNT_EXIST:
                return
            else:
                raise


class PSActor(ChangeTracker):
    def __init__(self, user, host):
        ChangeTracker.__init__(self)
        self.user = user
        self.host = host

    def __eq__(self, other):
        if isinstance(other, PSActor):
            return other.user == self.user and other.host == self.host

class PSActors(ChangeNotifierList):
    def __init__(self, ctrl_be):
        ChangeNotifierList.__init__(self)

        self.ctrl_be = ctrl_be

    def load(self):
        try:
            result = self.ctrl_be.exec_query('SELECT user, host FROM performance_schema.setup_actors')

            if result is not None:
                while result.nextRow():
                    # Inserts each instrument creating a tree structure
                    # with them
                    user = result.stringByIndex(1)
                    host = result.stringByIndex(2)

                    self.append(PSActor(user, host))

                # Clears out the change records
                self.reset_changes()

        except QueryError, err:
            if err.error == MYSQL_ERR_TABLE_DOESNT_EXIST:
                return
            else:
                raise

    def get_commit_statements(self):
        changes = self.get_changes()

        queries = []

        for actor in changes['adds']:
            queries.append("INSERT INTO performance_schema.setup_actors VALUES ('%s', '%s', '%%')" % (actor.user, actor.host))

        for actor in changes['deletes']:
            queries.append("DELETE FROM performance_schema.setup_actors WHERE user = '%s' AND host = '%s'" % (actor.user, actor.host))

        return queries


class PSObject(ChangeTracker):
    def __init__(self, type, schema, name, enabled, timed):
        ChangeTracker.__init__(self)
        self.type = type
        self.schema = schema
        self.name = name
        self.enabled = enabled
        self.timed = timed

    def __eq__(self, other):
        if isinstance(other, PSObject):
            return other.type == self.type and \
                   other.schema == self.schema and \
                   other.name == self.name

class PSObjects(ChangeNotifierList):
    def __init__(self, ctrl_be, config_enable):
        ChangeNotifierList.__init__(self)

        self.ctrl_be = ctrl_be
        self.config_enable = config_enable

    def load(self):
        try:
            result = self.ctrl_be.exec_query('SELECT * FROM performance_schema.setup_objects')

            if result is not None:
                while result.nextRow():
                    # Inserts each instrument creating a tree structure
                    # with them
                    type = result.stringByIndex(1)
                    schema = result.stringByIndex(2)
                    name = result.stringByIndex(3)

                    # The enabled column was introduced in 5.6.3, in previous
                    # versions enabled was done for all matching records
                    index = 4
                    if self.config_enable:
                        enabled = result.stringByIndex(4) == 'YES'
                        index = index + 1
                    else:
                        enabled = True
                    timed = result.stringByIndex(index) == 'YES'

                    self.append(PSObject(type, schema, name, enabled, timed))

                # Clears out the change records
                self.reset_changes()

        except QueryError, err:
            if err.error == MYSQL_ERR_TABLE_DOESNT_EXIST:
                return
            else:
                raise

    def get_commit_statements(self):
        changes = self.get_changes()

        queries = []

        for object in changes['updates']:
            change_set = object.get_changes()
            change_cols = []
            for change in change_set:
                (col, old_value, new_value) = change
                new_value = 'YES' if new_value else 'NO'
                change_cols.append("%s = '%s'" % (col, new_value))

            query = "UPDATE performance_schema.setup_objects SET %s WHERE object_type = '%s' AND object_schema = '%s' AND object_name ='%s'" % (', '.join(change_cols), object.type, object.schema, object.name)
            queries.append(query)

        for object in changes['adds']:
            enabled = 'YES' if object.enabled else 'NO'
            timed = 'YES' if object.timed else 'NO'
            queries.append("INSERT INTO performance_schema.setup_objects VALUES ('%s', '%s', '%s', '%s', '%s')" % (object.type, object.schema, object.name, enabled, timed))

        for object in changes['deletes']:
            queries.append("DELETE FROM performance_schema.setup_objects WHERE object_type = '%s' AND object_schema = '%s' AND object_name ='%s'" % (object.type, object.schema, object.name))

        return queries


class PSTimerType(object):
    def __init__(self, name, frequency, resolution, overhead):
        self.name = name
        self.frequency = frequency
        self.resolution = resolution
        self.overhead = overhead

    def __eq__(self, other):
        if isinstance(other, PSTimerType):
            return other.name == self.name


class PSTimer(ChangeTracker):
    def __init__(self, timer):
        ChangeTracker.__init__(self)
        self.timer = timer


class PSTimers(ChangeNotifierDict):
    def __init__(self, ctrl_be, *args):
        ChangeNotifierDict.__init__(self)

        self.ctrl_be = ctrl_be

    def load(self):
        try:
            result = self.ctrl_be.exec_query('SELECT name, timer_name FROM performance_schema.setup_timers')

            if result is not None:
                while result.nextRow():
                    # Inserts each timer into the dictionary
                    name = result.stringByIndex(1)
                    timer_name = result.stringByIndex(2)

                    self[name] = PSTimer(timer_name)

                # Clears out the change records
                self.reset_changes()

        except QueryError, err:
            if err.error == MYSQL_ERR_TABLE_DOESNT_EXIST:
                return
            else:
                raise

    def get_commit_statements(self):
        changes = self.get_changes()

        queries = []

        for key in changes:
            queries.append("UPDATE performance_schema.setup_timers SET timer_name = '%s'  WHERE name = '%s'" % (changes[key][0][2], key))

        return queries

class PSThread(ChangeTracker):
    def __init__(self, id, name, instrumented):
        ChangeTracker.__init__(self)
        self.id = id
        self.name = name
        self.instrumented = instrumented
        self.thread_type = ""
        self.plist_id = ""
        self.plist_user = ""
        self.plist_host = ""
        self.plist_db = ""
        self.plist_command = ""
        self.plist_time = ""
        self.plist_state = ""
        self.plist_info = ""
        self.parent_id = ""


class PSThreads(ChangeNotifierDict):
    def __init__(self, ctrl_be, *args):
        ChangeNotifierDict.__init__(self)

        self.ctrl_be = ctrl_be

    def load(self):
        try:
            result = self.ctrl_be.exec_query('SELECT THREAD_ID, NAME, TYPE, PROCESSLIST_ID, PROCESSLIST_USER,'
                                             'PROCESSLIST_HOST, PROCESSLIST_DB, PROCESSLIST_COMMAND, PROCESSLIST_TIME,'
                                             'PROCESSLIST_STATE, SUBSTRING(PROCESSLIST_INFO, 1, 80) AS INFO,'
                                             'PARENT_THREAD_ID, INSTRUMENTED FROM performance_schema.threads')

            if result is not None:
                while result.nextRow():
                    # Inserts each thread into the dctionary
                    id = result.intByName('THREAD_ID')
                    name = result.stringByName('NAME')
                    instrumented = result.stringByName('INSTRUMENTED')  == 'YES'

                    thread = PSThread(id, name, instrumented)
                    
                    thread.thread_type = result.stringByName('TYPE')
                    thread.plist_id = result.stringByName('PROCESSLIST_ID') or ""
                    thread.plist_user = result.stringByName('PROCESSLIST_USER') or ""
                    thread.plist_host = result.stringByName('PROCESSLIST_HOST') or ""
                    thread.plist_db = result.stringByName('PROCESSLIST_DB') or ""
                    thread.plist_command = result.stringByName('PROCESSLIST_COMMAND') or ""
                    thread.plist_time = result.stringByName('PROCESSLIST_TIME') or ""
                    thread.plist_state = result.stringByName('PROCESSLIST_STATE') or ""
                    thread.plist_info = result.stringByName('INFO') or ""
                    thread.parent_id = result.stringByName('PARENT_THREAD_ID') or ""

                    # At this point the threads is considered loaded
                    # so resets the change tracking system
                    thread.reset_changes()
                    self[id] = thread

                # Clears out the change records
                self.reset_changes()

        except QueryError, err:
            if err.error == MYSQL_ERR_TABLE_DOESNT_EXIST:
                return
            else:
                raise


    def refresh(self):
        self.clear()
        self.load()


    def get_commit_statements(self):
        changes = self.get_changes()

        queries = []

        instrumented = []
        non_instrumented = []

        for thread in changes:
            if changes[thread][0][2]:
                instrumented.append(str(thread))
            else:
                non_instrumented.append(str(thread))

        if instrumented:
            queries.append("UPDATE performance_schema.threads SET INSTRUMENTED = 'YES'  WHERE THREAD_ID IN (%s)" % (','.join(instrumented)))
            
        if non_instrumented:
            queries.append("UPDATE performance_schema.threads SET INSTRUMENTED = 'NO'  WHERE THREAD_ID IN (%s)" % (','.join(non_instrumented)))

        return queries

class PSConfiguration(ChangeCounter):
    def __init__(self, ctrl_be):
        ChangeCounter.__init__(self)

        self.sections = {}

        self.ctrl_be = ctrl_be
        self.sections['instruments'] = PSInstruments(self.ctrl_be)
        self.sections['consumers'] = PSConsumers(self.ctrl_be)
        self.sections['timers'] = self.timers = PSTimers(self.ctrl_be)
        
        # setup_actors and setup_objects was available up to 5.6
        if self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6, 0):
            self.sections['actors'] = PSActors(self.ctrl_be)
            
            config_enable = True if self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6, 3) else False
            self.sections['objects'] = PSObjects(self.ctrl_be, config_enable)

            self.sections['threads'] = PSThreads(self.ctrl_be)

        # PS Status variables
        self.variables = PSVariables(self.ctrl_be)

        # Tracks changes on elements
        for element in self.sections.values():
            self.count_changes_on(element)

        self.timer_types = []

    def load_timer_types(self):
        try:
            # Records having any of frequency, resolution or overhead as NULL
            # indicate the timer is not available
            query = "SELECT * "\
                    "FROM performance_schema.performance_timers "\
                    "WHERE timer_frequency IS NOT NULL "\
                    "AND timer_resolution IS NOT NULL "\
                    "AND timer_overhead IS NOT NULL "\
                    "ORDER BY timer_name ASC"

            result = self.ctrl_be.exec_query(query)

            if result is not None:
                while result.nextRow():
                    name = result.stringByIndex(1)
                    frequency = result.stringByIndex(2)
                    resolution = result.stringByIndex(3)
                    overhead = result.stringByIndex(4)
                    self.timer_types.append(PSTimerType(name, frequency, resolution, overhead))

        except QueryError, err:
            if err.error == MYSQL_ERR_TABLE_DOESNT_EXIST:
                return
            else:
                raise


    def load(self):
        for element in self.sections.values():
            element.load()

        self.variables.load()
        self.load_timer_types()


    def commit_changes(self):

        if self.change_count:

            # Gets the statements needed to commit the changes on the different
            # sections.
            for element in self.sections.values():
                if element.change_count:
                    statements = element.get_commit_statements()

                    for statement in statements:
                        try:
                            self.ctrl_be.exec_sql(statement)
                        except QueryError, err:
                            log_error('ERROR : [%s] %s [%s]\n' % (err.error, err.msg, err.errortext))
                            raise
            
            self.reset_changes()


    def revert_changes(self):
        # Reverts the changes on elements with changes
        for element in self.sections.values():
            if element.change_count:
                element.revert_changes()

    def reset_changes(self):
        # Reverts the changes on elements with changes
        for element in self.sections.values():
            if element.change_count:
                element.reset_changes()
        



