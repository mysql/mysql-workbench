# Copyright (c) 2013, 2014 Oracle and/or its affiliates. All rights reserved.
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

from workbench.log import log_debug3

class ChangeNotifier(object):
    def __init__(self):
        self.__change_notification_cb = None

    def set_notification_cb(self, callback):
        self.__change_notification_cb = callback

    def unset_notification_cb(self, callback):
        if self.__change_notification_cb == callback:
            self.__change_notification_cb = None

    def notify_change(self, change, attr, value):
        if self.__change_notification_cb:
            self.__change_notification_cb(change, attr, value)


class ChangeCounter(ChangeNotifier):
    """
    This is a helper class to count changes reported
    """
    def __init__(self):
        ChangeNotifier.__init__(self)

        self.change_count = 0

    def count_change(self, change, attr, value):

        increment = 1 if change else -1
        self.change_count += increment 

        # Propagates change notification
        self.notify_change(change, attr, value)

    def count_changes_on(self, source):
        source.set_notification_cb(self.count_change)

    def stop_change_count_on(self, source):
        source.unset_notification_cb(self.count_change)


class ChangeNotifierList(list, ChangeCounter):
    """
    Implementation of a list that keeps track of the changes occurred 
    on its elements.

    To use this class the elements should met the next characteristics:
    - They must subclass ChangeTracker
    - They must implement __eq__ for item location on the list

    NOTE: Given the independence of the __eq__ operator on the items
          the item received as a parameter on the remove method is 
          used ONLY for identification purposes.

          The object that gets actually removed/backed up is the one
          existing on the list.
    """
    def __init__(self):
        list.__init__(self)
        ChangeCounter.__init__(self)

        self.__additions = []
        self.__deletions = []

    def append(self, item):
        if self.__deletions.count(item):
            self.__deletions.remove(item)
            change = False
        else:
            change = True
            self.__additions.append(item)

        self.count_change(change, None, None)
        self.count_changes_on(item)
        list.append(self, item)

    def remove(self, item):
        # Non existing item is just ignored
        if self.count(item):

            # Replaces the received object with the real
            # one stored on the list
            index = self.index(item)
            item = self[index]

            if self.__additions.count(item):
                change = False
                self.__additions.remove(item)
            else:
                change = True
                self.__deletions.append(item)

            self.count_change(change, None, None)
            self.stop_change_count_on(item)
            list.remove(self, item)

    def has_changed(self):
        updates = False
        for item in self:
            if item.has_changed():
                updates = True

        return updates or len(self.__deletions) > 0 or len(self.__additions) > 0

    def get_changes(self):
        changes = {}
        updates = []
        for item in self:
            if item.has_changed():
                try:
                    self.__additions.index(item)
                # Only items that were not added should be considered updates
                except ValueError:
                    updates.append(item)

        changes['updates'] = updates
        changes['deletes'] = self.__deletions
        changes['adds'] = self.__additions

        return changes

    def revert_changes(self):
        for item in self:
            if item.has_changed():
                item.revert_changes()
                
        items = self.__deletions[:]
        for item in items:
            self.append(item)

        items = self.__additions[:]
        for item in items:
            self.remove(item)

    def reset_changes(self):
        for item in self:
            if item.has_changed():
                item.reset_changes()

        self.__deletions = []
        self.__additions = []


class ChangeNotifierDict(dict, ChangeCounter):
    def __init__(self, *args):
        dict.__init__(self, args)
        ChangeCounter.__init__(self)


    def __setitem__(self, key, val):
        """
        All elements on this dictionary should notify about changes
        """
        dict.__setitem__(self, key, val)

        # Counts the changes that occur on the val
        self.count_changes_on(val)


    def reset_changes(self):
        """
        Resets the entire change tracking system, i.e. when
        the changes are committed.
        """
        self._clear_changes()

    def revert_changes(self):
        """
        Undoes all the changes done on the elements of this dictionary.
        """
        self._clear_changes(True)

    def _clear_changes(self, revert = False):
        if self.change_count:
            for item in self.keys():
                if revert:
                    self[item].revert_changes()
                else:
                    self[item].reset_changes()


    def get_changes(self):
        changes = {}
        for key in self.keys():
            if self[key].has_changed():
                changes[key] = self[key].get_changes()

        return changes

