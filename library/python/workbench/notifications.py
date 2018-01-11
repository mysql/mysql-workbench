# Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

import grt


class NotificationCenter(object):
    __instance = None
    observers = []

    def __new__(cls, *args, **kwargs):
        if cls.__instance is None:
            cls.__instance = super(NotificationCenter, cls).__new__(cls, *args, **kwargs)
        return cls.__instance

    def _the_observer(self, name, sender, args):
        for obs, nam, obj in self.observers:
            if (nam is None or name == nam) and (obj is None or obj == sender):
                try:
                    obs(name, sender, args)
                except:
                    grt.log_error("PyNotificationCenter", "Error calling notification observer for %s\n" % name)
                    import traceback
                    traceback.print_exc()

    def add_observer(self, observer, name = None, object = None):
        self.observers.append((observer, name, object))

    def remove_observer(self, observer, name = None):
        for i, (obs, n, obj) in enumerate(self.observers):
            if observer == obs and (name is None or name == n):
                del self.observers[i]
                break

    def send(self, name, sender, info):
        grt.send_grt_notification(name, sender, info)

nc = NotificationCenter()
grt._set_grt_notification_observer(nc._the_observer)