class ChangeTracker(ChangeNotifier):
    """
    ChangeTracker is a class in charge of keeping track of the 
    changes done to the attributes in a subclass.

    As 'change' we understand any change done from a starting point
    which by default is after __init__ is called (not necessarily).

    i.e. when an attribue is created by the first time that is considered
    it's starting point, from there, if the value is changed it is already
    considered a change.

    The starting point can be also re-defined by calling reset_changes.
    """
    def __init__(self):
        ChangeNotifier.__init__(self)
        self.__changed = {}
        self.__ignoring = 0
        self.__value_set_notification_cb = None

    def __setattr__(self, name, value):
        # Verifies the value being set is a valid attribute
        # Also ensures the value is changing from the current value
        if name in self.__dict__ and \
           name != '_ChangeTracker__changed' and \
           name != '_ChangeTracker__ignoring' and \
           name != '_ChangeTracker__notify_value_set_cb' and \
           name != '_ChangeTracker__value_set_notification_cb' and \
           name != '_ChangeNotifier__change_notification_cb' and \
           name != '_ChangeCounterchange_count' and \
           not self.__ignoring and \
           self.__dict__[name] != value:

            log_message = "Changed %s from %s to %s at %s\n" % (name, self.__dict__[name], value, self)

            # If the value was already changed and the new value
            # reverts the change then it removes the attribute from
            # the changed map
            if name in self.__dict__["_ChangeTracker__changed"]:
                if self.__dict__["_ChangeTracker__changed"][name] == value:
                    del self.__dict__["_ChangeTracker__changed"][name]

                    # Sends message indicating a change has been undone
                    self.notify_change(False, name, value)

                    log_message = "Reverted change on %s to %s at %s\n" % (name, value, self)

            # If this is the first change to the attribute, registers the
            # Original value on the changed map
            else:
                self.__dict__["_ChangeTracker__changed"][name] = self.__dict__[name]

                # Sends message indicating a change has been done
                self.notify_change(True, name, value)
            
            # If configured, notifies about a value being set
            if self.__value_set_notification_cb:
                self.__value_set_notification_cb(name, value)

            # Logs the change
            log_debug3("%s\n" % log_message)

        # Updates the value
        self.__dict__[name] = value

    def set_value_set_notification(self, callback):
        self.__value_set_notification_cb = callback

    def has_changed(self, name = None):
        """
        Verifies if there are changes on the class attributes.
        If name is given it will verify for changes on that specific attribute.
        If not, will verify for changes on any attribute.
        """
        if name:
            return name in self.__changed
        else:
            return len(self.__changed) > 0

    def get_changes(self, name = None):
        """
        Retrieves the changes on the class attributes as tuples.
        If name is given it will return a tuple containing the (initial, current) values
        If not, it will return a list of tuples as (attribute, initial, current)

        If there are no changes it will return None.
        """
        if name and name in self.__changed:
            return (self.__changed[name], self.__dict__[name])
        elif name is None and len(self.__changed):
            return [(att, self.__changed[att], self.__dict__[att]) for att in self.__changed]
        else:
            return None

    def set_ignoring(self, value):
        """ 
        Used to turn ON/OFF the change detection mechanism.
        """ 
        increase = 1 if value else -1
        self.__ignoring = self.__ignoring + increase

    def reset_changes(self):
        """
        Clears any registered changes to create a new starting point.
        """
        for attr in self.__changed.keys():
            self.notify_change(False, attr, self.__dict__[attr])

        self.__changed={}


    def revert_changes(self):
        """
        Reverts the changes applied.
        """
        for attr in self.__changed.keys():
            self.__setattr__(attr, self.__changed[attr])

class ignore_changes(object):
    """
    IgnoreChanges Decorator
    It's purpose is to add the decorator on those methods
    for which the change detection will be turned off.

    It will only have effect on those classes childs of ChangeTracker.
    """
    def __init__(self, func):
        self.func = func
        self.instance = None

    def __call__(self, *args):
        if isinstance(self.instance, ChangeTracker):
            self.instance.set_ignoring(True)
            ret_val = self.func(*args)
            self.instance.set_ignoring(False)
            return ret_val

    def __get__(self, obj, objtype):
        self.instance = obj
        import functools
        return functools.partial(self.__call__, obj)
        